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

#ifndef RG_AUDIO_FILE_TIME_STRETCHER_H
#define RG_AUDIO_FILE_TIME_STRETCHER_H

#include <QObject>
#include "AudioFile.h"
#include "base/Exception.h"
#include "misc/Strings.h"

#include <QPointer>

class QProgressDialog;

namespace Rosegarden {

class AudioFileManager;

class AudioFileTimeStretcher : public QObject
{
    Q_OBJECT
    
public:
    AudioFileTimeStretcher(AudioFileManager *mgr);
    virtual ~AudioFileTimeStretcher();

    /**
     * Stretch an audio file and return the ID of the stretched
     * version.  May throw SoundFile::BadSoundFileException,
     * AudioFileManager::BadAudioPathException, CancelledException
     */
    AudioFileId getStretchedAudioFile(AudioFileId source,
                                      float ratio);

    class CancelledException : public Exception
    {
    public:
        CancelledException() : Exception(qstrtostr(QObject::tr("Cancelled"))) { }
        ~CancelledException() throw() { }
    };

    void setProgressDialog(QPointer<QProgressDialog> progressDialog)
            { m_progressDialog = progressDialog; }

signals:
    // DEPRECATED
    void setValue(int);

public slots:
    /**
     * Cancel an ongoing getStretchedAudioFile
     *
     * ??? UNUSED
     */
    void slotStopTimestretch();
    
protected:
    AudioFileManager *m_manager;

    bool m_timestretchCancelled;

    QPointer<QProgressDialog> m_progressDialog;
};

}

#endif
