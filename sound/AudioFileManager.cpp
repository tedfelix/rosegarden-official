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
unsigned int
AudioFileManager::addFile(const std::string &name,
                          const std::string &fileName)
{
    unsigned int id = getFirstUnusedID();

    AudioFile *aF = new AudioFile(id, name, fileName);
    m_audioFiles.push_back(aF);

    return id;
}


void
AudioFileManager::removeFile(const unsigned int &id)
{
    vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         it++)
    {
        if ((*it)->getID() == id)
            m_audioFiles.erase(it);
    }
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

void
AudioFileManager::addFile(const std::string &name,
                          const std::string &fileName,
                          const unsigned int &id)
{
    // make sure we don't have one hanging around already
    removeFile(id);

    // and insert
    AudioFile *aF = new AudioFile(id, name, fileName);
    m_audioFiles.push_back(aF);
}



}



