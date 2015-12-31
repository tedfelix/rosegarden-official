/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOFILEMANAGER_H
#define RG_AUDIOFILEMANAGER_H

#include <string>
#include <vector>
#include <set>
#include <map>

#include <QStringList>
#include <QPixmap>
#include <QObject>
#include <QUrl>

#include "AudioFile.h"
#include "PeakFileManager.h"
#include "PeakFile.h"

#include "base/XmlExportable.h"
#include "base/Exception.h"
#include "misc/Strings.h"

// AudioFileManager loads and maps audio files to their
// internal references (ids).  A point of contact for
// AudioFile information - loading a Composition should
// use this class to pick up the AudioFile references,
// editing the AudioFiles in a Composition will be
// made through this manager.

// This is in the sound library because it's so closely
// connected to other sound classes like the AudioFile
// ones.  However, the audio file manager itself within
// Rosegarden is stored in the GUI process.  This class
// is not (and should not be) used elsewhere within the
// sound or sequencer libraries.

class QProcess;

namespace Rosegarden
{

typedef std::vector<AudioFile*>::const_iterator AudioFileManagerIterator;

class AudioFileManager : public QObject, public XmlExportable
{
    Q_OBJECT
public:
    AudioFileManager();
    virtual ~AudioFileManager();
    
    class BadAudioPathException : public Exception
    {
    public:
        BadAudioPathException(QString path) :
            Exception(QObject::tr("Bad audio file path ") + path), m_path(path) { }
        BadAudioPathException(QString path, QString file, int line) :
            Exception(QObject::tr("Bad audio file path ") + path, file, line), m_path(path) { }
        BadAudioPathException(const SoundFile::BadSoundFileException &e) :
            Exception(QObject::tr("Bad audio file path (malformed file?) ") + e.getPath()), m_path(e.getPath()) { }

        ~BadAudioPathException() throw() { }

        QString getPath() const { return m_path; }

    private:
        QString m_path;
    };

private:
    AudioFileManager(const AudioFileManager &aFM);
    AudioFileManager& operator=(const AudioFileManager &);

public:

    // Create an audio file from an absolute path - we use this
    // interface to add an actual file.  This only works with files
    // that are already in a format RG understands natively.  If you
    // are not sure about that, use importFile or importURL instead.
    //
    AudioFileId addFile(const QString &filePath);
    // throw BadAudioPathException

    // Create an audio file by importing (i.e. converting and/or
    // resampling) an existing file using the conversion library.  If
    // you are not sure whether to use addFile or importFile, go for
    // importFile.
    //
    AudioFileId importFile(const QString &filePath,
			   int targetSampleRate = 0);
    // throw BadAudioPathException, BadSoundFileException

    // Create an audio file by importing from a URL
    //
    AudioFileId importURL(const QUrl &filePath,
			  int targetSampleRate = 0);
    // throw BadAudioPathException, BadSoundFileException

    // Insert an audio file into the AudioFileManager and get the
    // first allocated id for it.  Used from the RG file as we already
    // have both name and filename/path.
    //
    AudioFileId insertFile(const std::string &name,
                           const QString &fileName);
    // throw BadAudioPathException

    // Convert an audio file from arbitrary external format to an
    // internal format suitable for use by addFile, using packages in
    // Rosegarden.  This replaces the Perl script previously used. It
    // returns 0 for OK.  This is used by importFile and importURL
    // which normally provide the more suitable interface for import.
    // 
    int convertAudioFile(const QString inFile, const QString outFile);

    bool insertFile(const std::string &name, const QString &fileName,
                    AudioFileId id);
    // throw BadAudioPathException

    // Remove a file from the AudioManager by id
    //
    bool removeFile(AudioFileId id);

    // Does a specific file id exist?
    //
    bool fileExists(AudioFileId id);

    // Does a specific file path exist?  Return ID or -1.
    //
    int fileExists(const QString &path);

    // get audio file by id
    //
    AudioFile* getAudioFile(AudioFileId id);

    // Get the list of files
    //
    std::vector<AudioFile*>::const_iterator begin() const
        { return m_audioFiles.begin(); }

    std::vector<AudioFile*>::const_iterator end() const
        { return m_audioFiles.end(); }

    // Clear down all audio file references
    //
    void clear();

    // Get and set the record path
    //
    QString getAudioPath() const { return m_audioPath; }
    void setAudioPath(const QString &path);

    // Throw if the current audio path does not exist or is not writable
    //
    void testAudioPath() throw(BadAudioPathException);

    // Get a new audio filename at the audio record path, inserting the
    // projectFilename and instrumentAlias into the filename for easier
    // recognition away from the file's original context
    //
    AudioFile *createRecordingAudioFile(QString projectName, QString instrumentAlias);
    // throw BadAudioPathException

    // Return whether a file was created by recording within this "session"
    //
    bool wasAudioFileRecentlyRecorded(AudioFileId id);

    // Return whether a file was created by derivation within this "session"
    //
    bool wasAudioFileRecentlyDerived(AudioFileId id);

    // Indicate that a new "session" has started from the point of
    // view of recorded and derived audio files (e.g. that the
    // document has been saved)
    //
    void resetRecentlyCreatedFiles();
    
    // Create an empty file "derived from" the source (used by e.g. stretcher)
    // 
    AudioFile *createDerivedAudioFile(AudioFileId source,
				      const char *prefix);

    // return the last file in the vector - the last created
    //
    AudioFile* getLastAudioFile();

    // Export to XML
    //
    virtual std::string toXmlString() const;

    // Convenience function generate all previews on the audio file.
    //
    void generatePreviews();
    // throw BadSoundFileException, BadPeakFileException

    // Generate for a single audio file
    //
    bool generatePreview(AudioFileId id);
    // throw BadSoundFileException, BadPeakFileException

    // Get a preview for an AudioFile adjusted to Segment start and
    // end parameters (assuming they fall within boundaries).
    // 
    // We can get back a set of values (floats) or a Pixmap if we 
    // supply the details.
    //
    std::vector<float> getPreview(AudioFileId id,
                                  const RealTime &startTime, 
                                  const RealTime &endTime,
                                  int width,
                                  bool withMinima);
    // throw BadPeakFileException, BadAudioPathException

    // Draw a fixed size (fixed by QPixmap) preview of an audio file
    //
    void drawPreview(AudioFileId id,
                     const RealTime &startTime, 
                     const RealTime &endTime,
                     QPixmap *pixmap);
    // throw BadPeakFileException, BadAudioPathException

    // Usually used to show how an audio Segment makes up part of
    // an audio file.
    //
    void drawHighlightedPreview(AudioFileId it,
                                const RealTime &startTime,
                                const RealTime &endTime,
                                const RealTime &highlightStart,
                                const RealTime &highlightEnd,
                                QPixmap *pixmap);
    // throw BadPeakFileException, BadAudioPathException

    // Get a short file name from a long one (with '/'s)
    //
    QString getShortFilename(const QString &fileName) const;

    // Get a directory from a full file path
    //
    QString getDirectory(const QString &path) const;

    // Attempt to subsititute a tilde '~' for a home directory
    // to make paths a little more generic when saving.  Also
    // provide the inverse function as convenience here.
    //
    QString substituteHomeForTilde(const QString &path) const;
    QString substituteTildeForHome(const QString &path) const;

    // Show entries for debug purposes
    //
    void print(); 

    // Get a split point vector from a peak file
    //
    std::vector<SplitPointPair> 
        getSplitPoints(AudioFileId id,
                       const RealTime &startTime,
                       const RealTime &endTime,
                       int threshold,
                       const RealTime &minTime = RealTime(0, 100000000));
    // throw BadPeakFileException, BadAudioPathException

    // Get the peak file manager
    //
    const PeakFileManager& getPeakFileManager() const { return m_peakManager; }

    // Get the peak file manager
    //
    PeakFileManager& getPeakFileManager() { return m_peakManager; }

    int getExpectedSampleRate() const { return m_expectedSampleRate; }
    void setExpectedSampleRate(int rate) { m_expectedSampleRate = rate; }

    std::set<int> getActualSampleRates() const;

signals:
    void setValue(int);
    void setOperationName(QString);

public slots:
    // Cancel a running preview
    //
    void slotStopPreview();

    void slotStopImport();

private:
    QString getFileInPath(const QString &file);

    /// Reset ID counter based on actual Audio Files in Composition
    void updateAudioFileID(AudioFileId id);

    /// Fetch a unique ID for Audio Files
    AudioFileId getUniqueAudioFileID();

    std::vector<AudioFile*> m_audioFiles;
    QString m_audioPath;

    PeakFileManager m_peakManager;

    // All audio files are stored in m_audioFiles.  These additional
    // sets of pointers just refer to those that have been created by
    // recording or derivations within the current session, and thus
    // that the user may wish to remove at the end of the session if
    // the document is not saved.
    std::set<AudioFile *> m_recordedAudioFiles;
    std::set<AudioFile *> m_derivedAudioFiles;

    int m_expectedSampleRate;
    
    //Audio ID tracking
    unsigned int m_lastAudioFileID;

};

}

#endif // RG_AUDIOFILEMANAGER_H
