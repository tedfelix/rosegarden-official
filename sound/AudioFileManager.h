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

#include <string>
#include <vector>

#include "AudioFile.h"
#include "XmlExportable.h"

#ifndef _AUDIOFILEMANAGER_H_
#define _AUDIOFILEMANAGER_H_

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

class AudioFileManager : public XmlExportable
{
public:
    AudioFileManager();
    ~AudioFileManager();

    // insert an audio file into the AudioFileManager and get the
    // first allocated id for it.
    //
    unsigned int insertFile(const std::string &name,
                            const std::string &fileName);

    // And insert an AudioFile and specify an id
    //
    bool insertFile(const std::string &name, const std::string &fileName,
                    unsigned int id);

    // Remove a file from the AudioManager by id
    //
    bool removeFile(unsigned int id);

    // does a specific file id exist?
    //
    bool fileExists(unsigned int id);

    // Get the list of files
    //
    std::vector<AudioFile*>::const_iterator begin()
        { return m_audioFiles.begin(); }

    std::vector<AudioFile*>::const_iterator end()
        { return m_audioFiles.end(); }

    // Clear down all audio file references
    //
    void clear();

    // Get and set the record path
    //
    std::string getAudioPath() { return m_audioPath; }
    void setAudioPath(const std::string &path);

    // Get a new audio filename at the audio record path
    //
    std::string createRecordingAudioFile();

    // return the last file in the vector - the last created
    //
    AudioFile* getLastAudioFile();

    // Export to XML
    //
    virtual std::string toXmlString();

private:
    std::string getFileInPath(const std::string &file);

    unsigned int getFirstUnusedID();

    std::vector<AudioFile*> m_audioFiles;
    std::string m_audioPath;

};

}

#endif // _AUDIOFILEMANAGER_H_
