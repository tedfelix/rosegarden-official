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

// Read in a specified number of bytes and return them
// as a string.
//
const std::string
SoundFile::getBytes(std::ifstream *file,
                    const unsigned int &numberOfBytes)
{
    string rS;
    FileByte fileByte;

    if (file->eof())
        throw(std::string("SoundFile::getBytes() - EOF encountered"));

    while((rS.length() < numberOfBytes) && file->read(&fileByte, 1))
        rS += fileByte;

    if (rS.length() < numberOfBytes)
        throw(std::string("SoundFile::getBytes() - couldn't get all bytes"));

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



// Turn a little endian binary string into an integer
//
int
SoundFile::getLittleEndian(const std::string &s)
{
    int r = 0;
    int shift = 0;

    for (unsigned int i = 0; i < s.length(); i++)
    {
        r += (int)(((FileByte)s[i]) << (i * 8));
    }

    return r;
}


}

