// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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


#include <iostream>
#include <fstream>
#include <string>
#include <dirent.h> // for new recording file
#include <cstdio>   // sprintf

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

#include <kapp.h>
#include <klocale.h>

#include <qpixmap.h>
#include <qpainter.h>

#include "AudioFile.h"
#include "AudioFileManager.h"
#include "WAVAudioFile.h"
#include "BWFAudioFile.h"

namespace Rosegarden
{

AudioFileManager::AudioFileManager()
{
    // Set this through the set method so that the tilde gets
    // shaken out.
    //
    setAudioPath("~/rosegarden");

    // Retransmit progress
    //
    connect(&m_peakManager, SIGNAL(setProgress(int)),
            this,           SIGNAL(setProgress(int)));
}

AudioFileManager::~AudioFileManager()
{
    clear();
}

// Add a file from an absolute path
//
AudioFileId
AudioFileManager::addFile(const std::string &filePath)
{
    QString ext =
        QString(filePath.substr(filePath.length() - 3, 3).c_str()).lower();

    if (ext != "wav")
    {
        throw(i18n("Unsupported audio file format"));
    }

    // identify file type
    AudioFileType subType = RIFFAudioFile::identifySubType(filePath);

    // prepare for audio file
    AudioFile *aF = 0;
    AudioFileId id = getFirstUnusedID();

    if (subType == BWF)
    {
        std::cout << "FOUND BWF" << std::endl;
        aF = new BWFAudioFile(id, getShortFilename(filePath), filePath);
    }
    else if (subType == WAV)
    {
        aF = new WAVAudioFile(id, getShortFilename(filePath), filePath);
    }

    // Add file type on extension
    try
    { 
        if (aF->open() == false)
        {
            delete aF;
            throw(i18n("Can't open audiofile"));
        }
    }
    catch(std::string e)
    {
        // catch and rethrow
        //
        delete aF;
        throw(e);
    }

    m_audioFiles.push_back(aF);

    return id;
}

// Convert long filename to shorter version
std::string
AudioFileManager::getShortFilename(const std::string &fileName)
{
    std::string rS = fileName;
    unsigned int pos = rS.find_last_of("/");

    if (pos > 0 && ( pos + 1 ) < rS.length())
        rS = rS.substr(pos + 1, rS.length());

    return rS;
}

// Turn a long path into a directory ending with a slash
//
std::string
AudioFileManager::getDirectory(const std::string &path)
{
    std::string rS = path;
    unsigned int pos = rS.find_last_of("/");

    if (pos > 0 && ( pos + 1 ) < rS.length())
        rS = rS.substr(0, pos + 1);

    return rS;
}


// Create a new AudioFile with unique ID and label - insert from
// our RG4 file
//
AudioFileId
AudioFileManager::insertFile(const std::string &name,
                             const std::string &fileName)
{
    // first try to expany any beginning tilde
    std::string foundFileName = substituteTildeForHome(fileName);

    // if we've expanded and there's no absolute path available
    // then try to find it in audio file directory.
    //
    if (foundFileName.substr(0,1) != std::string("/"))
       foundFileName = getFileInPath(foundFileName);

    // bail if we haven't found any reasonable filename
    if (foundFileName == "")
        return false;

    AudioFileId id = getFirstUnusedID();

    WAVAudioFile *aF = new WAVAudioFile(id, name, foundFileName);

    // if we don't recognise the file then don't insert it
    //
    if (aF->open() == false)
    {
        delete aF;
        throw(std::string(
                "AudioFileManager::insertFile - don't recognise file type"));
    }
    m_audioFiles.push_back(aF);

    return id;
}


bool
AudioFileManager::removeFile(AudioFileId id)
{
    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it)
    {
        if ((*it)->getId() == id)
        {
            delete(*it);
            m_audioFiles.erase(it);
            return true;
        }
    }

    return false;
}

AudioFileId
AudioFileManager::getFirstUnusedID()
{
    AudioFileId rI = 0;

    if (m_audioFiles.size() == 0) return rI;

    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it)
    {
        if (rI < (*it)->getId())
            rI = (*it)->getId();
    }

    rI++;

    return rI;
}

bool
AudioFileManager::insertFile(const std::string &name,
                             const std::string &fileName,
                             AudioFileId id)
{
    // first try to expany any beginning tilde
    std::string foundFileName = substituteTildeForHome(fileName);

    // if we've expanded and there's no absolute path available
    // then try to find it in audio file directory.
    //
    if (foundFileName.substr(0,1) != std::string("/"))
       foundFileName = getFileInPath(foundFileName);

    // If no joy here then we can't find this file
    if (foundFileName == "")
        return false;

    // make sure we don't have a file of this ID hanging around already
    removeFile(id);

    // and insert
    WAVAudioFile *aF = new WAVAudioFile(id, name, foundFileName);

    // Test the file
    if (aF->open() == false)
    {
        delete aF;
        return false;
    }

    m_audioFiles.push_back(aF);
    return true;
}

// Add a given path to our sample search path
//
void
AudioFileManager::setAudioPath(const std::string &path)
{
    std::string hPath = path;

    // add a trailing / if we don't have one
    //
    if (hPath[hPath.size() - 1] != '/')
        hPath += std::string("/");

    // get the home directory
    if (hPath[0] == '~')
    {
        hPath.erase(0, 1);
        hPath = std::string(getenv("HOME")) + hPath;
    }

    m_audioPath = hPath;

}


// See if we can find a given file in our search path
// return the first occurence of a match or the empty
// std::string if no match.
//
std::string
AudioFileManager::getFileInPath(const std::string &file)
{
    std::string search;
    std::string rS;
    std::ifstream *fd; // check our file can be found

    rS = "";

    // if we lead with a '/' then assume we have an
    // absolute path already
    //
    if (file[0] == '/')
        return file;

    //std::vector<std::string>::iterator it;

    search = m_audioPath + file;

    // Check we can open the file and return the full path if we can
    //
    fd = new std::ifstream(search.c_str(), std::ios::in | std::ios::binary);
    if (*fd)
        rS = search;
    fd->close();

    return rS;
}


// Does a specific file id exist on the manager?
//
bool
AudioFileManager::fileExists(AudioFileId id)
{
    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it)
    {
        if ((*it)->getId() == id)
            return true;
    }

    return false;

}

void
AudioFileManager::clear()
{
    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it)
        delete(*it);

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());

    // Clear the PeakFileManager too
    //
    m_peakManager.clear();
}

std::string
AudioFileManager::createRecordingAudioFile()
{
    AudioFileId newId = getFirstUnusedID();
    int audioFileNumber = 0;

    // search for used RG-AUDIO files in the record directory
    DIR *dir = opendir(m_audioPath.c_str());
    std::string prefix = "RG-AUDIO-";
    std::string file;

    if (dir)
    {
        dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            file = entry->d_name;

            // hmm, why aren't file types working?
            // we should be able to filter by d_type but
            // it's currently returning NULL

#if (__GNUC__ < 3)
            if (file.compare(prefix, 0, 9) == 0)
#else
            if (file.compare(0, 9, prefix) == 0)
#endif
            {
                // get the number
                file.erase(0, 9);

                // match and remove post dot
                int pos = file.find(".");
                file.erase(pos, file.length());

                // store
                if (atoi(file.c_str()) > audioFileNumber)
                    audioFileNumber = atoi(file.c_str());
            }
        }

    }
    else
    {
        std::cerr << "AudioFileManager::createRecordingAudioFile - "
                  << "can't access recording directory \'"
                  << m_audioPath << "\'" << std::endl;
    }

    // start from 1, not 0 if we've not found anything
    if (audioFileNumber == 0)
        audioFileNumber = 1;
    else
        audioFileNumber++;

    // composite with number and return
    char number[100];
    sprintf(number, "%.4d", audioFileNumber);
    file = prefix + number + ".wav";

    // insert file into vector
    WAVAudioFile *aF = new WAVAudioFile(newId, file, m_audioPath + file);
    m_audioFiles.push_back(aF);

    // what we return is the full path to the file
    return m_audioPath + file;
} 

AudioFile*
AudioFileManager::getLastAudioFile()
{
    std::vector<AudioFile*>::iterator it = m_audioFiles.begin();
    AudioFile* audioFile = 0;

    while (it != m_audioFiles.end())
    {
        audioFile = (*it);
        it++;
    }

    return audioFile;
}

std::string
AudioFileManager::substituteHomeForTilde(const std::string &path)
{
    std::string rS = path;
    std::string homePath = std::string(getenv("HOME"));

    // if path length is less than homePath then just return unchanged
    if (rS.length() < homePath.length())
        return rS;

    // if the first section matches the path then substitute
    if (rS.substr(0, homePath.length()) == homePath)
    {
        rS.erase(0, homePath.length());
        rS = "~" + rS;
    }

    return rS;
}

std::string
AudioFileManager::substituteTildeForHome(const std::string &path)
{
    std::string rS = path;
    std::string homePath = std::string(getenv("HOME"));

    if (rS.substr(0, 2) == std::string("~/"))
    {
        rS.erase(0, 1); // erase tilde and prepend HOME env
        rS = homePath + rS;
    }

    return rS;
}



// Export audio files and assorted bits and bobs - make sure
// that we store the files in a format that's user independent
// so that people can pack up and swap their songs (including
// audio files) and shift them about easily.
//
std::string
AudioFileManager::toXmlString()
{
    std::stringstream audioFiles;
    std::string audioPath = substituteHomeForTilde(m_audioPath);

    audioFiles << "<audiofiles>" << std::endl;
    audioFiles << "    <audioPath value=\""
               << audioPath << "\"/>" << std::endl;

    std::string fileName;
    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); ++it)
    {
        fileName = (*it)->getFilename();

        // attempt two substitutions - If the prefix to the filename
        // is the same as the audio path then we can dock the prefix
        // as it'll be added again next time.  If the path doesn't
        // have the audio path in it but has our home directory in it
        // then swap this out for a tilde '~'
        //
        std::cout << "DIR = " << getDirectory(fileName) << " : "
                " PATH = " << m_audioPath << std::endl;
        if (getDirectory(fileName) == m_audioPath)
            fileName = getShortFilename(fileName);
        else
            fileName = substituteHomeForTilde(fileName);

        audioFiles << "    <audio id=\""
                   << (*it)->getId()
                   << "\" file=\""
                   << fileName
                   << "\" label=\""
                   << encode((*it)->getName())
                   << "\"/>" << std::endl;
    }

    audioFiles << "</audiofiles>" << std::endl;

#if (__GNUC__ < 3)
    audioFiles << std::ends;
#else
    audioFiles << std::endl;
#endif

    return audioFiles.str();
}

// Generate preview peak files or peak chunks according
// to file type.
//
void
AudioFileManager::generatePreviews()
{
    std::cout << "AudioFileManager::generatePreviews - "
              << "for " << m_audioFiles.size() << " files"
              << std::endl;


    // Generate peaks if we need to
    //
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); ++it)
    {
        if (!m_peakManager.hasValidPeaks(*it))
            m_peakManager.generatePeaks(*it, 1);
    }
}


// Generate a preview for a specific audio file - say if
// one has just been added to the AudioFileManager.
// Also used for generating previews if the file has been
// modified.
//
bool
AudioFileManager::generatePreview(AudioFileId id)
{
    AudioFile *audioFile = getAudioFile(id);
    
    if (audioFile == 0)
        return false;

    if (!m_peakManager.hasValidPeaks(audioFile))
        m_peakManager.generatePeaks(audioFile, 1);

    return true;
}

AudioFile*
AudioFileManager::getAudioFile(AudioFileId id)
{
    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         it++)
    {
        if ((*it)->getId() == id)
            return (*it);
    }
    return 0;
}

std::vector<float>
AudioFileManager::getPreview(AudioFileId id,
                             const RealTime &startTime,
                             const RealTime &endTime,
                             int width,
                             bool withMinima)
{
    AudioFile *audioFile = getAudioFile(id);
    
    if (audioFile == 0)
        return std::vector<float>();

    if (!m_peakManager.hasValidPeaks(audioFile))
        throw std::string("<no peakfile>");

    return m_peakManager.getPreview(audioFile,
                                    startTime,
                                    endTime,
                                    width,
                                    withMinima);
}

void
AudioFileManager::drawPreview(AudioFileId id,
                              const RealTime &startTime,
                              const RealTime &endTime,
                              QPixmap *pixmap)
{
    AudioFile *audioFile = getAudioFile(id);

    if (!m_peakManager.hasValidPeaks(audioFile))
        throw std::string("<no peakfile>");

    std::vector<float> values = m_peakManager.getPreview
                                        (audioFile,
                                         startTime,
                                         endTime,
                                         pixmap->width(),
                                         false);

    QPainter painter(pixmap);
    pixmap->fill(kapp->palette().color(QPalette::Active,
                                       QColorGroup::Base));
    painter.setPen(kapp->palette().color(QPalette::Active,
                                         QColorGroup::Dark));

    if (values.size() == 0)
    {
        std::cerr << "AudioFileManager::drawPreview - "
                  << "no preview values returned!" << std::endl;
        return;
    }

    float yStep = pixmap->height() / 2;
    int channels = audioFile->getChannels();
    float ch1Value, ch2Value;

    if (channels == 0)
    {
        std::cerr << "AudioFileManager::drawPreview - "
                  << "no channels in audio file!" << std::endl;
        return;
    }


    // Render pixmap
    //
    for (int i = 0; i < pixmap->width(); i++)
    {
        // Always get two values for our pixmap no matter how many
        // channels in AudioFile as that's all we can display.
        //
        if (channels == 1)
        {
            ch1Value = values[i];
            ch2Value = values[i];
        }
        else
        {
            ch1Value = values[i * channels];
            ch2Value = values[i * channels + 1];
        }

        painter.drawLine(i, static_cast<int>(yStep + ch1Value * yStep),
                         i, static_cast<int>(yStep - ch2Value * yStep));
    }
}

void
AudioFileManager::drawHighlightedPreview(AudioFileId id,
                                         const RealTime &startTime,
                                         const RealTime &endTime,
                                         const RealTime &highlightStart,
                                         const RealTime &highlightEnd,
                                         QPixmap *pixmap)
{
    AudioFile *audioFile = getAudioFile(id);

    if (!m_peakManager.hasValidPeaks(audioFile))
        throw std::string("<no peakfile>");

    std::vector<float> values = m_peakManager.getPreview
                                        (audioFile,
                                         startTime,
                                         endTime,
                                         pixmap->width(),
                                         false);

    int startWidth = (int)(double(pixmap->width()) * (highlightStart /
                                                      (endTime - startTime)));
    int endWidth = (int)(double(pixmap->width()) * (highlightEnd /
                                                    (endTime - startTime)));

    QPainter painter(pixmap);
    pixmap->fill(kapp->palette().color(QPalette::Active,
                                       QColorGroup::Base));

    float yStep = pixmap->height() / 2;
    int channels = audioFile->getChannels();
    float ch1Value, ch2Value;

    // Render pixmap
    //
    for (int i = 0; i < pixmap->width(); ++i)
    {
        // Always get two values for our pixmap no matter how many
        // channels in AudioFile as that's all we can display.
        //
        if (channels == 1)
        {
            ch1Value = values[i];
            ch2Value = values[i];
        }
        else
        {
            ch1Value = values[i * channels];
            ch2Value = values[i * channels + 1];
        }

        if (i < startWidth || i > endWidth)
            painter.setPen(kapp->palette().color(QPalette::Active,
                                                 QColorGroup::Mid));
        else
            painter.setPen(kapp->palette().color(QPalette::Active,
                                                 QColorGroup::Dark));

        painter.drawLine(i, static_cast<int>(yStep + ch1Value * yStep),
                         i, static_cast<int>(yStep - ch2Value * yStep));
    }
}


void
AudioFileManager::print()
{
    std::cout << "AudioFileManager - " << m_audioFiles.size() << " entr";

    if (m_audioFiles.size() == 1)
        std::cout << "y";
    else
        std::cout << "ies";

    std::cout << std::endl << std::endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); ++it)
    {
        std::cout << (*it)->getId() << " : " << (*it)->getName()
                  << " : \"" << (*it)->getFilename() << "\"" << std::endl;
    }
}

std::vector<SplitPointPair>
AudioFileManager::getSplitPoints(AudioFileId id,
                                 const RealTime &startTime,
                                 const RealTime &endTime,
                                 int threshold)
{
    AudioFile *audioFile = getAudioFile(id);

    if (audioFile == 0) return std::vector<SplitPointPair>();

    return m_peakManager.getSplitPoints(audioFile,
                                        startTime,
                                        endTime,
                                        threshold);
}

}


