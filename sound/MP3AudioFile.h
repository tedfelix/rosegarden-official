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

#include "config.h"

#ifndef _MP3AUDIOFILE_H_
#define _MP3AUDIOFILE_H_

#ifdef HAVE_LIBMAD

#include "AudioFile.h"

namespace Rosegarden
{

class MP3AudioFile : public AudioFile
{
public:
    MP3AudioFile(const unsigned int &id,
                 const std::string &name,
                 const std::string &fileName);

    MP3AudioFile(const std::string &fileName,
                  unsigned int channels,
                  unsigned int sampleRate,
                  unsigned int bytesPerSecond,
                  unsigned int bytesPerSample,
                  unsigned int bitsPerSample);

    ~MP3AudioFile();

    // Override these methods for the WAV
    //
    virtual bool open();
    virtual bool write();
    virtual void close();

    // Show the information we have on this file
    //
    virtual void printStats();

    // Get all header information
    //
    void parseHeader();

    // Offset to start of sample data
    //
    virtual std::streampos getDataOffset();

    // Peak file name
    //
    virtual std::string getPeakFilename()
        { return (m_fileName + std::string(".pk")); }

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

    // Get the length of the sample in Seconds/Microseconds
    //
    virtual RealTime getLength();

    virtual unsigned int getBytesPerFrame() { return 0; }

};

}

#endif // HAVE_LIBMAD

#endif // _MP3AUDIOFILE_H_

