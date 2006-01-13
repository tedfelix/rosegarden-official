// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
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
#include <pthread.h>

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
#include <qdatetime.h>
#include <qfile.h>

#include "AudioFile.h"
#include "AudioFileManager.h"
#include "WAVAudioFile.h"
#include "BWFAudioFile.h"
#include "MP3AudioFile.h"
#include "rosestrings.h"

namespace Rosegarden
{

static pthread_mutex_t _audioFileManagerLock;

class MutexLock {
public:
    MutexLock(pthread_mutex_t *mutex) : m_mutex(mutex) {
	pthread_mutex_lock(m_mutex);
    }
    ~MutexLock() {
	pthread_mutex_unlock(m_mutex);
    }
private:
    pthread_mutex_t *m_mutex;
};

AudioFileManager::AudioFileManager()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#ifdef HAVE_PTHREAD_MUTEX_RECURSIVE
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
#ifdef PTHREAD_MUTEX_RECURSIVE
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
#endif
    pthread_mutex_init(&_audioFileManagerLock, &attr);

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
    MutexLock lock(&_audioFileManagerLock);
    
    QString ext =
        QString(filePath.substr(filePath.length() - 3, 3).c_str()).lower();

    // Check for file existing already in manager by path
    //
    int check = fileExists(filePath);
    if (check != -1) {
	return AudioFileId(check);
    }

    // prepare for audio file
    AudioFile *aF = 0;
    AudioFileId id = getFirstUnusedID();

    if (ext == "wav")
    {
        // identify file type
        AudioFileType subType = RIFFAudioFile::identifySubType(filePath);

        if (subType == BWF)
        {
#ifdef DEBUG_AUDIOFILEMANAGER
            std::cout << "FOUND BWF" << std::endl;
#endif
            aF = new BWFAudioFile(id, getShortFilename(filePath), filePath);
        }
        else if (subType == WAV)
        {
            aF = new WAVAudioFile(id, getShortFilename(filePath), filePath);
        }

        // Ensure we have a valid file handle
        //
        if (aF == 0) {
	    throw i18n("Unsupported audio file type."); 
	}

        // Add file type on extension
        try
        { 
            if (aF->open() == false)
            {
                delete aF;
                throw(i18n("Failed to open audio file."));
            }
        }
        catch(std::string e)
        {
            // catch and rethrow
            //
            delete aF;
            throw(e);
        }
    }
#ifdef HAVE_LIBMAD
    else if (ext == "mp3")
    {
        aF = new MP3AudioFile(id, getShortFilename(filePath), filePath);

        try
        { 
            if (aF->open() == false)
            {
                delete aF;
                throw(i18n("Problem opening MP3 file"));
            }
        }
        catch(std::string e)
        {
            // catch and rethrow
            //
            delete aF;
            throw(e);
        }
    }
#endif // HAVE_LIBMAD
    else
    {
        throw(i18n("Unsupported audio file format \"%1\".").arg(ext));
    }

    if (aF)
    {
        m_audioFiles.push_back(aF);
        return id;
    }

    return 0;
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
    MutexLock lock(&_audioFileManagerLock);

    // first try to expand any beginning tilde
    //
    std::string foundFileName = substituteTildeForHome(fileName);

    // If we've expanded and we can't find the file
    // then try to find it in audio file directory.
    //
    QFileInfo info(foundFileName.c_str());
    if (!info.exists()) foundFileName = getFileInPath(foundFileName);

#ifdef DEBUG_AUDIOFILEMANAGER_INSERT_FILE
    std::cout << "AudioFileManager::insertFile - "
              << "expanded fileName = \"" 
              << foundFileName << "\"" << std::endl;
#endif

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
    MutexLock lock(&_audioFileManagerLock);

    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it)
    {
        if ((*it)->getId() == id)
        {
	    m_peakManager.removeAudioFile(*it);
	    m_recordedAudioFiles.erase(*it);
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
    MutexLock lock(&_audioFileManagerLock);

    // first try to expany any beginning tilde
    std::string foundFileName = substituteTildeForHome(fileName);

    // If we've expanded and we can't find the file
    // then try to find it in audio file directory.
    //
    QFileInfo info(foundFileName.c_str());
    if (!info.exists()) foundFileName = getFileInPath(foundFileName);

#ifdef DEBUG_AUDIOFILEMANAGER_INSERT_FILE
    std::cout << "AudioFileManager::insertFile - "
              << "expanded fileName = \"" 
              << foundFileName << "\"" << std::endl;
#endif

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
    MutexLock lock(&_audioFileManagerLock);

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

void
AudioFileManager::testAudioPath() throw (BadAudioPathException)
{
    QFileInfo info(m_audioPath);
    if (!(info.exists() && info.isDir() && !info.isRelative() &&
	  info.isWritable() && info.isReadable()))
	throw BadAudioPathException(m_audioPath.data());
}


// See if we can find a given file in our search path
// return the first occurence of a match or the empty
// std::string if no match.
//
std::string
AudioFileManager::getFileInPath(const std::string &file)
{
    MutexLock lock(&_audioFileManagerLock);

    QFileInfo info(file.c_str());

    if (info.exists())
        return file;

    // Build the search filename from the audio path and
    // the file basename.
    //
    QString searchFile = QString(m_audioPath.c_str()) + info.fileName();
    QFileInfo searchInfo(searchFile);

    if (searchInfo.exists())
        return searchFile.latin1();

    std::cout << "AudioFileManager::getFileInPath - "
              << "searchInfo = " << searchFile << std::endl;

    return "";
}


// Check for file path existence
//
int
AudioFileManager::fileExists(const std::string &path)
{
    MutexLock lock(&_audioFileManagerLock);

    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it)
    {
        if ((*it)->getFilename() == path)
            return (*it)->getId();
    }

    return -1;

}

// Does a specific file id exist on the manager?
//
bool
AudioFileManager::fileExists(AudioFileId id)
{
    MutexLock lock(&_audioFileManagerLock);

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
    MutexLock lock(&_audioFileManagerLock);

    std::vector<AudioFile*>::iterator it;

    for (it = m_audioFiles.begin();
         it != m_audioFiles.end();
         ++it) {
	m_recordedAudioFiles.erase(*it);
        delete(*it);
    }

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());

    // Clear the PeakFileManager too
    //
    m_peakManager.clear();
}

AudioFile *
AudioFileManager::createRecordingAudioFile()
{
    MutexLock lock(&_audioFileManagerLock);

    AudioFileId newId = getFirstUnusedID();
    QString fileName = "";

    while (fileName == "") {

	fileName = QString("rg-%1-%2.wav")
	    .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"))
	    .arg(newId + 1);

	if (QFile(m_audioPath.c_str() + fileName).exists()) {
	    fileName = "";
	    ++newId;
	}
    }

    // insert file into vector
    WAVAudioFile *aF = new WAVAudioFile(newId, fileName.data(), m_audioPath + fileName.data());
    m_audioFiles.push_back(aF);
    m_recordedAudioFiles.insert(aF);

    return aF;
} 

std::vector<std::string>
AudioFileManager::createRecordingAudioFiles(unsigned int n)
{
    std::vector<std::string> v;
    for (unsigned int i = 0; i < n; ++i) {
	AudioFile *af = createRecordingAudioFile();
	if (af) v.push_back(m_audioPath + af->getFilename().data());
	// !af should not happen, and we have no good recovery if it does
    }
    return v;
}

bool
AudioFileManager::wasAudioFileRecentlyRecorded(AudioFileId id)
{
    AudioFile *file = getAudioFile(id);
    if (file) return (m_recordedAudioFiles.find(file) !=
		      m_recordedAudioFiles.end());
    return false;
}

void
AudioFileManager::resetRecentlyRecordedFiles()
{
    m_recordedAudioFiles.clear();
}

AudioFile*
AudioFileManager::getLastAudioFile()
{
    MutexLock lock(&_audioFileManagerLock);

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
    MutexLock lock(&_audioFileManagerLock);

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
#ifdef DEBUG_AUDIOFILEMANAGER
        std::cout << "DIR = " << getDirectory(fileName) << " : "
                " PATH = " << m_audioPath << std::endl;
#endif

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
    MutexLock lock(&_audioFileManagerLock);

#ifdef DEBUG_AUDIOFILEMANAGER
    std::cout << "AudioFileManager::generatePreviews - "
              << "for " << m_audioFiles.size() << " files"
              << std::endl;
#endif


    // Generate peaks if we need to
    //
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); ++it)
    {
        if (!m_peakManager.hasValidPeaks(*it))
            m_peakManager.generatePeaks(*it, 1);
    }
}

// Attempt to stop a preview
//
void
AudioFileManager::stopPreview()
{
    MutexLock lock(&_audioFileManagerLock);

    m_peakManager.stopPreview();
}



// Generate a preview for a specific audio file - say if
// one has just been added to the AudioFileManager.
// Also used for generating previews if the file has been
// modified.
//
bool
AudioFileManager::generatePreview(AudioFileId id)
{
    MutexLock lock(&_audioFileManagerLock);

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
    MutexLock lock(&_audioFileManagerLock);

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
    MutexLock lock(&_audioFileManagerLock);

    AudioFile *audioFile = getAudioFile(id);
    
    if (audioFile == 0) {
        return std::vector<float>();
    }

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
    MutexLock lock(&_audioFileManagerLock);

    AudioFile *audioFile = getAudioFile(id);

    if (!m_peakManager.hasValidPeaks(audioFile)) {
        throw std::string("<no peakfile>");
    }

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
#ifdef DEBUG_AUDIOFILEMANAGER
        std::cerr << "AudioFileManager::drawPreview - "
                  << "no preview values returned!" << std::endl;
#endif
        return;
    }

    float yStep = pixmap->height() / 2;
    int channels = audioFile->getChannels();
    float ch1Value, ch2Value;

    if (channels == 0)
    {
#ifdef DEBUG_AUDIOFILEMANAGER
        std::cerr << "AudioFileManager::drawPreview - "
                  << "no channels in audio file!" << std::endl;
#endif
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

        painter.drawLine(i, static_cast<int>(yStep - ch1Value * yStep),
                         i, static_cast<int>(yStep + ch2Value * yStep));
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
    MutexLock lock(&_audioFileManagerLock);

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
	if ((i * channels + (channels - 1)) >= int(values.size())) break;

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
    MutexLock lock(&_audioFileManagerLock);

#ifdef DEBUG_AUDIOFILEMANAGER
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
#endif
}

std::vector<SplitPointPair>
AudioFileManager::getSplitPoints(AudioFileId id,
                                 const RealTime &startTime,
                                 const RealTime &endTime,
                                 int threshold,
                                 const RealTime &minTime)
{
    MutexLock lock(&_audioFileManagerLock);

    AudioFile *audioFile = getAudioFile(id);

    if (audioFile == 0) return std::vector<SplitPointPair>();

    return m_peakManager.getSplitPoints(audioFile,
                                        startTime,
                                        endTime,
                                        threshold,
                                        minTime);
}

}


#include "AudioFileManager.moc"
