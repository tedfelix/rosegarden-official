// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _STAFF_H_
#define _STAFF_H_

#include "ViewElement.h"
#include "Segment.h"

#include <iostream>
#include <cassert>

namespace Rosegarden 
{

/**
 * Staff is the base class for classes which represent a Segment as an
 * on-screen graphic.  It manages the relationship between Segment/Event
 * and specific implementations of ViewElement.
 *
 * The template argument T must be a subclass of ViewElement.
 *
 * Staff was formerly known as ViewElementsManager.
 */
class Staff : public SegmentObserver
{
public: 
    virtual ~Staff();

    /**
     * Create a new ViewElementList wrapping all Events in the
     * segment, or return the previously created one
     */
    ViewElementList *getViewElementList();

    /**
     * Create a new ViewElementList wrapping Events in the
     * [from, to[ interval, or return the previously created one
     * (even if passed new arguments)
     */
    ViewElementList *getViewElementList(Segment::iterator from,
					   Segment::iterator to);

    /**
     * Return the Segment wrapped by this object 
     */
    Segment &getSegment() { return m_segment; }

    /**
     * Return the Segment wrapped by this object 
     */
    const Segment &getSegment() const { return m_segment; }

    /**
     * SegmentObserver method - called after the event has been added to
     * the segment
     */
    virtual void eventAdded(const Segment *, Event *);

    /**
     * SegmentObserver method - called after the event has been removed
     * from the segment, and just before it is deleted
     */
    virtual void eventRemoved(const Segment *, Event *);

    /** 
     * SegmentObserver method - called after the segment's end marker
     * time has been changed
     */
    virtual void endMarkerTimeChanged(const Segment *, bool shorten);

protected:
    Staff(Segment &);
    virtual ViewElement* makeViewElement(Event*) = 0;
    
    /**
     * Return true if the event should be wrapped
     * Useful for piano roll where we only want to wrap notes
     * (always true by default)
     */
    virtual bool wrapEvent(Event *);

    Segment &m_segment;
    ViewElementList *m_viewElementList;
    ViewElementList::iterator findEvent(Rosegarden::Event *);

private: // not provided
    Staff(const Staff &);
    Staff &operator=(const Staff &);
};


}

#endif

