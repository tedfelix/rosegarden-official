/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOSEGMENTRESCALECOMMAND_H
#define RG_AUDIOSEGMENTRESCALECOMMAND_H

#include "document/Command.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>

#include <QPointer>

class QProgressDialog;

namespace Rosegarden
{

class Segment;
class AudioFileManager;
class AudioFileTimeStretcher;
class RosegardenDocument;

class AudioSegmentRescaleCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AudioSegmentRescaleCommand)

public:
    AudioSegmentRescaleCommand(RosegardenDocument *doc,
                               Segment *segment, float ratio);
    AudioSegmentRescaleCommand(RosegardenDocument *doc,
                               Segment *segment, float ratio,
                               timeT newStartTime,
                               timeT newEndMarkerTime);
    ~AudioSegmentRescaleCommand() override;

    void execute() override;
    void unexecute() override;

    AudioFileTimeStretcher *getStretcher() { return m_stretcher; }
    int getNewAudioFileId() const { return m_fid; }

    /// Used by m_stretcher during execute().
    void setProgressDialog(QPointer<QProgressDialog> progressDialog);
    
    static QString getGlobalName() { return tr("Stretch or S&quash..."); }

private:
    AudioFileManager *m_afm;
    AudioFileTimeStretcher *m_stretcher;
    Segment *m_segment;
    Segment *m_newSegment;
    bool m_timesGiven;
    timeT m_startTime;
    timeT m_endMarkerTime;
    int m_fid;
    float m_ratio;
    bool m_detached;
};



}

#endif
