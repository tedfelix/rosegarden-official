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

#ifndef _AUDIO_PROCESS_H_
#define _AUDIO_PROCESS_H_

#include "config.h"

#include "SoundDriver.h"
#include "Instrument.h"
#include "RealTime.h"
#include "RingBuffer.h"
#include "RunnablePluginInstance.h"

namespace Rosegarden
{
	
class AudioFileReader;
class AudioFileWriter;

class AudioMixer
{
public:
    typedef float sample_t;

    AudioMixer(SoundDriver *driver,
	       AudioFileReader *fileReader,
	       unsigned int sampleRate,
	       unsigned int blockSize);
    virtual ~AudioMixer();

    void kick(bool waitForLocks = true);

    int getLock();
    int tryLock();
    int releaseLock();

    void setPlugin(InstrumentId id, int position, unsigned int pluginId);
    void removePlugin(InstrumentId id, int position);
    void removeAllPlugins();
    void setPluginPortValue(InstrumentId id, int position,
			    unsigned int port, float value);
    void setPluginBypass(InstrumentId, int position, bool bypass);
    void resetAllPlugins();

    /**
     * Prebuffer.  This should be called only when the transport is
     * not running.
     */
    void fillBuffers(const RealTime &currentTime);

    /**
     * Empty and discard buffer contents.
     */
    void emptyBuffers(RealTime currentTime = RealTime::zeroTime);

    /**
     * An instrument is "dormant" if every readable sample on every
     * one of its buffers is zero.  It can therefore be safely ignored
     * during playback.
     */
    bool isInstrumentDormant(InstrumentId id) {
	return m_bufferMap[id].dormant;
    }

    /**
     * We always have at least two channels (and hence buffers) by
     * this point, because even on a mono instrument we still have a
     * Pan setting which will have been applied by the time we get to
     * these buffers.
     */
    RingBuffer<sample_t> *getRingBuffer(InstrumentId id, unsigned int channel) {
	if (channel < m_bufferMap[id].buffers.size()) {
	    return m_bufferMap[id].buffers[channel];
	} else {
	    return 0;
	}
    }

protected:
    static void *threadRun(void *arg);

    void processBlocks(bool forceFill);
    bool processBlock(InstrumentId id, PlayableAudioFileList&, bool forceFill);
    void generateBuffers();

    SoundDriver      *m_driver;
    AudioFileReader  *m_fileReader;

    unsigned int      m_sampleRate;
    unsigned int      m_blockSize;

    pthread_t         m_thread;
    pthread_mutex_t   m_lock;

    typedef std::map<int, RunnablePluginInstance *> PluginList;
    typedef std::map<InstrumentId, PluginList> PluginMap;
    PluginMap m_plugins;

    // maintain the same number of these as the maximum number of
    // channels on any audio instrument
    std::vector<sample_t *> m_processBuffers;

    struct BufferRec
    {
	BufferRec() : dormant(true), filledTo(RealTime::zeroTime), buffers() { }
	~BufferRec();

	bool dormant;
	size_t zeroFrames;

	RealTime filledTo;
	std::vector<RingBuffer<sample_t> *> buffers;

	float gainLeft;
	float gainRight;
	float volume;
    };

    typedef std::map<InstrumentId, BufferRec> BufferMap;
    BufferMap m_bufferMap;
};


class AudioFileReader
{
public:
    AudioFileReader(SoundDriver *driver,
		    unsigned int sampleRate);
    virtual ~AudioFileReader();

    bool kick(bool waitForLocks = true);

    int getLock();
    int tryLock();
    int releaseLock();

    void updateReadyStatuses(PlayableAudioFileList &audioQueue);
    void updateDefunctStatuses();

protected:
    static void *threadRun(void *arg);

    SoundDriver    *m_driver;
    unsigned int    m_sampleRate;

    pthread_t       m_thread;
    pthread_mutex_t m_lock;
};


class AudioFileWriter
{
public:
    typedef float sample_t;

    AudioFileWriter(SoundDriver *driver,
		    unsigned int sampleRate);
    virtual ~AudioFileWriter();

    void kick(bool waitForLocks = true);

    int getLock();
    int tryLock();
    int releaseLock();

    bool createRecordFile(InstrumentId id, const std::string &fileName);
    bool closeRecordFile(InstrumentId id, AudioFileId &returnedId);

    void write(InstrumentId id, const sample_t *, int channel, size_t samples);

protected:
    static void *threadRun(void *arg);

    SoundDriver    *m_driver;
    unsigned int    m_sampleRate;

    pthread_t       m_thread;
    pthread_mutex_t m_lock;

    typedef std::pair<AudioFile *, RecordableAudioFile *> FilePair;
    typedef std::map<InstrumentId, FilePair> FileMap;
    FileMap m_files;
};


}

#endif

