// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*- /*
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


#include "PeakFile.h"
#include "AudioFile.h"
#include "Audio.h"

namespace Rosegarden
{

PeakFile::PeakFile(AudioFile *audioFile):
    SoundFile(audioFile->getPeakFilename()),
    m_audioFile(audioFile)
{
}

PeakFile::~PeakFile()
{
}

bool
PeakFile::open()
{
    m_inFile = new std::ifstream(m_fileName.c_str(),
                                 std::ios::in | std::ios::binary);
    if (!(*m_inFile))
        return false;

    try
    {
        parseHeader();
    }
    catch(std::string s)
    {
        std::cerr << "PeakFile::open - EXCEPTION \"" << s << "\""
                  << std::endl;
        return false;
    }
    
    return true;
}

void
PeakFile::parseHeader()
{
    if (!(*m_inFile)) return;

    m_inFile->seekg(0, std::ios::beg);

    std::string levelIdentifier = getBytes(4);

#if (__GNUC__ < 3)
    if (levelIdentifier.compare(Rosegarden::AUDIO_BWF_PEAK_ID, 0, 4) != 0)
#else
    if (levelIdentifier.compare(0, 4, Rosegarden::AUDIO_BWF_PEAK_ID) != 0)
#endif
    {
        throw(std::string("PeakFile::parseHeader - can't find LEVL identifier"));
    }

    // Get the length of the header minus the first 8 bytes
    //
    int length = getIntegerFromLittleEndian(getBytes(4));

    std::string headerStr = getBytes(length);

}

bool
PeakFile::write()
{
    return write(5); // default update every 5%
}

bool
PeakFile::write(unsigned short undatePercentage)
{
    if (m_outFile)
    {
        m_outFile->close();
        delete m_outFile; 
    }

    // create and test that we've made it
    m_outFile = new std::ofstream(m_fileName.c_str(),
                                  std::ios::out | std::ios::binary);
    if (!(*m_outFile))
        return false;

    // write out the header
    writeHeader(m_outFile);

    // and now the peak values
    writePeaks(m_outFile);

    return false;
}

// Close the peak file and tidy up
//
void
PeakFile::close()
{
    if (m_outFile == 0)
        return;

    m_outFile->close();
    delete m_outFile;
    m_outFile = 0;
}

bool
PeakFile::isValid()
{
    return false;
}

bool
PeakFile::writeToHandle(std::ofstream *file,
                        unsigned short updatePercentage)
{
    return false;
}

void
PeakFile::writeHeader(std::ofstream *file)
{
    if(!file || !(*file)) return;

    std::string header;
    
    // The "levl" identifer for this chunk
    //
    header += AUDIO_BWF_PEAK_ID;

    // Add a four byte version of the size of the header chunk (120
    // bytes from this point onwards)
    //
    header += getLittleEndianFromInteger(128, 4);

    // A four byte version number (always 0 for the minute)
    //
    header += getLittleEndianFromInteger(0, 4);

    // Format of the peak points - 1 = unsigned char
    //                             2 = unsigned short
    header += getLittleEndianFromInteger(1, 4);

    putBytes(file, header);
}

void
PeakFile::writePeaks(std::ofstream *file)
{
    if(!file || !(*file)) return;

    std::string peaks;

    putBytes(file, peaks);
}


};



