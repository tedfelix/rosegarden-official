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

#include <string>
#include <iostream>
#include <fstream>


#ifndef _RIFFPEAKMANAGER_H_
#define _RIFFPEAKMANAGER_H_

namespace Rosegarden
{

class RIFFPeakManager
{
public:
    // updatePercentage tells this object how often to throw a
    // percentage complete message - active between 0-100 only
    // if it's set to 5 then we send an update exception every
    // five percent.  The percentage complete is sent with 
    // each exception.
    //
    RIFFPeakManager(const std::string &fileName,
                    streampos dataChunk,
                    unsigned bitsPerSample,
                    unsigned int channels,
                    bool internal,
                    unsigned short updatePercentage);
    ~RIFFPeakManager();


protected:

    // Generate the peak chunk if necessary (checks for peak file
    // or peak chunk existence).
    //
    void generate();

    // Calculate actual peaks from in file handle and write out -
    // based on interal file specs (channels, bits per sample).
    //
    void calculatePeaks(std::ifstream *in, std::ofstream *out);

    void readHeader();
    void writeHeader();

    std::ifstream *m_inFile;
    std::ofstream *m_outFile;
    std::string    m_riffFileName;
    streampos      m_dataChunk;
    unsigned int   m_bitsPerSample;
    unsigned int   m_channels;
    bool           m_internal;          // do we store peak data in RIFF file?
    unsigned short m_updatePercentage;  // how often we send updates 


};

}


#endif // _RIFFPEAKMANAGER_H_


