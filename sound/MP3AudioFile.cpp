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


#include "MP3AudioFile.h"

#ifdef HAVE_LIBMAD

namespace Rosegarden
{

MP3AudioFile::MP3AudioFile(const unsigned int &id,
                           const std::string &name,
                           const std::string &fileName):
    AudioFile(id, name, fileName)
{
    m_type = MP3;
}


MP3AudioFile::MP3AudioFile(const std::string &fileName,
                           unsigned int channels,
                           unsigned int sampleRate,
                           unsigned int bytesPerSecond,
                           unsigned int bytesPerSample,
                           unsigned int bitsPerSample):
    AudioFile(0, std::string(""), fileName)
{
    m_type = MP3;
}


MP3AudioFile::~MP3AudioFile()
{
}

bool
MP3AudioFile::open()
{
    // if already open
    if (m_inFile && (*m_inFile))
        return true;

    m_inFile = new std::ifstream(m_fileName.c_str(),
                                 std::ios::in | std::ios::binary);

    if (!(*m_inFile))
    {
        m_type = UNKNOWN;
        return false;
    }

    // Get the file size and store it for comparison later
    m_fileSize = m_fileInfo->size();

    try
    {
        parseHeader();
    }
    catch(std::string s)
    {
        //throw(s);
#ifdef DEBUG_MP3
        std::cout << "MP3AudioFile::open() parseHeader threw " << s << endl;
#endif
        return false;
    }

    return true;
}

bool
MP3AudioFile::write()
{
}

void
MP3AudioFile::close()
{
}

void
MP3AudioFile::parseHeader()
{
}

std::streampos
MP3AudioFile::getDataOffset()
{
    return 0;
}

bool 
MP3AudioFile::scanTo(const RealTime &time)
{
    return false;
}

bool
MP3AudioFile::scanTo(std::ifstream *file, const RealTime &time)
{
    return false;
}


// Scan forward in a file by a certain amount of time
//
bool 
MP3AudioFile::scanForward(const RealTime &time)
{
    return false;
}

bool
MP3AudioFile::scanForward(std::ifstream *file, const RealTime &time)
{
    return false;
}


// Return a number of samples - caller will have to
// de-interleave n-channel samples themselves.
//
std::string
MP3AudioFile::getSampleFrames(std::ifstream *file,
                              unsigned int frames)
{
    return "";
}

std::string
MP3AudioFile::getSampleFrames(unsigned int frames)
{
    return "";
}


// Return a number of (possibly) interleaved samples
// over a time slice from current file pointer position.
//
std::string 
MP3AudioFile::getSampleFrameSlice(std::ifstream *file,
                                  const RealTime &time)
{
    return "";
}

std::string
MP3AudioFile::getSampleFrameSlice(const RealTime &time)
{
    return "";
}


// Append a string of samples to an already open (for writing)
// audio file.
//
bool
MP3AudioFile::appendSamples(const std::string &buffer)
{
    return false;
}


// Get the length of the sample in Seconds/Microseconds
//
RealTime 
MP3AudioFile::getLength()
{
    return Rosegarden::RealTime(0, 0);
}

void
MP3AudioFile::printStats()
{
}





}

#endif // HAVE_LIBMAD
