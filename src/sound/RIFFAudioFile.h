/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


// Resource Interchange File Formt - a chunk based audio 
// file format.  Type of chunk varies with specialisation
// of this class - WAV files are a specialisation with just
// a format chunk, BWF has more chunks.
//
//

#ifndef RG_RIFFAUDIOFILE_H
#define RG_RIFFAUDIOFILE_H

#include <string>
#include <vector>

#include "AudioFile.h"
#include "base/RealTime.h"

#include <QCoreApplication>

namespace Rosegarden
{

class RIFFAudioFile : public AudioFile
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::RIFFAudioFile)
public:
    RIFFAudioFile(unsigned int id,
                  const std::string &name,
                  const QString &fileName);

    RIFFAudioFile(const QString &fileName,
                  unsigned int channels,
                  unsigned int sampleRate,
                  unsigned int bytesPerSecond,
                  unsigned int bytesPerFrame,
                  unsigned int bitsPerSample);

    ~RIFFAudioFile();

    typedef enum {
        PCM,
        FLOAT
    } SubFormat;

    // Our main control methods - again keeping abstract at this level
    //
    //virtual bool open() = 0;
    //virtual bool write() = 0;
    //virtual void close() = 0;

    // Show the information we have on this file
    //
    virtual void printStats();

    // Slightly dodgy code here - we keep these functions here
    // because I don't want to duplicate them in PlayableRIFFAudioFile
    // and also don't want that class to inherit this one.
    //
    // Of course the file handle we use in might be pointing to
    // any file - for the most part we just assume it's an audio
    // file.
    //
    //
    // Move file pointer to relative time in data chunk -
    // shouldn't be less than zero.  Returns true if the
    // scan time was valid and successful.
    // 
    virtual bool scanTo(const RealTime &time);
    virtual bool scanTo(std::ifstream *file, const RealTime &time);

    // Scan forward in a file by a certain amount of time
    //
    virtual bool scanForward(const RealTime &time);
    virtual bool scanForward(std::ifstream *file, const RealTime &time);

    // Return a number of samples - caller will have to
    // de-interleave n-channel samples themselves.
    //
    virtual std::string getSampleFrames(std::ifstream *file,
                                        unsigned int frames);
    virtual unsigned int getSampleFrames(std::ifstream *file,
                                         char *buf,
                                         unsigned int frames);
    virtual std::string getSampleFrames(unsigned int frames);

    // Return a number of (possibly) interleaved samples
    // over a time slice from current file pointer position.
    //
    virtual std::string getSampleFrameSlice(std::ifstream *file,
                                            const RealTime &time);
    virtual std::string getSampleFrameSlice(const RealTime &time);

    // Append a string of samples to an already open (for writing)
    // audio file.
    //
    virtual bool appendSamples(const std::string &buffer);
    virtual bool appendSamples(const char *buf, unsigned int frames);

    // Get the length of the sample in Seconds/Microseconds
    //
    virtual RealTime getLength();

    // Accessors
    //
    virtual unsigned int getBytesPerFrame() { return m_bytesPerFrame; }
    unsigned int getBytesPerSecond() { return m_bytesPerSecond; }

    // Allow easy identification of wav file type
    //
    static AudioFileType identifySubType(const QString &filename);

    // Convert a single sample from byte format, given the right
    // number of bytes for the sample width
    float convertBytesToSample(const unsigned char *bytes);

    // Decode and de-interleave the given samples that were retrieved
    // from this file or another with the same format as it.  Place
    // the results in the given float buffer.  Return true for
    // success.  This function does crappy resampling if necessary.
    // 
    virtual bool decode(const unsigned char *sourceData,
                        size_t sourceBytes,
                        size_t targetSampleRate,
                        size_t targetChannels,
                        size_t targetFrames,
                        std::vector<float *> &targetData,
                        bool addToResultBuffers = false) = 0;

protected:
    //virtual void parseHeader(const std::string &header);
    //virtual void parseBody();

    // Find and read in the format chunk of a RIFF file - without
    // this chunk we don't actually have a RIFF file.
    //
    void readFormatChunk();

    // Write out the Format chunk from the internal data we have
    //
    void writeFormatChunk();

    SubFormat m_subFormat;
    unsigned int   m_bytesPerSecond;
    unsigned int   m_bytesPerFrame;
};

}


#endif // RG_RIFFAUDIOFILE_H
