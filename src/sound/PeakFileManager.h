/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_PEAKFILEMANAGER_H
#define RG_PEAKFILEMANAGER_H

#include <vector>

#include <QObject>
#include <QString>
#include <QPointer>

class QProgressDialog;

#include "sound/SoundFile.h"
#include "PeakFile.h"  // for SplitPointPair

namespace Rosegarden
{

class AudioFile;
class PeakFile;
class RealTime;

/**
 * Accepts an AudioFIle and turns the sample data into peak data for
 * storage in a peak file or a BWF format peak chunk.
 */
class PeakFileManager : public QObject
{
    Q_OBJECT
public:
    PeakFileManager();
    ~PeakFileManager() override;
    
    class BadPeakFileException : public Exception
    {
    public:
        BadPeakFileException(QString path) :
            Exception(QObject::tr("Bad peak file ") + path), m_path(path) { }
        BadPeakFileException(QString path, QString file, int line) :
            Exception(QObject::tr("Bad peak file ") + path, file, line), m_path(path) { }
        BadPeakFileException(const SoundFile::BadSoundFileException &e) :
            Exception(QObject::tr("Bad peak file (malformed audio?) ") + e.getPath()), m_path(e.getPath()) { }

        ~BadPeakFileException() throw() override { }

        QString getPath() const { return m_path; }

    private:
        QString m_path;
    };

private:
    PeakFileManager(const PeakFileManager &pFM);
    PeakFileManager& operator=(const PeakFileManager &);

public:
    /**
     * Check that a given audio file has a valid and up to date
     * peak file or peak chunk.
     *
     * throws BadSoundFileException, BadPeakFileException
     */
    bool hasValidPeaks(AudioFile *audioFile);

    /// For callers of generatePeaks().
    void setProgressDialog(QPointer<QProgressDialog> progressDialog)
            { m_progressDialog = progressDialog; }

    /// Generate a peak file from file details.
    /**
     * if the peak file already exists _and_ it's up to date then we don't
     * do anything.  For BWF files we generate an internal peak chunk.
     *
     * throw BadSoundFileException, BadPeakFileException
     */
    void generatePeaks(AudioFile *audioFile);

    /**
     * throws BadSoundFileException, BadPeakFileException
     */
    std::vector<float> getPreview(AudioFile *audioFile,
                                  const RealTime &startTime,
                                  const RealTime &endTime,
                                  int   width,
                                  bool  showMinima);
    /// Removes peak file from PeakFileManager - doesn't affect audioFile
    bool removeAudioFile(AudioFile *audioFile);

    void clear();
                    
    std::vector<SplitPointPair> 
        getSplitPoints(AudioFile *audioFile,
                       const RealTime &startTime,
                       const RealTime &endTime,
                       int threshold,
                       const RealTime &minTime);

    std::vector<PeakFile *>::const_iterator begin() const
                { return m_peakFiles.begin(); }

    std::vector<PeakFile *>::const_iterator end() const
                { return m_peakFiles.end(); }

protected:
    /// Insert PeakFile based on AudioFile if it doesn't already exist.
    bool insertAudioFile(AudioFile *audioFile);
    /// Auto-inserts PeakFile into manager if it doesn't already exist.
    PeakFile *getPeakFile(AudioFile *audioFile);

    std::vector<PeakFile *> m_peakFiles;

    QPointer<QProgressDialog> m_progressDialog;
};


}


#endif // RG_PEAKFILEMANAGER_H
