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

class AudioThread
{
public:
    typedef float sample_t;

    AudioThread(std::string name, // for diagnostics
		SoundDriver *driver,
		unsigned int sampleRate);

    virtual ~AudioThread();

    virtual void run();

    int getLock();
    int tryLock();
    int releaseLock();
    void signal();

protected:
    virtual void threadRun() = 0;

    std::string       m_name;

    SoundDriver      *m_driver;
    unsigned int      m_sampleRate;

    pthread_t         m_thread;
    pthread_mutex_t   m_lock;
    pthread_cond_t    m_condition;

private:
    static void *staticThreadRun(void *arg);
};
    

class AudioInstrumentMixer;

class AudioBussMixer : public AudioThread
{
public:
    AudioBussMixer(SoundDriver *driver,
		   AudioInstrumentMixer *instrumentMixer,
		   unsigned int sampleRate,
		   unsigned int blockSize);
    virtual ~AudioBussMixer();

    void kick(bool wantLock = true);
    
    /**
     * Prebuffer.  This should be called only when the transport is
     * not running.
     */
    void fillBuffers();

    /**
     * Empty and discard buffer contents.
     */
    void emptyBuffers();

    int getBussCount() {
	return m_bussCount;
    }

    /**
     * A buss is "dormant" if every readable sample on every one of
     * its buffers is zero.  It can therefore be safely ignored during
     * playback.
     */
    bool isBussDormant(int buss) {
	return m_bufferMap[buss].dormant;
    }

    /**
     * Busses are currently always stereo.
     */
    RingBuffer<sample_t> *getRingBuffer(int buss, unsigned int channel) {
	if (channel < m_bufferMap[buss].buffers.size()) {
	    return m_bufferMap[buss].buffers[channel];
	} else {
	    return 0;
	}
    }

protected:
    virtual void threadRun();

    void processBlocks();
    void generateBuffers();

    AudioInstrumentMixer   *m_instrumentMixer;
    unsigned int            m_blockSize;
    int                     m_bussCount;

    std::vector<sample_t *> m_processBuffers;

    struct BufferRec
    {
	BufferRec() : dormant(true), buffers() { }
	~BufferRec();

	bool dormant;

	std::vector<RingBuffer<sample_t> *> buffers;
    };

    typedef std::map<int, BufferRec> BufferMap;
    BufferMap m_bufferMap;
};		   


class AudioFileReader;
class AudioFileWriter;

class AudioInstrumentMixer : public AudioThread
{
public:
    AudioInstrumentMixer(SoundDriver *driver,
			 AudioFileReader *fileReader,
			 unsigned int sampleRate,
			 unsigned int blockSize);
    virtual ~AudioInstrumentMixer();

    void kick(bool wantLock = true);

    void setBussMixer(AudioBussMixer *mixer) { m_bussMixer = mixer; }

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
    RingBuffer<sample_t, 2> *getRingBuffer(InstrumentId id, unsigned int channel) {
	if (channel < m_bufferMap[id].buffers.size()) {
	    return m_bufferMap[id].buffers[channel];
	} else {
	    return 0;
	}
    }

protected:
    virtual void threadRun();

    void processBlocks(bool forceFill, bool &readSomething);
    void processEmptyBlocks(InstrumentId id);
    bool processBlock(InstrumentId id, PlayableAudioFileList&, bool forceFill,
		      bool &readSomething);
    void generateBuffers();

    AudioFileReader  *m_fileReader;
    AudioBussMixer   *m_bussMixer;
    unsigned int      m_blockSize;

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

	bool empty;
	bool dormant;
	size_t zeroFrames;

	RealTime filledTo;
	size_t channels;
	std::vector<RingBuffer<sample_t, 2> *> buffers;

	float gainLeft;
	float gainRight;
	float volume;
    };

    typedef std::map<InstrumentId, BufferRec> BufferMap;
    BufferMap m_bufferMap;
};


class AudioFileReader : public AudioThread
{
public:
    AudioFileReader(SoundDriver *driver,
		    unsigned int sampleRate);
    virtual ~AudioFileReader();

    bool kick(bool wantLock = true);

    void updateReadyStatuses(PlayableAudioFileList &audioQueue);
    void updateDefunctStatuses();

protected:
    virtual void threadRun();
};


class AudioFileWriter : public AudioThread
{
public:
    AudioFileWriter(SoundDriver *driver,
		    unsigned int sampleRate);
    virtual ~AudioFileWriter();

    void kick(bool wantLock = true);

    bool createRecordFile(InstrumentId id, const std::string &fileName);
    bool closeRecordFile(InstrumentId id, AudioFileId &returnedId);

    void write(InstrumentId id, const sample_t *, int channel, size_t samples);

protected:
    virtual void threadRun();

    typedef std::pair<AudioFile *, RecordableAudioFile *> FilePair;
    typedef std::map<InstrumentId, FilePair> FileMap;
    FileMap m_files;
};


}

#endif

