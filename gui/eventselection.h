// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef EVENTSELECTION_H
#define EVENTSELECTION_H

#include <set>

#include "Event.h"

namespace Rosegarden { class Segment; }

/**
 * This class holds a selection of Events, used for cut'n paste
 * operations
 *
 * When created, the EventSelection holds pointers to Events in a
 * Segment. 
 */
class EventSelection
{
public:
    typedef std::multiset<Rosegarden::Event*, Rosegarden::Event::EventCmp> eventcontainer;
    
    EventSelection(Rosegarden::Segment&);

    ~EventSelection();

    /**
     * Remove the selected events from the original segment
     * and shallow-copy them internally
     * (just copy the pointers)
     */
    void cut();

    /**
     * Deep-copy the selected events from the original segment
     * (create new Events from the selected ones)
     */
    void copy();

    /**
     * Copy the selected Events to the specified segment
     * This requires that enough rest space is available at the
     * paste point.
     *
     * @return false if the paste could not be performed (if there
     * wasn't enough rest space to hold all notes)
     */
    bool pasteToSegment(Rosegarden::Segment&, Rosegarden::timeT);

    /**
     * Add an event to the selection.
     * @see NotationSelector#getSelection()
     */
    void addEvent(Rosegarden::Event* e) { m_segmentEvents.insert(e); }

    bool contains(Rosegarden::Event *e) const;

    Rosegarden::timeT getBeginTime() const {
	updateBeginEndTime(); return m_beginTime;
    }

    Rosegarden::timeT getEndTime() const {
	updateBeginEndTime(); return m_endTime;
    }

    Rosegarden::timeT getTotalDuration()   const;
    unsigned int      getNbEvents()        const { return m_ownEvents.size(); }
    unsigned int      getAddedEvents()     const { return m_segmentEvents.size(); }

    const eventcontainer &getSegmentEvents() const { return m_segmentEvents; }
    eventcontainer &getSegmentEvents()		   { return m_segmentEvents; }

    const Rosegarden::Segment &getSegment() const { return m_originalSegment; }
    Rosegarden::Segment &getSegment()             { return m_originalSegment; }

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
    void updateBeginEndTime() const;

    Rosegarden::Segment& m_originalSegment;

    /// iterators pointing to Events from the original Segment
    eventcontainer m_segmentEvents;

    /**
     * our own set of Events copied from m_segmentEvents.
     * These are the events we paste from.
     */
    eventcontainer m_ownEvents;

    mutable Rosegarden::timeT m_beginTime;
    mutable Rosegarden::timeT m_endTime;
};

#endif
