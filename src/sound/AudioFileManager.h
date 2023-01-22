/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

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

#include <QObject>
#include <QUrl>
#include <QPointer>
#include <QProgressDialog>

class QPixmap;

#include "AudioFile.h"
#include "PeakFileManager.h"

#include "base/XmlExportable.h"
#include "base/Exception.h"


namespace Rosegarden
{


class RosegardenDocument;

typedef std::vector<AudioFile *> AudioFileVector;


/// Repository of AudioFile objects.
/**
 * Not to be confused with AudioManagerDialog which is the "Audio File
 * Manager" dialog.
 *
 * RosegardenDocument::m_audioFileManager is the only instance.
 *
 * AudioFileManager loads and maps audio files to their
 * internal references (ids).  A point of contact for
 * AudioFile information - loading a Composition should
 * use this class to pick up the AudioFile references,
 * editing the AudioFiles in a Composition will be
 * made through this manager.
 *
 * This is in the sound library because it's so closely
 * connected to other sound classes like the AudioFile
 * ones.  However, the audio file manager itself within
 * Rosegarden is stored in the GUI process.  This class
 * is not (and should not be) used elsewhere within the
 * sound or sequencer libraries.  (Does this mean we can
 * remove the lock?  testAudioPath() is called by SequenceManager,
 * but I'm not sure which thread that runs in.  AudioPeaksThread
 * calls getPreview().)
 *
 * On the lock front, notice that cbegin() and cend() are in no way
 * thread-safe.  What is the locking for?  AudioPeaksThread?  If
 * so, it should be explicitly geared to that and nothing else.
 */
class AudioFileManager : public QObject, public XmlExportable
{
    Q_OBJECT
public:
    // AudioFileManager needs to know who its document is so that it
    // can convert relative paths to absolute.
    // RosegardenDocument::currentDocument doesn't always point to the
    // correct document.  E.g. at load time.
    explicit AudioFileManager(RosegardenDocument *doc);
    ~AudioFileManager() override;

    /// Create an AudioFile object from an absolute path
    /**
     * We use this interface to add an actual file.  This only works
     * with files that are already in a format RG understands natively.
     * If you are not sure about that, use importFile() or importURL()
     * instead.
     *
     * throws BadAudioPathException
     */
    AudioFileId addFile(const QString &filePath);

    /// Create an audio file by importing from a URL
    /**
     * throws BadAudioPathException, BadSoundFileException
     */
    AudioFileId importURL(const QUrl &url,
                          int targetSampleRate);

    /// Used by RoseXmlHandler to add an audio file.
    /**
     * throws BadAudioPathException
     */
    bool insertFile(const std::string &name, const QString &fileName,
                    AudioFileId id);
    /// We have audio files at load time, assume user has set location.
    void setAudioLocationConfirmed()  { m_audioLocationConfirmed = true; }

    /// Does a specific file id exist?
    bool fileExists(AudioFileId id);

    /// Does a specific file path exist?  Return ID or -1.
    int fileExists(const QString &absoluteFilePath);

    AudioFile* getAudioFile(AudioFileId id);

    /// Get an iterator into the list of AudioFile objects.
    /**
     * NOT THREAD SAFE.
     */
    AudioFileVector::const_iterator cbegin() const
        { return m_audioFiles.cbegin(); }

    /// NOT THREAD SAFE.
    AudioFileVector::const_iterator cend() const
        { return m_audioFiles.cend(); }

    /// NOT THREAD SAFE.
    AudioFileVector::iterator begin()
        { return m_audioFiles.begin(); }

    /// NOT THREAD SAFE.
    AudioFileVector::iterator end()
        { return m_audioFiles.end(); }

    /// NOT THREAD SAFE.
    bool empty() const  { return m_audioFiles.empty(); }

    /// Remove one audio file.
    bool removeFile(AudioFileId id);
    /// Remove all audio files.
    void clear();

    /// Set the relative audio file path.  E.g. "./audio"
    void setRelativeAudioPath(const QString &newPath, bool doMoveFiles = false);
    QString getRelativeAudioPath() const  { return m_relativeAudioPath; }
    /// Get the absolute audio path.  E.g. "/home/ted/Documents/project1/audio/"
    QString getAbsoluteAudioPath() const;

    /// Throw if the current audio path does not exist or is not writable
    /**
     * throws BadAudioPathException
     */
    void testAudioPath();

    /**
     * Get a new audio filename at the audio record path, inserting the
     * projectFilename and instrumentAlias into the filename for easier
     * recognition away from the file's original context.
     *
     * throws BadAudioPathException
     */
    AudioFile *createRecordingAudioFile(
            QString projectName, QString instrumentAlias);

    /// Return whether a file was created by recording since the last save.
    bool wasAudioFileRecentlyRecorded(AudioFileId id);

    /// Return whether a file was created by derivation since the last save.
    bool wasAudioFileRecentlyDerived(AudioFileId id);

    /**
     * Indicate that a new "session" has started from the point of
     * view of recorded and derived audio files (e.g. that the
     * document has been saved)
     */
    void resetRecentlyCreatedFiles();

    /// Create an empty file "derived from" the source (used by e.g. stretcher)
    AudioFile *createDerivedAudioFile(AudioFileId source,
                                      const char *prefix);

    /// Export audio files and assorted bits and bobs
    /**
     * The files are stored in a format that's user independent so
     * that people can pack up and swap their songs (including audio
     * files) and shift them about easily.
     */
    std::string toXmlString() const override;

    /// Generate previews for all audio files.
    /**
     * Generates preview peak files or peak chunks according to file type.
     *
     * throw BadSoundFileException, BadPeakFileException
     */
    void generatePreviews();

    /// Generate preview for a single audio file.
    /**
     * Generate a preview for a specific audio file - say if
     * one has just been added to the AudioFileManager.
     * Also used for generating previews if the file has been
     * modified.
     *
     * throws BadSoundFileException, BadPeakFileException
     */
    bool generatePreview(AudioFileId id);

    /**
     * Get a preview for an AudioFile adjusted to Segment start and
     * end parameters (assuming they fall within boundaries).
     *
     * We can get back a set of values (floats) or a Pixmap if we
     * supply the details.
     *
     * throws BadPeakFileException, BadAudioPathException
     */
    std::vector<float> getPreview(AudioFileId id,
                                  const RealTime &startTime,
                                  const RealTime &endTime,
                                  int width,
                                  bool withMinima);

    /// Draw a fixed size (fixed by QPixmap) preview of an audio file
    /**
     * throws BadPeakFileException, BadAudioPathException
     */
    void drawPreview(AudioFileId id,
                     const RealTime &startTime,
                     const RealTime &endTime,
                     QPixmap *pixmap);

    /**
     * Usually used to show how an audio Segment makes up part of
     * an audio file.
     *
     * throws BadPeakFileException, BadAudioPathException
     */
    void drawHighlightedPreview(AudioFileId id,
                                const RealTime &startTime,
                                const RealTime &endTime,
                                const RealTime &highlightStart,
                                const RealTime &highlightEnd,
                                QPixmap *pixmap);

    /// Get a split point vector from a peak file
    /**
     * throws BadPeakFileException, BadAudioPathException
     */
    std::vector<SplitPointPair>
        getSplitPoints(AudioFileId id,
                       const RealTime &startTime,
                       const RealTime &endTime,
                       int threshold,
                       const RealTime &minTime = RealTime(0, 100000000));

    int getExpectedSampleRate() const  { return m_expectedSampleRate; }
    void setExpectedSampleRate(int rate)  { m_expectedSampleRate = rate; }

    std::set<int> getActualSampleRates() const;

    /// Provide a progress dialog to be used to show progress.
    void setProgressDialog(QPointer<QProgressDialog> progressDialog)
            { m_progressDialog = progressDialog; }

    /// Ask the user to pick a save location for audio files.
    /**
     * Checks for the first save with audio files and shows a dialog
     * to help the user select a location.
     *
     * Call this after saving the .rg file.
     */
    void save();

    /// Show entries for debug purposes
    void print();

    class BadAudioPathException : public Exception
    {
    public:
        explicit BadAudioPathException(QString path) :
            Exception(QObject::tr("Bad audio file path ") + path), m_path(path) { }
        BadAudioPathException(QString path, QString file, int line) :
            Exception(QObject::tr("Bad audio file path ") + path, file, line), m_path(path) { }
        explicit BadAudioPathException(const SoundFile::BadSoundFileException &e) :
            Exception(QObject::tr("Bad audio file path (malformed file?) ") + e.getPath()), m_path(e.getPath()) { }

        ~BadAudioPathException() throw() override { }

        QString getPath() const { return m_path; }

    private:
        QString m_path;
    };

private:
    // Hide copy ctor and op=.
    AudioFileManager(const AudioFileManager &aFM);
    AudioFileManager &operator=(const AudioFileManager &);

    // We can't use RosegardenDocument::currentDocument as it might
    // be pointing to the wrong document.
    RosegardenDocument *m_document;

    /// The audio files we are managing.
    /**
     * These objects are owned by this class.  The dtor deletes them.
     *
     * The IDs are stored in the AudioFile objects.  See AudioFile::getId().
     *
     * ??? Would this be better as an AudioFileMap:
     *       std::map<AudioFileId, QSharedPointer<AudioFile>>
     *     See getAudioFile().
     */
    AudioFileVector m_audioFiles;

    /**
     * Create an audio file by importing (i.e. converting and/or
     * resampling) an existing file using the conversion library.  If
     * you are not sure whether to use addFile() or importFile(), go for
     * importFile().
     *
     * Called by importURL().
     *
     * throws BadAudioPathException, BadSoundFileException
     */
    AudioFileId importFile(const QString &fileName,
                           int targetSampleRate);

    /**
     * Convert an audio file from arbitrary external format to an
     * internal format suitable for use by addFile, using packages in
     * Rosegarden.  This replaces the Perl script previously used. It
     * returns 0 for OK.  This is used by importFile and importURL
     * which normally provide the more suitable interface for import.
     */
    int convertAudioFile(const QString &inFile, const QString &outFile);

    /// Convert a relative path or file path to absolute.
    /**
     * Expands "~" and "." to an absolute path.
     */
    QString toAbsolute(const QString &relativePath) const;

    /// Get a short file name from a long one (with '/'s)
    QString getShortFilename(const QString &fileName) const;

    /// Get a directory from a full file path
    QString getDirectory(const QString &path) const;

    /// Reset ID counter based on actual Audio Files in Composition
    void updateAudioFileID(AudioFileId id);

    /// Fetch a new unique Audio File ID.
    AudioFileId getUniqueAudioFileID();
    /// Last Audio File ID that was handed out by getUniqueAudioFileID().
    unsigned int m_lastAudioFileID;

    /// Always in internal format with either a "." or a "~".
    /**
     * Propose that we use QFileInfo terminology.
     *   Path - directory  ./directory
     *   FilePath - directory and filename  ./directory/filename.ext
     *   FileName - filename.ext
     *   Absolute - Starting from root.  /tmp/filename.ext
     *   Relative - Using "." or "~".  ./audio/filename.ext
     *   Canonical - Absolute simplified.
     */
    QString m_relativeAudioPath;

    /// Whether the user has confirmed the audio file path.
    bool m_audioLocationConfirmed;

    void moveFiles(const QString &newPath);

    PeakFileManager m_peakManager;

    // All audio files are stored in m_audioFiles.  These additional
    // sets of pointers just refer to those that have been created by
    // recording or derivations since the last save, and thus
    // that the user may wish to remove if the document is not saved.
    std::set<AudioFile *> m_recordedAudioFiles;
    std::set<AudioFile *> m_derivedAudioFiles;

    int m_expectedSampleRate;

    /// Progress Dialog passed in by clients.
    QPointer<QProgressDialog> m_progressDialog;
};


}

#endif // RG_AUDIOFILEMANAGER_H
