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


#ifndef _AUDIOFILEMANAGER_H_
#define _AUDIOFILEMANAGER_H_

#include "AudioFile.h"
#include <string>
#include <vector>

// AudioFileManager loads and maps audio files to their
// internal references (ids).  A point of contact for
// AudioFile information - loading a Composition should
// use this class to pick up the AudioFile references,
// editing the AudioFiles in a Composition will be
// made through this manager.
//
//
//

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

typedef std::vector<AudioFile*>::const_iterator AudioFileManagerIterator;

class AudioFileManager
{
public:
    AudioFileManager();
    ~AudioFileManager();

    // insert an audio file into the AudioFileManager and get the
    // first allocated id for it.
    // 
    //
    int insertFile(const std::string &name, const std::string &fileName);

    // And insert an AudioFile and specify an id
    //
    bool insertFile(const std::string &name, const std::string &fileName,
                    const unsigned int &id);

    // Remove a file from the AudioManager by id
    //
    bool removeFile(const unsigned int &id);

    // Add a directory to the search path
    //
    void addSearchPath(const std::string &path);

    // does a specific file id exist?
    //
    bool fileExists(const unsigned int &id);

    // Get the list of files
    //
    std::vector<AudioFile*>::const_iterator begin()
        { return m_audioFiles.begin(); }

    std::vector<AudioFile*>::const_iterator end()
        { return m_audioFiles.end(); }

private:
    std::string getFileInPath(const std::string &file);

    unsigned int getFirstUnusedID();

    std::vector<AudioFile*> m_audioFiles;
    std::vector<std::string> m_audioSearchPath;

};

}

#endif // _AUDIOFILEMANAGER_H_
