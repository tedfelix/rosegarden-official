// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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


#include "AudioFile.h"
#include "AudioFileManager.h"


namespace Rosegarden
{

AudioFileManager::AudioFileManager()
{
    // Set this through the set method so that the tilde gets
    // shaken out.
    //
    setAudioPath("~/rosegarden");
}

AudioFileManager::~AudioFileManager()
{
}

// Create a new AudioFile with unique ID and label
//
unsigned int
AudioFileManager::insertFile(const std::string &name,
                             const std::string &fileName)
{
    // see if we can find the file
    std::string foundFileName = getFileInPath(fileName);

    if (foundFileName == "")
        return false;

    unsigned int id = getFirstUnusedID();

    AudioFile *aF = new AudioFile(id, name, foundFileName);

    // if we don't recognise the file then don't insert it
    //
    if (aF->open() == false)
    {
        delete aF;
        throw("AudioFileManager::insertFile - don't recognise file type");
    }
    m_audioFiles.push_back(aF);

    return (unsigned int)id;
}


bool
AudioFileManager::removeFile(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         it++)
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

unsigned int
AudioFileManager::getFirstUnusedID()
{
    unsigned int rI = 0;

    if (m_audioFiles.size() == 0) return rI;

    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         it++)
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
                             unsigned int id)
{
    std::string foundFileName = getFileInPath(fileName);

    if (foundFileName == "")
        return false;

    // make sure we don't have one hanging around already
    removeFile(id);


    // and insert
    AudioFile *aF = new AudioFile(id, name, foundFileName);

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
AudioFileManager::fileExists(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         it++)
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
         it++)
        delete(*it);

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());
}

std::string
AudioFileManager::createRecordingAudioFile()
{
    unsigned int newId = getFirstUnusedID();
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
    AudioFile *aF = new AudioFile(newId, file, file);
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


// Export audio file
std::string
AudioFileManager::toXmlString()
{
    std::stringstream audioFiles;

    audioFiles << "<audiofiles>" << std::endl;
    audioFiles << "    <audiopath value=\""
               << m_audioPath << "\"/>" << std::endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        audioFiles << "    <audio id=\""
                   << (*it)->getId()
                   << "\" file=\""
                   << (*it)->getFilename()
                   << "\" label=\""
                   << encode((*it)->getName())
                   << "\"/>" << std::endl;
    }

    audioFiles << "</audiofiles>" << std::endl;
    audioFiles << std::ends;

    return audioFiles.str();
}

// Generate previews and cache them locally - we can probably
// do this to file at some point.  Resolution is currently
// just arbirtary.
//
void
AudioFileManager::generatePreviews()
{
    std::vector<AudioFile*>::iterator it;
    m_previewMap.clear();

    std::cout << "AudioFileManager::generatePreviews - "
              << "for " << m_audioFiles.size() << " files"
              << std::endl;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         it++)
    {
        std::cout << "AudioFileManager::generatePreviews - "
                  << "generating preview for \""
                  << (*it)->getFilename() << "\"" << std::endl;
        m_previewMap[(*it)->getId()] =
            (*it)->getPreview(Rosegarden::RealTime(0, 2000));
    }
}


// Generate a preview for a specific audio file - say if
// one has just been added to the AudioFileManager.
// Also used for generating previews if the file has been
// modified.
//
bool
AudioFileManager::generatePreview(unsigned int id)
{
    AudioFile *audioFile = getAudioFile(id);
    
    if (audioFile == 0)
        return false;

    // clear any previous entry
    m_previewMap[id].clear();

    // insert new map
    m_previewMap[id] = audioFile->getPreview(Rosegarden::RealTime(0, 2000));

    return true;
}

AudioFile*
AudioFileManager::getAudioFile(unsigned int id)
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




}


