// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#include <sstream>

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
        //throw(s);
        return false;
    }

    return true;
}

// Open the file for writing, write out the header and move
// to the data chunk to accept samples.  We fill in all the
// totals when we close().
//
bool
WAVAudioFile::write()
{
    // close if we're open
    if (m_outFile)
    {
        m_outFile->close();
        delete m_outFile;
    }

    // open for writing
    m_outFile = new std::ofstream(m_fileName.c_str(),
                                  std::ios::out | std::ios::binary);

    if (!(*m_outFile))
        return false;

    // write out format header chunk and prepare for sample writing
    //
    writeFormatChunk();

    return true;
}

void
WAVAudioFile::close()
{
    if (m_outFile == 0)
        return;

    m_outFile->seekp(0, std::ios::end);
    unsigned int totalSize = m_outFile->tellp();

    // seek to first length position
    m_outFile->seekp(4, std::ios::beg);

    // write complete file size minus 8 bytes to here
    putBytes(m_outFile, getLittleEndianFromInteger(totalSize - 8, 4));

    // reseek from start forward 40
    m_outFile->seekp(40, std::ios::beg);

    // write the data chunk size to end
    putBytes(m_outFile, getLittleEndianFromInteger(totalSize - 44, 4));

    m_outFile->close();

    delete m_outFile;
    m_outFile = 0;
}

// Set the AudioFile meta data according to WAV file format specification.
//
void
WAVAudioFile::parseHeader()
{
    // Read the format chunk and populate the file data.  A plain WAV
    // file only has this chunk.  Exceptions tumble through.
    //
    readFormatChunk();
   
}

std::streampos
WAVAudioFile::getDataOffset()
{
    return 0;
}



}
