// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AudioProcess.h"

#include "config.h"

#ifdef HAVE_LADSPA
#include "LADSPAPluginInstance.h"
#endif

#include "PlayableAudioFile.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"
#include "Profiler.h"
#include "AudioLevel.h"

#include <sys/time.h>

#include <cmath>

//#define DEBUG_MIXER 1
//#define DEBUG_MIXER_LIGHTWEIGHT 1
//#define DEBUG_LOCKS 1
//#define DEBUG_READER 1
//#define DEBUG_WRITER 1

namespace Rosegarden
{

AudioMixer::AudioMixer(SoundDriver *driver,
		       AudioFileReader *fileReader,
		       unsigned int sampleRate,
		       unsigned int blockSize) :
    m_driver(driver),
    m_fileReader(fileReader),
    m_sampleRate(sampleRate),
    m_blockSize(blockSize),
    m_thread(0),
    m_submasterCount(0)
{
    // Leave the buffer map and process buffer list empty for now.
    // The number of channels per fader can change between plays, so
    // we always examine the buffers in fillBuffers and are prepared
    // to regenerate from scratch if necessary.

    pthread_mutex_t  initialisingMutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&m_lock, &initialisingMutex, sizeof(pthread_mutex_t));

    pthread_cond_t initialisingCondition = PTHREAD_COND_INITIALIZER;
    memcpy(&m_condition, &initialisingCondition, sizeof(pthread_cond_t));

    pthread_create(&m_thread, NULL, threadRun, this);
    pthread_detach(m_thread);
}

AudioMixer::~AudioMixer()
{
    std::cerr << "AudioMixer::~AudioMixer" << std::endl;

    if (m_thread) {
	pthread_cancel(m_thread);
	m_thread = 0;
    }
    pthread_mutex_destroy(&m_lock);

    // BufferRec dtor will handle the BufferMap

    for (std::vector<sample_t *>::iterator i = m_processBuffers.begin();
	 i != m_processBuffers.end(); ++i) {
	delete *i;
    }

    std::cerr << "AudioMixer::~AudioMixer exiting" << std::endl;
}

AudioMixer::BufferRec::~BufferRec()
{
    for (size_t i = 0; i < buffers.size(); ++i) delete buffers[i];
}

int
AudioMixer::getLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioMixer::getLock()" << std::endl;
#endif
    rv = pthread_mutex_lock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK" << std::endl;
#endif
    return rv;
}

int
AudioMixer::tryLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioMixer::tryLock()" << std::endl;
#endif
    rv = pthread_mutex_trylock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK (rv is " << rv << ")" << std::endl;
#endif
    return rv;
}

int
AudioMixer::releaseLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioMixer::releaseLock()" << std::endl;
#endif
    rv = pthread_mutex_unlock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK" << std::endl;
#endif
    return rv;
}

void
AudioMixer::signal()
{
#ifdef DEBUG_LOCKS
    std::cerr << "AudioMixer::signal()" << std::endl;
#endif
    pthread_cond_signal(&m_condition);
}

void
AudioMixer::setPlugin(InstrumentId id, int position, unsigned int pluginId)
{
    removePlugin(id, position);

    getLock();

#ifdef HAVE_LADSPA

    // Get a descriptor - if this fails we can't initialise anyway
    const LADSPA_Descriptor *des =
	m_driver->getMappedStudio()->createPluginDescriptor(pluginId);

    if (des) {

        // create and store
        LADSPAPluginInstance *instance =
            new LADSPAPluginInstance
	    (id, pluginId, position, m_sampleRate, m_blockSize, des);

        m_plugins[id][position] = instance;

        instance->activate();
    }
#endif

    releaseLock();
}
    
void 
AudioMixer::removePlugin(InstrumentId id, int position)
{
    getLock();

    PluginList::iterator i = m_plugins[id].find(position);
    if (i != m_plugins[id].end()) {

#ifdef HAVE_LADSPA
	LADSPAPluginInstance *instance = dynamic_cast<LADSPAPluginInstance *>
	    (i->second);

	if (instance) {

	    // Deactivate and cleanup
	    //
	    instance->deactivate();
	    instance->cleanup();
	
	    // Potentially unload the shared library in which the plugin
	    // came from if none of its siblings are in use.
	    //
	    m_driver->getMappedStudio()->unloadPlugin(instance->getLADSPAId());
	
	    delete instance;
	}
#endif

	m_plugins[id].erase(i);
	releaseLock();
	return;
    }

    releaseLock();
}

void
AudioMixer::removeAllPlugins()
{
    getLock();

    for (PluginMap::iterator j = m_plugins.begin();
	 j != m_plugins.end(); ++j) {

	InstrumentId id = j->first;

	for (PluginList::iterator i = m_plugins[id].begin();
	     i != m_plugins[id].end();) {

#ifdef HAVE_LADSPA
	    LADSPAPluginInstance *instance = dynamic_cast<LADSPAPluginInstance *>
		(i->second);

	    if (instance) {

		// Deactivate and cleanup
		//
		instance->deactivate();
		instance->cleanup();
		
		// Potentially unload the shared library in which the plugin
		// came from if none of its siblings are in use.
		//
		m_driver->getMappedStudio()->unloadPlugin(instance->getLADSPAId());
	
		delete instance;
	    }
#endif

	    PluginList::iterator k = i;
	    ++k;
	    m_plugins[id].erase(i);
	    i = k;
	}
    }

    releaseLock();
}


void
AudioMixer::setPluginPortValue(InstrumentId id, int position,
			       unsigned int port, float value)
{
    getLock();
    
    PluginList::iterator i = m_plugins[id].find(position);
    if (i != m_plugins[id].end()) {
	i->second->setPortValue(port, value);
    }
    
    releaseLock();
}

void
AudioMixer::setPluginBypass(InstrumentId id, int position, bool bypass)
{
    getLock();
    
    PluginList::iterator i = m_plugins[id].find(position);
    if (i != m_plugins[id].end()) {
	i->second->setBypassed(bypass);
    }
    
    releaseLock();
}

void
AudioMixer::resetAllPlugins()
{
    getLock();

    for (PluginMap::iterator j = m_plugins.begin();
	 j != m_plugins.end(); ++j) {

	InstrumentId id = j->first;

	for (PluginList::iterator i = m_plugins[id].begin();
	     i != m_plugins[id].end(); ++i) {

#ifdef HAVE_LADSPA
	    LADSPAPluginInstance *instance = dynamic_cast<LADSPAPluginInstance *>
		(i->second);

	    if (instance) {
		instance->deactivate();
		instance->activate();
	    }
#endif
	}
    }

    releaseLock();
}

void
AudioMixer::generateBuffer(InstrumentId id,
			   unsigned int channels,
			   size_t bufferSamples,
			   unsigned int &maxChannels)
{
    BufferRec &rec = m_bufferMap[id];
    
    rec.channels = channels;
    
    // We always have stereo buffers (for output of pan)
    // even on a mono instrument.
    if (channels < 2) channels = 2;
    if (channels > maxChannels) maxChannels = channels;
    
    for (size_t i = 0; i < rec.buffers.size(); ++i) {
	delete rec.buffers[i];
    }
    rec.buffers.clear();
    
    for (unsigned int i = 0; i < channels; ++i) {
	RingBuffer<sample_t> *rb = new RingBuffer<sample_t>(bufferSamples);
	if (!rb->mlock()) {
	    std::cerr << "WARNING: AudioMixer::generateBuffers: couldn't lock ring buffer into real memory, performance may be impaired" << std::endl;
	}
	rec.buffers.push_back(rb);
    }	    
}

void
AudioMixer::generateBuffers()
{
    InstrumentId instrumentBase;
    int instruments;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instruments);

    unsigned int maxChannels = 0;

    RealTime bufferLength = m_driver->getAudioMixBufferLength();
    int bufferSamples = RealTime::realTime2Frame(bufferLength, m_sampleRate);
    bufferSamples = ((bufferSamples / m_blockSize) + 1) * m_blockSize;

#ifdef DEBUG_MIXER
    std::cerr << "AudioMixer::generateBuffers: Buffer length is " << bufferLength << "; buffer samples " << bufferSamples << " (sample rate " << m_sampleRate << ")" << std::endl;
#endif

    // master

    generateBuffer(0, 2, bufferSamples, maxChannels);

    // submasters

    m_submasterCount =
	m_driver->getMappedStudio()->getObjectCount(MappedStudio::AudioBuss);
    if (m_submasterCount > 0) --m_submasterCount; // buss zero is master

    for (InstrumentId id = 1; id < 1 + m_submasterCount; ++id) {

	generateBuffer(id, 2, bufferSamples, maxChannels);

	WorkBufferMap::iterator bi = m_submasterWorkBuffers.find(id);
	if (bi != m_submasterWorkBuffers.end()) {
	    BufferSet &s = bi->second;
	    delete[] s[0];
	    delete[] s[1];
	    m_submasterWorkBuffers.erase(bi);
	}
	m_submasterWorkBuffers[id].push_back(new sample_t[m_blockSize]);
	m_submasterWorkBuffers[id].push_back(new sample_t[m_blockSize]);
    }

    // instruments
    
    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instruments;
	 ++id) {

	// Get a fader for this instrument - if we can't then this
	// isn't a valid audio instrument
	MappedAudioFader *fader = m_driver->getMappedStudio()->getAudioFader(id);

	if (!fader) {
	    std::cerr << "WARNING: AudioMixer::generateBuffers: no fader for audio instrument " << id << std::endl;
	    continue;
	}

	float fch = 2;
	(void)fader->getProperty(MappedAudioFader::Channels, fch);
	unsigned int channels = (unsigned int)fch;

	generateBuffer(id, channels, bufferSamples, maxChannels);
    }

    while (m_processBuffers.size() > maxChannels) {
	BufferSet::iterator bi = m_processBuffers.end();
	--bi;
	delete[] *bi;
	m_processBuffers.erase(bi);
    }
    while (m_processBuffers.size() < maxChannels) {
	m_processBuffers.push_back(new sample_t[m_blockSize]);
    }
}

void
AudioMixer::fillBuffers(const RealTime &currentTime)
{
    emptyBuffers(currentTime);

    getLock();

#ifdef DEBUG_MIXER
    std::cerr << "AudioMixer::fillBuffers(" << currentTime <<")" << std::endl;
#endif

    bool discard;
    processBlocks(true, discard);

    releaseLock();
}

void
AudioMixer::emptyBuffers(RealTime currentTime)
{
    getLock();

#ifdef DEBUG_MIXER
    std::cerr << "AudioMixer::emptyBuffers(" << currentTime <<")" << std::endl;
#endif

    generateBuffers();

    for (BufferMap::iterator itr = m_bufferMap.begin();
	 itr != m_bufferMap.end(); ++itr) {

	BufferRec &rec = itr->second;

	rec.reset(currentTime);

	for (size_t i = 0; i < rec.buffers.size(); ++i) {
	    rec.buffers[i]->reset();
	}
    }

    releaseLock();
}

static inline void denormalKill(float *buffer, int size)
{
    for (int i = 0; i < size; ++i) {
	buffer[i] += 1e-18f;
	buffer[i] -= 1e-18f;
    }
}

void
AudioMixer::processBlocks(bool forceFill, bool &waitingForFiles)
{
    waitingForFiles = false;
    if (m_bufferMap[0].buffers.size() < 2) return; // no master set up yet

    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);

    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instrumentCount; ++id) {
	m_bufferMap[id].empty = m_plugins[id].empty();
    }

    PlayableAudioFileList audioQueue = m_driver->getAudioPlayQueueNotDefunct();
    std::map<InstrumentId, PlayableAudioFileList> files;

    for (PlayableAudioFileList::iterator i = audioQueue.begin();
	 i != audioQueue.end(); ++i) {

	InstrumentId id = (*i)->getInstrument();
	files[id].push_back(*i);
	m_bufferMap[id].empty = false;
    }

    int minBlocks = 0;

    for (InstrumentId id = 0; id <= m_submasterCount; ++id) {
	int lb = m_bufferMap[id].buffers[0]->getWriteSpace() / m_blockSize;
	int rb = m_bufferMap[id].buffers[1]->getWriteSpace() / m_blockSize;
	if (id == 0 || lb < minBlocks || rb < minBlocks) {
	    minBlocks = std::min(lb, rb);
	}
    }

    if (minBlocks == 0) {
	return;
    }

    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instrumentCount; ++id) {

	int blocksHere = 0;

	if (m_bufferMap[id].empty) {
	    blocksHere = canProcessEmptyBlocks(id);

#ifdef DEBUG_MIXER
	    if (m_driver->isPlaying()) {
		if (id == 1000) std::cerr << "AudioMixer::processBlocks(" << id << "): can process " << blocksHere << " empty blocks" << std::endl;
	    }
#endif

	} else {
	    blocksHere = canProcessBlocks(id, files[id], forceFill);

#ifdef DEBUG_MIXER
	    if (m_driver->isPlaying()) {
		if (id == 1000) std::cerr << "AudioMixer::processBlocks(" << id << "): can process " << blocksHere << " blocks" << std::endl;
	    }
#endif
	}

	if (id == instrumentBase || blocksHere < minBlocks) {
	    minBlocks = blocksHere;
	}
    }

    if (minBlocks == 0) {
	waitingForFiles = true;
	return;
    }

    for (InstrumentId id = 0; id <= m_submasterCount; ++id) {
	
	MappedAudioBuss *buss =
	    m_driver->getMappedStudio()->getAudioBuss(id);

	if (!buss) {

	    std::cerr << "WARNING: No audio buss found for buss no "
		      << id << std::endl;

	    m_bufferMap[id].gainLeft  = 0.0;
	    m_bufferMap[id].gainRight = 0.0;
	    m_bufferMap[id].volume    = 0.0;
	} else {
	    	    
	    float level = 0.0;
	    (void)buss->getProperty(MappedAudioBuss::Level, level);

	    float volume = AudioLevel::dB_to_multiplier(level);

	    float pan = 0.0;
//!!!	    (void)fader->getProperty(MappedAudioFader::Pan, pan);

	    m_bufferMap[id].gainLeft = 
		volume * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);

	    m_bufferMap[id].gainRight =
		volume * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
	    
	    m_bufferMap[id].volume = volume;
	}
    }

    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instrumentCount; ++id) {
	
	MappedAudioFader *fader =
	    m_driver->getMappedStudio()->getAudioFader(id);

	if (!fader) {
	    m_bufferMap[id].gainLeft  = 0.0;
	    m_bufferMap[id].gainRight = 0.0;
	    m_bufferMap[id].volume    = 0.0;
	} else {
	    	    
	    float faderLevel = 0.0;
	    (void)fader->getProperty(MappedAudioFader::FaderLevel, faderLevel);

	    float volume = AudioLevel::dB_to_multiplier(faderLevel);

	    float pan = 0.0;
	    (void)fader->getProperty(MappedAudioFader::Pan, pan);

	    m_bufferMap[id].gainLeft = 
		volume * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);

	    m_bufferMap[id].gainRight =
		volume * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
	    
	    m_bufferMap[id].volume = volume;
	}
    }

#ifdef DEBUG_MIXER
    if (m_driver->isPlaying()) {
	std::cerr << "AudioMixer::processBlocks: planning to do " << minBlocks << " blocks" << std::endl;
    }
#endif
    // Call the processBlock methods to process individual instruments,
    // and mix in to the submasters and master as we go along.

    for (int block = 0; block < minBlocks; ++block) {

	for (WorkBufferMap::iterator wi = m_submasterWorkBuffers.begin();
	     wi != m_submasterWorkBuffers.end(); ++wi) {

	    memset(wi->second[0], 0, m_blockSize * sizeof(sample_t));
	    memset(wi->second[1], 0, m_blockSize * sizeof(sample_t));
	}

	for (InstrumentId id = instrumentBase;
	     id < instrumentBase + instrumentCount; ++id) {

	    if (m_bufferMap[id].empty) {
		processEmptyBlock(id);
	    } else {
		processBlock(id, files[id]);
	    }
	}

	bool first = true;

	for (WorkBufferMap::iterator wi = m_submasterWorkBuffers.begin();
	     wi != m_submasterWorkBuffers.end(); ++wi) {

	    InstrumentId id = wi->first;
	    BufferSet &workset = wi->second;
	    BufferRec &rec = m_bufferMap[id];

	    for (int ch = 0; ch < 2; ++ch) {
		
		// Write submaster ringbuffer

		float gain = ((ch == 0) ? rec.gainLeft : rec.gainRight);

		if (gain == 0.0) {

		    rec.buffers[ch]->zero(m_blockSize);

		    if (first) {
			memset(m_processBuffers[ch], 0,
			       m_blockSize * sizeof(sample_t));
		    }

		} else {

		    for (size_t i = 0; i < m_blockSize; ++i) {
			workset[ch][i] *= gain;
		    }
		    
		    rec.buffers[ch]->write(workset[ch], m_blockSize);

		    // Accumulate into master buffers

		    if (first) {
			memcpy(m_processBuffers[ch], workset[ch],
			       m_blockSize * sizeof(sample_t));
		    } else {
			for (size_t i = 0; i < m_blockSize; ++i) {
			    m_processBuffers[ch][i] += workset[ch][i];
			}
		    }
		}
	    }

	    first = false;
	}

	// Write master ringbuffer

	BufferRec &rec = m_bufferMap[0];

	for (int ch = 0; ch < 2; ++ch) {
	    
	    float gain = ((ch == 0) ? rec.gainLeft : rec.gainRight);

	    for (size_t i = 0; i < m_blockSize; ++i) {
		m_processBuffers[ch][i] *= gain;
	    }

	    rec.buffers[ch]->write(m_processBuffers[ch], m_blockSize);
	}
    }
}


int
AudioMixer::canProcessEmptyBlocks(InstrumentId id)
{
#ifdef DEBUG_MIXER
    if (m_driver->isPlaying()) {
	if (id == 1000) std::cerr << "AudioMixer::canProcessEmptyBlock(" << id << ")" << std::endl;
    }
#endif
  
    BufferRec &rec = m_bufferMap[id];

    unsigned int channels = rec.buffers.size();
    if (channels > m_processBuffers.size()) channels = m_processBuffers.size();
    if (channels == 0) return 0; // buffers just haven't been set up yet

    unsigned int targetChannels = channels;
    if (targetChannels < 2) targetChannels = 2; // fill at least two buffers

    size_t minWriteSpace = 0;
    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	size_t thisWriteSpace = rec.buffers[ch]->getWriteSpace();
	if (ch == 0 || thisWriteSpace < minWriteSpace) {
	    minWriteSpace = thisWriteSpace;
	    if (minWriteSpace < m_blockSize) return 0;
	}
    }

    return minWriteSpace / m_blockSize;
}

void
AudioMixer::processEmptyBlock(InstrumentId id)
{
#ifdef DEBUG_MIXER
    if (m_driver->isPlaying()) {
	if (id == 1000) std::cerr << "AudioMixer::processEmptyBlock(" << id << ")" << std::endl;
    }
#endif
  
    BufferRec &rec = m_bufferMap[id];
    unsigned int channels = rec.buffers.size();
    if (channels > m_processBuffers.size()) channels = m_processBuffers.size();
    if (channels == 0) return; // buffers just haven't been set up yet

    unsigned int targetChannels = channels;
    if (targetChannels < 2) targetChannels = 2; // fill at least two buffers

    rec.zeroFrames += m_blockSize;
    bool dormant = true;
                
    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	rec.buffers[ch]->zero(m_blockSize);
	if (rec.buffers[ch]->getReadSpace() > rec.zeroFrames) dormant = false;
    }

    rec.dormant = dormant;
    rec.filledTo = rec.filledTo +
	RealTime::frame2RealTime(m_blockSize, m_sampleRate);
}
	    

int
AudioMixer::canProcessBlocks(InstrumentId id,
			     PlayableAudioFileList &audioQueue,
			     bool forceFill)
{
    BufferRec &rec = m_bufferMap[id];
    RealTime bufferTime = rec.filledTo;

#ifdef DEBUG_MIXER
    if (m_driver->isPlaying()) {
	if (id == 1000) std::cerr << "AudioMixer::canProcessBlock(" << id << "): buffer time is " << bufferTime << std::endl;
    }
#endif

    for (PlayableAudioFileList::iterator it = audioQueue.begin();
	 it != audioQueue.end(); ++it) {
	    
	PlayableAudioFile *file = *it;
	    
	if (file->getStatus() == PlayableAudioFile::IDLE ||
	    file->getStatus() == PlayableAudioFile::READY) {
	    file->setReadyTime(bufferTime);
	    file->setStatus(PlayableAudioFile::READY);
	}
    }

    if (forceFill) {
#ifdef DEBUG_MIXER
	if (id == 1000) std::cerr << "AudioMixer::canProcessBlock(" << id << "): calling file reader to force buffering" << std::endl;
#endif
	m_fileReader->updateReadyStatuses(audioQueue);
    }

    unsigned int channels = rec.channels;
    if (channels > rec.buffers.size()) channels = rec.buffers.size();
    if (channels > m_processBuffers.size()) channels = m_processBuffers.size();
    if (channels == 0) return 0; // buffers just haven't been set up yet

    unsigned int targetChannels = channels;
    if (targetChannels < 2) targetChannels = 2; // fill at least two buffers

    size_t minWriteSpace = 0;
    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	size_t thisWriteSpace = rec.buffers[ch]->getWriteSpace();
	if (ch == 0 || thisWriteSpace < minWriteSpace) {
	    minWriteSpace = thisWriteSpace;
	    if (minWriteSpace < m_blockSize) return 0;
	}
    }

    for (PlayableAudioFileList::iterator it = audioQueue.begin();
	 it != audioQueue.end(); ++it) {
	    
	PlayableAudioFile *file = *it;
	    
	if (file->getStatus() == PlayableAudioFile::READY &&
	    file->isBufferable(bufferTime)) {

	    // This file should be playing, but the disc thread
	    // hasn't got around to buffering it yet.  There's
	    // nothing we can do in this situation.

#ifdef DEBUG_MIXER
	    if (id == 1000) std::cerr << "AudioMixer::canProcessBlock(" << id <<"): some unbuffered files, can't continue with this instrument" << std::endl;
#endif
	    return 0;
	}
    }

#ifdef DEBUG_MIXER
    if (id == 1000 && m_driver->isPlaying()) std::cerr << "AudioMixer::canProcessBlock(" << id <<"): minWriteSpace is " << minWriteSpace << std::endl;
#else
#ifdef DEBUG_MIXER_LIGHTWEIGHT
    if (id == 1000 && m_driver->isPlaying()) std::cout << minWriteSpace << "/" << rec.buffers[0]->getSize() << std::endl;
#endif
#endif

#ifdef DEBUG_MIXER
    if (id == 1000 && audioQueue.size() > 0) std::cerr << "AudioMixer::canProcessBlock(" << id <<"): " << audioQueue.size() << " audio file(s) to consider" << std::endl;
#endif

    bool haveFiles = false;
    size_t minReadSpace = 0;

    for (PlayableAudioFileList::iterator it = audioQueue.begin();
	 it != audioQueue.end(); ++it) {

	if ((*it)->getStatus() == PlayableAudioFile::PLAYING) {

	    size_t frames = (*it)->getSampleFramesAvailable();

#ifdef DEBUG_MIXER
	    if (id == 1000) std::cerr << "AudioMixer::canProcessBlock(" << id <<"): playing audio file has " << frames << " frames available" << std::endl;
#endif

	    if ((*it)->isFullyBuffered() && frames < m_blockSize) {
		frames = m_blockSize;
	    }

	    if (!haveFiles || frames < minReadSpace) {
		if (frames < m_blockSize) return 0;
		minReadSpace = frames;
	    }

	    haveFiles = true;
	}
    }
	
    if (haveFiles) {
	return std::min(minWriteSpace, minReadSpace) / m_blockSize;
    } else {
	return minWriteSpace / m_blockSize;
    }
}

void
AudioMixer::processBlock(InstrumentId id, PlayableAudioFileList &audioQueue)
{
    BufferRec &rec = m_bufferMap[id];
    RealTime bufferTime = rec.filledTo;
    
    unsigned int channels = rec.channels;
    if (channels > rec.buffers.size()) channels = rec.buffers.size();
    if (channels > m_processBuffers.size()) channels = m_processBuffers.size();
    if (channels == 0) return; // buffers just haven't been set up yet

    unsigned int targetChannels = channels;
    if (targetChannels < 2) targetChannels = 2; // fill at least two buffers

    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	memset(m_processBuffers[ch], 0, sizeof(sample_t) * m_blockSize);
    }
	
    // Mix in a block from each playing file on this instrument.
    
    for (PlayableAudioFileList::iterator it = audioQueue.begin();
	 it != audioQueue.end(); ++it) {
	
	if ((*it)->getStatus() != PlayableAudioFile::PLAYING) continue;
	unsigned int fileChannels = (*it)->getTargetChannels();
	unsigned int ch = 0;
	
	for (ch = 0; ch < channels; ++ch) {
	    if (ch >= fileChannels) break;
	    (*it)->addSamples(m_processBuffers[ch], ch, m_blockSize);
	}
	
	while (ch < fileChannels) {
	    (*it)->skipSamples(ch, m_blockSize);
	    ++ch;
	}
    }

    PluginList &plugins = m_plugins[id];
#ifdef DEBUG_MIXER
    if (id == 1000 && !plugins.empty()) std::cerr << "AudioMixer::processBlock(" << id << "): have " << plugins.size() << " plugin(s)" << std::endl;
#endif

    // Apply plugins.  There are various copy-reducing
    // optimisations available here, but we're not even going to
    // think about them yet.  Note that we force plugins to mono
    // on a mono track, even though we have stereo output buffers
    // -- stereo only comes into effect at the pan stage, and
    // these are pre-fader plugins.

    for (PluginList::iterator pli = plugins.begin();
	 pli != plugins.end(); ++pli) {

	RunnablePluginInstance *plugin = pli->second;
	if (plugin->isBypassed()) continue;

	unsigned int ch = 0;

	// If a plugin has more input channels than we have
	// available, we duplicate up to stereo and leave any
	// remaining channels empty.

	while (ch < plugin->getAudioInputCount()) {

	    if (ch < channels || ch < 2) {
		memcpy(plugin->getAudioInputBuffers()[ch],
		       m_processBuffers[ch % channels],
		       m_blockSize * sizeof(sample_t));
	    } else {
		memset(plugin->getAudioInputBuffers()[ch], 0,
		       m_blockSize * sizeof(sample_t));
	    }
	    ++ch;
	}

#ifdef DEBUG_MIXER
	std::cerr << "Running plugin with " << plugin->getAudioInputCount()
		  << " inputs, " << plugin->getAudioOutputCount() << " outputs" << std::endl;
#endif

	plugin->run();

	ch = 0;

	while (ch < plugin->getAudioOutputCount()) {

	    denormalKill(plugin->getAudioOutputBuffers()[ch],
			 m_blockSize);

	    if (ch < channels) {
		memcpy(m_processBuffers[ch],
		       plugin->getAudioOutputBuffers()[ch],
		       m_blockSize * sizeof(sample_t));
	    } else if (ch == 1) {
		// stereo output from plugin on a mono track
		for (size_t i = 0; i < m_blockSize; ++i) {
		    m_processBuffers[0][i] +=
			plugin->getAudioOutputBuffers()[ch][i];
		    m_processBuffers[0][i] /= 2;
		}
	    } else {
		break;
	    }

	    ++ch;
	}
    }

    // special handling for pan on mono tracks

    bool allZeros = true;

    if (targetChannels == 2 && channels == 1) {

	for (size_t i = 0; i < m_blockSize; ++i) {

	    sample_t sample = m_processBuffers[0][i];

	    m_processBuffers[0][i] = sample * rec.gainLeft;
	    m_processBuffers[1][i] = sample * rec.gainRight;

	    if (allZeros && sample != 0.0)
		allZeros = false;
	}
		
	rec.buffers[0]->write(m_processBuffers[0], m_blockSize);
	rec.buffers[1]->write(m_processBuffers[1], m_blockSize);

    } else {

	for (unsigned int ch = 0; ch < targetChannels; ++ch) {
		
	    float gain = ((ch == 0) ? rec.gainLeft  :
			  (ch == 1) ? rec.gainRight : rec.volume);

	    for (size_t i = 0; i < m_blockSize; ++i) {

		// handle volume and pan
		m_processBuffers[ch][i] *= gain;

		if (allZeros && (m_processBuffers[ch][i] != 0.0))
		    allZeros = false;
	    }
		
	    rec.buffers[ch]->write(m_processBuffers[ch], m_blockSize);
	}
    }

    bool dormant = true;

    if (allZeros) {
	rec.zeroFrames += m_blockSize;
	for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	    if (rec.buffers[ch]->getReadSpace() > rec.zeroFrames) {
		dormant = false;
	    }
	}
    } else {
	rec.zeroFrames = 0;
	dormant = false;
    }

    if (!allZeros) {
	//!!! for the moment let's just use a fixed submaster
	//!!! note also fixed number of submaster channels for the mo'
	InstrumentId submaster = 1 + (id % 1000) / 4;//!!! hack

	BufferSet &s = m_submasterWorkBuffers[submaster];

	for (unsigned int ch = 0; ch < 2; ++ch) {
	    
	    for (size_t i = 0; i < m_blockSize; ++i) {
		s[ch][i] += m_processBuffers[ch][i];
	    }
	}
    }

#ifdef DEBUG_MIXER
    if (id == 1000 && m_driver->isPlaying()) std::cerr << "AudioMixer::processBlock(" << id <<"): setting dormant to " << dormant << std::endl;
#endif

    rec.dormant = dormant;
    bufferTime = bufferTime + RealTime::frame2RealTime(m_blockSize,
						       m_sampleRate);

    rec.filledTo = bufferTime;
    return;
}

void
AudioMixer::kick(bool wantLock)
{
    if (wantLock) getLock();

//    Rosegarden::Profiler profiler("AudioMixer::kick");
    
    bool discard;
    processBlocks(false, discard);
    m_fileReader->signal();
    m_fileReader->updateDefunctStatuses();

    if (wantLock) releaseLock();
}


void *
AudioMixer::threadRun(void *arg)
{
    AudioMixer *inst = static_cast<AudioMixer *>(arg);
    if (!inst) return 0;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    inst->getLock();

    while (1) {

	bool waitingForFiles = false;

	if (inst->m_driver->areClocksRunning()) {
	    inst->processBlocks(false, waitingForFiles);
	    inst->m_fileReader->signal();
	    inst->m_fileReader->updateDefunctStatuses();
//	    inst->kick(false, waitingForFiles);
	}

//	if (waitingForFiles) continue;

	RealTime t = inst->m_driver->getAudioMixBufferLength();
	t = t / 2;

//	if (t > RealTime(0, 20000000)) t = RealTime(0, 20000000);

//	int sleepusec = bufferLength.usec() + bufferLength.sec * 1000000;
//	sleepusec /= 4;
//	if (sleepusec > 20000) sleepusec = 20000;

	//!!! actually this is probably too demanding... we probably
	// only want to do this every occasionally as before... restore
	// a nanosleep or something here? ... but not in write thread.
	
	struct timeval now;
	gettimeofday(&now, 0);
	t = t + RealTime(now.tv_sec, now.tv_usec * 1000);

	struct timespec timeout;
	timeout.tv_sec = t.sec;
	timeout.tv_nsec = t.nsec;
	pthread_cond_timedwait(&inst->m_condition, &inst->m_lock, &timeout);

    }

    inst->releaseLock();
    return 0;
}



AudioFileReader::AudioFileReader(SoundDriver *driver,
				 unsigned int sampleRate) :
    m_driver(driver),
    m_sampleRate(sampleRate),
    m_thread(0)
{
    pthread_mutex_t  initialisingMutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&m_lock, &initialisingMutex, sizeof(pthread_mutex_t));

    pthread_cond_t initialisingCondition = PTHREAD_COND_INITIALIZER;
    memcpy(&m_condition, &initialisingCondition, sizeof(pthread_cond_t));

    pthread_create(&m_thread, NULL, threadRun, this);
    pthread_detach(m_thread);
}

AudioFileReader::~AudioFileReader()
{
    std::cerr << "AudioFileReader::~AudioFileReader" << std::endl;

    if (m_thread) {
	pthread_cancel(m_thread);
	m_thread = 0;
    }
    pthread_mutex_destroy(&m_lock);

    std::cerr << "AudioFileReader::~AudioFileReader exiting" << std::endl;
}

int
AudioFileReader::getLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioFileReader::getLock()" << std::endl;
#endif
    rv = pthread_mutex_lock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK" << std::endl;
#endif
    return rv;
}

int
AudioFileReader::tryLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioFileReader::tryLock()" << std::endl;
#endif
    rv = pthread_mutex_trylock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK (rv is " << rv << ")" << std::endl;
#endif
    return rv;
}

int
AudioFileReader::releaseLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioFileReader::releaseLock()" << std::endl;
#endif
    rv = pthread_mutex_unlock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK" << std::endl;
#endif
    return rv;
}

void
AudioFileReader::signal()
{
#ifdef DEBUG_LOCKS
    std::cerr << "AudioFileReader::signal()" << std::endl;
#endif
    pthread_cond_signal(&m_condition);
}


void
AudioFileReader::updateReadyStatuses(PlayableAudioFileList &audioQueue)
{
    getLock();

    for (PlayableAudioFileList::iterator it = audioQueue.begin();
	 it != audioQueue.end(); ++it) {

	PlayableAudioFile *file = *it;

	if (file->getStatus() == PlayableAudioFile::READY) {
#ifdef DEBUG_READER
	    std::cerr << "AudioFileReader::updateReadyStatuses: found a READY file, asking it to buffer" << std::endl;
#endif
	    if (file->fillBuffers(file->getReadyTime())) {
#ifdef DEBUG_READER
		std::cerr << "AudioFileReader::updateReadyStatuses: (it did)" << std::endl;
#endif
		file->setStatus(PlayableAudioFile::PLAYING);
	    }
	}
    }

    releaseLock();
}

void
AudioFileReader::updateDefunctStatuses()
{
    PlayableAudioFileList audioQueue = m_driver->getAudioPlayQueueNotDefunct();

    bool someDefunct = false;
	
    for (PlayableAudioFileList::iterator it = audioQueue.begin();
	 it != audioQueue.end(); ++it) {

	PlayableAudioFile *file = *it;
	if (file->getStatus() == PlayableAudioFile::DEFUNCT) {
	    someDefunct = true;
	    continue;
	}

	if (file->isFinished()) {
#ifdef DEBUG_READER
	    std::cerr << "AudioFileReader::updateDefunctStatuses: setting a finished file to defunct" << std::endl;
#endif
	    file->setStatus(PlayableAudioFile::DEFUNCT);
	    someDefunct = true;
	}
    }

    if (someDefunct) {
	getLock();
#ifdef DEBUG_READER
	std::cerr << "AudioFileReader::updateDefunctStatuses: found some defunct, clearing" << std::endl;
#endif
	m_driver->clearDefunctFromAudioPlayQueue();
	releaseLock();
    }
}

bool
AudioFileReader::kick(bool wantLock)
{
    if (wantLock) getLock();

//    Rosegarden::Profiler profiler("AudioFileReader::kick");

    PlayableAudioFileList audioQueue;
    PlayableAudioFileList::iterator it;
	
    audioQueue = m_driver->getAudioPlayQueueNotDefunct();

    bool someFilled = false;
	
    for (it = audioQueue.begin(); it != audioQueue.end(); ++it)
    {
	PlayableAudioFile *file = *it;

	if (file->getStatus() == PlayableAudioFile::PLAYING) {

#ifdef DEBUG_READER
	    std::cerr << "AudioFileReader::kick: found a PLAYING file to update" << std::endl;
#endif
	    file->updateBuffers();

	} else if (file->getStatus() == PlayableAudioFile::READY) {
#ifdef DEBUG_READER
	    std::cerr << "AudioFileReader::kick: found a READY file (ready time " << file->getReadyTime() << "), asking it to buffer" << std::endl;
#endif
	    if (file->fillBuffers(file->getReadyTime())) {
#ifdef DEBUG_READER
		std::cerr << "AudioFileReader::kick: (it did)" << std::endl;
#endif
		file->setStatus(PlayableAudioFile::PLAYING);
		someFilled = true;
	    }
	}
    }
    
    if (wantLock) releaseLock();

    return someFilled;
}

void *
AudioFileReader::threadRun(void *arg)
{
    AudioFileReader *inst = static_cast<AudioFileReader*>(arg);
    if (!inst) return 0;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    inst->getLock();

    while (1) {

	inst->kick(false);
//	if (!inst->kick(false)) {

	    RealTime t = inst->m_driver->getAudioReadBufferLength();
	    t = t / 2;

	    struct timeval now;
	    gettimeofday(&now, 0);
	    t = t + RealTime(now.tv_sec, now.tv_usec * 1000);

	    struct timespec timeout;
	    timeout.tv_sec = t.sec;
	    timeout.tv_nsec = t.nsec;
	    pthread_cond_timedwait(&inst->m_condition, &inst->m_lock, &timeout);
//	}
    }

    inst->releaseLock();

    return 0;
}



AudioFileWriter::AudioFileWriter(SoundDriver *driver,
				 unsigned int sampleRate) :
    m_driver(driver),
    m_sampleRate(sampleRate),
    m_thread(0)
{
    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);
    
    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instrumentCount; ++id) {

	// prefill with zero files in all slots, so that we can
	// refer to the map without a lock (as the number of
	// instruments won't change)
	
	m_files[id] = FilePair(0, 0);
    }

    pthread_mutex_t  initialisingMutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&m_lock, &initialisingMutex, sizeof(pthread_mutex_t));

    pthread_cond_t initialisingCondition = PTHREAD_COND_INITIALIZER;
    memcpy(&m_condition, &initialisingCondition, sizeof(pthread_cond_t));

    pthread_create(&m_thread, NULL, threadRun, this);
    pthread_detach(m_thread);
}

AudioFileWriter::~AudioFileWriter()
{
    std::cerr << "AudioFileWriter::~AudioFileWriter" << std::endl;

    if (m_thread) {
	pthread_cancel(m_thread);
	m_thread = 0;
    }
    pthread_mutex_destroy(&m_lock);

    std::cerr << "AudioFileWriter::~AudioFileWriter exiting" << std::endl;
}

int
AudioFileWriter::getLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioFileWriter::getLock()" << std::endl;
#endif
    rv = pthread_mutex_lock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK" << std::endl;
#endif
    return rv;
    return pthread_mutex_lock(&m_lock);
}

int
AudioFileWriter::tryLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioFileWriter::tryLock()" << std::endl;
#endif
    rv = pthread_mutex_trylock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK (rv is " << rv << ")" << std::endl;
#endif
    return rv;
    return pthread_mutex_trylock(&m_lock);
}

int
AudioFileWriter::releaseLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << "AudioFileWriter::releaseLock()" << std::endl;
#endif
    rv = pthread_mutex_unlock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK" << std::endl;
#endif
    return rv;
}

void
AudioFileWriter::signal()
{
#ifdef DEBUG_LOCKS
    std::cerr << "AudioFileWriter::signal()" << std::endl;
#endif
    pthread_cond_signal(&m_condition);
}


bool
AudioFileWriter::createRecordFile(InstrumentId id,
				  const std::string &fileName)
{
    getLock();

    if (m_files[id].first) {
	releaseLock();
	return false; // already have one
    }
    
    MappedAudioFader *fader = m_driver->getMappedStudio()->getAudioFader
	(m_driver->getAudioMonitoringInstrument());

    RealTime bufferLength = m_driver->getAudioWriteBufferLength();
    int bufferSamples = RealTime::realTime2Frame(bufferLength, m_sampleRate);
    bufferSamples = ((bufferSamples / 1024) + 1) * 1024;

    if (fader)
    {
	float fch = 2;
	(void)fader->getProperty(MappedAudioFader::Channels, fch);
        int channels = (int)fch;

        int bytesPerSample = 2 * channels;
        int bitsPerSample = 16;

        AudioFile *recordFile =
            new WAVAudioFile(fileName,
                             channels,             // channels
                             m_sampleRate,      // samples per second
                             m_sampleRate *
                                  bytesPerSample,  // bytes per second
                             bytesPerSample,       // bytes per sample
                             bitsPerSample);       // bits per sample

        // open the file for writing
        //
	if (!recordFile->write()) {
	    delete recordFile;
	    releaseLock();
	    return false;
	}

	RecordableAudioFile *raf = new RecordableAudioFile(recordFile,
							   bufferSamples);
	m_files[id].second = raf;
	m_files[id].first = recordFile;
	releaseLock();
	return true;
    }

    releaseLock();
    return false;
}	


void
AudioFileWriter::write(InstrumentId id,
		       const sample_t *samples,
		       int channel,
		       size_t sampleCount)
{
    if (!m_files[id].first) return; // no file
    m_files[id].second->buffer(samples, channel, sampleCount);
}

bool
AudioFileWriter::closeRecordFile(InstrumentId id, AudioFileId &returnedId)
{
    if (!m_files[id].first) return false;

    returnedId = m_files[id].first->getId();
    m_files[id].second->setStatus(RecordableAudioFile::DEFUNCT);
#ifdef DEBUG_WRITER
    std::cerr << "AudioFileWriter::closeRecordFile: instrument " << id << " file set defunct" << std::endl;
#endif
    return true;
}
    

void
AudioFileWriter::kick(bool wantLock)
{
    if (wantLock) getLock();

//    Rosegarden::Profiler profiler("AudioFileWriter::kick");

    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);
    
    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instrumentCount; ++id) {

	if (!m_files[id].first) continue;

	RecordableAudioFile *raf = m_files[id].second;

	if (raf->getStatus() == RecordableAudioFile::DEFUNCT) {

#ifdef DEBUG_WRITER
	    std::cerr << "AudioFileWriter::kick: found defunct file on instrument " << id << std::endl;
#endif

	    m_files[id].first = 0;
	    delete raf; // also deletes the AudioFile
	    m_files[id].second = 0;

	} else {
#ifdef DEBUG_WRITER
	    std::cerr << "AudioFileWriter::kick: writing file on instrument " << id << std::endl;
#endif

	    raf->write();
	}
    }
    
    if (wantLock) releaseLock();
}

void *
AudioFileWriter::threadRun(void *arg)
{
    AudioFileWriter *inst = static_cast<AudioFileWriter*>(arg);
    if (!inst) return 0;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    inst->getLock();

    while (1) {

	inst->kick(false);

	RealTime t = inst->m_driver->getAudioWriteBufferLength();
	t = t / 2;
	
	struct timeval now;
	gettimeofday(&now, 0);
	t = t + RealTime(now.tv_sec, now.tv_usec * 1000);

	struct timespec timeout;
	timeout.tv_sec = t.sec;
	timeout.tv_nsec = t.nsec;
	pthread_cond_timedwait(&inst->m_condition, &inst->m_lock, &timeout);
    }

    inst->releaseLock();

    return 0;
}


}

