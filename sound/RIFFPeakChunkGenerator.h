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


// Accepts a file handle positioned somewhere in sample data (could
// be at the start) along with the necessary meta information for
// decoding (channels, bits per sample) and turns the sample data
// into peak data and generates a BWF format peak chunk file.  This
// file can exist by itself (in the case this is being generated 
// by a WAV) or be accomodated inside a BWF format file.
//
//
//

#include <iostream>
#include <fstream>


#ifndef _RIFFPEAKCHUNKGENERATOR_H_
#define _RIFFPEAKCHUNKGENERATOR_H_

namespace Rosegarden
{

class RIFFPeakChunkGenerator
{
public:
    // updatePercentage tells this object how often to throw a
    // percentage complete message - active between 0-100 only
    // if it's set to 5 then we send an update exception every
    // five percent.  The percentage complete is sent with 
    // each exception.
    //
    RIFFPeakChunkGenerator(std::ifstream *in,
                           std::ofstream *out,
                           unsigned bitsPerSample,
                           unsigned int channels,
                           unsigned short updatePercentage);
    ~RIFFPeakChunkGenerator();

    // Generate the peak chunk into the out file stream.
    // This process will send updates of its progress by exception
    // every m_updatePercentage%
    void generate();

protected:

    std::ifstream *m_inFile;
    std::ofstream *m_outFile;
    unsigned int   m_bitsPerSample;
    unsigned int   m_channels;
    unsigned short m_updatePercentage;


};

}


#endif // _RIFFPEAKCHUNKGENERATOR_H_


