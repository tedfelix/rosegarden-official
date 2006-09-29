/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "NoteInsertionCommand.h"

#include <klocale.h>
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentMatrixHelper.h"
#include "base/SegmentNotationHelper.h"
#include "document/BasicCommand.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "base/BaseProperties.h"


namespace Rosegarden
{

using namespace BaseProperties;

NoteInsertionCommand::NoteInsertionCommand(Segment &segment, timeT time,
        timeT endTime, Note note, int pitch,
        Accidental accidental,
        bool autoBeam,
        bool matrixType,
        NoteStyleName noteStyle) :
        BasicCommand(i18n("Insert Note"), segment,
                     getModificationStartTime(segment, time),
                     (autoBeam ? segment.getBarEndForTime(endTime) : endTime)),
        m_insertionTime(time),
        m_note(note),
        m_pitch(pitch),
        m_accidental(accidental),
        m_autoBeam(autoBeam),
        m_matrixType(matrixType),
        m_noteStyle(noteStyle),
        m_lastInsertedEvent(0)
{
    // nothing
}

NoteInsertionCommand::~NoteInsertionCommand()
{
    // nothing
}

timeT
NoteInsertionCommand::getModificationStartTime(Segment &segment,
        timeT time)
{
    // We may be splitting a rest to insert this note, so we'll have
    // to record the change from the start of that rest rather than
    // just the start of the note

    timeT barTime = segment.getBarStartForTime(time);
    Segment::iterator i = segment.findNearestTime(time);

    if (i != segment.end() &&
            (*i)->getAbsoluteTime() < time &&
            (*i)->getAbsoluteTime() + (*i)->getDuration() > time &&
            (*i)->isa(Note::EventRestType)) {
        return std::min(barTime, (*i)->getAbsoluteTime());
    }

    return barTime;
}

void
NoteInsertionCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    // If we're attempting to insert at the same time and pitch as an
    // existing note, then we remove the existing note first (so as to
    // change its duration, if the durations differ)
    Segment::iterator i, j;
    segment.getTimeSlice(m_insertionTime, i, j);
    while (i != j) {
        if ((*i)->isa(Note::EventType)) {
            long pitch;
            if ((*i)->get
                    <Int>(PITCH, pitch) && pitch == m_pitch) {
                helper.deleteNote(*i);
                break;
            }
        }
        ++i;
    }

    // insert via a model event, so as to apply the note style

    Event *e = new Event
               (Note::EventType, m_insertionTime, m_note.getDuration());

    e->set
    <Int>(PITCH, m_pitch);
    e->set
    <Int>(VELOCITY, 100);

    if (m_accidental != Accidentals::NoAccidental) {
        e->set
        <String>(ACCIDENTAL, m_accidental);
    }

    if (m_noteStyle != NoteStyleFactory::DefaultStyle) {
        e->set
        <String>(NotationProperties::NOTE_STYLE, m_noteStyle);
    }

    if (m_matrixType) {
        i = SegmentMatrixHelper(segment).insertNote(e);
    } else {
        i = helper.insertNote(e);
        // e is just a model for SegmentNotationHelper::insertNote
        delete e;
    }

    if (i != segment.end())
        m_lastInsertedEvent = *i;

    if (m_autoBeam) {

        // We auto-beam the bar if it contains no beamed groups
        // after the insertion point and if it contains no tupled
        // groups at all.

        timeT barStartTime = segment.getBarStartForTime(m_insertionTime);
        timeT barEndTime = segment.getBarEndForTime(m_insertionTime);

        for (Segment::iterator j = i;
                j != segment.end() && (*j)->getAbsoluteTime() < barEndTime;
                ++j) {
            if ((*j)->has(BEAMED_GROUP_ID))
                return ;
        }

        for (Segment::iterator j = i;
                j != segment.end() && (*j)->getAbsoluteTime() >= barStartTime;
                --j) {
            if ((*j)->has(BEAMED_GROUP_TUPLET_BASE))
                return ;
            if (j == segment.begin())
                break;
        }

        helper.autoBeam(m_insertionTime, m_insertionTime, GROUP_TYPE_BEAMED);
    }
}

}
