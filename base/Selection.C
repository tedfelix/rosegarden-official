/*
    Rosegarden-4 v0.2
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

EventSelection::EventSelection(Segment& t) :
    m_originalSegment(t),
    m_beginTime(0),
    m_endTime(0),
    m_haveRealStartTime(false)
{
    t.addObserver(this);
}

EventSelection::EventSelection(Segment& t, timeT beginTime, timeT endTime) :
    m_originalSegment(t),
    m_beginTime(0),
    m_endTime(0),
    m_haveRealStartTime(false)
{
    t.addObserver(this);

    Segment::iterator i = t.findTime(beginTime);
    Segment::iterator j = t.findTime(endTime);

    if (i != t.end()) {
	m_beginTime = (*i)->getAbsoluteTime();
	while (i != j) {
	    m_endTime = (*i)->getAbsoluteTime() + (*i)->getDuration();
	    m_segmentEvents.insert(*i);
	    ++i;
	}
	m_haveRealStartTime = true;
    }
}

EventSelection::EventSelection(const EventSelection &sel) :
    SegmentObserver(),
    m_originalSegment(sel.m_originalSegment),
    m_segmentEvents(sel.m_segmentEvents),
    m_beginTime(sel.m_beginTime),
    m_endTime(sel.m_endTime),
    m_haveRealStartTime(sel.m_haveRealStartTime)
{
}

EventSelection::~EventSelection()
{
    m_originalSegment.removeObserver(this);
}

void EventSelection::addEvent(Event *e)
{ 
    if (contains(e)) return;

    if (e->getAbsoluteTime() < m_beginTime || !m_haveRealStartTime) {
	m_beginTime = e->getAbsoluteTime();
	m_haveRealStartTime = true;
    }
    if (e->getAbsoluteTime() + e->getDuration() > m_endTime) {
	m_endTime = e->getAbsoluteTime() + e->getDuration();
    }
    m_segmentEvents.insert(e);
}

void EventSelection::addFromSelection(EventSelection *sel)
{
    for (eventcontainer::iterator i = sel->getSegmentEvents().begin();
	 i != sel->getSegmentEvents().end(); ++i) {
	if (!contains(*i)) addEvent(*i);
    }
}

void EventSelection::removeEvent(Event *e) 
{
    std::pair<eventcontainer::iterator,eventcontainer::iterator> 
	interval = m_segmentEvents.equal_range(e);

    for (eventcontainer::iterator it = interval.first;
         it != interval.second; it++)
    {
        if (*it == e) {
	    m_segmentEvents.erase(it); 
	    return;
	}
    }
}

bool EventSelection::contains(Event *e) const
{
    std::pair<eventcontainer::iterator,eventcontainer::iterator> 
	interval = m_segmentEvents.equal_range(e);

    for (eventcontainer::iterator it = interval.first;
         it != interval.second; ++it)
    {
        if (*it == e) return true;
    }

    return false;
}

bool EventSelection::contains(const std::string &type) const
{
    for (eventcontainer::iterator i = m_segmentEvents.begin();
	 i != m_segmentEvents.end(); ++i) {
	if ((*i)->isa(type)) return true;
    }
    return false;
}

timeT EventSelection::getTotalDuration() const
{
    return getEndTime() - getStartTime();
}

void
EventSelection::eventRemoved(const Segment *s, Event *e)
{
    if (s == &m_originalSegment /*&& contains(e)*/) {
//!!!	m_segmentEvents.erase(m_segmentEvents.find(e));
	std::cerr << "Removing event at " << e->getAbsoluteTime() << " from selection" << endl;
        removeEvent(e);
    }
}

}
