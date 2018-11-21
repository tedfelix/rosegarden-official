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


#define RG_MODULE_STRING "[MoveCommand]"

#include "MoveCommand.h"

#include "misc/Debug.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "document/BasicCommand.h"
#include <QString>


namespace Rosegarden
{

MoveCommand::MoveCommand(Segment &s, timeT delta, bool useNotationTimings,
                         EventSelection &sel) :
        BasicCommand(getGlobalName(), s,
                     delta < 0 ? sel.getStartTime() + delta : sel.getStartTime(),
                     delta < 0 ? sel.getEndTime() + 1 : sel.getEndTime() + 1 + delta,
                     true),
        m_selection(&sel),
        m_delta(delta),
        m_useNotationTimings(useNotationTimings),
        m_lastInsertedEvent(nullptr)
{
    // nothing else
}

QString
MoveCommand::getGlobalName(timeT delta)
{
    if (delta == 0) {
        return tr("&Move Events");
    } else if (delta < 0) {
        return tr("&Move Events Back");
    } else {
        return tr("&Move Events Forward");
    }
}

void
MoveCommand::modifySegment()
{
    RG_DEBUG << "MoveCommand::modifySegment: delta is " << m_delta
    << ", useNotationTimings " << m_useNotationTimings
    << ", start time " << m_selection->getStartTime()
    << ", end time " << m_selection->getEndTime() << endl;

    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;

    timeT a0 = m_selection->getStartTime();
    timeT a1 = m_selection->getEndTime();
    timeT b0 = a0 + m_delta;
    timeT b1 = b0 + (a1 - a0);

    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        RG_DEBUG << "MoveCommand::modifySegment: event at " << (*i)->getAbsoluteTime() << " type " << (*i)->getType();

        if ((*i)->isa(Note::EventRestType)) continue;

        toErase.push_back(*i);
        timeT newTime =
            (m_useNotationTimings ?
             (*i)->getNotationAbsoluteTime() : (*i)->getAbsoluteTime()) + m_delta;

        Event *e;
        if (m_useNotationTimings) {
            e = new Event(**i, newTime, (*i)->getDuration(), (*i)->getSubOrdering(),
                          newTime, (*i)->getNotationDuration());
        } else {
            e = new Event(**i, newTime);
        }

        toInsert.push_back(e);
    }

    Segment &segment(m_selection->getSegment());

    for (size_t j = 0; j < toErase.size(); ++j) {
        Segment::iterator jtr(segment.findSingle(toErase[j]));
        if (jtr != segment.end()) {
            RG_DEBUG << "found event " << j;
            segment.erase(jtr);
        } else {
            RG_DEBUG << "failed to find event " << j;
        }
    }

    for (size_t j = 0; j < toInsert.size(); ++j) {

        Segment::iterator jtr = segment.end();

        // somewhat like the NoteOverlay part of PasteEventsCommand::modifySegment
        /* nah -- let's do a de-counterpoint afterwards perhaps
        	if (m_useNotationTimings && toInsert[j]->isa(Note::EventType)) {
        	    long pitch = 0;
        	    Accidental explicitAccidental = NoAccidental;
        	    toInsert[j]->get<String>(ACCIDENTAL, explicitAccidental);
        	    if (toInsert[j]->get<Int>(PITCH, pitch)) {
        		jtr = SegmentNotationHelper(segment).insertNote
        		    (toInsert[j]->getAbsoluteTime(),
        		     Note::getNearestNote(toInsert[j]->getDuration()),
        		     pitch, explicitAccidental);
        		delete toInsert[j];
        		toInsert[j] = *jtr;
        	    }
        	} else {
        */
        jtr = segment.insert(toInsert[j]);
        //	}

        // insert new event back into selection
        m_selection->addEvent(toInsert[j]);

        if (jtr != segment.end())
            m_lastInsertedEvent = toInsert[j];
    }

    if (m_useNotationTimings) {
        SegmentNotationHelper(segment).deCounterpoint(b0, b1);
    }

    segment.normalizeRests(a0, a1);
    segment.normalizeRests(b0, b1);
}

}
