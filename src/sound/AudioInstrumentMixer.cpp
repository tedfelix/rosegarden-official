/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioInstrumentMixer]"
#define RG_NO_DEBUG_PRINT

#include "AudioInstrumentMixer.h"

#include "RunnablePluginInstance.h"
#include "PlayableAudioFile.h"
#include "MappedStudio.h"  // MappedAudioFader
#include "base/AudioLevel.h"
#include "AudioPlayQueue.h"
#include "PluginFactory.h"
#include "ControlBlock.h"
#include "misc/Debug.h"

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
    // Pregenerate empty plugin slots

    InstrumentId audioInstrumentBase;
    int audioInstrumentCount;
    m_driver->getAudioInstrumentNumbers(
            audioInstrumentBase, audioInstrumentCount);

    // For each audio instrument...
    for (int i = 0; i < audioInstrumentCount; ++i) {
        const InstrumentId instrumentID = audioInstrumentBase + i;
        PluginList &pluginVector = m_plugins[instrumentID];
        pluginVector.resize(Instrument::PLUGIN_COUNT, nullptr);
    }

    InstrumentId synthInstrumentBase;
    int synthInstrumentCount;
    m_driver->getSoftSynthInstrumentNumbers(
            synthInstrumentBase, synthInstrumentCount);

    // For each synth instrument...
    for (int i = 0; i < synthInstrumentCount; ++i) {
        const InstrumentId instrumentID = synthInstrumentBase + i;
        m_synths[instrumentID] = nullptr;
        // Synths can also have effect plugins.
        PluginList &pluginVector = m_plugins[instrumentID];
        pluginVector.resize(Instrument::PLUGIN_COUNT, nullptr);
    }

    // Make room for the busses here.
    const int bussCount = AudioBussMixer::getMaxBussCount();

    // Submaster busses start at 1.  Buss 0 is the master out and it cannot
    // have effects plugins.
    for (int i = 1; i < bussCount + 1; ++i) {
        PluginList &pluginVector = m_plugins[i];
        pluginVector.resize(Instrument::PLUGIN_COUNT, nullptr);
    }

#if 0
    RG_DEBUG << "ctor: m_plugins size:" << m_plugins.size();
    for (const auto &pair : m_plugins) {
        RG_DEBUG << "      [" << pair.first << "].size():" << pair.second.size();
    }
#endif

    // Leave the buffer map and process buffer list empty for now.
    // The buffer length can change between plays, so we always
    // examine the buffers in fillBuffers and are prepared to
    // regenerate from scratch if necessary.  Don't like it though.

    // Keep track of the instance globally.  See getInstance().
    // We do this last to ensure that the object is completely
    // initialized before other threads call getInstance().
    // This is likely pretty questionable since the ctor hasn't
    // exited yet.
    aimInstance = this;
}

AudioInstrumentMixer *
AudioInstrumentMixer::getInstance()
{
    return aimInstance;
}

AudioInstrumentMixer::~AudioInstrumentMixer()
{
#if 0
    RG_DEBUG << "dtor: m_plugins size:" << m_plugins.size();
    for (const auto &pair : m_plugins) {
        RG_DEBUG << "      [" << pair.first << "].size():" << pair.second.size();
    }
#endif

    aimInstance = nullptr;

    //RG_DEBUG << "dtor";
    // BufferRec dtor will handle the BufferMap

    removeAllPlugins();

    for (auto& pair : m_processBuffers) {
        auto& pBuf = pair.second;
        for (std::vector<sample_t *>::iterator i = pBuf.begin();
             i != pBuf.end(); ++i) {
            delete[] *i;
        }
    }

    //RG_DEBUG << "dtor exiting";
}

AudioInstrumentMixer::BufferRec::~BufferRec()
{
    for (size_t i = 0; i < buffers.size(); ++i)
        delete buffers[i];
}


void
AudioInstrumentMixer::setPlugin(
        const InstrumentId instrumentId,
        const int pluginPosition,
        const QString &identifier)
{
    // Not RT safe

    //RG_DEBUG << "setPlugin(" << id << "," << position << "," << identifier << ")";

    int channels = 2;
    if (m_bufferMap.find(instrumentId) != m_bufferMap.end()) {
        channels = m_bufferMap[instrumentId].channels;
    }

    RunnablePluginInstance *instance = nullptr;

    PluginFactory *factory = PluginFactory::instanceFor(identifier);
    if (factory) {
        instance = factory->instantiatePlugin(identifier,
                                              instrumentId,
                                              pluginPosition,
                                              m_sampleRate,
                                              m_blockSize,
                                              channels,
                                              this);
        if (instance && !instance->isOK()) {
            RG_WARNING << "setPlugin(" << instrumentId << ", " << pluginPosition << ": instance is not OK";
            delete instance;
            instance = nullptr;
        }
    } else {
        RG_WARNING << "setPlugin(): No factory for identifier" << identifier;
    }

    RunnablePluginInstance *oldInstance = nullptr;

    if (pluginPosition == int(Instrument::SYNTH_PLUGIN_POSITION)) {

        oldInstance = m_synths[instrumentId];
        m_synths[instrumentId] = instance;
        if (! oldInstance) m_numSoftSynths++;

    } else {

        PluginList &pluginVector = m_plugins[instrumentId];

        if (pluginPosition < int(Instrument::PLUGIN_COUNT)) {
            if (pluginPosition >= (int)pluginVector.size()) {
                // This should never happen given that we pre-sized
                // the plugin vector to PLUGIN_COUNT.  Previously this
                // code added the necessary entries.  Problem with that
                // is that it modifies a vector that is supposed to be
                // fixed size so it is thread safe.
                RG_WARNING << "setPlugin(): pluginPosition" << pluginPosition << "beyond plugin vector size" << pluginVector.size() << "for instrument ID" << instrumentId;

                delete instance;
            } else {
                // We're good.  Hook it in...
                oldInstance = pluginVector[pluginPosition];
                pluginVector[pluginPosition] = instance;
            }
        } else {
            RG_WARNING << "setPlugin(): No pluginPosition" << pluginPosition << "for instrument" << instrumentId;

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

    //RG_DEBUG << "removePlugin(" << id << ", " << position << ")";

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

    //RG_DEBUG << "removeAllPlugins()";

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
    RG_DEBUG << "resetAllPlugins()!";
    if (discardEvents)
        RG_DEBUG << "(discardEvents true)";
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
            RG_DEBUG << "resetAllPlugins(): (re)setting" << channels << "channels on synth for instrument" << id;
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
                RG_DEBUG << "resetAllPlugins(): (re)setting" << channels << "channels on plugin for instrument" << id;
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

    //RG_DEBUG << "destroyAllPlugins()";

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

    size_t bufferSamples1 = m_blockSize;

    if (!m_driver->getLowLatencyMode()) {
        RealTime bufferLength = m_driver->getAudioMixBufferLength();
        size_t bufferSamples = (size_t)RealTime::realTime2Frame(bufferLength, m_sampleRate);
        bufferSamples = ((bufferSamples / m_blockSize) + 1) * m_blockSize;
#ifdef DEBUG_MIXER

        RG_DEBUG << "generateBuffers(): Buffer length is" << bufferLength << "; buffer samples" << bufferSamples << " (sample rate" << m_sampleRate << ")";
#endif

    }

    for (int instrumentIndex = 0; instrumentIndex < audioInstruments + synthInstruments; ++instrumentIndex) {

        InstrumentId id;
        if (instrumentIndex < audioInstruments)
            id = audioInstrumentBase + instrumentIndex;
        else
            id = synthInstrumentBase + (instrumentIndex - audioInstruments);

        // Get a fader for this instrument - if we can't then this
        // isn't a valid audio track.
        MappedAudioFader *fader = m_driver->getMappedStudio()->getAudioFader(id);

        if (!fader) {
#ifdef DEBUG_MIXER
            RG_DEBUG << "generateBuffers(): no fader for audio instrument" << id;
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
                if (rec.buffers[i]->getSize() != bufferSamples1) {
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
                new RingBuffer<sample_t, 2>(bufferSamples1);

            if (!rb->mlock()) {
                //RG_WARNING << "generateBuffers(): WARNING: couldn't lock ring buffer into real memory, performance may be impaired";
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

}

void
AudioInstrumentMixer::fillBuffers(const RealTime &currentTime)
{
    // Not RT safe

    emptyBuffers(currentTime);

    getLock();

#ifdef DEBUG_MIXER
    RG_DEBUG << "fillBuffers(" << currentTime << ")";
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
    RG_DEBUG << "allocateBuffers()";
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
    RG_DEBUG << "emptyBuffers(" << currentTime << ")";
#endif

    generateBuffers();

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    for (int instrumentIndex = 0; instrumentIndex < audioInstruments + synthInstruments; ++instrumentIndex) {

        InstrumentId id;
        if (instrumentIndex < audioInstruments)
            id = audioInstrumentBase + instrumentIndex;
        else
            id = synthInstrumentBase + (instrumentIndex - audioInstruments);

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
        RG_DEBUG << "processBlocks()";
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
        RG_DEBUG << "processBlock(" << id << "): buffer time is" << bufferTime;
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
            RG_DEBUG << "processBlock(" << id << "): nominal channels" << rec.channels << ", ring buffers" << rec.buffers.size() << ", process buffers" << pBuf.size();
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
                    RG_DEBUG << "processBlock(" << id << "): only" << minWriteSpace << "write space on channel" << ch << "for block size" << m_blockSize;
                //		}
#endif

                return false;
            }
        }
    }

    PluginList &plugins = m_plugins[id];

#ifdef DEBUG_MIXER
    if ((id % 100) == 0 && m_driver->isPlaying())
        RG_DEBUG << "processBlock(" << id << "): minWriteSpace is" << minWriteSpace;
#else
#ifdef DEBUG_MIXER_LIGHTWEIGHT
    if ((id % 100) == 0 && m_driver->isPlaying())
        RG_DEBUG << minWriteSpace << "/" << rec.buffers[0]->getSize();
#endif
#endif

#ifdef DEBUG_MIXER
    if ((id % 100) == 0 && playCount > 0)
        RG_DEBUG << "processBlock(" << id << "):" << playCount << "audio file(s) to consider";
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
                RG_DEBUG << "processBlock(" << id << "): will be asking for more";
#endif

            haveMore = true;
        }

#ifdef DEBUG_MIXER
        if ((id % 100) == 0)
            RG_DEBUG << "processBlock(" << id << "): file has" << frames << "frames available";
#endif

        if (!acceptable) {

            //RG_DEBUG << "processBlock(" << id << "): file" << file->getAudioFile()->getFilename() << "has" << frames << "frames available, says isBuffered" << file->isBuffered();

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
            RG_DEBUG << "processBlock(" << id << "): won't be asking for more";
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
                RG_DEBUG << "processBlock(): file starts at offset" << offset << ", block size now" << blockSize;
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
        RG_DEBUG << "Running plugin with" << plugin->getAudioInputCount() << "inputs," << plugin->getAudioOutputCount() << "outputs";
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
        RG_DEBUG << "processBlock(" << id << "): setting dormant to" << dormant;
#endif

    rec.dormant = dormant;
    bufferTime = bufferTime + RealTime::frame2RealTime(m_blockSize,
                 m_sampleRate);

    rec.filledTo = bufferTime;

#ifdef DEBUG_MIXER

    if ((id % 100) == 0)
        RG_DEBUG << "processBlock(" << id << "): done, returning" << haveMore;
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
