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
#include "Event.h"

namespace Rosegarden {

class Segment;
class Composition;


/**
 * EventSelection stores a (possibly non-contiguous) selection
 * of Events taken from a single Segment, used for cut'n paste
 * operations.
 *
 *!!! It's not completely clear that we need this _and_ the
 * Clipboard object.  EventSelection isn't general enough to
 * contain all possible sorts of selection (it's specifically
 * limited to bits of a single segment) but Clipboard is, and
 * may well serve both roles if it was asked to.
 */

class EventSelection
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventcontainer;

    /**
     * Construct an empty EventSelection based on the given Segment.
     */
    EventSelection(Segment &);

    ~EventSelection();

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
    timeT getBeginTime() const { return m_beginTime; }

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
     * Set the SELECTED property on all selected events in the
     * segment, and unset from all non-selected events.
     */
    void recordSelectionOnSegment();

    /**
     * Unset the SELECTED property from all events in the segment
     * that are in this selection.
     */
    void removeSelectionFromSegment();
    
private:
    EventSelection(const EventSelection&);

protected:
    //--------------- Data members ---------------------------------

    Segment& m_originalSegment;

    /// iterators pointing to Events from the original Segment
    eventcontainer m_segmentEvents;

    timeT m_beginTime;
    timeT m_endTime;
    bool m_haveRealBeginTime;
};


/**
 * SegmentSelection is a selection consisting of one or more
 * whole Segments.
 *
 * It isn't implemented yet.
 */

class SegmentSelection  // Sounds like a box of chocolates
{
public:
    SegmentSelection(Rosegarden::Composition &comp);
    ~SegmentSelection();

private:

   Composition &m_originalComposition;

};

}

#endif
