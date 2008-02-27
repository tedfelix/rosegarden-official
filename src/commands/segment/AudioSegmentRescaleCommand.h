/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_AUDIOSEGMENTRESCALECOMMAND_H_
#define _RG_AUDIOSEGMENTRESCALECOMMAND_H_

#include <kcommand.h>
#include <qstring.h>
#include "base/Event.h"
#include <klocale.h>

namespace Rosegarden
{

class Segment;
class AudioFileManager;
class AudioFileTimeStretcher;
class RosegardenGUIDoc;
class ProgressDialog;

class AudioSegmentRescaleCommand : public KNamedCommand
{
public:
    AudioSegmentRescaleCommand(RosegardenGUIDoc *doc,
                               Segment *segment, float ratio);
    AudioSegmentRescaleCommand(RosegardenGUIDoc *doc,
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
    
    static QString getGlobalName() { return i18n("Stretch or S&quash..."); }

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
