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

#include "Audio.h"
#include "WAVAudioFile.h"
#include "RealTime.h"
#include "Sound.h"

using std::cout;
using std::cerr;
using std::endl;


namespace Rosegarden
{

WAVAudioFile::WAVAudioFile(const unsigned int &id,
                           const std::string &name,
                           const std::string &fileName):
    RIFFAudioFile(id, name, fileName)
{
    m_type = WAV;
}

WAVAudioFile::WAVAudioFile(const std::string &fileName,
                           unsigned int channels = 1,
                           unsigned int sampleRate = 48000,
                           unsigned int bytesPerSecond = 6000,
                           unsigned int bytesPerSample = 2,
                           unsigned int bitsPerSample = 16):
    RIFFAudioFile(0, "", fileName)
{
    m_type = WAV;
    m_bitsPerSample = bitsPerSample;
    m_sampleRate = sampleRate;
    m_bytesPerSecond = bytesPerSecond;
    m_bytesPerSample = bytesPerSample;
    m_channels = channels;
}

WAVAudioFile::~WAVAudioFile()
{
}

bool
WAVAudioFile::open()
{
    m_inFile = new std::ifstream(m_fileName.c_str(),
                                 std::ios::in | std::ios::binary);

    if (!(*m_inFile))
    {
        m_type = UNKNOWN;
        return false;
    }

    // Get the file size and store it for comparison later
    //
    m_inFile->seekg(0, std::ios::end);
    m_fileSize = m_inFile->tellg();
    m_inFile->seekg(0, std::ios::beg);

    try
    {
        parseHeader();
    }
    catch(std::string s)
    {
        std::cerr << "WAVAudioFile::open - EXCEPTION: \"" << s << "\""
                  << std::endl;
    }

    return true;
}

bool
WAVAudioFile::write()
{
    return true;
}

void
WAVAudioFile::close()
{
}

// Set the m_dataChunkIndex and other meta data
//
void
WAVAudioFile::parseHeader()
{
}

}
