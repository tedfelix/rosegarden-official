// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <klocale.h>
#include "rosestrings.h"
#include "rosedebug.h"

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::timeT;

MatrixInsertionCommand::MatrixInsertionCommand(Rosegarden::Segment &segment,
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

    Rosegarden::SegmentMatrixHelper helper(getSegment());
    m_lastInsertedEvent = new Event(*m_event);
    helper.insertNote(m_lastInsertedEvent);
}

MatrixEraseCommand::MatrixEraseCommand(Rosegarden::Segment &segment,
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

MatrixModifyCommand::MatrixModifyCommand(Rosegarden::Segment &segment,
                                         Rosegarden::Event *oldEvent,
                                         Rosegarden::Event *newEvent,
                                         bool isMove):
      BasicCommand((isMove ? i18n("Move Note") : i18n("Modify Note")),
                   segment,
                   std::min(newEvent->getAbsoluteTime(),
                            oldEvent->getAbsoluteTime()), 
                   std::max(oldEvent->getAbsoluteTime() +
                            oldEvent->getDuration(), 
                            newEvent->getAbsoluteTime() +
                            newEvent->getDuration()),
                   true),
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

        Rosegarden::Segment &segment(getSegment());
        segment.insert(m_newEvent);
        segment.eraseSingle(m_oldEvent);
        segment.normalizeRests(normalizeStart, normalizeEnd);
    }
}
