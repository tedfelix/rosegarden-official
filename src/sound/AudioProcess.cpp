/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AudioProcess.h"

#include "RunnablePluginInstance.h"
#include "PlayableAudioFile.h"
#include "RecordableAudioFile.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"
#include "base/Profiler.h"
#include "base/AudioLevel.h"
#include "AudioPlayQueue.h"
#include "PluginFactory.h"
#include "ControlBlock.h"

#include "misc/Strings.h"
#include <sys/time.h>
#include <pthread.h>

#include <cmath>

#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <alloca.h>
#endif

//#define DEBUG_THREAD_CREATE_DESTROY 1
//#define DEBUG_BUSS_MIXER 1
//#define DEBUG_MIXER 1
//#define DEBUG_MIXER_LIGHTWEIGHT 1
//#define DEBUG_LOCKS 1
//#define DEBUG_READER 1
//#define DEBUG_WRITER 1

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

AudioThread::AudioThread(std::string name,
                         SoundDriver *driver,
                         unsigned int sampleRate) :
        m_name(name),
        m_driver(driver),
        m_sampleRate(sampleRate),
        m_thread(0),
        m_running(false),
        m_exiting(false)
{
#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << "AudioThread::AudioThread() [" << m_name << "]" << std::endl;
#endif

    pthread_mutex_t initialisingMutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&m_lock, &initialisingMutex, sizeof(pthread_mutex_t));

    pthread_cond_t initialisingCondition = PTHREAD_COND_INITIALIZER;
    memcpy(&m_condition, &initialisingCondition, sizeof(pthread_cond_t));
}

AudioThread::~AudioThread()
{
#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << "AudioThread::~AudioThread() [" << m_name << "]" << std::endl;
#endif

    if (m_thread) {
        pthread_mutex_destroy(&m_lock);
        m_thread = 0;
    }

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << "AudioThread::~AudioThread() exiting" << std::endl;
#endif
}

void
AudioThread::run()
{
#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << m_name << "::run()" << std::endl;
#endif

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    int priority = getPriority();

    if (priority > 0) {

        if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {

            std::cerr << m_name << "::run: WARNING: couldn't set FIFO scheduling "
            << "on new thread" << std::endl;
            pthread_attr_init(&attr); // reset to safety

        } else {

            struct sched_param param;
            memset(&param, 0, sizeof(struct sched_param));
            param.sched_priority = priority;

            if (pthread_attr_setschedparam(&attr, &param)) {
                std::cerr << m_name << "::run: WARNING: couldn't set priority "
                << priority << " on new thread" << std::endl;
                pthread_attr_init(&attr); // reset to safety
            }
        }
    }

    pthread_attr_setstacksize(&attr, 1048576);
    int rv = pthread_create(&m_thread, &attr, staticThreadRun, this);

    if (rv != 0 && priority > 0) {
#ifdef DEBUG_THREAD_CREATE_DESTROY
        std::cerr << m_name << "::run: WARNING: unable to start RT thread;"
        << "\ntrying again with normal scheduling" << std::endl;
#endif

        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 1048576);
        rv = pthread_create(&m_thread, &attr, staticThreadRun, this);
    }

    if (rv != 0) {
        // This is quite fatal.
        std::cerr << m_name << "::run: ERROR: failed to start thread!" << std::endl;
        ::exit(1);
    }

    m_running = true;

#ifdef DEBUG_THREAD_CREATE_DESTROY

    std::cerr << m_name << "::run() done" << std::endl;
#endif
}

void
AudioThread::terminate()
{
#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::string name = m_name;
    std::cerr << name << "::terminate()" << std::endl;
#endif

    m_running = false;

    if (m_thread) {

        pthread_cancel(m_thread);

#ifdef DEBUG_THREAD_CREATE_DESTROY

        std::cerr << name << "::terminate(): cancel requested" << std::endl;
#endif

        int rv = pthread_join(m_thread, nullptr);
        rv = rv; // shut up compiler warning when the code below is not compiled

#ifdef DEBUG_THREAD_CREATE_DESTROY

        std::cerr << name << "::terminate(): thread exited with return value " << rv << std::endl;
#endif

    }

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << name << "::terminate(): done" << std::endl;
#endif
}

void *
AudioThread::staticThreadRun(void *arg)
{
    AudioThread *inst = static_cast<AudioThread *>(arg);
    if (!inst)
        return nullptr;

    pthread_cleanup_push(staticThreadCleanup, arg);

    inst->getLock();
    inst->m_exiting = false;
    inst->threadRun();

#ifdef DEBUG_THREAD_CREATE_DESTROY

    std::cerr << inst->m_name << "::staticThreadRun(): threadRun exited" << std::endl;
#endif

    inst->releaseLock();
    pthread_cleanup_pop(0);

    return nullptr;
}

void
AudioThread::staticThreadCleanup(void *arg)
{
    AudioThread *inst = static_cast<AudioThread *>(arg);
    if (!inst || inst->m_exiting)
        return ;

#ifdef DEBUG_THREAD_CREATE_DESTROY

    std::string name = inst->m_name;
    std::cerr << name << "::staticThreadCleanup()" << std::endl;
#endif

    inst->m_exiting = true;
    inst->releaseLock();

#ifdef DEBUG_THREAD_CREATE_DESTROY

    std::cerr << name << "::staticThreadCleanup() done" << std::endl;
#endif
}

int
AudioThread::getLock()
{
    int rv;
#ifdef DEBUG_LOCKS

    std::cerr << m_name << "::getLock()" << std::endl;
#endif

    rv = pthread_mutex_lock(&m_lock);
#ifdef DEBUG_LOCKS

    std::cerr << "OK" << std::endl;
#endif

    return rv;
}

int
AudioThread::tryLock()
{
    int rv;
#ifdef DEBUG_LOCKS

    std::cerr << m_name << "::tryLock()" << std::endl;
#endif

    rv = pthread_mutex_trylock(&m_lock);
#ifdef DEBUG_LOCKS

    std::cerr << "OK (rv is " << rv << ")" << std::endl;
#endif

    return rv;
}

int
AudioThread::releaseLock()
{
    int rv;
#ifdef DEBUG_LOCKS

    std::cerr << m_name << "::releaseLock()" << std::endl;
#endif

    rv = pthread_mutex_unlock(&m_lock);
#ifdef DEBUG_LOCKS

    std::cerr << "OK" << std::endl;
#endif

    return rv;
}

void
AudioThread::signal()
{
#ifdef DEBUG_LOCKS
    std::cerr << m_name << "::signal()" << std::endl;
#endif

    pthread_cond_signal(&m_condition);
}


AudioBussMixer::AudioBussMixer(SoundDriver *driver,
                               AudioInstrumentMixer *instrumentMixer,
                               unsigned int sampleRate,
                               unsigned int blockSize) :
        AudioThread("AudioBussMixer", driver, sampleRate),
        m_instrumentMixer(instrumentMixer),
        m_blockSize(blockSize),
        m_bussCount(0)
{
    // nothing else here
}

AudioBussMixer::~AudioBussMixer()
{
    for (size_t i = 0; i < m_processBuffers.size(); ++i) {
        delete[] m_processBuffers[i];
    }
}

AudioBussMixer::BufferRec::~BufferRec()
{
    for (size_t i = 0; i < buffers.size(); ++i)
        delete buffers[i];
}

void
AudioBussMixer::generateBuffers()
{
    // Not RT safe

#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::generateBuffers" << std::endl;
#endif

    // This returns one too many, as the master is counted as buss 0
    m_bussCount =
        m_driver->getMappedStudio()->getObjectCount(MappedStudio::AudioBuss) - 1;

#ifdef DEBUG_BUSS_MIXER

    std::cerr << "AudioBussMixer::generateBuffers: have " << m_bussCount << " busses" << std::endl;
#endif

    size_t bufferSamples = m_blockSize;

    if (!m_driver->getLowLatencyMode()) {
        RealTime bufferLength = m_driver->getAudioMixBufferLength();
        size_t bufferSamples = (size_t)RealTime::realTime2Frame(bufferLength, m_sampleRate);
        bufferSamples = ((bufferSamples / m_blockSize) + 1) * m_blockSize;
    }

    for (int i = 0; i < m_bussCount; ++i) {

        BufferRec &rec = m_bufferMap[i];

        if (rec.buffers.size() == 2)
            continue;

        for (unsigned int ch = 0; ch < 2; ++ch) {
            RingBuffer<sample_t> *rb = new RingBuffer<sample_t>(bufferSamples);
            if (!rb->mlock()) {
                //		std::cerr << "WARNING: AudioBussMixer::generateBuffers: couldn't lock ring buffer into real memory, performance may be impaired" << std::endl;
            }
            rec.buffers.push_back(rb);
        }

        MappedAudioBuss *mbuss =
            m_driver->getMappedStudio()->getAudioBuss(i + 1); // master is 0

        if (mbuss) {

            float level = 0.0;
            (void)mbuss->getProperty(MappedAudioBuss::Level, level);

            float pan = 0.0;
            (void)mbuss->getProperty(MappedAudioBuss::Pan, pan);

            setBussLevels(i + 1, level, pan);
        }
    }

    if (m_processBuffers.empty()) {
        m_processBuffers.push_back(new sample_t[m_blockSize]);
        m_processBuffers.push_back(new sample_t[m_blockSize]);
    }
}

void
AudioBussMixer::fillBuffers(const RealTime &currentTime)
{
    // Not RT safe

#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::fillBuffers" << std::endl;
#endif

    emptyBuffers();
    m_instrumentMixer->fillBuffers(currentTime);
    kick();
}

void
AudioBussMixer::emptyBuffers()
{
    // Not RT safe

    getLock();

#ifdef DEBUG_BUSS_MIXER

    std::cerr << "AudioBussMixer::emptyBuffers" << std::endl;
#endif

    // We can't generate buffers before this, because we don't know how
    // many busses there are
    generateBuffers();

    for (int i = 0; i < m_bussCount; ++i) {
        m_bufferMap[i].dormant = true;
        for (int ch = 0; ch < 2; ++ch) {
            if (int(m_bufferMap[i].buffers.size()) > ch) {
                m_bufferMap[i].buffers[ch]->reset();
            }
        }
    }

    releaseLock();
}

void
AudioBussMixer::kick(bool wantLock, bool signalInstrumentMixer)
{
    // Needs to be RT safe if wantLock is not specified

    if (wantLock)
        getLock();

#ifdef DEBUG_BUSS_MIXER

    std::cerr << "AudioBussMixer::kick" << std::endl;
#endif

    processBlocks();

#ifdef DEBUG_BUSS_MIXER

    std::cerr << "AudioBussMixer::kick: processed" << std::endl;
#endif

    if (wantLock)
        releaseLock();

    if (signalInstrumentMixer) {
        m_instrumentMixer->signal();
    }
}

void
AudioBussMixer::setBussLevels(int bussId, float dB, float pan)
{
    // No requirement to be RT safe

    if (bussId == 0)
        return ; // master
    int buss = bussId - 1;

    BufferRec &rec = m_bufferMap[buss];

    float volume = AudioLevel::dB_to_multiplier(dB);

     // Basic balance control.  Panning laws are not applied to submasters.
    rec.gainLeft = volume * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);
    rec.gainRight = volume * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
}

void
AudioBussMixer::updateInstrumentConnections()
{
    // Not RT safe

    if (m_bussCount <= 0)
        generateBuffers();

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    for (int buss = 0; buss < m_bussCount; ++buss) {

        MappedAudioBuss *mbuss =
            m_driver->getMappedStudio()->getAudioBuss(buss + 1); // master is 0

        if (!mbuss) {
#ifdef DEBUG_BUSS_MIXER
            std::cerr << "AudioBussMixer::updateInstrumentConnections: buss " << buss << " not found" << std::endl;
#endif

            continue;
        }

        BufferRec &rec = m_bufferMap[buss];

        while (int(rec.instruments.size()) < audioInstruments + synthInstruments) {
            rec.instruments.push_back(false);
        }

        std::vector<InstrumentId> instruments = mbuss->getInstruments();

        for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

            InstrumentId id;
            if (i < audioInstruments)
                id = audioInstrumentBase + i;
            else
                id = synthInstrumentBase + (i - audioInstruments);

            size_t j = 0;
            for (j = 0; j < instruments.size(); ++j) {
                if (instruments[j] == id) {
                    rec.instruments[i] = true;
                    break;
                }
            }
            if (j == instruments.size())
                rec.instruments[i] = false;
        }
    }
}

void
AudioBussMixer::processBlocks()
{
    // Needs to be RT safe

    if (m_bussCount == 0)
        return ;

#ifdef DEBUG_BUSS_MIXER

    if (m_driver->isPlaying())
        std::cerr << "AudioBussMixer::processBlocks" << std::endl;
#endif

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    bool *processedInstruments = (bool *)alloca
                                 ((audioInstruments + synthInstruments) * sizeof(bool));

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {
        processedInstruments[i] = false;
    }

    size_t minBlocks = 0;
    bool haveMinBlocks = false;

    for (int buss = 0; buss < m_bussCount; ++buss) {

        BufferRec &rec = m_bufferMap[buss];

        float gain[2];
        gain[0] = rec.gainLeft;
        gain[1] = rec.gainRight;

        // The dormant calculation here depends on the buffer length
        // for this mixer being the same as that for the instrument mixer

        size_t minSpace = 0;

        for (int ch = 0; ch < 2; ++ch) {

            size_t w = rec.buffers[ch]->getWriteSpace();
            if (ch == 0 || w < minSpace)
                minSpace = w;

#ifdef DEBUG_BUSS_MIXER

            std::cerr << "AudioBussMixer::processBlocks: buss " << buss << ": write space " << w << " on channel " << ch << std::endl;
#endif

            if (minSpace == 0)
                break;

            for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

                // is this instrument on this buss?
                if (int(rec.instruments.size()) <= i ||
                        !rec.instruments[i])
                    continue;

                InstrumentId id;
                if (i < audioInstruments)
                    id = audioInstrumentBase + i;
                else
                    id = synthInstrumentBase + (i - audioInstruments);

                if (m_instrumentMixer->isInstrumentEmpty(id))
                    continue;

                RingBuffer<sample_t, 2> *rb =
                    m_instrumentMixer->getRingBuffer(id, ch);
                if (rb) {
                    size_t r = rb->getReadSpace(1);
                    if (r < minSpace)
                        minSpace = r;

#ifdef DEBUG_BUSS_MIXER

                    if (id == 1000) {
                        std::cerr << "AudioBussMixer::processBlocks: buss " << buss << ": read space " << r << " on instrument " << id << ", channel " << ch << std::endl;
                    }
#endif

                    if (minSpace == 0)
                        break;
                }
            }

            if (minSpace == 0)
                break;
        }

        size_t blocks = minSpace / m_blockSize;
        if (!haveMinBlocks || (blocks < minBlocks)) {
            minBlocks = blocks;
            haveMinBlocks = true;
        }

#ifdef DEBUG_BUSS_MIXER
        if (m_driver->isPlaying())
            std::cerr << "AudioBussMixer::processBlocks: doing " << blocks << " blocks at block size " << m_blockSize << std::endl;
#endif

        for (size_t block = 0; block < blocks; ++block) {

            memset(m_processBuffers[0], 0, m_blockSize * sizeof(sample_t));
            memset(m_processBuffers[1], 0, m_blockSize * sizeof(sample_t));

            bool dormant = true;

            for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

                // is this instrument on this buss?
                if (int(rec.instruments.size()) <= i ||
                        !rec.instruments[i])
                    continue;

                if (processedInstruments[i]) {
                    // we aren't set up to process any instrument to
                    // more than one buss
                    continue;
                } else {
                    processedInstruments[i] = true;
                }

                InstrumentId id;
                if (i < audioInstruments)
                    id = audioInstrumentBase + i;
                else
                    id = synthInstrumentBase + (i - audioInstruments);

                if (m_instrumentMixer->isInstrumentEmpty(id))
                    continue;

                if (m_instrumentMixer->isInstrumentDormant(id)) {

                    for (int ch = 0; ch < 2; ++ch) {
                        RingBuffer<sample_t, 2> *rb =
                            m_instrumentMixer->getRingBuffer(id, ch);

                        if (rb)
                            rb->skip(m_blockSize,
                                     1);
                    }
                } else {
                    dormant = false;

                    for (int ch = 0; ch < 2; ++ch) {
                        RingBuffer<sample_t, 2> *rb =
                            m_instrumentMixer->getRingBuffer(id, ch);

                        if (rb)
                            rb->readAdding(m_processBuffers[ch],
                                           m_blockSize,
                                           1);
                    }
                }
            }

            if (m_instrumentMixer) {
                AudioInstrumentMixer::PluginList &plugins =
                    m_instrumentMixer->getBussPlugins(buss + 1);

                // This will have to do for now!
                if (!plugins.empty())
                    dormant = false;

                for (AudioInstrumentMixer::PluginList::iterator pli =
                            plugins.begin(); pli != plugins.end(); ++pli) {

                    RunnablePluginInstance *plugin = *pli;
                    if (!plugin || plugin->isBypassed())
                        continue;

                    unsigned int ch = 0;

                    while (ch < plugin->getAudioInputCount()) {
                        if (ch < 2) {
                            memcpy(plugin->getAudioInputBuffers()[ch],
                                   m_processBuffers[ch],
                                   m_blockSize * sizeof(sample_t));
                        } else {
                            memset(plugin->getAudioInputBuffers()[ch], 0,
                                   m_blockSize * sizeof(sample_t));
                        }
                        ++ch;
                    }

#ifdef DEBUG_BUSS_MIXER
                    std::cerr << "Running buss plugin with " << plugin->getAudioInputCount()
                    << " inputs, " << plugin->getAudioOutputCount() << " outputs" << std::endl;
#endif

                    // We don't currently maintain a record of our
                    // frame time in the buss mixer.  This will screw
                    // up any plugin that requires a good frame count:
                    // at the moment that only means DSSI effects
                    // plugins using run_multiple_synths, which would
                    // be an unusual although plausible combination
                    plugin->run(RealTime::zeroTime);

                    ch = 0;

                    while (ch < 2 && ch < plugin->getAudioOutputCount()) {

                        denormalKill(plugin->getAudioOutputBuffers()[ch],
                                     m_blockSize);

                        memcpy(m_processBuffers[ch],
                               plugin->getAudioOutputBuffers()[ch],
                               m_blockSize * sizeof(sample_t));

                        ++ch;
                    }
                }
            }

            for (int ch = 0; ch < 2; ++ch) {
                if (dormant) {
                    rec.buffers[ch]->zero(m_blockSize);
                } else {
                    for (size_t j = 0; j < m_blockSize; ++j) {
                        m_processBuffers[ch][j] *= gain[ch];
                    }
                    rec.buffers[ch]->write(m_processBuffers[ch], m_blockSize);
                }
            }

            rec.dormant = dormant;

#ifdef DEBUG_BUSS_MIXER

            if (m_driver->isPlaying())
                std::cerr << "AudioBussMixer::processBlocks: buss " << buss << (dormant ? " dormant" : " not dormant") << std::endl;
#endif

        }
    }

    // any unprocessed instruments need to be skipped, or they'll block

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

        if (processedInstruments[i])
            continue;

        InstrumentId id;
        if (i < audioInstruments)
            id = audioInstrumentBase + i;
        else
            id = synthInstrumentBase + (i - audioInstruments);

        if (m_instrumentMixer->isInstrumentEmpty(id))
            continue;

        for (int ch = 0; ch < 2; ++ch) {
            RingBuffer<sample_t, 2> *rb =
                m_instrumentMixer->getRingBuffer(id, ch);

            if (rb)
                rb->skip(m_blockSize * minBlocks,
                         1);
        }
    }


#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::processBlocks: done" << std::endl;
#endif
}

void
AudioBussMixer::threadRun()
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


AudioInstrumentMixer::AudioInstrumentMixer(SoundDriver *driver,
        AudioFileReader *fileReader,
        unsigned int sampleRate,
        unsigned int blockSize) :
        AudioThread("AudioInstrumentMixer", driver, sampleRate),
        m_fileReader(fileReader),
        m_bussMixer(nullptr),
        m_blockSize(blockSize)
{
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

AudioInstrumentMixer::~AudioInstrumentMixer()
{
    //std::cerr << "AudioInstrumentMixer::~AudioInstrumentMixer" << std::endl;
    // BufferRec dtor will handle the BufferMap

    removeAllPlugins();

    for (std::vector<sample_t *>::iterator i = m_processBuffers.begin();
            i != m_processBuffers.end(); ++i) {
        delete[] *i;
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
                                              channels);
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

    while ((unsigned int)m_processBuffers.size() > maxChannels) {
        std::vector<sample_t *>::iterator bi = m_processBuffers.end();
        --bi;
        delete[] *bi;
        m_processBuffers.erase(bi);
    }
    while ((unsigned int)m_processBuffers.size() < maxChannels) {
        m_processBuffers.push_back(new sample_t[m_blockSize]);
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

void
AudioInstrumentMixer::processBlocks(bool &readSomething)
{
    // Needs to be RT safe

#ifdef DEBUG_MIXER
    if (m_driver->isPlaying())
        std::cerr << "AudioInstrumentMixer::processBlocks" << std::endl;
#endif

    //    Profiler profiler("processBlocks", true);

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
    static PlayableAudioFile *playing[MAX_FILES_PER_INSTRUMENT];

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
                                   PlayableAudioFile **playing,
                                   size_t playCount,
                                   bool &readSomething)
{
    // Needs to be RT safe

    //    Profiler profiler("processBlock", true);

    BufferRec &rec = m_bufferMap[id];
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
    if (channels > (unsigned int)m_processBuffers.size())
        channels = (unsigned int)m_processBuffers.size();
    if (channels == 0) {
#ifdef DEBUG_MIXER
        if ((id % 100) == 0)
            std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): nominal channels " << rec.channels << ", ring buffers " << rec.buffers.size() << ", process buffers " << m_processBuffers.size() << std::endl;
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

        bool acceptable = false;
        PlayableAudioFile *file = playing[fileNo];

        size_t frames = file->getSampleFramesAvailable();
        acceptable = ((frames >= m_blockSize) || file->isFullyBuffered());

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
        memset(m_processBuffers[ch], 0, sizeof(sample_t) * m_blockSize);
    }

    RunnablePluginInstance *synth = m_synths[id];

    if (synth && !synth->isBypassed()) {

        synth->run(bufferTime);

        unsigned int ch = 0;

        while (ch < synth->getAudioOutputCount() && ch < channels) {
            denormalKill(synth->getAudioOutputBuffers()[ch],
                         m_blockSize);
            memcpy(m_processBuffers[ch],
                   synth->getAudioOutputBuffers()[ch],
                   m_blockSize * sizeof(sample_t));
            ++ch;
        }
    }

    if (haveBlock) {

        // Mix in a block from each playing file on this instrument.

        for (size_t fileNo = 0; fileNo < playCount; ++fileNo) {

            PlayableAudioFile *file = playing[fileNo];

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
                file->addSamples(m_processBuffers, channels, blockSize, offset);
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

        plugin->run(bufferTime);

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

            float gain = ((ch == 0) ? rec.gainLeft :
                          (ch == 1) ? rec.gainRight : rec.volume);

            for (size_t i = 0; i < m_blockSize; ++i) {

                // handle volume and pan
                m_processBuffers[ch][i] *= gain;

                if (allZeros && m_processBuffers[ch][i] != 0.0)
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



AudioFileReader::AudioFileReader(SoundDriver *driver,
                                 unsigned int sampleRate) :
        AudioThread("AudioFileReader", driver, sampleRate)
{
    // nothing else here
}

AudioFileReader::~AudioFileReader()
{}

void
AudioFileReader::fillBuffers(const RealTime &currentTime)
{
    getLock();

    // Tell every audio file the play start time.

    const AudioPlayQueue *queue = m_driver->getAudioQueue();

    RealTime bufferLength = m_driver->getAudioReadBufferLength();
    int bufferFrames = (int)RealTime::realTime2Frame(bufferLength, m_sampleRate);

    int poolSize = queue->getMaxBuffersRequired() * 2 + 4;
    PlayableAudioFile::setRingBufferPoolSizes(poolSize, bufferFrames);

    const AudioPlayQueue::FileSet &files = queue->getAllScheduledFiles();

#ifdef DEBUG_READER

    std::cerr << "AudioFileReader::fillBuffers: have " << files.size() << " audio files total" << std::endl;
#endif

    for (AudioPlayQueue::FileSet::const_iterator fi = files.begin();
            fi != files.end(); ++fi) {
        (*fi)->clearBuffers();
    }

    int allocated = 0;
    for (AudioPlayQueue::FileSet::const_iterator fi = files.begin();
            fi != files.end(); ++fi) {
        (*fi)->fillBuffers(currentTime);
        if ((*fi)->getEndTime() >= currentTime) {
            if (++allocated == poolSize)
                break;
        } // else the file's ring buffers will have been returned
    }

    releaseLock();
}

bool
AudioFileReader::kick(bool wantLock)
{
    if (wantLock)
        getLock();

    RealTime now = m_driver->getSequencerTime();
    const AudioPlayQueue *queue = m_driver->getAudioQueue();

    bool someFilled = false;

    // Tell files that are playing or will be playing in the next few
    // seconds to update.

    AudioPlayQueue::FileSet playing;

    queue->getPlayingFiles
    (now, RealTime(3, 0) + m_driver->getAudioReadBufferLength(), playing);

    for (AudioPlayQueue::FileSet::iterator fi = playing.begin();
            fi != playing.end(); ++fi) {

        if (!(*fi)->isBuffered()) {
            // fillBuffers has not been called on this file.  This
            // happens when a file is unmuted during playback.  The
            // results are unpredictable because we can no longer
            // synchronise with the correct JACK callback slice at
            // this point, but this is better than allowing the file
            // to update from its start as would otherwise happen.
            (*fi)->fillBuffers(now);
            someFilled = true;
        } else {
            if ((*fi)->updateBuffers())
                someFilled = true;
        }
    }

    if (wantLock)
        releaseLock();

    return someFilled;
}

void
AudioFileReader::threadRun()
{
    while (!m_exiting) {

        //	struct timeval now;
        //	gettimeofday(&now, 0);
        //	RealTime t = RealTime(now.tv_sec, now.tv_usec * 1000);

        bool someFilled = false;

        if (m_driver->areClocksRunning()) {
            someFilled = kick(false);
        }

        if (someFilled) {

            releaseLock();
            getLock();

        } else {

            RealTime bt = m_driver->getAudioReadBufferLength();
            bt = bt / 2;
            if (bt < RealTime(0, 10000000))
                bt = RealTime(0, 10000000); // 10ms minimum

            struct timeval now;
            gettimeofday(&now, nullptr);
            RealTime t = bt + RealTime(now.tv_sec, now.tv_usec * 1000);

            struct timespec timeout;
            timeout.tv_sec = t.sec;
            timeout.tv_nsec = t.nsec;

            pthread_cond_timedwait(&m_condition, &m_lock, &timeout);
            pthread_testcancel();
        }
    }
}



AudioFileWriter::AudioFileWriter(SoundDriver *driver,
                                 unsigned int sampleRate) :
        AudioThread("AudioFileWriter", driver, sampleRate)
{
    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);

    for (InstrumentId id = instrumentBase;
            id < instrumentBase + instrumentCount; ++id) {

        // prefill with zero files in all slots, so that we can
        // refer to the map without a lock (as the number of
        // instruments won't change)

        m_files[id] = FilePair(0, nullptr);
    }
}

AudioFileWriter::~AudioFileWriter()
{}


bool
AudioFileWriter::openRecordFile(InstrumentId id,
                                const QString &fileName)
{
    getLock();

    if (m_files[id].first) {
        releaseLock();
        std::cerr << "AudioFileWriter::openRecordFile: already have record file for instrument " << id << "!" << std::endl;
        return false; // already have one
    }

#ifdef DEBUG_WRITER
    std::cerr << "AudioFileWriter::openRecordFile: instrument id is " << id << std::endl;
#endif

    MappedAudioFader *fader = m_driver->getMappedStudio()->getAudioFader(id);

    RealTime bufferLength = m_driver->getAudioWriteBufferLength();
    size_t bufferSamples = (size_t)RealTime::realTime2Frame(bufferLength, m_sampleRate);
    bufferSamples = ((bufferSamples / 1024) + 1) * 1024;

    if (fader) {
        float fch = 2;
        (void)fader->getProperty(MappedAudioFader::Channels, fch);
        int channels = (int)fch;

        RIFFAudioFile::SubFormat format = m_driver->getAudioRecFileFormat();

        int bytesPerSample = (format == RIFFAudioFile::PCM ? 2 : 4) * channels;
        int bitsPerSample = (format == RIFFAudioFile::PCM ? 16 : 32);

        AudioFile *recordFile = nullptr;

        try {
            recordFile =
                new WAVAudioFile(fileName,
                                 channels,             // channels
                                 m_sampleRate,         // samples per second
                                 m_sampleRate *
                                 bytesPerSample,       // bytes per second
                                 bytesPerSample,       // bytes per frame
                                 bitsPerSample);       // bits per sample

            // open the file for writing
            //
            if (!recordFile->write()) {
                std::cerr << "AudioFileWriter::openRecordFile: failed to open " << fileName << " for writing" << std::endl;
                delete recordFile;
                releaseLock();
                return false;
            }
        } catch (const SoundFile::BadSoundFileException &e) {
            std::cerr << "AudioFileWriter::openRecordFile: failed to open " << fileName << " for writing: " << e.getMessage() << std::endl;
            delete recordFile;
            releaseLock();
            return false;
        }

        RecordableAudioFile *raf = new RecordableAudioFile(recordFile,
                                   bufferSamples);
        m_files[id].second = raf;
        m_files[id].first = recordFile;

#ifdef DEBUG_WRITER

        std::cerr << "AudioFileWriter::openRecordFile: created " << channels << "-channel file at " << fileName << " (id is " << recordFile->getId() << ")" << std::endl;
#endif

        releaseLock();
        return true;
    }

    std::cerr << "AudioFileWriter::openRecordFile: no audio fader for record instrument " << id << "!" << std::endl;
    releaseLock();
    return false;
}


void
AudioFileWriter::write(InstrumentId id,
                       const sample_t *samples,
                       int channel,
                       size_t sampleCount)
{
    if (!m_files[id].first)
        return ; // no file
    if (m_files[id].second->buffer(samples, channel, sampleCount) < sampleCount) {
        m_driver->reportFailure(MappedEvent::FailureDiscOverrun);
    }
}

bool
AudioFileWriter::closeRecordFile(InstrumentId id, AudioFileId &returnedId)
{
    if (!m_files[id].first)
        return false;

    returnedId = m_files[id].first->getId();
    m_files[id].second->setStatus(RecordableAudioFile::DEFUNCT);

#ifdef DEBUG_WRITER

    std::cerr << "AudioFileWriter::closeRecordFile: instrument " << id << " file set defunct (file ID is " << returnedId << ")" << std::endl;
#endif

    // Don't reset the file pointers here; that will be done in the
    // next call to kick().  Doesn't really matter when that happens,
    // but let's encourage it to happen soon just for certainty.
    signal();

    return true;
}

bool
AudioFileWriter::haveRecordFileOpen(InstrumentId id)
{
    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);

    if (id < instrumentBase || id >= instrumentBase + instrumentCount) {
        return false;
    }

    return (m_files[id].first &&
            (m_files[id].second->getStatus() != RecordableAudioFile::DEFUNCT));
}

bool
AudioFileWriter::haveRecordFilesOpen()
{
    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);

    for (InstrumentId id = instrumentBase; id < instrumentBase + instrumentCount; ++id) {

        if (m_files[id].first &&
                (m_files[id].second->getStatus() != RecordableAudioFile::DEFUNCT)) {
#ifdef DEBUG_WRITER
            std::cerr << "AudioFileWriter::haveRecordFilesOpen: found open record file for instrument " << id << std::endl;
#endif

            return true;
        }
    }
#ifdef DEBUG_WRITER
    std::cerr << "AudioFileWriter::haveRecordFilesOpen: nope" << std::endl;
#endif

    return false;
}

void
AudioFileWriter::kick(bool wantLock)
{
    if (wantLock)
        getLock();

    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);

    for (InstrumentId id = instrumentBase;
            id < instrumentBase + instrumentCount; ++id) {

        if (!m_files[id].first)
            continue;

        RecordableAudioFile *raf = m_files[id].second;

        if (raf->getStatus() == RecordableAudioFile::DEFUNCT) {

#ifdef DEBUG_WRITER
            std::cerr << "AudioFileWriter::kick: found defunct file on instrument " << id << std::endl;
#endif

            m_files[id].first = nullptr;
            delete raf; // also deletes the AudioFile
            m_files[id].second = nullptr;

        } else {
#ifdef DEBUG_WRITER
            std::cerr << "AudioFileWriter::kick: writing file on instrument " << id << std::endl;
#endif

            raf->write();
        }
    }

    if (wantLock)
        releaseLock();
}

void
AudioFileWriter::threadRun()
{
    while (!m_exiting) {

        kick(false);

        RealTime t = m_driver->getAudioWriteBufferLength();
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

