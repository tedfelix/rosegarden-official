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

#include "AudioFile.h"
#include "AudioFileManager.h"


namespace Rosegarden
{

AudioFileManager::AudioFileManager()
{
}

AudioFileManager::~AudioFileManager()
{
}

// Create a new AudioFile with unique ID and label
//
int
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
        return -1;
    }

    m_audioFiles.push_back(aF);

    return (int)id;
}


bool
AudioFileManager::removeFile(const unsigned int &id)
{
    vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         it++)
    {
        if ((*it)->getID() == id)
        {
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

    vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         it++)
    {
        if (rI < (*it)->getID())
            rI = (*it)->getID();
    }

    rI++;

    return rI;
}

bool
AudioFileManager::insertFile(const std::string &name,
                             const std::string &fileName,
                             const unsigned int &id)
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
AudioFileManager::addSearchPath(const std::string &path)
{
    std::string hPath = path;

    // add a trailing / if we don't have one
    //
    if (hPath[hPath.size() - 1] != '/')
        hPath += std::string("/");

    m_audioSearchPath.push_back(hPath);
}


// See if we can find a given file in our search path
// return the first occurence of a match or the empty
// string if no match.
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

    std::vector<std::string>::iterator it;

    for (it = m_audioSearchPath.begin();
         it != m_audioSearchPath.end();
         it++)
    {
        search = (*it) + file;

        // Check we can open the file and return if we can
        //
        fd = new ifstream(search.c_str(), ios::in | ios::binary);
        if (*fd)
        {
            rS = search;
            fd->close();
            break;
        }
        fd->close();
    }

    return rS;
}




}



