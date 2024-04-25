/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
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
#include "base/RealTime.h"
#include "RingBuffer.h"
#include "AudioFile.h"
#include "AudioCache.h"
#include "PlayableData.h"

#include <QMutex>

#include <vector>
#include <fstream>

namespace Rosegarden
{


class RingBufferPool;


/// Copies data from an AudioFile to a buffer.
/**
 * SoundDriver has an AudioPlayQueue of these.  See SoundDriver::m_audioQueue.
 *
 * AlsaDriver has an AudioPlayQueue of these.  See AlsaDriver::m_audioQueue.
 *
 * AudioInstrumentMixer gets audio data from PlayableAudioFile objects.
 */
 class PlayableAudioFile : public PlayableData
{
public:
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

    //void setStartTime(const RealTime &time) { m_startTime = time; }
    RealTime getStartTime() const override { return m_startTime; }

    //void setDuration(const RealTime &time) { m_duration = time; }
    RealTime getDuration() const override { return m_duration; }
    RealTime getEndTime() const override { return m_startTime + m_duration; }

    //void setStartIndex(const RealTime &time) { m_startIndex = time; }
    //RealTime getStartIndex() const { return m_startIndex; }

    bool isSmallFile() const override { return m_isSmallFile; }

    // Get audio file for interrogation
    //
    AudioFile* getAudioFile() const override { return m_audioFile; }

    // Get instrument ID - we need to be able to map back
    // at the GUI.
    //
    InstrumentId getInstrument() const override { return m_instrumentId; }

    /**
     * Return the number of frames currently buffered.  The next call
     * to getSamples on any channel is guaranteed to return at least
     * this many samples.
     *
     * Used by AudioInstrumentMixer.
     */
    size_t getSampleFramesAvailable() override;

    /**
     * Read samples from the given channel on the file and add them
     * into the destination.
     *
     * If insufficient frames are available, this will leave the
     * excess samples unchanged.
     *
     * Returns the actual number of samples written.
     *
     * If offset is non-zero, the samples will be written starting at
     * offset frames from the start of the target block.
     *
     * Used by AudioInstrumentMixer.
     */
    size_t addSamples
        (std::vector<sample_t *> &target,
         size_t channels, size_t nframes, size_t offset = 0) override;

    unsigned int getTargetChannels() const override;

    //unsigned int getTargetSampleRate();
    //unsigned int getBitsPerSample();


    // Clear out and refill the ring buffer for immediate
    // (asynchronous) play.
    //
    //void fillBuffers();

    // Clear out and refill the ring buffer (in preparation for
    // playback) according to the proposed play time.
    //
    // This call and updateBuffers are not thread-safe (for
    // performance reasons).  They should be called for all files
    // sequentially within a single thread.
    //
    bool fillBuffers(const RealTime &currentTime) override;

    void clearBuffers() override;

    // Update the buffer during playback.
    //
    // This call and fillBuffers are not thread-safe (for performance
    // reasons).  They should be called for all files sequentially
    // within a single thread.
    //
    bool updateBuffers() override;

    // Has fillBuffers been called and completed yet?
    //
    bool isBuffered() const override
    { return m_currentScanPoint > m_startIndex; }

    // Has all the data in this file now been read into the buffers?
    //
    bool isFullyBuffered() const override
    { return m_isSmallFile || m_fileEnded; }

    // Stop playing this file.
    //
    void cancel() override { m_fileEnded = true; }

    // Segment id that allows us to crosscheck against playing audio
    // segments.
    //
    int getRuntimeSegmentId() const override { return m_runtimeSegmentId; }
    void setRuntimeSegmentId(int id) { m_runtimeSegmentId = id; }

    // Auto fading of a playable audio file
    //
    //bool isAutoFading() const { return m_autoFade; }
    void setAutoFade(bool value) { m_autoFade = value; }

    //RealTime getFadeInTime() const { return m_fadeInTime; }
    void setFadeInTime(const RealTime &time)
        { m_fadeInTime = time; }

    //RealTime getFadeOutTime() const { return m_fadeOutTime; }
    void setFadeOutTime(const RealTime &time)
        { m_fadeOutTime = time; }


private:
    // Hide copy ctor and op=.
    PlayableAudioFile(const PlayableAudioFile &);
    PlayableAudioFile &operator=(const PlayableAudioFile &);

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
    unsigned int getSourceChannels() const;
    unsigned int getSourceSampleRate() const;
    unsigned int getBytesPerFrame() const;

    // Originating Instrument Id
    //
    InstrumentId          m_instrumentId;

    int                   m_targetChannels;
    int                   m_targetSampleRate;

    bool                  m_fileEnded;
    bool                  m_firstRead;
    int                   m_runtimeSegmentId = -1;


    static AudioCache     m_smallFileCache;
    bool                  m_isSmallFile;

    static std::vector<sample_t *> m_workBuffers;
    static size_t m_workBufferSize;

    /**
     * m_workBuffers is used by the AudioThread and the SequencerThread,
     * so it needs synchronization.
     */
    static QMutex m_workBuffersMutex;
    static void clearWorkBuffers();

    static char          *m_rawFileBuffer;
    static size_t         m_rawFileBufferSize;

    RingBuffer<sample_t>  **m_ringBuffers;
    static RingBufferPool  *m_ringBufferPool;

    RealTime              m_currentScanPoint;
    size_t                m_smallFileScanFrame;

    bool m_autoFade = false;
    RealTime m_fadeInTime;
    RealTime m_fadeOutTime;
};


}

#endif
