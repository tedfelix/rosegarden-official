/*
    Rosegarden-4 v0.1
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

#include "Selection.h"
#include "Segment.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"

namespace Rosegarden {

EventSelection::EventSelection(Segment& t)
    : m_originalSegment(t),
      m_beginTime(0),
      m_endTime(0),
      m_haveRealBeginTime(false)
{
}

EventSelection::EventSelection(Segment& t, timeT beginTime, timeT endTime)
    : m_originalSegment(t),
      m_beginTime(0),
      m_endTime(0),
      m_haveRealBeginTime(false)
{
    Segment::iterator i = t.findTime(beginTime);
    Segment::iterator j = t.findTime(endTime);

    if (i != t.end()) {
	m_beginTime = (*i)->getAbsoluteTime();
	while (i != j) {
	    m_endTime = (*i)->getAbsoluteTime() + (*i)->getDuration();
	    m_segmentEvents.insert(*i);
	    ++i;
	}
	m_haveRealBeginTime = true;
    }
}

EventSelection::~EventSelection()
{
}

void EventSelection::addEvent(Event *e)
{ 
    if (e->getAbsoluteTime() < m_beginTime || !m_haveRealBeginTime) {
	m_beginTime = e->getAbsoluteTime();
	m_haveRealBeginTime = true;
    }
    if (e->getAbsoluteTime() + e->getDuration() > m_endTime) {
	m_endTime = e->getAbsoluteTime() + e->getDuration();
    }
    m_segmentEvents.insert(e);
}

bool EventSelection::contains(Event *e) const
{
    return m_segmentEvents.find(e) != m_segmentEvents.end();
}

timeT EventSelection::getTotalDuration() const
{
    return getEndTime() - getBeginTime();
}

void
EventSelection::recordSelectionOnSegment(PropertyName property)
{
    removeSelectionFromSegment(property);
    for (eventcontainer::iterator i = m_segmentEvents.begin();
	 i != m_segmentEvents.end(); ++i) {
	(*i)->setMaybe<Bool>(property, true);
    }
}

void
EventSelection::removeSelectionFromSegment(PropertyName property)
{ 
    for (Segment::iterator i = m_originalSegment.begin();
	 i != m_originalSegment.end(); ++i) {
	(*i)->unset(property);
    }
}


}
