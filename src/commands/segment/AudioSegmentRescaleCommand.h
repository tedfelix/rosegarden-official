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

#ifndef RG_AUDIOSEGMENTRESCALECOMMAND_H
#define RG_AUDIOSEGMENTRESCALECOMMAND_H

#include "document/Command.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>

namespace Rosegarden
{

class Segment;
class AudioFileManager;
class AudioFileTimeStretcher;
class RosegardenDocument;
class ProgressDialog;

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
    virtual ~AudioSegmentRescaleCommand();

    virtual void execute();
    virtual void unexecute();

    AudioFileTimeStretcher *getStretcher() { return m_stretcher; }
    int getNewAudioFileId() const { return m_fid; }

    void connectProgressDialog(ProgressDialog *dlg);
    void disconnectProgressDialog(ProgressDialog *dlg);
    
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
