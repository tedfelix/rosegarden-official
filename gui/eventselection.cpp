// -*- c-basic-offset: 4 -*-

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

#include "eventselection.h"

#include "Segment.h"
#include "SegmentNotationHelper.h"
#include "notationproperties.h"

#include "rosedebug.h"

using Rosegarden::timeT;
using Rosegarden::Event;
using Rosegarden::Bool;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;

EventSelection::EventSelection(Segment& t)
    : m_originalSegment(t),
      m_beginTime(-1),
      m_endTime(-1)
{
}


EventSelection::~EventSelection()
{
    for(eventcontainer::iterator i = m_ownEvents.begin();
        i != m_ownEvents.end(); ++i) {
        delete *i;
    }
}

bool EventSelection::contains(Event *e) const
{
    return m_segmentEvents.find(e) != m_segmentEvents.end();
}

void EventSelection::cut()
{
    if (!m_segmentEvents.size()) return;

    SegmentNotationHelper nt(m_originalSegment);

    // copy Events from original Segment and erase them
    // from that Segment
    //
    for(eventcontainer::iterator i = m_segmentEvents.begin();
        i != m_segmentEvents.end(); ++i) {

        m_ownEvents.insert(new Event(*(*i)));

        // delete Event from Segment
        nt.deleteEvent(*i);
    }

    updateBeginEndTime();

    m_segmentEvents.clear();
}

void EventSelection::copy()
{
    // copy  Events from original Segment
    for(eventcontainer::iterator i = m_segmentEvents.begin();
        i != m_segmentEvents.end(); ++i) {

        // store copy of Event
        m_ownEvents.insert(new Event(*(*i)));
    }

    updateBeginEndTime();

    m_segmentEvents.clear();
}

bool EventSelection::pasteToSegment(Segment& t, timeT atTime)
{
    SegmentNotationHelper nt(t);
    
    timeT duration = getTotalDuration();
    if (! nt.removeRests(atTime, duration)) {
        return false;
    }

    timeT offsetTime = atTime - (*(m_ownEvents.begin()))->getAbsoluteTime();

    for(eventcontainer::iterator i = m_ownEvents.begin();
        i != m_ownEvents.end(); ++i) {

        Event* e = new Event(*(*i));

        e->setAbsoluteTime(e->getAbsoluteTime() + offsetTime);

        t.insert(e);
    }

    return true;
}

timeT EventSelection::getTotalDuration() const
{
    updateBeginEndTime();
    return getEndTime() - getBeginTime();
}

void EventSelection::updateBeginEndTime() const
{
    if ((m_beginTime != -1) && (m_endTime != -1))
        // this has already been calculated
        return;

    if (m_ownEvents.empty() && m_segmentEvents.empty())
        // selection is empty
        return;

    const eventcontainer* selectedEvents = m_ownEvents.empty() ?
        &m_segmentEvents : &m_ownEvents;
    
    eventcontainer::const_iterator iter = selectedEvents->begin();
    
    m_beginTime = (*iter)->getAbsoluteTime();
    iter = selectedEvents->end(); --iter;
    m_endTime = (*iter)->getAbsoluteTime() + (*iter)->getDuration();

    kdDebug(KDEBUG_AREA) << "EventSelection::updateBeginEndTime() : begin : "
                         << m_beginTime << ", end : " << m_endTime << endl;
}

void
EventSelection::recordSelectionOnSegment()
{
    removeSelectionFromSegment();
    for (eventcontainer::iterator i = m_segmentEvents.begin();
	 i != m_segmentEvents.end(); ++i) {
	(*i)->setMaybe<Bool>(NotationProperties::SELECTED, true);
    }
}

void
EventSelection::removeSelectionFromSegment()
{ 
    for (Segment::iterator i = m_originalSegment.begin();
	 i != m_originalSegment.end(); ++i) {
	(*i)->unset(NotationProperties::SELECTED);
    }
}

