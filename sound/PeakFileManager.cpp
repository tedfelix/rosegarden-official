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
    m_updatePercentage(0)
{
}

PeakFileManager::~PeakFileManager()
{
}

// Inserts PeakFile based on AudioFile if it doesn't already exist
bool
PeakFileManager::insertAudioFile(AudioFile *audioFile)
{
    std::vector<PeakFile*>::iterator it;

    for (it = m_peakFiles.begin(); it != m_peakFiles.end(); it++)
    {
        if ((*it)->getAudioFile()->getId() == audioFile->getId())
            return false;
    }

    /*
    std::cout << "PeakFileManager::insertAudioFile - creating peak file "
              << m_peakFiles.size() + 1
              << " for \"" << audioFile->getFilename()
              << "\"" << std::endl;
    */

    // Insert
    m_peakFiles.push_back(new PeakFile(audioFile));

    return true;
}

// Removes peak file from PeakFileManager - doesn't affect audioFile
//
bool
PeakFileManager::removeAudioFile(AudioFile *audioFile)
{
    std::vector<PeakFile*>::iterator it;

    for (it = m_peakFiles.begin(); it != m_peakFiles.end(); it++)
    {
        if ((*it)->getAudioFile()->getId() == audioFile->getId())
        {
            m_peakFiles.erase(it);
            return true;
        }
    }

    return false;
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
            // Insert - if we fail we return as empty
            //
            if (insertAudioFile(audioFile) == false)
                return 0;
        }
    }

    return ptr;
}


// Does a given AudioFile have a valid peak file or peak chunk?
//
bool
PeakFileManager::hasValidPeaks(AudioFile *audioFile)
{
    if (audioFile->getType() == WAV)
    {
        // Check external peak file
        PeakFile *peakFile = getPeakFile(audioFile);

        if (peakFile == 0)
        {
            std::cerr << "PeakFileManager::hasValidPeaks - no peak file found"
                      << std::endl;
            return false;
        }
        // If it doesn't open and parse correctly
        if (peakFile->open() == false)
            return false;

        // or if the data is old or invalid
        if (peakFile->isValid() == false)
            return false;

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
                               Progress *progress,
                               unsigned short updatePercentage)
{
    std::cout << "PeakFileManager::generatePeaks - generating peaks for \""
              << audioFile->getFilename() << "\"" << std::endl;

    if (audioFile->getType() == WAV)
    {
        PeakFile *peakFile = getPeakFile(audioFile);

        // Just write out a peak file
        //
        if(peakFile->write(progress, updatePercentage) == false)
        {
            std::string rS = std::string("Can't write peak file for \"") +
                             audioFile->getFilename() +
                             std::string("\" - no preview generated");
            throw(rS);
        }

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

}

std::vector<float>
PeakFileManager::getPreview(AudioFile *audioFile,
                            const RealTime &startIndex,
                            const RealTime &endIndex,
                            int width,
                            bool showMinima)
{
    std::vector<float> rV;

    // If we've got no channels then the audio file hasn't
    // completed (recording) - so don't generate a preview
    //
    if (audioFile->getChannels() == 0)
        return rV;

    if (audioFile->getType() == WAV)
    {
        PeakFile *peakFile = getPeakFile(audioFile);

        // just write out a peak file
        try
        {
            peakFile->open();
            rV = peakFile->getPreview(startIndex,
                                      endIndex,
                                      width,
                                      showMinima);
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

void
PeakFileManager::clear()
{
    std::vector<PeakFile*>::iterator it;

    for (it = m_peakFiles.begin(); it != m_peakFiles.end(); it++)
        delete (*it);

    m_peakFiles.erase(m_peakFiles.begin(), m_peakFiles.end());
}


std::vector<SplitPointPair>
PeakFileManager::getSplitPoints(AudioFile *audioFile,
                                const RealTime &startIndex,
                                const RealTime &endIndex,
                                int threshold)
{
    PeakFile *peakFile = getPeakFile(audioFile);

    if (peakFile == 0) return std::vector<SplitPointPair>();

    return peakFile->getSplitPoints(startIndex,
                                    endIndex,
                                    threshold);

}




}


