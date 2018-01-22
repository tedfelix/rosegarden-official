
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INSERTTRIGGERNOTECOMMAND_H
#define RG_INSERTTRIGGERNOTECOMMAND_H

#include "base/NotationTypes.h"
#include "base/TriggerSegment.h"
#include "document/BasicCommand.h"
#include "gui/editors/notation/NoteStyle.h"
#include <string>
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;


class InsertTriggerNoteCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::InsertTriggerNoteCommand)

public:
    InsertTriggerNoteCommand(Segment &,
                             timeT time,
			     timeT duration,
                             int pitch,
                             int velocity,
                             NoteStyleName noteStyle,
                             TriggerSegmentId id,
                             bool retune,
                             std::string timeAdjust,
                             Mark mark);
    virtual ~InsertTriggerNoteCommand();

protected:
    virtual void modifySegment();

    timeT m_time;
    timeT m_duration;
    int m_pitch;
    int m_velocity;
    NoteStyleName m_noteStyle;
    TriggerSegmentId m_id;
    bool m_retune;
    std::string m_timeAdjust;
    Mark m_mark;
};



}

#endif
