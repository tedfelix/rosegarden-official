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

#include <mad.h>

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
                           unsigned int /*channels*/,
                           unsigned int /*sampleRate*/,
                           unsigned int /*bytesPerSecond*/,
                           unsigned int /*bytesPerSample*/,
                           unsigned int /*bitsPerSample*/):
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
        throw(s);
    }

    return true;
}

bool
MP3AudioFile::write()
{
    return false;
}

void
MP3AudioFile::close()
{
}

void
MP3AudioFile::parseHeader()
{
    const std::string MP3_TAG("TAG");
    if (m_inFile == 0)
        return;

    // seek to beginning
    m_inFile->seekg(0, std::ios::beg);

    // get the header string
    //
    std::string hS = getBytes(256);
    int headerLength = 0;

    if (hS.substr(0, 3) == MP3_TAG) 
        headerLength = 127;
    else if (hS.substr(0, 4) == std::string("RIFF"))
        headerLength = 20; // guess for the moment
    else
    {
        headerLength = 10;
    }

    mad_synth synth;
    mad_frame frame;
    mad_stream stream;

    mad_synth_init(&synth);
    mad_stream_init(&stream);
    mad_frame_init(&frame);

    /*
    mad_stream_buffer(&stream, hS.data(), hS.length());

    if (mad_header_decode(&frame.header, &stream) == -1)
    {
        throw("Can't decode header");
    }

    mad_frame_decode(&frame, &stream);

    m_sampleRate = frame.header.samplerate;

    mad_synth_frame(&synth, &frame);
    struct mad_pcm *pcm = &synth.pcm;

    m_channels = pcm->channels;
    */
}

std::streampos
MP3AudioFile::getDataOffset()
{
    return 0;
}

bool 
MP3AudioFile::scanTo(const RealTime &/*time*/)
{
    return false;
}

bool
MP3AudioFile::scanTo(std::ifstream * /*file*/, const RealTime &/*time*/)
{
    return false;
}


// Scan forward in a file by a certain amount of time
//
bool 
MP3AudioFile::scanForward(const RealTime &/*time*/)
{
    return false;
}

bool
MP3AudioFile::scanForward(std::ifstream * /*file*/, const RealTime &/*time*/)
{
    return false;
}


// Return a number of samples - caller will have to
// de-interleave n-channel samples themselves.
//
std::string
MP3AudioFile::getSampleFrames(std::ifstream * /*file*/,
                              unsigned int /*frames*/)
{
    return "";
}

std::string
MP3AudioFile::getSampleFrames(unsigned int /*frames*/)
{
    return "";
}


// Return a number of (possibly) interleaved samples
// over a time slice from current file pointer position.
//
std::string 
MP3AudioFile::getSampleFrameSlice(std::ifstream * /*file*/,
                                  const RealTime &/*time*/)
{
    return "";
}

std::string
MP3AudioFile::getSampleFrameSlice(const RealTime & /*time*/)
{
    return "";
}


// Append a string of samples to an already open (for writing)
// audio file.
//
bool
MP3AudioFile::appendSamples(const std::string &/*buffer*/)
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
