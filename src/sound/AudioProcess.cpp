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

#include "AudioProcess.h"

#include "AudioInstrumentMixer.h"
#include "RunnablePluginInstance.h"
#include "PlayableAudioFile.h"
#include "RecordableAudioFile.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"
#include "base/AudioLevel.h"
#include "AudioPlayQueue.h"

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
//#define DEBUG_LOCKS 1
//#define DEBUG_READER 1
//#define DEBUG_WRITER 1


namespace Rosegarden
{


/* Branch-free optimizer-resistant denormal killer courtesy of Simon
   Jenkins on LAD: */
// ??? These are defined in two places.  Pull out.

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


AudioThread::AudioThread(const std::string& name,
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
        (void)rv;// shut up compiler warning when the code below is not compiled

#ifdef DEBUG_THREAD_CREATE_DESTROY

        std::cerr << name << "::terminate(): thread exited with return value " << rv << std::endl;
#endif

    }

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << name << "::terminate(): done" << std::endl;
#endif
}

// Due to a limitation of ASan, we can't run it on this routine.
// Remove and recheck periodically.  This is exercised by loading
// an .rg file with audio, play, stop, close.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101476
__attribute__((no_sanitize_address))
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
    for (size_t i = 0; i < m_processBuffersBuss.size(); ++i) {
        delete[] m_processBuffersBuss[i];
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

    size_t bufferSamples1 = m_blockSize;

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
            RingBuffer<sample_t> *rb = new RingBuffer<sample_t>(bufferSamples1);
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

    if (m_processBuffersBuss.empty()) {
        m_processBuffersBuss.push_back(new sample_t[m_blockSize]);
        m_processBuffersBuss.push_back(new sample_t[m_blockSize]);
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

    // cppcheck-suppress allocaCalled
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

            memset(m_processBuffersBuss[0], 0, m_blockSize * sizeof(sample_t));
            memset(m_processBuffersBuss[1], 0, m_blockSize * sizeof(sample_t));

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
                            rb->readAdding(m_processBuffersBuss[ch],
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
                                   m_processBuffersBuss[ch],
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
                    plugin->run(RealTime::zero());

                    ch = 0;

                    while (ch < 2 && ch < plugin->getAudioOutputCount()) {

                        denormalKill(plugin->getAudioOutputBuffers()[ch],
                                     m_blockSize);

                        memcpy(m_processBuffersBuss[ch],
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
                        m_processBuffersBuss[ch][j] *= gain[ch];
                    }
                    rec.buffers[ch]->write(m_processBuffersBuss[ch], m_blockSize);
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

/* unused
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
*/

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
