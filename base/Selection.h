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

#ifndef SELECTION_H
#define SELECTION_H

#include <set>
#include "PropertyName.h"
#include "Event.h"
#include "Segment.h"

namespace Rosegarden {

class Composition;


/**
 * EventSelection records a (possibly non-contiguous) selection
 * of Events in a single Segment, used for cut'n paste operations.
 * It does not take a copy of those Events, it just remembers
 * which ones they are.
 */

class EventSelection : public SegmentObserver
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventcontainer;

    /**
     * Construct an empty EventSelection based on the given Segment.
     */
    EventSelection(Segment &);

    /**
     * Construct an EventSelection selecting all the events in the
     * given range of the given Segment.
     */
    EventSelection(Segment &, timeT beginTime, timeT endTime);

    virtual ~EventSelection();

    /**
     * Add an Event to the selection.  The Event should come from
     * the Segment that was passed to the constructor.
     */
    void addEvent(Event* e);

    /**
     * Test whether a given Event (in the Segment) is part of
     * this selection.
     */
    bool contains(Event *e) const;

    /**
     * Return the time at which the first Event in the selection
     * begins.
     */
    timeT getStartTime() const { return m_beginTime; }

    /**
     * Return the time at which the last Event in the selection ends.
     */
    timeT getEndTime() const { return m_endTime; }

    /**
     * Return the total duration spanned by the selection.
     */
    timeT getTotalDuration() const;

    /**
     * Return the number of events added to this selection.
     */
    unsigned int getAddedEvents() const { return m_segmentEvents.size(); }

    const eventcontainer &getSegmentEvents() const { return m_segmentEvents; }
    eventcontainer &getSegmentEvents()		   { return m_segmentEvents; }

    const Segment &getSegment() const { return m_originalSegment; }
    Segment &getSegment()             { return m_originalSegment; }

    /**
     * Set the given boolean property on all selected events in the
     * segment, and unset from all non-selected events.
     */
    void recordSelectionOnSegment(PropertyName property);

    /**
     * Unset the given boolean property from all events in the segment
     * that are in this selection.
     */
    void removeSelectionFromSegment(PropertyName property);

    // SegmentObserver methods
    virtual void eventAdded(const Segment *, Event *) { }
    virtual void eventRemoved(const Segment *, Event *);
    virtual void endMarkerTimeChanged(const Segment *, bool) { }
    
private:
    EventSelection(const EventSelection&);

protected:
    //--------------- Data members ---------------------------------

    Segment& m_originalSegment;

    /// pointers to Events in the original Segment
    eventcontainer m_segmentEvents;

    timeT m_beginTime;
    timeT m_endTime;
    bool m_haveRealStartTime;
};


/**
 * SegmentSelection is much simpler than EventSelection, we don't
 * need to do much with this really
 */

class SegmentSelection : public std::set<Segment *>
{
    // Actually, the std::set is all we need.  The only reason
    // this is a class instead of a typedef is so we can
    // predeclare it
};


}

#endif
