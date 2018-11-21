/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2018 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SoundFile]"

#include "SoundFile.h"
#include "base/Profiler.h"
#include "misc/Debug.h"

//#define DEBUG_SOUNDFILE 1

namespace Rosegarden

{

SoundFile::SoundFile(const QString &fileName):
        m_fileName(fileName),
        m_readChunkPtr( -1),
        m_readChunkSize(4096),  // 4k blocks
        m_inFile(nullptr),
        m_outFile(nullptr),
        m_loseBuffer(false),
        m_fileSize(0)
{}

// Tidies up for any dervied classes
//
SoundFile::~SoundFile()
{
    if (m_inFile) {
        m_inFile->close();
        delete m_inFile;
    }

    if (m_outFile) {
        m_outFile->close();
        delete m_outFile;
    }

}

// Read in a specified number of bytes and return them
// as a string.
//
std::string
SoundFile::getBytes(std::ifstream *file, unsigned int numberOfBytes)
{
    if (file->eof()) {
        // Reset the input stream so it's operational again
        //
        file->clear();

        throw(BadSoundFileException(m_fileName, "SoundFile::getBytes() - EOF encountered"));
    }

    if (!(*file)) {
        RG_WARNING << "SoundFile::getBytes() -  stream is not well";
    }


    std::string rS;
    char *fileBytes = new char[numberOfBytes];

    file->read(fileBytes, numberOfBytes);

    for (int i = 0; i < file->gcount(); i++)
        rS += (unsigned char)fileBytes[i];

#ifdef DEBUG_SOUNDFILE
    // complain but return
    //
    if (rS.length() < numberOfBytes)
        RG_WARNING << "SoundFile::getBytes() - couldn't get all bytes ("
        << rS.length() << " from " << numberOfBytes << ")";
#endif

    // clear down
    delete [] fileBytes;

    return rS;
}

// Read a specified number of bytes into a buffer.
//
size_t
SoundFile::getBytes(std::ifstream *file, char *buf, size_t n)
{
    if (!(*file)) {
        RG_WARNING << "SoundFile::getBytes() -  stream is not well";
        return 0;
    }

    if (file->eof()) {
        file->clear();
        return 0;
    }

    file->read(buf, n);
    return file->gcount();
}

// A buffered read based on the current file handle.
//
std::string
SoundFile::getBytes(unsigned int numberOfBytes)
{
    if (m_inFile == nullptr)
        throw(BadSoundFileException(m_fileName, "SoundFile::getBytes - no open file handle"));

    if (m_inFile->eof()) {
        // Reset the input stream so it's operational again
        //
        m_inFile->clear();

        throw(BadSoundFileException(m_fileName, "SoundFile::getBytes() - EOF encountered"));
    }


    // If this flag is set we dump the buffer and re-read it -
    // should be set if specialised class is scanning about
    // when we're doing buffered reads
    //
    if (m_loseBuffer) {
        m_readChunkPtr = -1;
        m_loseBuffer = false;
    }

    std::string rS;
    char *fileBytes = new char[m_readChunkSize];
    int oldLength;

    while (rS.length() < numberOfBytes && !m_inFile->eof()) {
        if (m_readChunkPtr == -1) {
            // clear buffer
            m_readBuffer = "";

            // reset read pointer
            m_readChunkPtr = 0;

            // Try to read the whole chunk
            //
            m_inFile->read(fileBytes, m_readChunkSize);

            // file->gcount holds the number of bytes we've actually read
            // so copy them across into our string
            //
            for (int i = 0; i < m_inFile->gcount(); i++)
                m_readBuffer += (unsigned char)fileBytes[i];
        }

        // Can we fulfill our request at this pass?  If so read the
        // bytes across and we'll exit at the end of this loop.
        // m_readChunkPtr keeps our position for next time.
        //
        if (numberOfBytes - rS.length() <= m_readBuffer.length() -
                m_readChunkPtr) {
            oldLength = rS.length();

            rS += m_readBuffer.substr(m_readChunkPtr,
                                      numberOfBytes - oldLength);

            m_readChunkPtr += rS.length() - oldLength;
        } else {
            // Fill all we can this time and reset the m_readChunkPtr
            // so that we fetch another chunk of bytes from the file.
            //
            rS += m_readBuffer.substr(m_readChunkPtr,
                                      m_readChunkSize - m_readChunkPtr);
            m_readChunkPtr = -1;
        }

        // If we're EOF here we must've read and copied across everything
        // we can do.  Reset and break out.
        //
        if (m_inFile->eof()) {
            m_inFile->clear();
            break;
        }

    }

#ifdef DEBUG_SOUNDFILE
    // complain but return
    //
    if (rS.length() < numberOfBytes)
        RG_WARNING << "SoundFile::getBytes() buffered - couldn't get all bytes ("
        << rS.length() << " from " << numberOfBytes << ")";
#endif

    delete [] fileBytes;

    // Reset and return if EOF
    //
    if (m_inFile->eof())
        m_inFile->clear();

    return rS;
}


// Write out a sequence of FileBytes to the stream
//
void
SoundFile::putBytes(std::ofstream *file,
                    const std::string oS)
{
    for (size_t i = 0; i < oS.length(); i++) {
        *file << (FileByte) oS[i];

        // Every 1024 bytes, kick the event loop.
        if (i % 1024 == 0)
            qApp->processEvents();
    }
}

void
SoundFile::putBytes(std::ofstream *file, const char *buffer, size_t n)
{
    file->write(buffer, n);
}


// Clip off any path from the filename
QString
SoundFile::getShortFilename() const
{
    QString rS = m_fileName;
    size_t pos = rS.lastIndexOf("/");

    if (pos > 0 && ( pos + 1 ) < (size_t)rS.length())
        rS = rS.mid(pos + 1, rS.length());

    return rS;
}


// Turn a little endian binary std::string into an integer
//
int
SoundFile::getIntegerFromLittleEndian(const std::string &s)
{
    int r = 0;

    for (size_t i = 0; i < s.length(); i++) {
        r += (int)(((FileByte)s[i]) << (i * 8));
    }

    return r;
}


// Turn a value into a little endian string of "length"
//
std::string
SoundFile::getLittleEndianFromInteger(unsigned int value, unsigned int length)
{
    std::string r = "";

    do {
        r += (unsigned char)((long)((value >> (8 * r.length())) & 0xff));
    } while (r.length() < length);

    return r;
}

int
SoundFile::getIntegerFromBigEndian(const std::string &/*s*/)
{
    return 0;
}

std::string
SoundFile::getBigEndianFromInteger(unsigned int /*value*/, unsigned int /*length*/)
{
    std::string r;

    return r;
}


}

