/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "BWFAudioFile.h"
#include "base/RealTime.h"

#include <sstream>

namespace Rosegarden
{

BWFAudioFile::BWFAudioFile(const unsigned int &id,
                           const std::string &name,
                           const QString &fileName):
        RIFFAudioFile(id, name, fileName)
{
    m_type = WAV;

}

BWFAudioFile::BWFAudioFile(const QString &fileName,
                           unsigned int channels = 1,
                           unsigned int sampleRate = 48000,
                           unsigned int bytesPerSecond = 6000,
                           unsigned int bytesPerFrame = 2,
                           unsigned int bitsPerSample = 16):
        RIFFAudioFile(0, "", fileName)
{
    m_type = WAV;
    m_bitsPerSample = bitsPerSample;
    m_sampleRate = sampleRate;
    m_bytesPerSecond = bytesPerSecond;
    m_bytesPerFrame = bytesPerFrame;
    m_channels = channels;
}

BWFAudioFile::~BWFAudioFile()
{}

bool
BWFAudioFile::open()
{
    // if already open
    if (m_inFile && (*m_inFile))
        return true;

    m_inFile = new std::ifstream(m_fileName.toLocal8Bit(),
                                 std::ios::in | std::ios::binary);

    if (!(*m_inFile)) {
        m_type = UNKNOWN;
        return false;
    }

    // Get the file size and store it for comparison later
    //
    m_fileSize = m_fileInfo->size();

    try {
        parseHeader();
    } catch (BadSoundFileException s) {
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
BWFAudioFile::write()
{
    // close if we're open
    if (m_outFile) {
        m_outFile->close();
        delete m_outFile;
    }

    // open for writing
    m_outFile = new std::ofstream(m_fileName.toLocal8Bit(),
                                  std::ios::out | std::ios::binary);

    if (!(*m_outFile))
        return false;

    // write out format header chunk and prepare for sample writing
    //
    writeFormatChunk();

    return true;
}

void
BWFAudioFile::close()
{
    if (m_outFile == 0)
        return ;

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
BWFAudioFile::parseHeader()
{
    // Read the format chunk and populate the file data.  A plain WAV
    // file only has this chunk.  Exceptions tumble through.
    //
    readFormatChunk();

}

std::streampos
BWFAudioFile::getDataOffset()
{
    return 0;
}



}
