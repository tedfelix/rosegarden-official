/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.
 
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

#define RG_MODULE_STRING "[PeakFileManager]"

#include "PeakFileManager.h"

#include <string>
#include <vector>

#include <QObject>

#include "AudioFile.h"
#include "base/RealTime.h"
#include "PeakFile.h"
#include "misc/Debug.h"

namespace Rosegarden
{


PeakFileManager::PeakFileManager():
        m_currentPeakFile(0)
{}

PeakFileManager::~PeakFileManager()
{}

// Inserts PeakFile based on AudioFile if it doesn't already exist
bool
PeakFileManager::insertAudioFile(AudioFile *audioFile)
{
    std::vector<PeakFile*>::iterator it;

    for (it = m_peakFiles.begin(); it != m_peakFiles.end(); ++it) {
        if ((*it)->getAudioFile()->getId() == audioFile->getId())
            return false;
    }

    /*
    RG_DEBUG << "PeakFileManager::insertAudioFile - creating peak file "
              << m_peakFiles.size() + 1
              << " for \"" << audioFile->getFilename()
              << "\"";
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

    for (it = m_peakFiles.begin(); it != m_peakFiles.end(); ++it) {
        if ((*it)->getAudioFile()->getId() == audioFile->getId()) {
            if (m_currentPeakFile == *it)
                m_currentPeakFile = 0;
            delete *it;
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

    while (ptr == 0) {
        for (it = m_peakFiles.begin(); it != m_peakFiles.end(); ++it)
            if ((*it)->getAudioFile()->getId() == audioFile->getId())
                ptr = *it;

        // If nothing is found then insert and retry
        //
        if (ptr == 0) {
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
    if (audioFile->getType() == WAV) {
        // Check external peak file
        PeakFile *peakFile = getPeakFile(audioFile);

        if (peakFile == 0) {
#ifdef DEBUG_PEAKFILEMANAGER
            RG_WARNING << "PeakFileManager::hasValidPeaks - no peak file found";
#endif

            return false;
        }
        // If it doesn't open and parse correctly
        if (peakFile->open() == false)
            return false;

        // or if the data is old or invalid
        if (peakFile->isValid() == false)
            return false;

    } else if (audioFile->getType() == BWF) {
        // check internal peak chunk
    } else {
#ifdef DEBUG_PEAKFILEMANAGER
        RG_WARNING << "PeakFileManager::hasValidPeaks - unsupported file type";
#endif

        return false;
    }

    return true;

}

// Generate the peak file.  Checks to see if peak file exists
// already and if so if it's up to date.  If it isn't then we
// regenerate.
//
void
PeakFileManager::generatePeaks(AudioFile *audioFile)
{
#ifdef DEBUG_PEAKFILEMANAGER
    RG_DEBUG << "PeakFileManager::generatePeaks - generating peaks for \""
    << audioFile->getFilename() << "\"";
#endif

    if (audioFile->getType() == WAV) {
        m_currentPeakFile = getPeakFile(audioFile);

        QObject::connect(m_currentPeakFile, SIGNAL(setValue(int)),
                         this, SIGNAL(setValue(int)));

        // Just write out a peak file
        //
        if (m_currentPeakFile->write() == false) {
            RG_WARNING << "Can't write peak file for " << audioFile->getFilename() << " - no preview generated";
            throw BadPeakFileException
            (audioFile->getFilename(), __FILE__, __LINE__);
        }

        // The m_currentPeakFile might have been cancelled (see stopPreview())
        //
        if (m_currentPeakFile) {
            // close writes out important things
            m_currentPeakFile->close();
            m_currentPeakFile->disconnect();
        }
    } else if (audioFile->getType() == BWF) {
        // write the file out and incorporate the peak chunk
    } else {
#ifdef DEBUG_PEAKFILEMANAGER
        RG_WARNING << "PeakFileManager::generatePeaks - unsupported file type";
#endif

        return ;
    }

    m_currentPeakFile = 0;

}

std::vector<float>
PeakFileManager::getPreview(AudioFile *audioFile,
                            const RealTime &startTime,
                            const RealTime &endTime,
                            int width,
                            bool showMinima)
{
    std::vector<float> rV;

    // If we've got no channels then the audio file hasn't
    // completed (recording) - so don't generate a preview
    //
    if (audioFile->getChannels() == 0)
        return rV;

    if (audioFile->getType() == WAV) {
        PeakFile *peakFile = getPeakFile(audioFile);

        // just write out a peak file
        try {
            peakFile->open();
            rV = peakFile->getPreview(startTime,
                                      endTime,
                                      width,
                                      showMinima);
        } catch (SoundFile::BadSoundFileException e) {
#ifdef DEBUG_PEAKFILEMANAGER
            RG_WARNING << "PeakFileManager::getPreview \"" << e << "\"";
#else

            ;
#endif

            throw BadPeakFileException(e);
        }
    } else if (audioFile->getType() == BWF) {
        // write the file out and incorporate the peak chunk
    }
#ifdef DEBUG_PEAKFILEMANAGER
    else {
        RG_WARNING << "PeakFileManager::getPreview - unsupported file type";
    }
#endif

    return rV;
}

void
PeakFileManager::clear()
{
    std::vector<PeakFile*>::iterator it;

    for (it = m_peakFiles.begin(); it != m_peakFiles.end(); ++it)
        delete (*it);

    m_peakFiles.erase(m_peakFiles.begin(), m_peakFiles.end());

    m_currentPeakFile = 0;
}


std::vector<SplitPointPair>
PeakFileManager::getSplitPoints(AudioFile *audioFile,
                                const RealTime &startTime,
                                const RealTime &endTime,
                                int threshold,
                                const RealTime &minTime)
{
    PeakFile *peakFile = getPeakFile(audioFile);

    if (peakFile == 0)
        return std::vector<SplitPointPair>();

    return peakFile->getSplitPoints(startTime,
                                    endTime,
                                    threshold,
                                    minTime);

}

void
PeakFileManager::stopPreview()
{
    if (m_currentPeakFile) {
        // Stop processing
        //
        QString fileName = m_currentPeakFile->getFilename();
        m_currentPeakFile->setProcessingPeaks(false);
        m_currentPeakFile->disconnect();

        QFile file(fileName);
        bool removed = file.remove();

#ifdef DEBUG_PEAKFILEMANAGER
        if (removed) {
            RG_DEBUG << "PeakFileManager::stopPreview() - removed preview";
        }
#else
        (void)removed;
#endif
        //delete m_currentPeakFile;
        m_currentPeakFile = 0;
    }
}




}


