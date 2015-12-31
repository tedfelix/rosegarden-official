/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PLAYABLE_AUDIO_FILE_H
#define RG_PLAYABLE_AUDIO_FILE_H

#include "base/Instrument.h"
#include "RingBuffer.h"
#include "AudioFile.h"
#include "AudioCache.h"

#include <string>
#include <map>

namespace Rosegarden
{

class RingBufferPool;


class PlayableAudioFile
{
public:
    typedef float sample_t;

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

    static void setRingBufferPoolSizes(size_t n, size_t nframes);

    void setStartTime(const RealTime &time) { m_startTime = time; }
    RealTime getStartTime() const { return m_startTime; }

    void setDuration(const RealTime &time) { m_duration = time; }
    RealTime getDuration() const { return m_duration; }
    RealTime getEndTime() const { return m_startTime + m_duration; }

    void setStartIndex(const RealTime &time) { m_startIndex = time; }
    RealTime getStartIndex() const { return m_startIndex; }

    bool isSmallFile() const { return m_isSmallFile; }

    // Get audio file for interrogation
    //
    AudioFile* getAudioFile() const { return m_audioFile; }

    // Get instrument ID - we need to be able to map back
    // at the GUI.
    //
    InstrumentId getInstrument() const { return m_instrumentId; }

    // Return the number of frames currently buffered.  The next call
    // to getSamples on any channel is guaranteed to return at least
    // this many samples.
    //
    size_t getSampleFramesAvailable();

    // Read samples from the given channel on the file and add them
    // into the destination.
    //
    // If insufficient frames are available, this will leave the
    // excess samples unchanged.
    //
    // Returns the actual number of samples written.
    //
    // If offset is non-zero, the samples will be written starting at
    // offset frames from the start of the target block.
    //
    size_t addSamples(std::vector<sample_t *> &target,
                      size_t channels, size_t nframes, size_t offset = 0);

    unsigned int getSourceChannels();
    unsigned int getTargetChannels();
    unsigned int getSourceSampleRate();
    unsigned int getTargetSampleRate();

    unsigned int getBitsPerSample();
    unsigned int getBytesPerFrame();

    // Clear out and refill the ring buffer for immediate
    // (asynchronous) play.
    //
    void fillBuffers();

    // Clear out and refill the ring buffer (in preparation for
    // playback) according to the proposed play time.
    //
    // This call and updateBuffers are not thread-safe (for
    // performance reasons).  They should be called for all files
    // sequentially within a single thread.
    //
    bool fillBuffers(const RealTime &currentTime);

    void clearBuffers();

    // Update the buffer during playback.
    //
    // This call and fillBuffers are not thread-safe (for performance
    // reasons).  They should be called for all files sequentially
    // within a single thread.
    //
    bool updateBuffers();

    // Has fillBuffers been called and completed yet?
    //
    bool isBuffered() const { return m_currentScanPoint > m_startIndex; }

    // Has all the data in this file now been read into the buffers?
    //
    bool isFullyBuffered() const { return m_isSmallFile || m_fileEnded; }

    // Stop playing this file.
    // 
    void cancel() { m_fileEnded = true; }

    // Segment id that allows us to crosscheck against playing audio
    // segments.
    //
    int getRuntimeSegmentId() const { return m_runtimeSegmentId; }
    void setRuntimeSegmentId(int id) { m_runtimeSegmentId = id; }

    // Auto fading of a playable audio file
    //
    bool isAutoFading() const { return m_autoFade; }
    void setAutoFade(bool value) { m_autoFade = value; }

    RealTime getFadeInTime() const { return m_fadeInTime; }
    void setFadeInTime(const RealTime &time) 
        { m_fadeInTime = time; }

    RealTime getFadeOutTime() const { return m_fadeOutTime; }
    void setFadeOutTime(const RealTime &time) 
        { m_fadeOutTime = time; }


protected: 
    void initialise(size_t bufferSize, size_t smallFileSize);
    void checkSmallFileCache(size_t smallFileSize);
    bool scanTo(const RealTime &time);
    void returnRingBuffers();

    RealTime              m_startTime;
    RealTime              m_startIndex;
    RealTime              m_duration;

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

    int                   m_targetChannels;
    int                   m_targetSampleRate;

    bool                  m_fileEnded;
    bool                  m_firstRead;
    static size_t         m_xfadeFrames;
    int                   m_runtimeSegmentId;

    static AudioCache     m_smallFileCache;
    bool                  m_isSmallFile;

    static std::vector<sample_t *> m_workBuffers;
    static size_t         m_workBufferSize;
    
    static char          *m_rawFileBuffer;
    static size_t         m_rawFileBufferSize;

    RingBuffer<sample_t>  **m_ringBuffers;
    static RingBufferPool  *m_ringBufferPool;

    RealTime              m_currentScanPoint;
    size_t                m_smallFileScanFrame;

    bool                  m_autoFade;
    RealTime  m_fadeInTime;
    RealTime  m_fadeOutTime;

private:
    PlayableAudioFile(const PlayableAudioFile &pAF); // not provided
};

}

#endif
