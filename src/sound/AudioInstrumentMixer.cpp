/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AudioInstrumentMixer.h"

#include "RunnablePluginInstance.h"
#include "PlayableAudioFile.h"
#include "MappedStudio.h"  // MappedAudioFader
#include "base/AudioLevel.h"
#include "AudioPlayQueue.h"
#include "PluginFactory.h"
#include "ControlBlock.h"

#include <sys/time.h>
#include <pthread.h>

#include <algorithm>  // std::max()

#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <alloca.h>
#endif

//#define DEBUG_MIXER 1
//#define DEBUG_MIXER_LIGHTWEIGHT 1

typedef float sample_t;


namespace Rosegarden
{


/* Branch-free optimizer-resistant denormal killer courtesy of Simon
   Jenkins on LAD: */

static inline float flushToZero(volatile float f)
{
    f += 9.8607615E-32f;
    return f - 9.8607615E-32f;
}

static inline void denormalKill(float *buffer, int size)
{
    for (int i = 0; i < size; ++i) {
        buffer[i] = flushToZero(buffer[i]);
    }
}


static AudioInstrumentMixer *aimInstance{nullptr};

AudioInstrumentMixer::AudioInstrumentMixer(SoundDriver *driver,
        AudioFileReader *fileReader,
        unsigned int sampleRate,
        unsigned int blockSize) :
        AudioThread("AudioInstrumentMixer", driver, sampleRate),
        m_fileReader(fileReader),
        m_bussMixer(nullptr),
        m_blockSize(blockSize),
        m_numSoftSynths(0)
{
    // Keep track of the instance globally.  See getInstance().
    aimInstance = this;

    // Pregenerate empty plugin slots

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

        InstrumentId id;
        if (i < audioInstruments)
            id = audioInstrumentBase + i;
        else
            id = synthInstrumentBase + (i - audioInstruments);

        PluginList &list = m_plugins[id];
        for (int j = 0; j < int(Instrument::PLUGIN_COUNT); ++j) {
            list.push_back(nullptr);
        }

        if (i >= audioInstruments) {
            m_synths[id] = nullptr;
        }
    }

    // Leave the buffer map and process buffer list empty for now.
    // The buffer length can change between plays, so we always
    // examine the buffers in fillBuffers and are prepared to
    // regenerate from scratch if necessary.  Don't like it though.
}

AudioInstrumentMixer *
AudioInstrumentMixer::getInstance()
{
    return aimInstance;
}

AudioInstrumentMixer::~AudioInstrumentMixer()
{
    aimInstance = nullptr;

    //std::cerr << "AudioInstrumentMixer::~AudioInstrumentMixer" << std::endl;
    // BufferRec dtor will handle the BufferMap

    removeAllPlugins();

    for (auto& pair : m_processBuffers) {
        auto& pBuf = pair.second;
        for (std::vector<sample_t *>::iterator i = pBuf.begin();
             i != pBuf.end(); ++i) {
            delete[] *i;
        }
    }

    //std::cerr << "AudioInstrumentMixer::~AudioInstrumentMixer exiting" << std::endl;
}

AudioInstrumentMixer::BufferRec::~BufferRec()
{
    for (size_t i = 0; i < buffers.size(); ++i)
        delete buffers[i];
}


void
AudioInstrumentMixer::setPlugin(InstrumentId id, int position, QString identifier)
{
    // Not RT safe

    //std::cerr << "AudioInstrumentMixer::setPlugin(" << id << ", " << position << ", " << identifier << ")" << std::endl;

    int channels = 2;
    if (m_bufferMap.find(id) != m_bufferMap.end()) {
        channels = m_bufferMap[id].channels;
    }

    RunnablePluginInstance *instance = nullptr;

    PluginFactory *factory = PluginFactory::instanceFor(identifier);
    if (factory) {
        instance = factory->instantiatePlugin(identifier,
                                              id,
                                              position,
                                              m_sampleRate,
                                              m_blockSize,
                                              channels,
                                              this);
        if (instance && !instance->isOK()) {
            std::cerr << "AudioInstrumentMixer::setPlugin(" << id << ", " << position
                      << ": instance is not OK" << std::endl;
            delete instance;
            instance = nullptr;
        }
    } else {
        std::cerr << "AudioInstrumentMixer::setPlugin: No factory for identifier "
                  << identifier << std::endl;
    }

    RunnablePluginInstance *oldInstance = nullptr;

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {

        oldInstance = m_synths[id];
        m_synths[id] = instance;
        if (! oldInstance) m_numSoftSynths++;

    } else {

        PluginList &list = m_plugins[id];

        if (position < int(Instrument::PLUGIN_COUNT)) {
            while (position >= (int)list.size()) {
                list.push_back(nullptr);
            }
            oldInstance = list[position];
            list[position] = instance;
        } else {
            std::cerr << "AudioInstrumentMixer::setPlugin: No position "
            << position << " for instrument " << id << std::endl;
            delete instance;
        }
    }

    if (oldInstance) {
        m_driver->claimUnwantedPlugin(oldInstance);
    }
}

void
AudioInstrumentMixer::removePlugin(InstrumentId id, int position)
{
    // Not RT safe

    //std::cerr << "AudioInstrumentMixer::removePlugin(" << id << ", " << position << ")" << std::endl;

    RunnablePluginInstance *oldInstance = nullptr;

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {

        if (m_synths[id]) {
            oldInstance = m_synths[id];
            m_synths[id] = nullptr;
            m_numSoftSynths--;
        }

    } else {

        PluginList &list = m_plugins[id];
        if (position < (int)list.size()) {
            oldInstance = list[position];
            list[position] = nullptr;
        }
    }

    if (oldInstance) {
        m_driver->claimUnwantedPlugin(oldInstance);
    }
}

void
AudioInstrumentMixer::removeAllPlugins()
{
    // Not RT safe

    //std::cerr << "AudioInstrumentMixer::removeAllPlugins" << std::endl;

    for (SynthPluginMap::iterator i = m_synths.begin();
            i != m_synths.end(); ++i) {
        if (i->second) {
            RunnablePluginInstance *instance = i->second;
            i->second = nullptr;
            m_driver->claimUnwantedPlugin(instance);
        }
    }
    m_numSoftSynths = 0;

    for (PluginMap::iterator j = m_plugins.begin();
            j != m_plugins.end(); ++j) {

        PluginList &list = j->second;

        for (PluginList::iterator i = list.begin(); i != list.end(); ++i) {
            RunnablePluginInstance *instance = *i;
            *i = nullptr;
            m_driver->claimUnwantedPlugin(instance);
        }
    }
}


RunnablePluginInstance *
AudioInstrumentMixer::getPluginInstance(InstrumentId id, int position)
{
    // Not RT safe
    // ??? What does that mean?  This is a reasonably fast routine.
    //     It only does a single map search and a single array fetch.
    //     It is also likely thread safe.  m_synths and m_plugins
    //     stay the same size.  The pointers they contain might get
    //     destroyed, but it is likely that they stop being used prior
    //     to destruction.

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {
        return m_synths[id];
    } else {
        PluginList &list = m_plugins[id];
        if (position < int(list.size()))
            return list[position];
    }
    return nullptr;
}


void
AudioInstrumentMixer::setPluginPortValue(InstrumentId id, int position,
                                         unsigned int port, float value)
{
    // Not RT safe

    RunnablePluginInstance *instance = getPluginInstance(id, position);

    if (instance) {
        instance->setPortValue(port, value);
    }
}

float
AudioInstrumentMixer::getPluginPortValue(InstrumentId id, int position,
        unsigned int port)
{
    // Not RT safe

    RunnablePluginInstance *instance = getPluginInstance(id, position);

    if (instance) {
        return instance->getPortValue(port);
    }

    return 0;
}

void
AudioInstrumentMixer::setPluginBypass(InstrumentId id, int position, bool bypass)
{
    // Not RT safe

    RunnablePluginInstance *instance = getPluginInstance(id, position);
    if (instance)
        instance->setBypassed(bypass);
}

QStringList
AudioInstrumentMixer::getPluginPrograms(InstrumentId id, int position)
{
    // Not RT safe

    QStringList programs;
    RunnablePluginInstance *instance = getPluginInstance(id, position);
    if (instance)
        programs = instance->getPrograms();
    return programs;
}

QString
AudioInstrumentMixer::getPluginProgram(InstrumentId id, int position)
{
    // Not RT safe

    QString program;
    RunnablePluginInstance *instance = getPluginInstance(id, position);
    if (instance)
        program = instance->getCurrentProgram();
    return program;
}

QString
AudioInstrumentMixer::getPluginProgram(InstrumentId id, int position, int bank,
                                       int program)
{
    // Not RT safe

    QString programName;
    RunnablePluginInstance *instance = getPluginInstance(id, position);
    if (instance)
        programName = instance->getProgram(bank, program);
    return programName;
}

unsigned long
AudioInstrumentMixer::getPluginProgram(InstrumentId id, int position, QString name)
{
    // Not RT safe

    unsigned long program = 0;
    RunnablePluginInstance *instance = getPluginInstance(id, position);
    if (instance)
        program = instance->getProgram(name);
    return program;
}

void
AudioInstrumentMixer::setPluginProgram(InstrumentId id, int position, QString program)
{
    // Not RT safe

    RunnablePluginInstance *instance = getPluginInstance(id, position);
    if (instance)
        instance->selectProgram(program);
}

QString
AudioInstrumentMixer::configurePlugin(InstrumentId id, int position, QString key, QString value)
{
    // Not RT safe

    RunnablePluginInstance *instance = getPluginInstance(id, position);
    if (instance)
        return instance->configure(key, value);
    return QString();
}

void AudioInstrumentMixer::savePluginState()
{
    for (SynthPluginMap::iterator j = m_synths.begin();
         j != m_synths.end(); ++j) {

        RunnablePluginInstance *instance = j->second;
        if (instance) instance->savePluginState();
    }

    for (PluginMap::iterator j = m_plugins.begin();
         j != m_plugins.end(); ++j) {

        InstrumentId id = j->first;

        for (PluginList::iterator i = m_plugins[id].begin();
	     i != m_plugins[id].end(); ++i) {

            RunnablePluginInstance *instance = *i;
	    if (instance) instance->savePluginState();
        }
    }
}

void AudioInstrumentMixer::getPluginPlayableAudio
(std::vector<PlayableData*>& playable)
{
    playable.clear();
    for (SynthPluginMap::iterator j = m_synths.begin();
         j != m_synths.end(); ++j) {

        RunnablePluginInstance *instance = j->second;
        if (instance) instance->getPluginPlayableAudio(playable);
    }

    for (PluginMap::iterator j = m_plugins.begin();
         j != m_plugins.end(); ++j) {

        InstrumentId id = j->first;

        for (PluginList::iterator i = m_plugins[id].begin();
             i != m_plugins[id].end(); ++i) {

            RunnablePluginInstance *instance = *i;
            if (instance) instance->getPluginPlayableAudio(playable);
        }
    }
}


void
AudioInstrumentMixer::discardPluginEvents()
{
    getLock();
    if (m_bussMixer) m_bussMixer->getLock();

    for (SynthPluginMap::iterator j = m_synths.begin();
            j != m_synths.end(); ++j) {

        RunnablePluginInstance *instance = j->second;
        if (instance) instance->discardEvents();
    }

    for (PluginMap::iterator j = m_plugins.begin();
            j != m_plugins.end(); ++j) {

        InstrumentId id = j->first;

        for (PluginList::iterator i = m_plugins[id].begin();
	     i != m_plugins[id].end(); ++i) {

            RunnablePluginInstance *instance = *i;
	    if (instance) instance->discardEvents();
        }
    }

    if (m_bussMixer) m_bussMixer->releaseLock();
    releaseLock();
}

void
AudioInstrumentMixer::resetAllPlugins(bool discardEvents)
{
    // Not RT safe

    // lock required here to protect against calling
    // activate/deactivate at the same time as run()

#ifdef DEBUG_MIXER
    std::cerr << "AudioInstrumentMixer::resetAllPlugins!" << std::endl;
    if (discardEvents) std::cerr << "(discardEvents true)" << std::endl;
#endif

    getLock();
    if (m_bussMixer)
        m_bussMixer->getLock();

    for (SynthPluginMap::iterator j = m_synths.begin();
            j != m_synths.end(); ++j) {

        InstrumentId id = j->first;

        int channels = 2;
        if (m_bufferMap.find(id) != m_bufferMap.end()) {
            channels = m_bufferMap[id].channels;
        }

        RunnablePluginInstance *instance = j->second;

        if (instance) {
#ifdef DEBUG_MIXER
            std::cerr << "AudioInstrumentMixer::resetAllPlugins: (re)setting " << channels << " channels on synth for instrument " << id << std::endl;
#endif

            if (discardEvents)
                instance->discardEvents();
            instance->setIdealChannelCount(channels);
        }
    }

    for (PluginMap::iterator j = m_plugins.begin();
            j != m_plugins.end(); ++j) {

        InstrumentId id = j->first;

        int channels = 2;
        if (m_bufferMap.find(id) != m_bufferMap.end()) {
            channels = m_bufferMap[id].channels;
        }

        for (PluginList::iterator i = m_plugins[id].begin();
                i != m_plugins[id].end(); ++i) {

            RunnablePluginInstance *instance = *i;

            if (instance) {
#ifdef DEBUG_MIXER
                std::cerr << "AudioInstrumentMixer::resetAllPlugins: (re)setting " << channels << " channels on plugin for instrument " << id << std::endl;
#endif

                if (discardEvents)
                    instance->discardEvents();
                instance->setIdealChannelCount(channels);
            }
        }
    }

    if (m_bussMixer)
        m_bussMixer->releaseLock();
    releaseLock();
}

void
AudioInstrumentMixer::destroyAllPlugins()
{
    // Not RT safe

    getLock();
    if (m_bussMixer)
        m_bussMixer->getLock();

    // Delete immediately, as we're probably exiting here -- don't use
    // the scavenger.

    //std::cerr << "AudioInstrumentMixer::destroyAllPlugins" << std::endl;

    for (SynthPluginMap::iterator j = m_synths.begin();
            j != m_synths.end(); ++j) {
        RunnablePluginInstance *instance = j->second;
        j->second = nullptr;
        delete instance;
    }
    m_numSoftSynths = 0;

    for (PluginMap::iterator j = m_plugins.begin();
            j != m_plugins.end(); ++j) {

        InstrumentId id = j->first;

        for (PluginList::iterator i = m_plugins[id].begin();
                i != m_plugins[id].end(); ++i) {

            RunnablePluginInstance *instance = *i;
            *i = nullptr;
            delete instance;
        }
    }

    // and tell the driver to get rid of anything already scavenged.
    m_driver->scavengePlugins();

    if (m_bussMixer)
        m_bussMixer->releaseLock();
    releaseLock();
}

size_t
AudioInstrumentMixer::getPluginLatency(unsigned int id)
{
    // Not RT safe

    size_t latency = 0;

    RunnablePluginInstance *synth = m_synths[id];
    if (synth)
        latency += m_synths[id]->getLatency();

    for (PluginList::iterator i = m_plugins[id].begin();
            i != m_plugins[id].end(); ++i) {
        RunnablePluginInstance *plugin = *i;
        if (plugin)
            latency += plugin->getLatency();
    }

    return latency;
}

void
AudioInstrumentMixer::generateBuffers()
{
    // Not RT safe

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    unsigned int maxChannels = 0;

    size_t bufferSamples = m_blockSize;

    if (!m_driver->getLowLatencyMode()) {
        RealTime bufferLength = m_driver->getAudioMixBufferLength();
        size_t bufferSamples = (size_t)RealTime::realTime2Frame(bufferLength, m_sampleRate);
        bufferSamples = ((bufferSamples / m_blockSize) + 1) * m_blockSize;
#ifdef DEBUG_MIXER

        std::cerr << "AudioInstrumentMixer::generateBuffers: Buffer length is " << bufferLength << "; buffer samples " << bufferSamples << " (sample rate " << m_sampleRate << ")" << std::endl;
#endif

    }

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

        InstrumentId id;
        if (i < audioInstruments)
            id = audioInstrumentBase + i;
        else
            id = synthInstrumentBase + (i - audioInstruments);

        // Get a fader for this instrument - if we can't then this
        // isn't a valid audio track.
        MappedAudioFader *fader = m_driver->getMappedStudio()->getAudioFader(id);

        if (!fader) {
#ifdef DEBUG_MIXER
            std::cerr << "AudioInstrumentMixer::generateBuffers: no fader for audio instrument " << id << std::endl;
#endif

            continue;
        }

        float fch = 2;
        (void)fader->getProperty(MappedAudioFader::Channels, fch);
        unsigned int channels = (unsigned int)fch;

        BufferRec &rec = m_bufferMap[id];

        rec.channels = channels;

        // We always have stereo buffers (for output of pan)
        // even on a mono instrument.
        if (channels < 2)
            channels = 2;
        if (channels > maxChannels)
            maxChannels = channels;

        bool replaceBuffers = (rec.buffers.size() > (size_t)channels);

        if (!replaceBuffers) {
            for (size_t i = 0; i < rec.buffers.size(); ++i) {
                if (rec.buffers[i]->getSize() != bufferSamples) {
                    replaceBuffers = true;
                    break;
                }
            }
        }

        if (replaceBuffers) {
            for (size_t i = 0; i < rec.buffers.size(); ++i) {
                delete rec.buffers[i];
            }
            rec.buffers.clear();
        }

        while (rec.buffers.size() < (size_t)channels) {

            // All our ringbuffers are set up for two readers: the
            // buss mix thread and the main process thread for
            // e.g. JACK.  The main process thread gets the zero-id
            // reader, so it gets the same API as if this was a
            // single-reader buffer; the buss mixer has to remember to
            // explicitly request reader 1.

            RingBuffer<sample_t, 2> *rb =
                new RingBuffer<sample_t, 2>(bufferSamples);

            if (!rb->mlock()) {
                //		std::cerr << "WARNING: AudioInstrumentMixer::generateBuffers: couldn't lock ring buffer into real memory, performance may be impaired" << std::endl;
            }
            rec.buffers.push_back(rb);
        }

        float level = 0.0;
        (void)fader->getProperty(MappedAudioFader::FaderLevel, level);

        float pan = 0.0;
        (void)fader->getProperty(MappedAudioFader::Pan, pan);

        setInstrumentLevels(id, level, pan);

        ProcessBufferType &pBuf = m_processBuffers[id];
        while ((unsigned int)pBuf.size() > channels) {
            std::vector<sample_t *>::iterator bi = pBuf.end();
            --bi;
            delete[] *bi;
            pBuf.erase(bi);
        }
        while ((unsigned int)pBuf.size() < channels) {
            pBuf.push_back(new sample_t[m_blockSize]);
        }

    }

    // Make room for up to 16 busses here, to avoid reshuffling later
    int busses = 16;
    if (m_bussMixer)
        busses = std::max(busses, m_bussMixer->getBussCount());
    for (int i = 0; i < busses; ++i) {
        PluginList &list = m_plugins[i + 1];
        while ((unsigned int)list.size() < Instrument::PLUGIN_COUNT) {
            list.push_back(nullptr);
        }
    }

}

void
AudioInstrumentMixer::fillBuffers(const RealTime &currentTime)
{
    // Not RT safe

    emptyBuffers(currentTime);

    getLock();

#ifdef DEBUG_MIXER

    std::cerr << "AudioInstrumentMixer::fillBuffers(" << currentTime << ")" << std::endl;
#endif

    bool discard;
    processBlocks(discard);

    releaseLock();
}

void
AudioInstrumentMixer::allocateBuffers()
{
    // Not RT safe

    getLock();

#ifdef DEBUG_MIXER

    std::cerr << "AudioInstrumentMixer::allocateBuffers()" << std::endl;
#endif

    generateBuffers();

    releaseLock();
}

void
AudioInstrumentMixer::emptyBuffers(RealTime currentTime)
{
    // Not RT safe

    getLock();

#ifdef DEBUG_MIXER

    std::cerr << "AudioInstrumentMixer::emptyBuffers(" << currentTime << ")" << std::endl;
#endif

    generateBuffers();

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

        InstrumentId id;
        if (i < audioInstruments)
            id = audioInstrumentBase + i;
        else
            id = synthInstrumentBase + (i - audioInstruments);

        m_bufferMap[id].dormant = true;
        m_bufferMap[id].muted = false;
        m_bufferMap[id].zeroFrames = 0;
        m_bufferMap[id].filledTo = currentTime;

        for (size_t i = 0; i < m_bufferMap[id].buffers.size(); ++i) {
            m_bufferMap[id].buffers[i]->reset();
        }
    }

    releaseLock();
}

void
AudioInstrumentMixer::setInstrumentLevels(InstrumentId id, float dB, float pan)
{
    // No requirement to be RT safe

    BufferRec &rec = m_bufferMap[id];

    float volume = AudioLevel::dB_to_multiplier(dB);

//  rec.gainLeft = volume * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);
//  rec.gainRight = volume * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);

    // Apply panning law.
    rec.gainLeft = volume * AudioLevel::panGainLeft(pan);
    rec.gainRight = volume * AudioLevel::panGainRight(pan);
    rec.volume = volume;
}

void
AudioInstrumentMixer::updateInstrumentMuteStates()
{
    ControlBlock *cb = ControlBlock::getInstance();

    for (BufferMap::iterator i = m_bufferMap.begin();
	 i != m_bufferMap.end(); ++i) {

	InstrumentId id = i->first;
	BufferRec &rec = i->second;

	if (id >= SoftSynthInstrumentBase) {
	    rec.muted = cb->isInstrumentMuted(id);
	} else {
	    rec.muted = cb->isInstrumentUnused(id);
	}
    }
}

void AudioInstrumentMixer::audioProcessingDone()
{
    for (SynthPluginMap::iterator i = m_synths.begin();
         i != m_synths.end(); ++i) {
        if (i->second) {
            RunnablePluginInstance *instance = i->second;
            if (instance) instance->audioProcessingDone();
        }
    }

    for (PluginMap::iterator j = m_plugins.begin();
         j != m_plugins.end(); ++j) {

        PluginList &list = j->second;

        for (PluginList::iterator i = list.begin(); i != list.end(); ++i) {
            RunnablePluginInstance *instance = *i;
            if (instance) instance->audioProcessingDone();
        }
    }
}

sample_t* AudioInstrumentMixer::getAudioBuffer
(InstrumentId id, unsigned int channel) const
{
    auto it = m_processBuffers.find(id);
    if (it == m_processBuffers.end()) return nullptr;

    auto& pBuf = (*it).second;
    if (channel >= pBuf.size()) return nullptr;

    return pBuf[channel];
}

void
AudioInstrumentMixer::processBlocks(bool &readSomething)
{
    // Needs to be RT safe

#ifdef DEBUG_MIXER
    if (m_driver->isPlaying())
        std::cerr << "AudioInstrumentMixer::processBlocks" << std::endl;
#endif

    const AudioPlayQueue *queue = m_driver->getAudioQueue();

    for (BufferMap::iterator i = m_bufferMap.begin();
            i != m_bufferMap.end(); ++i) {

        InstrumentId id = i->first;
        BufferRec &rec = i->second;

        // This "muted" flag actually only strictly means muted when
        // applied to synth instruments.  For audio instruments it's
        // only true if the instrument is not in use at all (see
        // updateInstrumentMuteStates above).  It's not safe to base
        // the empty calculation on muted state for audio tracks,
        // because that causes buffering problems when the mute is
        // toggled for an audio track while it's playing a file.

        bool empty = false;

        if (rec.muted) {
            empty = true;
        } else {
            if (id >= SoftSynthInstrumentBase) {
                empty = (!m_synths[id] || m_synths[id]->isBypassed());
            } else {
                empty = !queue->haveFilesForInstrument(id);
            }

            if (empty) {
                for (PluginList::iterator j = m_plugins[id].begin();
                        j != m_plugins[id].end(); ++j) {
                    if (*j != nullptr) {
                        empty = false;
                        break;
                    }
                }
            }
        }

        if (!empty && rec.empty) {

            // This instrument is becoming freshly non-empty.  We need
            // to set its filledTo field to match that of an existing
            // non-empty instrument, if we can find one.

            for (BufferMap::iterator j = m_bufferMap.begin();
                    j != m_bufferMap.end(); ++j) {

                if (j->first == i->first)
                    continue;
                if (j->second.empty)
                    continue;

                rec.filledTo = j->second.filledTo;
                break;
            }
        }

        rec.empty = empty;

        // For a while we were setting empty to true if the volume on
        // the track was zero, but that breaks continuity if there is
        // actually a file on the track -- processEmptyBlocks won't
        // read it, so it'll fall behind if we put the volume up again.
    }

    bool more = true;

    static const int MAX_FILES_PER_INSTRUMENT = 500;
    static PlayableData *playing[MAX_FILES_PER_INSTRUMENT];

    RealTime blockDuration = RealTime::frame2RealTime(m_blockSize, m_sampleRate);

    while (more) {

        more = false;

        for (BufferMap::iterator i = m_bufferMap.begin();
                i != m_bufferMap.end(); ++i) {

            InstrumentId id = i->first;
            BufferRec &rec = i->second;

            if (rec.empty) {
                rec.dormant = true;
                continue;
            }

            size_t playCount = MAX_FILES_PER_INSTRUMENT;

            if (id >= SoftSynthInstrumentBase)
                playCount = 0;
            else {
                queue->getPlayingFilesForInstrument(rec.filledTo,
                                                    blockDuration, id,
                                                    playing, playCount);
            }

            if (processBlock(id, playing, playCount, readSomething)) {
                more = true;
            }
        }
    }
}

bool
AudioInstrumentMixer::processBlock(InstrumentId id,
                                   PlayableData **playing,
                                   size_t playCount,
                                   bool &readSomething)
{
    // Needs to be RT safe

    BufferRec &rec = m_bufferMap[id];
    ProcessBufferType &pBuf = m_processBuffers[id];
    RealTime bufferTime = rec.filledTo;

#ifdef DEBUG_MIXER
    //    if (m_driver->isPlaying()) {
    if ((id % 100) == 0)
        std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): buffer time is " << bufferTime << std::endl;
    //    }
#endif

    unsigned int channels = rec.channels;
    if (channels > (unsigned int)rec.buffers.size())
        channels = (unsigned int)rec.buffers.size();
    if (channels > (unsigned int)pBuf.size())
        channels = (unsigned int)pBuf.size();
    if (channels == 0) {
#ifdef DEBUG_MIXER
        if ((id % 100) == 0)
            std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): nominal channels " << rec.channels << ", ring buffers " << rec.buffers.size() << ", process buffers " << pBuf.size() << std::endl;
#endif

        return false; // buffers just haven't been set up yet
    }

    unsigned int targetChannels = channels;
    if (targetChannels < 2)
        targetChannels = 2; // fill at least two buffers

    size_t minWriteSpace = 0;
    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
        size_t thisWriteSpace = rec.buffers[ch]->getWriteSpace();
        if (ch == 0 || thisWriteSpace < minWriteSpace) {
            minWriteSpace = thisWriteSpace;
            if (minWriteSpace < m_blockSize) {
#ifdef DEBUG_MIXER
                //		if (m_driver->isPlaying()) {
                if ((id % 100) == 0)
                    std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): only " << minWriteSpace << " write space on channel " << ch << " for block size " << m_blockSize << std::endl;
                //		}
#endif

                return false;
            }
        }
    }

    PluginList &plugins = m_plugins[id];

#ifdef DEBUG_MIXER

    if ((id % 100) == 0 && m_driver->isPlaying())
        std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): minWriteSpace is " << minWriteSpace << std::endl;
#else
#ifdef DEBUG_MIXER_LIGHTWEIGHT

    if ((id % 100) == 0 && m_driver->isPlaying())
        std::cout << minWriteSpace << "/" << rec.buffers[0]->getSize() << std::endl;
#endif
#endif

#ifdef DEBUG_MIXER

    if ((id % 100) == 0 && playCount > 0)
        std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): " << playCount << " audio file(s) to consider" << std::endl;
#endif

    bool haveBlock = true;
    bool haveMore = false;

    for (size_t fileNo = 0; fileNo < playCount; ++fileNo) {

        PlayableData *file = playing[fileNo];

        size_t frames = file->getSampleFramesAvailable();
        bool acceptable = ((frames >= m_blockSize) || file->isFullyBuffered());

        if (acceptable &&
                (minWriteSpace >= m_blockSize * 2) &&
                (frames >= m_blockSize * 2)) {

#ifdef DEBUG_MIXER
            if ((id % 100) == 0)
                std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): will be asking for more" << std::endl;
#endif

            haveMore = true;
        }

#ifdef DEBUG_MIXER
        if ((id % 100) == 0)
            std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): file has " << frames << " frames available" << std::endl;
#endif

        if (!acceptable) {

            //std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): file " << file->getAudioFile()->getFilename() << " has " << frames << " frames available, says isBuffered " << file->isBuffered() << std::endl;

            if (!m_driver->getLowLatencyMode()) {

                // Not a serious problem, just block on this
                // instrument and return to it a little later.
                haveBlock = false;

            } else {
                // In low latency mode, this is a serious problem if
                // the file has been buffered and simply isn't filling
                // fast enough.  Otherwise we have to assume that the
                // problem is something like a new file being dropped
                // in by unmute during playback, in which case we have
                // to accept that it won't be available for a while
                // and just read silence from it instead.
                if (file->isBuffered()) {
                    m_driver->reportFailure(MappedEvent::FailureDiscUnderrun);
                    haveBlock = false;
                } else {
                    // ignore happily.
                }
            }
        }
    }

    if (!haveBlock) {
        return false; // blocked;
    }

#ifdef DEBUG_MIXER
    if (!haveMore) {
        if ((id % 100) == 0)
            std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): won't be asking for more" << std::endl;
    }
#endif

    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
        memset(pBuf[ch], 0, sizeof(sample_t) * m_blockSize);
    }

    RunnablePluginInstance *synth = m_synths[id];

    if (synth && !synth->isBypassed()) {

        synth->run(bufferTime);

        unsigned int ch = 0;

        while (ch < synth->getAudioOutputCount() && ch < channels) {
            denormalKill(synth->getAudioOutputBuffers()[ch],
                         m_blockSize);
            memcpy(pBuf[ch],
                   synth->getAudioOutputBuffers()[ch],
                   m_blockSize * sizeof(sample_t));
            ++ch;
        }
    }

    if (haveBlock) {

        // Mix in a block from each playing file on this instrument.

        for (size_t fileNo = 0; fileNo < playCount; ++fileNo) {

            PlayableData *file = playing[fileNo];

            int offset = 0;
            int blockSize = (int)m_blockSize;

            if (file->getStartTime() > bufferTime) {
                offset = (int)RealTime::realTime2Frame
                         (file->getStartTime() - bufferTime, m_sampleRate);
                if (offset < blockSize)
                    blockSize -= offset;
                else
                    blockSize = 0;
#ifdef DEBUG_MIXER

                std::cerr << "AudioInstrumentMixer::processBlock: file starts at offset " << offset << ", block size now " << blockSize << std::endl;
#endif

            }

            //!!! This addSamples call is what is supposed to signal
            // to a playable audio file when the end of the file has
            // been reached.  But for some playables it appears the
            // file overruns, possibly due to rounding errors in
            // sample rate conversion, and so we stop reading from it
            // before it's actually done.  I don't particularly mind
            // that from a sound quality POV (after all it's badly
            // resampled already) but unfortunately it means we leak
            // pooled buffers.

            if (blockSize > 0) {
                file->addSamples(pBuf, channels, blockSize, offset);
                readSomething = true;
            }
        }
    }

    // Apply plugins.  There are various copy-reducing
    // optimisations available here, but we're not even going to
    // think about them yet.  Note that we force plugins to mono
    // on a mono track, even though we have stereo output buffers
    // -- stereo only comes into effect at the pan stage, and
    // these are pre-fader plugins.

    for (PluginList::iterator pli = plugins.begin();
            pli != plugins.end(); ++pli) {

        RunnablePluginInstance *plugin = *pli;
        if (!plugin || plugin->isBypassed())
            continue;

        unsigned int ch = 0;

        // If a plugin has more input channels than we have
        // available, we duplicate up to stereo and leave any
        // remaining channels empty.

        while (ch < plugin->getAudioInputCount()) {

            if (ch < channels || ch < 2) {
                memcpy(plugin->getAudioInputBuffers()[ch],
                       pBuf[ch % channels],
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

        plugin->run(bufferTime);

        ch = 0;

        while (ch < plugin->getAudioOutputCount()) {

            denormalKill(plugin->getAudioOutputBuffers()[ch],
                         m_blockSize);

            if (ch < channels) {
                memcpy(pBuf[ch],
                       plugin->getAudioOutputBuffers()[ch],
                       m_blockSize * sizeof(sample_t));
            } else if (ch == 1) {
                // stereo output from plugin on a mono track
                for (size_t i = 0; i < m_blockSize; ++i) {
                    pBuf[0][i] +=
                        plugin->getAudioOutputBuffers()[ch][i];
                    pBuf[0][i] /= 2;
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

            sample_t sample = pBuf[0][i];

            pBuf[0][i] = sample * rec.gainLeft;
            pBuf[1][i] = sample * rec.gainRight;

            if (allZeros && sample != 0.0)
                allZeros = false;
        }

        rec.buffers[0]->write(pBuf[0], m_blockSize);
        rec.buffers[1]->write(pBuf[1], m_blockSize);

    } else {

        for (unsigned int ch = 0; ch < targetChannels; ++ch) {

            float gain = ((ch == 0) ? rec.gainLeft :
                          (ch == 1) ? rec.gainRight : rec.volume);

            for (size_t i = 0; i < m_blockSize; ++i) {

                // handle volume and pan
                pBuf[ch][i] *= gain;

                if (allZeros && pBuf[ch][i] != 0.0)
                    allZeros = false;
            }

            rec.buffers[ch]->write(pBuf[ch], m_blockSize);
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

#ifdef DEBUG_MIXER
    if ((id % 100) == 0 && m_driver->isPlaying())
        std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): setting dormant to " << dormant << std::endl;
#endif

    rec.dormant = dormant;
    bufferTime = bufferTime + RealTime::frame2RealTime(m_blockSize,
                 m_sampleRate);

    rec.filledTo = bufferTime;

#ifdef DEBUG_MIXER

    if ((id % 100) == 0)
        std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): done, returning " << haveMore << std::endl;
#endif

    return haveMore;
}

void
AudioInstrumentMixer::kick(bool wantLock)
{
    // Needs to be RT safe if wantLock is not specified

    if (wantLock)
        getLock();

    bool readSomething = false;
    processBlocks(readSomething);
    if (readSomething)
        m_fileReader->signal();

    if (wantLock)
        releaseLock();
}


void
AudioInstrumentMixer::threadRun()
{
    while (!m_exiting) {

        if (m_driver->areClocksRunning()) {
            kick(false);
        }

        RealTime t = m_driver->getAudioMixBufferLength();
        t = t / 2;
        if (t < RealTime(0, 10000000))
            t = RealTime(0, 10000000); // 10ms minimum

        struct timeval now;
        gettimeofday(&now, nullptr);
        t = t + RealTime(now.tv_sec, now.tv_usec * 1000);

        struct timespec timeout;
        timeout.tv_sec = t.sec;
        timeout.tv_nsec = t.nsec;

        pthread_cond_timedwait(&m_condition, &m_lock, &timeout);
        pthread_testcancel();
    }
}


}
