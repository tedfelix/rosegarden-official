/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PeakFileManager]"

#include "PeakFileManager.h"

#include <vector>

#include <QProgressDialog>

#include "AudioFile.h"
#include "base/RealTime.h"
#include "PeakFile.h"
#include "misc/Debug.h"

namespace Rosegarden
{


PeakFileManager::PeakFileManager()
{
}

PeakFileManager::~PeakFileManager()
{
}

bool
PeakFileManager::insertAudioFile(AudioFile *audioFile)
{
    // For each PeakFile
    for (std::vector<PeakFile *>::iterator it = m_peakFiles.begin();
         it != m_peakFiles.end();
         ++it) {
        if ((*it)->getAudioFile()->getId() == audioFile->getId())
            return false;
    }

    //RG_DEBUG << "insertAudioFile() - creating peak file " << m_peakFiles.size() + 1 << " for \"" << audioFile->getFilename() << "\"";

    // Insert
    m_peakFiles.push_back(new PeakFile(audioFile));

    return true;
}

bool
PeakFileManager::removeAudioFile(AudioFile *audioFile)
{
    // For each PeakFile
    for (std::vector<PeakFile *>::iterator it = m_peakFiles.begin();
         it != m_peakFiles.end();
         ++it) {
        if ((*it)->getAudioFile()->getId() == audioFile->getId()) {
            delete *it;
            m_peakFiles.erase(it);
            return true;
        }
    }

    return false;
}

PeakFile *
PeakFileManager::getPeakFile(AudioFile *audioFile)
{
    PeakFile *ptr = 0;

    while (ptr == 0) {
        // For each PeakFile
        for (std::vector<PeakFile *>::iterator it = m_peakFiles.begin();
             it != m_peakFiles.end();
             ++it) {
            if ((*it)->getAudioFile()->getId() == audioFile->getId())
                ptr = *it;
        }

        // If nothing is found then insert and retry
        //
        if (ptr == 0) {
            // Insert - if we fail we return as empty
            //
            // ??? If this would return the pointer, we can get rid of the
            //     while loop.
            if (insertAudioFile(audioFile) == false)
                return 0;
        }
    }

    return ptr;
}

bool
PeakFileManager::hasValidPeaks(AudioFile *audioFile)
{
    if (audioFile->getType() == WAV) {
        // Check external peak file
        PeakFile *peakFile = getPeakFile(audioFile);

        if (peakFile == 0) {
#ifdef DEBUG_PEAKFILEMANAGER
            RG_WARNING << "hasValidPeaks() - no peak file found";
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
        RG_WARNING << "hasValidPeaks() - unsupported file type";
#endif

        return false;
    }

    return true;

}

void
PeakFileManager::generatePeaks(AudioFile *audioFile)
{
#ifdef DEBUG_PEAKFILEMANAGER
    RG_DEBUG << "generatePeaks() - generating peaks for \"" << audioFile->getFilename() << "\"";
#endif

    if (audioFile->getType() == WAV) {
        PeakFile *currentPeakFile = getPeakFile(audioFile);

        currentPeakFile->setProgressDialog(m_progressDialog);

        // Just write out a peak file
        //
        if (currentPeakFile->write() == false) {
            RG_WARNING << "generatePeaks() - Can't write peak file for " << audioFile->getFilename() << " - no preview generated";
            throw BadPeakFileException(
                    audioFile->getFilename(), __FILE__, __LINE__);
        }

        // If we were cancelled, don't leave a partial peak file lying
        // around.
        if (m_progressDialog  &&  m_progressDialog->wasCanceled()) {
            QFile file(currentPeakFile->getFilename());
            file.remove();
            return;
        }

        // close writes out important things
        currentPeakFile->close();

    } else if (audioFile->getType() == BWF) {
        // write the file out and incorporate the peak chunk
        RG_WARNING << "generatePeaks() - unsupported file type: BWF";
    } else {
        RG_WARNING << "generatePeaks() - unknown file type";
    }
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
            RG_WARNING << "getPreview() - \"" << e << "\"";
#endif

            throw BadPeakFileException(e);
        }
    } else if (audioFile->getType() == BWF) {
        // write the file out and incorporate the peak chunk
    }
#ifdef DEBUG_PEAKFILEMANAGER
    else {
        RG_WARNING << "getPreview() - unsupported file type";
    }
#endif

    return rV;
}

void
PeakFileManager::clear()
{
    // Delete the PeakFile objects.
    for (std::vector<PeakFile *>::iterator it = m_peakFiles.begin();
         it != m_peakFiles.end();
         ++it)
        delete (*it);

    m_peakFiles.erase(m_peakFiles.begin(), m_peakFiles.end());
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


}
