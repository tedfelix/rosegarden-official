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


#include "SoundFile.h"

namespace Rosegarden

{

SoundFile::SoundFile(const std::string &fileName):
    m_fileName(fileName),
    m_readChunkPtr(-1),
    m_readChunkSize(4096), // 4k blocks
    m_inFile(0),
    m_outFile(0)

{
}

SoundFile::~SoundFile()
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

// Read in a specified number of bytes and return them
// as a string.
//
std::string
SoundFile::getBytes(std::ifstream *file, unsigned int numberOfBytes)
{
    if (file->eof())
        throw(std::string("SoundFile::getBytes() - EOF encountered"));

    std::string rS;
    char *fileBytes = new char[numberOfBytes];

    file->read(fileBytes, numberOfBytes);

    for (unsigned int i = 0; i < file->gcount(); i++)
        rS += (unsigned char)fileBytes[i];

    // complain but return
    //
    if (rS.length() < numberOfBytes)
        std::cerr << "SoundFile::getBytes() - couldn't get all bytes ("
                  << rS.length() << " from " << numberOfBytes << ")"
                  << std::endl;

    // clear down
    delete [] fileBytes;

    return rS;
}

// A buffered read based on the current file handle.
//
std::string
SoundFile::getBytes(unsigned int numberOfBytes)
{
    if (m_inFile == 0)
        throw(std::string("SoundFile::getBytes - no open file handle"));

    std::string rS;
    char *fileBytes = new char[m_readChunkSize];

    while (rS.length() < numberOfBytes && !m_inFile->eof())
    {
        if (m_readChunkPtr == -1)
        {
            // clear buffer and load it with new values
            m_readBuffer.erase(0, m_readBuffer.length());

            // reset read pointer
            m_readChunkPtr = 0;

            // Try to read the whole chunk
            //
            m_inFile->read(fileBytes, m_readChunkSize);

            // file->gcount holds the number of bytes we've actually read
            // so copy them across into our string
            //
            for (unsigned int i = 0; i< m_inFile->gcount(); i++)
                m_readBuffer += (unsigned char)fileBytes[i];
        }

        // Can we fulfill our request at this pass?  If so read the
        // bytes across and we'll exit at the end of this loop.
        // m_readChunkPtr keeps our position for next time.
        //
        if (numberOfBytes - rS.length() <= m_readBuffer.length() -
                                           m_readChunkPtr)
        {
            rS += m_readBuffer.substr(m_readChunkPtr,
                                      numberOfBytes - rS.length());

            m_readChunkPtr += (numberOfBytes - rS.length());
        }
        else
        {
            // Fill all we can this time and reset the m_readChunkPtr
            // so that we fetch another chunk of bytes from the file.
            //
            rS += m_readBuffer.substr(m_readChunkPtr,
                                      m_readChunkSize - m_readChunkPtr);
            m_readChunkPtr = -1;
        }

        // We've reached the end of the file if we can't read
        // all of the required 
        if (m_inFile->gcount() != (unsigned int) m_readChunkSize
            || m_inFile->gcount() == 0)
        {
            //cout << "END OF FILE?? GCOUNT = " << m_inFile->gcount() << endl;
            m_readChunkPtr = -1;
            break;
        }

    }

    // complain but return
    //
    if (rS.length() < numberOfBytes)
        std::cerr << "SoundFile::getBytes() - couldn't get all bytes ("
                  << rS.length() << " from " << numberOfBytes << ")"
                  << std::endl;

    delete [] fileBytes;

    return rS;
}


// Write out a sequence of FileBytes to the stream
//
void
SoundFile::putBytes(std::ofstream *file,
                    const std::string oS)
{
    for (unsigned int i = 0; i < oS.length(); i++)
        *file << (FileByte) oS[i];
}


// Clip off any path from the filename
std::string
SoundFile::getShortFilename()
{
    std::string rS = m_fileName;
    unsigned int pos = rS.find_last_of("/");

    if (pos > 0 && ( pos + 1 ) < rS.length())
        rS = rS.substr(pos + 1, rS.length());

    return rS;
}


}

