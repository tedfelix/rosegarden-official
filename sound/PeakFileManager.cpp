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
#include "Progress.h"

namespace Rosegarden
{

   
PeakFileManager::PeakFileManager():
    /*
    m_riffFileName(""),
    m_dataChunk(0),
    m_bitsPerSample(0),
    m_channels(0),
    */
    m_updatePercentage(0)
{
}

PeakFileManager::~PeakFileManager()
{
}

// Inserts PeakFile based on AudioFile if it doesn't already exist
void
PeakFileManager::insertAudioFile(AudioFile *audioFile)
{
    std::vector<PeakFile*>::iterator it;

    for (it = m_peakFiles.begin(); it != m_peakFiles.end(); it++)
    {
        if ((*it)->getAudioFile()->getId() == audioFile->getId())
            return;
    }

    // insert
    m_peakFiles.push_back(new PeakFile(audioFile));
}

// Removes peak file from PeakFileManager - doesn't affect audioFile
//
void
PeakFileManager::removeAudioFile(AudioFile *audioFile)
{
    std::vector<PeakFile*>::iterator it;

    for (it = m_peakFiles.begin(); it != m_peakFiles.end(); it++)
    {
        if ((*it)->getAudioFile()->getId() == audioFile->getId())
        {
            m_peakFiles.erase(it);
            return;
        }
    }
}

// Auto-insert PeakFile into manager if it doesn't already exist
//
PeakFile*
PeakFileManager::getPeakFile(AudioFile *audioFile)
{
    std::vector<PeakFile*>::iterator it;
    PeakFile *ptr = 0;

    while (ptr == 0)
    {
        for (it = m_peakFiles.begin(); it != m_peakFiles.end(); it++)
            if ((*it)->getAudioFile()->getId() == audioFile->getId())
                ptr = *it;

        // If nothing is found then insert and retry
        //
        if (ptr == 0)
        {
            insertAudioFile(audioFile);
            for (it = m_peakFiles.begin(); it != m_peakFiles.end(); it++)
                if ((*it)->getAudioFile()->getId() == audioFile->getId())
                    ptr = *it;

        }
    }

    return ptr;
}


// Does a given AudioFile have a valid peak file or peak chunk?
//
bool
PeakFileManager::hasValidPeaks(AudioFile *audioFile)
{
    bool rV = true;

    if (audioFile->getType() == WAV)
    {
        // Check external peak file
        PeakFile *peakFile = getPeakFile(audioFile);

        // If it doesn't open and parse correctly
        if (peakFile->open() == false)
            rV = false;

        // or if the data is old or invalid
        if (peakFile->isValid() == false)
            rV = false;

    }
    else if (audioFile->getType() == BWF)
    {
        // check internal peak chunk
    }
    else
    {
        std::cout << "PeakFileManager::hasValidPeaks - unsupported file type"
                  << std::endl;
        rV = false;
    }

    return rV;

}

// Generate the peak file.  Checks to see if peak file exists
// already and if so if it's up to date.  If it isn't then we
// regenerate.
//
void
PeakFileManager::generatePeaks(AudioFile *audioFile,
                               Progress *progress,
                               unsigned short updatePercentage)
{
    std::cout << "PeakFileManager::generatePeaks - generating peaks for \""
              << audioFile->getFilename() << "\"" << std::endl;

    if (audioFile->getType() == WAV)
    {
        PeakFile *peakFile = getPeakFile(audioFile);

        // just write out a peak file
        peakFile->write(progress, updatePercentage);

        // close writes out important things
        peakFile->close();
    }
    else if (audioFile->getType() == BWF)
    {
        // write the file out and incorporate the peak chunk
    }
    else
    {
        std::cerr << "PeakFileManager::generatePeaks - unsupported file type"
                  << std::endl;
        return;
    }

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

/*
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
*/

// Generate a QPixmap to a given resolution - this function
// makes full use of the peak files of course to render
// a pixmap quickly for a given resolution.
//
//
void
PeakFileManager::drawPreview(AudioFile *audioFile,
                             const RealTime &startIndex,
                             const RealTime &endIndex,
                             QPixmap *pixmap)
{
    if (audioFile->getType() == WAV)
    {
        PeakFile *peakFile = getPeakFile(audioFile);

        try
        {
            peakFile->open();
            peakFile->drawPixmap(startIndex,
                                 endIndex,
                                 pixmap);
        }
        catch(std::string e)
        {
            std::cout << "PeakFileManager::getPreview "
                      << "\"" << e << "\"" << std::endl;
        }
    }
    else if (audioFile->getType() == BWF)
    {
        // write the file out and incorporate the peak chunk
    }
    else
    {
        std::cerr << "PeakFileManager::getPreview - unsupported file type"
                  << std::endl;
    }
        
}

std::vector<float>
PeakFileManager::getPreview(AudioFile *audioFile,
                            const RealTime &startIndex,
                            const RealTime &endIndex,
                            int resolution)
{
    std::vector<float> rV;

    if (audioFile->getType() == WAV)
    {
        PeakFile *peakFile = getPeakFile(audioFile);

        // just write out a peak file
        try
        {
            peakFile->open();
            rV = peakFile->getPreview(startIndex,
                                      endIndex,
                                      resolution);
        }
        catch(std::string e)
        {
            std::cout << "PeakFileManager::getPreview "
                      << "\"" << e << "\"" << std::endl;
        }
    }
    else if (audioFile->getType() == BWF)
    {
        // write the file out and incorporate the peak chunk
    }
    else
    {
        std::cerr << "PeakFileManager::getPreview - unsupported file type"
                  << std::endl;
    }

    return rV;
}

}
