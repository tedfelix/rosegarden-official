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

#ifndef RG_AUDIO_PROCESS_H
#define RG_AUDIO_PROCESS_H

#include "SoundDriver.h"
#include "base/Instrument.h"
#include "RingBuffer.h"
#include "RecordableAudioFile.h"


namespace Rosegarden
{


class RealTime;

typedef float sample_t;

class AudioThread
{
public:
    AudioThread(const std::string& name, // for diagnostics
                SoundDriver *driver,
                unsigned int sampleRate);

    virtual ~AudioThread();

    // This is to be called by the owning class after construction.
    void run();

    // This is to be called by the owning class to cause the thread to
    // exit and clean up, before destruction.
    void terminate();

    bool running() const { return m_running; }

    int getLock();
    int tryLock();
    int releaseLock();
    void signal();

protected:
    virtual void threadRun() = 0;
    virtual int getPriority() { return 0; }

    std::string       m_name;

    SoundDriver      *m_driver;
    unsigned int      m_sampleRate;

    pthread_t         m_thread;
    pthread_mutex_t   m_lock;
    pthread_cond_t    m_condition;
    bool              m_running;
    volatile bool     m_exiting;

private:
    static void *staticThreadRun(void *arg);
    static void  staticThreadCleanup(void *arg);
};


class AudioInstrumentMixer;

class AudioBussMixer : public AudioThread
{
public:
    AudioBussMixer(SoundDriver *driver,
                   AudioInstrumentMixer *instrumentMixer,
                   unsigned int sampleRate,
                   unsigned int blockSize);

    ~AudioBussMixer() override;

    void kick(bool wantLock = true, bool signalInstrumentMixer = true);

    /**
     * Prebuffer.  This should be called only when the transport is
     * not running.  This also calls fillBuffers on the instrument
     * mixer.
     */
    void fillBuffers(const RealTime &currentTime);

    /**
     * Empty and discard buffer contents.
     */
    void emptyBuffers();

    /**
     * Not sure this is the best place, but we really need this so that
     * we can make the system easily expandable.
     *
     * I think we can only actually have 8, so 16 should be ok.
     * Audio Mixer > Settings > Number of Submasters.
     */
    static int getMaxBussCount()  { return 16; }
    int getBussCount() const {
        return m_bussCount;
    }

    /**
     * A buss is "dormant" if every readable sample on every one of
     * its buffers is zero.  It can therefore be safely skipped during
     * playback.
     */
    bool isBussDormant(int buss) {
        return m_bufferMap[buss].dormant;
    }

    /**
     * Busses are currently always stereo.
     */
    RingBuffer<sample_t> *getRingBuffer(int buss, unsigned int channel) {
        if (channel < (unsigned int)m_bufferMap[buss].buffers.size()) {
            return m_bufferMap[buss].buffers[channel];
        } else {
            return nullptr;
        }
    }

    /// For call from MappedStudio.  Pan is in range -100.0 -> 100.0
    void setBussLevels(int bussId, float dB, float pan);

    /// For call regularly from anywhere in a non-RT thread
    void updateInstrumentConnections();

protected:
    void threadRun() override;

    void processBlocks();
    void generateBuffers();

    AudioInstrumentMixer   *m_instrumentMixer;
    size_t                  m_blockSize;
    int                     m_bussCount;

    std::vector<sample_t *> m_processBuffersBuss;

    struct BufferRec
    {
        BufferRec() : dormant(true), buffers(), instruments(),
                      gainLeft(0.0), gainRight(0.0) { }
        ~BufferRec();

        bool dormant;

        std::vector<RingBuffer<sample_t> *> buffers;
        std::vector<bool> instruments; // index is instrument id minus base

        float gainLeft;
        float gainRight;
    };

    typedef std::map<int, BufferRec> BufferMap;
    BufferMap m_bufferMap;
};

class AudioFileReader : public AudioThread
{
public:
    AudioFileReader(SoundDriver *driver,
                    unsigned int sampleRate);

    ~AudioFileReader() override;

    bool kick(bool wantLock = true);

    /**
     * Prebuffer.  This should be called only when the transport is
     * not running.
     */
    void fillBuffers(const RealTime &currentTime);

protected:
    void threadRun() override;
};


class AudioFileWriter : public AudioThread
{
public:
    AudioFileWriter(SoundDriver *driver,
                    unsigned int sampleRate);

    ~AudioFileWriter() override;

    void kick(bool wantLock = true);

    bool openRecordFile(InstrumentId id, const QString &fileName);
    bool closeRecordFile(InstrumentId id, AudioFileId &returnedId);

    bool haveRecordFileOpen(InstrumentId id);
    // unused bool haveRecordFilesOpen();

    void write(InstrumentId id,
               const sample_t *samples,
               int channel,
               size_t sampleCount);

protected:
    void threadRun() override;

    typedef std::pair<AudioFile *, RecordableAudioFile *> FilePair;
    typedef std::map<InstrumentId, FilePair> FileMap;
    FileMap m_files;
};


}

#endif
