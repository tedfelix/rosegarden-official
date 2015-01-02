/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOFILE_H
#define RG_AUDIOFILE_H

#include <string>
#include <vector>
#include <cmath>

#include <QFileInfo>

#include "SoundFile.h"
#include "base/RealTime.h"

/// An AudioFile maintains information pertaining to an audio sample.
/// This is an abstract base class from which we derive our actual
/// AudioFile types - WAV, BWF, AIFF etc.
///
///

namespace Rosegarden
{

typedef unsigned int AudioFileId;

/// The different types of audio file we support.
///
typedef enum
{

    UNKNOWN, /// not yet known
    WAV,     /// RIFF (Resource Interchange File Format) wav file 
    BWF,     /// RIFF Broadcast Wave File
    AIFF,    /// Audio Interchange File Format
    MP3

} AudioFileType;

class AudioFile : public SoundFile
{
public:
    /// The "read" constructor - open a file
    /// an assign a label and id to it.
    ///
    AudioFile(AudioFileId id,
              const std::string &label,
              const QString &fileName);

    /// The "write" constructor - we only need to
    /// specify a file&label and some parameters and
    /// then write it out.
    ///
    AudioFile(const QString &fileName,
              unsigned int channels,
              unsigned int sampleRate,
              unsigned int bitsPerSample);

    ~AudioFile();

    /// Id of this audio file (used by AudioFileManager)
    ///
    void setId(AudioFileId id) { m_id = id; }
    AudioFileId getId() const { return m_id; }

    /// Label of this sample (NOT the file&label on disk)
    ///
    void setLabel(const std::string &label) { m_label = label; }
    std::string getLabel() const { return m_label; }

    /// NOTE: I inherit getFilename() from SoundFile

    /// Used for waveform interpolation at a point
    ///
    float sinc(float value) { return sin(M_PI * value)/ M_PI * value; }

    /// Audio file identifier - set in constructor of file type
    ///
    AudioFileType getType() const { return m_type; }

    unsigned int getBitsPerSample() const { return m_bitsPerSample; }
    unsigned int getSampleRate() const { return m_sampleRate; }
    unsigned int getChannels() const { return m_channels; }
    
    /// We must continue our main control abstract methods from SoundFile
    /// and add our own close() method that will add any relevant header
    /// information to an audio file that has been written and is now
    /// being closed.
    ///
    virtual bool open() = 0;
    virtual bool write() = 0;
    virtual void close() = 0;

    /// Show the information we have on this file
    ///
    virtual void printStats() = 0;

    /// Move file pointer to relative time in data chunk - shouldn't be
    /// less than zero.  Returns true if the scan time was valid and
    /// successful.  Need two interfaces because when playing we use an
    /// external file handle (one per playback instance - PlayableAudioFile)
    /// 
    virtual bool scanTo(const RealTime &time) = 0;
    virtual bool scanTo(std::ifstream *file, const RealTime &time) = 0;

    /// Scan forward in a file by a certain amount of time - same
    /// double interface (simple one for peak file generation, other
    /// for playback).
    ///
    virtual bool scanForward(const RealTime &time) = 0;
    virtual bool scanForward(std::ifstream *file, const RealTime &time) = 0;

    /// Return a number of samples - caller will have to
    /// de-interleave n-channel samples themselves.
    ///
    virtual std::string getSampleFrames(std::ifstream *file,
                                        unsigned int frames) = 0;

    /// Return a number of samples - caller will have to
    /// de-interleave n-channel samples themselves.  Place
    /// results in buf, return actual number of frames read.
    ///
    virtual unsigned int getSampleFrames(std::ifstream *file,
                                         char *buf,
                                         unsigned int frames) = 0;

    /// Return a number of (possibly) interleaved samples
    /// over a time slice from current file pointer position.
    ///
    virtual std::string getSampleFrameSlice(std::ifstream *file,
                                            const RealTime &time) = 0;

    /// Append a string of samples to an already open (for writing)
    /// audio file.  Caller must have interleaved samples etc.
    ///
    virtual bool appendSamples(const std::string &buffer) = 0;

    /// Append a string of samples to an already open (for writing)
    /// audio file.  Caller must have interleaved samples etc.
    ///
    virtual bool appendSamples(const char *buffer, unsigned int frames) = 0;

    /// Get the length of the sample file in RealTime
    ///
    virtual RealTime getLength() = 0;

    /// Offset to start of sample data
    ///
    virtual std::streampos getDataOffset() = 0;

    /// Return the peak file filename
    ///
    virtual QString getPeakFilename() = 0;

    /// Return the modification timestamp
    ///
    QDateTime getModificationDateTime();

    /// Implement in actual file type
    ///
    virtual unsigned int getBytesPerFrame() = 0;

    /// Decode and de-interleave the given samples that were retrieved
    /// from this file or another with the same format as it.  Place
    /// the results in the given float buffer.  Return true for
    /// success.  This function does crappy resampling if necessary.
    /// 
    virtual bool decode(const unsigned char *sourceData,
                        size_t sourceBytes,
                        size_t targetSampleRate,
                        size_t targetChannels,
                        size_t targetFrames,
                        std::vector<float *> &targetData,
                        bool addToResultBuffers = false) = 0;

protected:
    

    AudioFileType  m_type;   /// AudioFile type
    AudioFileId    m_id;     /// AudioFile ID
    std::string    m_label;  /// AudioFile label (NOT file&label)

    unsigned int   m_bitsPerSample;
    unsigned int   m_sampleRate;
    unsigned int   m_channels;

    /// How many bytes do we read before we get to the data?
    /// Could be huge so we make it a long long. -1 means it
    /// hasn't been set yet.
    ///
    long long      m_dataChunkIndex;

    QFileInfo     *m_fileInfo;


};




}


#endif /// RG_AUDIOFILE_H
