/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

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

#include "AudioFile.h"
#include "base/Exception.h"
#include "misc/Strings.h"

#include <QObject>
#include <QPointer>

class QProgressDialog;

namespace Rosegarden {

class AudioFileManager;

class AudioFileTimeStretcher : public QObject
{
    Q_OBJECT
    
public:
    AudioFileTimeStretcher(AudioFileManager *afm);
    ~AudioFileTimeStretcher() override;

    /**
     * Stretch an audio file and return the ID of the stretched
     * version.
     *
     * Returns -1 on error.
     */
    AudioFileId getStretchedAudioFile(AudioFileId source,
                                      float ratio);

    void setProgressDialog(QPointer<QProgressDialog> progressDialog)
            { m_progressDialog = progressDialog; }

protected:
    AudioFileManager *m_audioFileManager;

    QPointer<QProgressDialog> m_progressDialog;
};

}

#endif
