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

#include <string>

#include "RIFFPeakManager.h"

namespace Rosegarden
{

// If the "internal" flag is false we firstly check for the audio file
// names existence.   If it's more recent than the sample file (infile)
// and if so just read from it for any peak requests.  If the peak file
// if older then we optionally regenerate the peak file information.
//
// If no output handle is given (it's 0) then we search for a peak
// file chunk within the infile (BWF format) and if we can't find
// one we optionally generate one.
//
// We shouldgenerate our own copies of file handles so that we don't
// modify the file handles passed in.
//
//
   
RIFFPeakManager::RIFFPeakManager(const std::string &fileName,
                                 streampos dataChunk,
                                 unsigned bitsPerSample,
                                 unsigned int channels,
                                 bool internal,
                                 unsigned short updatePercentage):
    m_riffFileName(fileName),
    m_dataChunk(dataChunk),
    m_bitsPerSample(bitsPerSample),
    m_channels(channels),
    m_internal(internal),
    m_updatePercentage(updatePercentage)
{
    generate();
}

RIFFPeakManager::~RIFFPeakManager()
{
    if (m_inFile)
    {
        m_inFile->close();
        delete m_inFile;
    }
}

// Generate the peak chunk into the out file stream.  This process will
// send updates of its progress by exception every m_updatePercentage%.
//
// Might change this to actually use signals and generate Pixmaps for
// us as well.
//
//

void
RIFFPeakManager::generate()
{

    if (m_internal)
    {
        // do BWF stuff

        // ...search for peak chunk
    }
    else
    {
        // For WAV we generate a peak file name and use this for
        // storing our peak data.
        //
        std::string peakFileName = m_riffFileName + ".pk";

        m_inFile = new std::ifstream(m_riffFileName.c_str(),
                                     std::ios::in | std::ios::binary);

        std::ifstream *peakFile = new std::ifstream(peakFileName.c_str(),
                                                    std::ios::in |
                                                    std::ios::binary);

        // if the peak file doesn't exist then generate it
        if (!(*peakFile))
        {
            // open file for writing
            cout << "CREATING WAV PEAK FILE" << endl;
            std::ofstream *peakOutFile =
                new std::ofstream(peakFileName.c_str(),
                                  std::ios::out | std::ios::binary);

            if (!(*peakOutFile))
            {

                std::string errorString =
                    std::string("RIFFPeakManager::generate - can't open peak file \"")
                    + peakFileName + std::string("\" for writing");
                throw(errorString);
            }


        }
        else
        {
            // if it does exist check the date of the peak data and
            // regenerate it if necessary
        }



    }
}

void
RIFFPeakManager::writeHeader()
{
}

void
RIFFPeakManager::readHeader()
{
}

void
RIFFPeakManager::calculatePeaks(std::ifstream *inFile, std::ofstream *outFile)
{
}


}

