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
#include <map>

#include "AudioFile.h"
#include "XmlExportable.h"
#include "PeakFileManager.h"


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

    // Create an audio file from an absolute path - we use this interface
    // to add an actual file.
    //
    unsigned int addFile(const std::string &filePath);

    // Insert an audio file into the AudioFileManager and get the
    // first allocated id for it.  Used from the RG file as we already
    // have both name and filename/path.
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

    // get audio file by id
    //
    AudioFile* getAudioFile(unsigned int id);

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

    // Last add audio file path - used to remember where we last
    // looked for audio files.
    //
    std::string getLastAddPath() { return m_lastAddPath; }
    void setLastAddPath(const std::string &path);

    // Get a new audio filename at the audio record path
    //
    std::string createRecordingAudioFile();

    // return the last file in the vector - the last created
    //
    AudioFile* getLastAudioFile();

    // Export to XML
    //
    virtual std::string toXmlString();

    // Convenience function generate all previews on the audio file
    //
    void generatePreviews();

    // Generate for a single audio file
    //
    bool generatePreview(unsigned int id);

    // Get a preview for an AudioFile adjusted to Segment start and
    // end parameters (assuming they fall within boundaries).
    // 
    // We can get back a set of values (floats) or a Pixmap if we 
    // supply the details.
    //
    std::vector<float> getPreview(unsigned int id,       // audio file id
                                  const RealTime &startIndex, 
                                  const RealTime &endIndex,
                                  int resolution);       // samples per point

    QPixmap getPreview(unsigned int id,       // audio file id
                       const RealTime &startIndex, 
                       const RealTime &endIndex,
                       int resolution,        // samples per pixel
		       int height);

    // Get a short file name from a long one (with '/'s)
    //
    std::string getShortFilename(const std::string &fileName);

    // Get a directory from a full file path
    //
    std::string getDirectory(const std::string &path);

    // Attempt to subsititute a tilde '~' for a home directory
    // to make paths a little more generic when saving.  Also
    // provide the inverse function as convenience here.
    //
    std::string substituteHomeForTilde(const std::string &path);
    std::string substituteTildeForHome(const std::string &path);

private:
    std::string getFileInPath(const std::string &file);

    unsigned int getFirstUnusedID();

    std::vector<AudioFile*>                       m_audioFiles;
    std::string                                   m_audioPath;
    std::string                                   m_lastAddPath;

    PeakFileManager                               m_peakManager;

};

}

#endif // _AUDIOFILEMANAGER_H_
