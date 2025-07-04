/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SetNoteTypeCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"


namespace Rosegarden
{


void
SetNoteTypeCommand::modifySegment()
{
    typedef std::vector<Event *> EventVec;
    EventVec toErase;
    EventVec toInsert;

    timeT endTime = getEndTime();
    SegmentNotationHelper segmentNotationHelper(m_selection->getSegment());

    for (EventContainer::iterator i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end();
         ++i) {

        if ((*i)->isa(Note::EventType)) {
            toErase.push_back(*i);

            Event *e;
            if (m_notationOnly) {
                e = new Event(**i,
                              (*i)->getAbsoluteTime(),
                              (*i)->getDuration(),
                              (*i)->getSubOrdering(),
                              (*i)->getNotationAbsoluteTime(),
                              Note(m_type).getDuration());
            } else {
                e = new Event(**i,
                              (*i)->getNotationAbsoluteTime(),
                              Note(m_type).getDuration());
            }

            if (e->getNotationAbsoluteTime() + e->getNotationDuration() > endTime) {
                endTime = e->getNotationAbsoluteTime() + e->getNotationDuration();
            }

            toInsert.push_back(e);
        }
    }

    for (EventVec::iterator i1 = toErase.begin(); i1 != toErase.end(); ++i1) {
        m_selection->getSegment().eraseSingle(*i1);
    }

    for (EventVec::iterator i1 = toInsert.begin(); i1 != toInsert.end(); ++i1) {
        Segment::iterator note =
            m_selection->getSegment().insert(*i1);
        // segmentNotationHelper sometimes erases the event and makes
        // new ones.  actualEvent is always the first event timewise,
        // though it may be followed by tied notes.
        Event *actualEvent =
            segmentNotationHelper.makeThisNoteViable(note, true);
        m_selection->addEvent(actualEvent);
    }

    m_selection->getSegment().normalizeRests(getStartTime(), endTime);
}


}
