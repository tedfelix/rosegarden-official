// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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


// Resource Interchange File Formt - a chunk based audio 
// file format.  Type of chunk varies with specialisation
// of this class - WAV files are a specialisation with just
// a format chunk, BWF has more chunks.
//
//

#ifndef _RIFFAUDIOFILE_H_
#define _RIFFAUDIOFILE_H_

#include <string>
#include <vector>

#include "AudioFile.h"
#include "RealTime.h"

namespace Rosegarden
{

class RIFFAudioFile : public AudioFile
{
public:
    RIFFAudioFile(unsigned int id,
                  const std::string &name,
                  const std::string &fileName);

    RIFFAudioFile(const std::string &fileName,
                  unsigned int channels,
                  unsigned int sampleRate,
                  unsigned int bytesPerSecond,
                  unsigned int bytesPerSample,
                  unsigned int bitsPerSample);

    ~RIFFAudioFile();

    // Our main control methods - again keeping abstract at this level
    //
    virtual bool open() = 0;
    virtual bool write() = 0;
    virtual void close() = 0;

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
    virtual bool scanTo(std::ifstream *file, const RealTime &time);

    // Scan forward in a file by a certain amount of time
    //
    virtual bool scanForward(std::ifstream *file, const RealTime &time);

    // Return a number of samples - caller will have to
    // de-interleave n-channel samples themselves.
    //
    virtual std::string getSampleFrames(std::ifstream *file,
                                        unsigned int frames);

    // Return a number of (possibly) interleaved samples
    // over a time slice from current file pointer position.
    //
    virtual std::string getSampleFrameSlice(std::ifstream *file,
                                            const RealTime &time);

    // Append a string of samples to an already open (for writing)
    // audio file.
    //
    virtual bool appendSamples(const std::string &buffer);

    // Get the length of the sample in Seconds/Microseconds
    //
    virtual RealTime getLength();

    // Write the header
    //
    virtual void writeHeader();

    // Return a normalised representation of the audio file waveform
    // at the required resolution - don't use this for a high
    // definition view of the waveform - just for overviews.  Could
    // in future be optimised with a local data file no doubt.
    // 
    virtual std::vector<float> getPreview(const RealTime &resolution);


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

    unsigned int   m_bytesPerSecond;
    unsigned int   m_bytesPerSample;
};

}


#endif // _RIFFAUDIOFILE_H_
