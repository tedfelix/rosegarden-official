// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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


#include "AudioFile.h"

namespace Rosegarden

{

AudioFile::AudioFile(unsigned int id,
                     const std::string &name,
                     const std::string &fileName):
    SoundFile(fileName),
    m_type(UNKNOWN),
    m_id(id),
    m_name(name),
    m_bitsPerSample(0),
    m_sampleRate(0),
    m_channels(0),
    m_fileSize(0),
    m_inFile(0),
    m_outFile(0)
{
}

AudioFile::AudioFile(const std::string &fileName,
                     unsigned int channels = 1,
                     unsigned int sampleRate = 48000,
                     unsigned int bitsPerSample = 16):
    SoundFile(fileName),
    m_type(UNKNOWN),
    m_id(0),
    m_name(""),
    m_bitsPerSample(bitsPerSample),
    m_sampleRate(sampleRate),
    m_channels(channels),
    m_fileSize(0),
    m_inFile(0),
    m_outFile(0)
{
}

AudioFile::~AudioFile()
{
    if (m_inFile)
    {
        m_inFile->close();
        delete m_inFile;
    }

    if (m_outFile)
    {
        m_outFile->close();
        delete m_outFile;
    }
}



// Turn a little endian binary std::string into an integer
//
int
AudioFile::getIntegerFromLittleEndian(const std::string &s)
{
    int r = 0;

    for (unsigned int i = 0; i < s.length(); i++)
    {
        r += (int)(((FileByte)s[i]) << (i * 8));
    }

    return r;
}

// Turn a value into a little endian string of "length"
//
std::string
AudioFile::getLittleEndianFromInteger(unsigned int value, unsigned int length)
{
    std::string r = "";

    do
    {
        r += (unsigned char)((long)((value >> (8 * r.length())) & 0xff));
    }
    while (r.length() < length);

    return r;
}

int
AudioFile::getIntegerFromBigEndian(const std::string &s)
{
    return 0;
}

std::string
AudioFile::getBigEndianFromInteger(unsigned int value, unsigned int length)
{
    std::string r;

    return r;
}


}

