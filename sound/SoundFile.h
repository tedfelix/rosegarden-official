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


#ifndef _SOUNDFILE_H_
#define _SOUNDFILE_H_

// SoundFile is an abstract base class - both MidiFile and AudioFile
// are derived from this class and have to implement the open() and
// write() methods.  There is some additional commonality which will
// hopefully leak through into this class and be removed from its
// descendants.
// 
// A SoundFile is a binary file of specific format.  This class helps
// to manipulate the binary data with some general methods based
// around std::strings.
//
// [rwb]
//
//

#include <iostream>
#include <fstream>
#include <string>


namespace Rosegarden
{


typedef unsigned char FileByte; 

class SoundFile
{
public:
    SoundFile(const std::string &fileName):m_fileName(fileName) {;}
    virtual ~SoundFile() {;}

    virtual bool open() = 0;
    virtual bool write() = 0;

    const std::string& getFilename() { return m_fileName; }
    void setFilename(const std::string &fileName) { m_fileName = fileName; }

    int getLittleEndian(const std::string &s);

protected:
    std::string m_fileName;

    // get some bytes from an input stream
    std::string getBytes(std::ifstream *file, unsigned int numberOfBytes);
 
    // write some bytes to an output stream
    void putBytes(std::ofstream *file,
                  const std::string outputString);


};

};


#endif // _SOUNDFILE_H_


