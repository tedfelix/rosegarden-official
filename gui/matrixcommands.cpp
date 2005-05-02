// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "matrixcommands.h"
#include "BaseProperties.h"
#include "SegmentMatrixHelper.h"
#include "Segment.h"
#include "Composition.h"

#include <klocale.h>
#include "rosestrings.h"
#include "rosedebug.h"

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::Int;
using namespace Rosegarden::BaseProperties;

MatrixInsertionCommand::MatrixInsertionCommand(Segment &segment,
                                               timeT time,
                                               timeT endTime,
                                               Event *event) :
    BasicCommand(i18n("Insert Note"), segment, time, endTime),
    m_event(new Event(*event,
                      std::min(time, endTime),
                      (time < endTime) ? endTime - time : time - endTime))
{
    // nothing
}

MatrixInsertionCommand::~MatrixInsertionCommand()
{
    delete m_event;
    // don't want to delete m_lastInsertedEvent, it's just an alias
}

void MatrixInsertionCommand::modifySegment()
{
    MATRIX_DEBUG << "MatrixInsertionCommand::modifySegment()\n";

    if (!m_event->has(VELOCITY)) {
	m_event->set<Int>(VELOCITY, 100);
    }

    Rosegarden::SegmentMatrixHelper helper(getSegment());
    m_lastInsertedEvent = new Event(*m_event);
    helper.insertNote(m_lastInsertedEvent);
}

MatrixPercussionInsertionCommand::MatrixPercussionInsertionCommand(Segment &segment,
								   timeT time,
								   Event *event) :
    BasicCommand(i18n("Insert Percussion Note"), segment,
		 getEffectiveStartTime(segment, time, *event),
		 getEndTime(segment, time, *event)),
    m_event(0),
    m_time(time)
{
    timeT endTime = getEndTime(segment, time, *event);
    m_event = new Event(*event, time, endTime - time);
}

MatrixPercussionInsertionCommand::~MatrixPercussionInsertionCommand()
{
    delete m_event;
    // don't want to delete m_lastInsertedEvent, it's just an alias
}

void MatrixPercussionInsertionCommand::modifySegment()
{
    MATRIX_DEBUG << "MatrixPercussionInsertionCommand::modifySegment()\n";

    if (!m_event->has(VELOCITY)) {
	m_event->set<Int>(VELOCITY, 100);
    }

    Segment &s = getSegment();

    Segment::iterator i = s.findTime(m_time);

    int pitch = 0;
    if (m_event->has(PITCH)) {
	pitch = m_event->get<Int>(PITCH);
    }

    while (i != s.begin()) {

	--i;

	if ((*i)->getAbsoluteTime() < m_time &&
	    (*i)->isa(Note::EventType)) {

	    if ((*i)->has(PITCH) &&
		(*i)->get<Int>(PITCH) == pitch) {

		if ((*i)->getAbsoluteTime() + (*i)->getDuration() > m_time) {
		    Rosegarden::Event *newPrevious = new Rosegarden::Event
			(**i, (*i)->getAbsoluteTime(), m_time - (*i)->getAbsoluteTime());
		    s.erase(i);
		    i = s.insert(newPrevious);
		} else {
		    break;
		}
	    }
	}
    }

    Rosegarden::SegmentMatrixHelper helper(s);
    m_lastInsertedEvent = new Event(*m_event);
    helper.insertNote(m_lastInsertedEvent);
}

timeT
MatrixPercussionInsertionCommand::getEffectiveStartTime(Segment &segment,
							timeT time,
							Rosegarden::Event &event)
{
    Rosegarden::timeT startTime = time;

    int pitch = 0;
    if (event.has(PITCH)) {
	pitch = event.get<Int>(PITCH);
    }

    Segment::iterator i = segment.findTime(time);
    while (i != segment.begin()) {
	--i;

	if ((*i)->has(PITCH) &&
	    (*i)->get<Int>(PITCH) == pitch) {

	    if ((*i)->getAbsoluteTime() < time &&
		(*i)->isa(Note::EventType)) {
		if ((*i)->getAbsoluteTime() + (*i)->getDuration() > time) {
		    startTime = (*i)->getAbsoluteTime();
		} else {
		    break;
		}
	    }
	}
    }

    return startTime;
}

timeT
MatrixPercussionInsertionCommand::getEndTime(Segment &segment,
					     Rosegarden::timeT time,
					     Rosegarden::Event &event)
{
    Rosegarden::timeT endTime = segment.getEndMarkerTime();

    int pitch = 0;
    if (event.has(PITCH)) {
	pitch = event.get<Int>(PITCH);
    }

    for (Segment::iterator i = segment.findTime(time);
	 segment.isBeforeEndMarker(i); ++i) {

	if ((*i)->has(PITCH) &&
	    (*i)->get<Int>(PITCH) == pitch) {

	    if ((*i)->getAbsoluteTime() > time &&
		(*i)->isa(Rosegarden::Note::EventType)) {
		endTime = (*i)->getAbsoluteTime();
	    }
	}
    }

    Rosegarden::Composition *comp = segment.getComposition();
    std::pair<Rosegarden::timeT, Rosegarden::timeT> barRange =
	comp->getBarRangeForTime(time);
    Rosegarden::timeT barDuration = barRange.second - barRange.first;
    
    if (endTime > time + barDuration) {
	endTime = time + barDuration;
    }

    return endTime;
}



MatrixEraseCommand::MatrixEraseCommand(Segment &segment,
                                       Event *event) :
    BasicCommand(i18n("Erase Note"),
                 segment,
		 event->getAbsoluteTime(),
		 event->getAbsoluteTime() + event->getDuration(),
		 true),
    m_event(event),
    m_relayoutEndTime(getEndTime())
{
    // nothing
}

timeT MatrixEraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

void MatrixEraseCommand::modifySegment()
{
    Rosegarden::SegmentMatrixHelper helper(getSegment());

    std::string eventType = m_event->getType();

    if (eventType == Note::EventType) {

	helper.deleteNote(m_event, false);

    }
}

MatrixModifyCommand::MatrixModifyCommand(Segment &segment,
                                         Rosegarden::Event *oldEvent,
                                         Rosegarden::Event *newEvent,
                                         bool isMove,
					 bool normalize):
      BasicCommand((isMove ? i18n("Move Note") : i18n("Modify Note")),
                   segment,
                   std::min(newEvent->getAbsoluteTime(),
                            oldEvent->getAbsoluteTime()), 
                   std::max(oldEvent->getAbsoluteTime() +
                            oldEvent->getDuration(), 
                            newEvent->getAbsoluteTime() +
                            newEvent->getDuration()),
                   true),
      m_normalize(normalize),
      m_oldEvent(oldEvent),
      m_newEvent(newEvent)
{
}

void MatrixModifyCommand::modifySegment()
{
    std::string eventType = m_oldEvent->getType();

    if (eventType == Note::EventType) {

	timeT normalizeStart = std::min(m_newEvent->getAbsoluteTime(),
					m_oldEvent->getAbsoluteTime());

	timeT normalizeEnd = std::max(m_newEvent->getAbsoluteTime() +
				      m_newEvent->getDuration(),
				      m_oldEvent->getAbsoluteTime() +
				      m_oldEvent->getDuration());

        Segment &segment(getSegment());
        segment.insert(m_newEvent);
        segment.eraseSingle(m_oldEvent);

	if (m_normalize) {
	    segment.normalizeRests(normalizeStart, normalizeEnd);
	}
    }
}
