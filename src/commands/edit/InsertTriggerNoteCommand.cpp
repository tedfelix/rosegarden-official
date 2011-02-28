/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InsertTriggerNoteCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentMatrixHelper.h"
#include "base/Composition.h"
#include "base/TriggerSegment.h"
#include "document/BasicCommand.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "base/BaseProperties.h"
#include "misc/Strings.h"

namespace Rosegarden
{

using namespace BaseProperties;

InsertTriggerNoteCommand::InsertTriggerNoteCommand(Segment &segment,
        timeT time,
        Note note,
        int pitch,
        int velocity,
        NoteStyleName noteStyle,
        TriggerSegmentId id,
        bool retune,
        std::string timeAdjust,
        Mark mark) :
        BasicCommand(tr("Insert Trigger Note"), segment,
                     time, time + note.getDuration()),
        m_time(time),
        m_note(note),
        m_pitch(pitch),
        m_velocity(velocity),
        m_noteStyle(noteStyle),
        m_id(id),
        m_retune(retune),
        m_timeAdjust(timeAdjust),
        m_mark(mark)
{
    // nothing
}

InsertTriggerNoteCommand::~InsertTriggerNoteCommand()
{
    // nothing
}

void
InsertTriggerNoteCommand::modifySegment()
{
    // Insert via a model event, so as to apply the note style.
    // This is a subset of the work done by NoteInsertionCommand

    Event *e = new Event(Note::EventType, m_time, m_note.getDuration());

    e->set<Int>(PITCH, m_pitch);
    e->set<Int>(VELOCITY, m_velocity);

    if (m_noteStyle != NoteStyleFactory::DefaultStyle) {
        e->set<String>(NotationProperties::NOTE_STYLE, qstrtostr(m_noteStyle));
    }

    e->set<Int>(TRIGGER_SEGMENT_ID, m_id);
    e->set<Bool>(TRIGGER_SEGMENT_RETUNE, m_retune);
    e->set<String>(TRIGGER_SEGMENT_ADJUST_TIMES, m_timeAdjust);

    if (m_mark != Marks::NoMark) {
        Marks::addMark(*e, m_mark, true);
    }

    Segment &s(getSegment());
    Segment::iterator i = SegmentMatrixHelper(s).insertNote(e);

    Segment::iterator j = i;
    while (++j != s.end()) {
        if ((*j)->getAbsoluteTime() >
            (*i)->getAbsoluteTime() + (*i)->getDuration())
            break;
        if ((*j)->isa(Note::EventType)) {
            if ((*j)->getAbsoluteTime() ==
                (*i)->getAbsoluteTime() + (*i)->getDuration()) {
                if ((*j)->has(TIED_BACKWARD) && (*j)->get<Bool>(TIED_BACKWARD) &&
                    (*j)->has(PITCH) && ((*j)->get<Int>(PITCH) == m_pitch)) {
                    (*i)->set<Bool>(TIED_FORWARD, true);
                }
            }
        }
    }

    TriggerSegmentRec *rec =
        getSegment().getComposition()->getTriggerSegmentRec(m_id);

    if (rec)
        rec->updateReferences();
}

}
