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
#include <vector>

#include "PeakFileManager.h"
#include "AudioFile.h"
#include "RealTime.h"
#include "PeakFile.h"

namespace Rosegarden
{

   
PeakFileManager::PeakFileManager():
    m_riffFileName(""),
    m_dataChunk(0),
    m_bitsPerSample(0),
    m_channels(0),
    m_updatePercentage(0)
{
}

PeakFileManager::~PeakFileManager()
{
}

bool
PeakFileManager::hasValidPeaks(AudioFile *audioFile)
{
    if (audioFile->getType() == WAV)
    {
        // check external peak file
        PeakFile *peakFile = new PeakFile(audioFile);
    }
    else if (audioFile->getType() == BWF)
    {
        // check internal peak chunk
    }
    else
    {
        std::cout << "PeakFileManager::hasValidPeaks - unsupported file type"
                  << std::endl;
        return false;
    }

    return true;

}

// Generate the peak file.  Checks to see if peak file exists
// already and if so if it's up to date.  If it isn't then we
// regenerate.
//
void
PeakFileManager::generatePeaks(AudioFile *audioFile,
        /*const std::string &fileName,
                               streampos dataChunk,
                               unsigned bitsPerSample,
                               unsigned int channels,*/
                               unsigned short updatePercentage)
{
    /*
    m_riffFileName = fileName;
    m_dataChunk = dataChunk;
    m_bitsPerSample = bitsPerSample;
    m_channels = channels;
    m_updatePercentage = updatePercentage;

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
                std::string("PeakFileManager::generate - can't open peak file \"")
                + peakFileName + std::string("\" for writing");
            throw(errorString);
        }


    }
    else
    {
        // if it does exist check the date of the peak data and
        // regenerate it if necessary
    }
    */
}

void
PeakFileManager::writeHeader()
{
}

void
PeakFileManager::readHeader()
{
}

void
PeakFileManager::calculatePeaks(std::ifstream *inFile, std::ofstream *outFile)
{
    throw(std::string("PeakFileManager::calculatePeaks - not enough sample data to create a peak file"));
}

// Generate a QPixmap to a given resolution - this function
// makes full use of the peak files of course to render
// a pixmap quickly for a given resolution.
//
//
QPixmap
PeakFileManager::getPreview(AudioFile *audioFile,
                            const RealTime &startIndex,
                            const RealTime &endIndex,
                            int resolution,
                            int height)
{
    return QPixmap();
}

std::vector<float>
PeakFileManager::getPreview(AudioFile *audioFile,
                            const RealTime &startIndex,
                            const RealTime &endIndex,
                            int resolution)
{
    std::vector<float> returnVector;

    return returnVector;
}

}
