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

#ifndef _PLAYABLE_AUDIO_FILE_H_
#define _PLAYABLE_AUDIO_FILE_H_

#include "Instrument.h"
#include "RingBuffer.h"
#include "RealTime.h"
#include "AudioFile.h"

#include <string>
#include <map>

namespace Rosegarden
{

// PlayableAudioFile is queued on the m_audioPlayQueue and
// played by processAudioQueue() in Sequencer.  State changes
// through playback and it's finally discarded when done.
//
// To enable us to have multiple file handles on real AudioFile
// we store the file handle with this class.
//
//
class PlayableAudioFile
{
public:
    typedef float sample_t;

    // PlayableAudioFile does not set its own status; the code that
    // uses it can set it appropriately.

    typedef enum
    {
        IDLE,     // on the queue for some point in the future
	READY,
        PLAYING,
        DEFUNCT   // finished, ready to garbage collect
    } PlayStatus;

    PlayableAudioFile(InstrumentId instrumentId,
                      AudioFile *audioFile,
                      const RealTime &startTime,
                      const RealTime &startIndex,
                      const RealTime &duration,
		      size_t bufferSize = 4096,
		      size_t smallFileSize = 131072,
		      int targetChannels = -1, // default same as file
		      int targetSampleRate = -1); // default same as file
    ~PlayableAudioFile();

    bool mlock();

    void setStatus(const PlayStatus &status) { m_status = status; }
    PlayStatus getStatus() const { return m_status; }

    void setStartTime(const RealTime &time) { m_startTime = time; }
    RealTime getStartTime() const { return m_startTime; }

    void setDuration(const RealTime &time) { m_duration = time; }
    RealTime getDuration() const { return m_duration; }
    RealTime getEndTime() const { return m_startTime + m_duration; }

    void setStartIndex(const RealTime &time) { m_startIndex = time; }
    RealTime getStartIndex() const { return m_startIndex; }

    // Get audio file for interrogation
    //
    AudioFile* getAudioFile() const { return m_audioFile; }

    // Get instrument ID - we need to be able to map back
    // at the GUI.
    //
    InstrumentId getInstrument() const { return m_instrumentId; }

    // Reaches through to AudioFile interface using our local file handle
    //
    bool scanTo(const RealTime &time);

    // Return the number of frames currently buffered.  The next call
    // to getSamples on any channel is guaranteed to return at least
    // this many samples.
    //
    size_t getSampleFramesAvailable();

    // Read samples from the given channel on the file and write
    // them into the destination.
    //
    // If insufficient frames are available, this will return zeros
    // for the excess.  Note that it is theoretically possible for
    // different numbers of samples to be available on different
    // channels -- getSampleFramesAvailable will tell you the
    // minimum across all channels.
    //
    // Returns the actual number of samples written.
    //
    size_t getSamples(sample_t *destination, int channel, size_t samples);

    // Read samples from the given channel on the file and add them
    // into the destination.
    //
    // If insufficient frames are available, this will leave the
    // excess samples unchanged.
    //
    // Returns the actual number of samples written.
    //
    size_t addSamples(sample_t *destination, int channel, size_t samples);

    // Skip a bunch of samples from the file.  Useful if you only
    // want to read a subset of the available channels.
    //
    size_t skipSamples(int channel, size_t samples);

    // Return a single sample from a single channel.  Called
    // repeatedly this is obviously slower than calling
    // getSampleFrames once, but it's not too bad if you're
    // keen to save buffer memory elsewhere.
    //
    sample_t getSample(int channel);

    unsigned int getSourceChannels();
    unsigned int getTargetChannels();
    unsigned int getSourceSampleRate();
    unsigned int getTargetSampleRate();

    unsigned int getBitsPerSample();
    unsigned int getBytesPerFrame();

    // Test whether the file would be buffered if we called fillBuffers
    // with the given time argument.
    //
    bool isBufferable(const RealTime &currentTime);

    // Clear out and refill the ring buffer for immediate
    // (asynchronous) play.
    //
    void fillBuffers();

    // Clear out and refill the ring buffer (in preparation for
    // playback) according to the proposed play time -- which is
    // assumed to be the start time of the next process slice, rather
    // than necessarily the time now.  Returns true if the play time
    // was sufficiently close for the ring buffer to have been updated
    // with some real data.  You should not set PLAYING status on this
    // file until you have seen a true return from this method.
    //
    bool fillBuffers(const RealTime &currentTime);

    // Update the buffer during playback.  This should only be called
    // when the file's status is PLAYING, i.e. after fillBuffer
    // above has found some actual work to do.
    //
    void updateBuffers();

    // Has all the data in this file now been read into the buffers?
    //
    bool isFullyBuffered() const { return m_fileEnded; }

    // Has all the data in this file now been read out of the buffers?
    //
    bool isFinished() const;

    void setReadyTime(RealTime rt) { m_readyTime = rt; }
    RealTime getReadyTime() const { return m_readyTime; }

    // Segment id that allows us to crosscheck against playing audio
    // segments.
    //
    int getRuntimeSegmentId() const { return m_runtimeSegmentId; }
    void setRuntimeSegmentId(int id) { m_runtimeSegmentId = id; }

protected: 
    void initialise(size_t bufferSize);
    void checkSmallFileCache();

    RealTime              m_startTime;
    RealTime              m_startIndex;
    RealTime              m_duration;
    PlayStatus            m_status;

    // Performance file handle - must open non-blocking to
    // allow other potential PlayableAudioFiles access to
    // the same file.
    //
    std::ifstream        *m_file;

    // AudioFile handle
    //
    AudioFile            *m_audioFile;

    // Originating Instrument Id
    //
    InstrumentId          m_instrumentId;

    size_t                m_ringBufferThreshold;
    int                   m_targetChannels;
    int                   m_targetSampleRate;

    // Have we initialised yet?  Don't do this from the constructor as we want
    // the disk thread to pick up the slack.
    //
    bool                  m_initialised;
    bool                  m_fileEnded;

    int                   m_runtimeSegmentId;
    RealTime              m_readyTime;

    // simple cache: map AudioFile ptr to a buffer containing whole contents
    typedef std::pair<int, std::string> SmallFileData;
    typedef std::map<void *, SmallFileData> SmallFileMap;

    static SmallFileMap   m_smallFileCache;
    size_t                m_smallFileIndex;
    size_t                m_smallFileSize;
    bool                  m_isSmallFile;

    std::vector<RingBuffer<sample_t> *> m_ringBuffers; // one per channel

private:
    PlayableAudioFile(const PlayableAudioFile &pAF); // not provided
};


// A wrapper class for writing out a recording file.  We assume the
// data is provided by a process thread and the writes are requested
// by a disk thread.
//
class RecordableAudioFile
{
public:
    typedef float sample_t;

    typedef enum
    {
        IDLE,
	RECORDING,
        DEFUNCT
    } RecordStatus;

    RecordableAudioFile(AudioFile *audioFile, // should be already open for writing
			size_t bufferSize);
    ~RecordableAudioFile();

    void setStatus(const RecordStatus &status) { m_status = status; }
    RecordStatus getStatus() const { return m_status; }

    void buffer(const sample_t *data, int channel, size_t frames);
    void write();

protected:
    AudioFile            *m_audioFile;
    RecordStatus          m_status;

    std::vector<RingBuffer<sample_t> *> m_ringBuffers; // one per channel
};

}

#endif
