
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

#ifndef VIEWELEMENTSMANAGER_H
#define VIEWELEMENTSMANAGER_H

#include "Track.h"
#include "Event.h"
#include "notationelement.h"

/**
 *
 * ViewElementsManager manages the relationship between Track/Event
 * and NotationElementList/NotationElement objects.
 * 
 * @author Guillaume Laurent, Chris Cannam, Rich Bown
 */


//!!! I really think this class may become obsolete with the
//introduction of the TrackObserver; we should no longer need to wrap
//events carefully before inserting them into the NotationElementList,
//because we could instead make the NotationElementList a
//TrackObserver and get it to wrap its own events. 

class ViewElementsManager : public Rosegarden::TrackObserver
{
public: 
    ViewElementsManager(Rosegarden::Track&);
    virtual ~ViewElementsManager();


    /**
     * Create a new NotationElementList wrapping Events in the
     * [from, to[ interval or return the previously created one
     * (even if passed new arguments)
     */
    NotationElementList* notationElementList(Rosegarden::Track::iterator from,
                                             Rosegarden::Track::iterator to);

    /**
     * Scan [from, to[ for events which aren't wrapped in ViewElements
     * and wrap them
     */
    void insertNewEvents(Rosegarden::Track::iterator from,
                         Rosegarden::Track::iterator to);

    /**
     * Wrap Event in a ViewElement if it doesn't have one already, and
     * inserts it in all ViewElements list.
     *
     * If insertInTrack is true, insert the Event itself in the wrapped
     * Track as well.
     */
    void wrapAndInsert(Rosegarden::Event*, bool insertInTrack = false);

    // overload these for each ViewElement type

    /**
     * Insert a new NotationElement
     *
     * Think about using wrapAndInsert() before using this. This is
     * for cases where you need control over the NotationElement after
     * its creation (like changing the note it represents) and therefore
     * need to create the NotationElement yourself. See
     * NotationView::chordEvent() for an example of this.
     *
     * If insertInTrack is true, also insert the Event which the
     * NotationElement points to in the wrapped Track.
     */
    void insert(NotationElement*, bool insertInTrack = false);

    /**
     * Erase the element pointed to by iterator
     * Also erase the corresponding Event from the wrapped Track
     */
    void erase(NotationElementList::iterator);

    /**
     * Erase the element
     * Also erase the corresponding Event from the wrapped Track
     */
    void eraseSingle(NotationElement*);

    /**
     * Try to collapse the element (note or rest)
     * with the next or previous one if this is possible
     * without breaking the bar count
     */
    void tryCollapse(NotationElement*);

    Rosegarden::Track& getTrack() { return m_track; }


    // TrackObserver methods:

    // called after the event has been added to the track:
    virtual void eventAdded(Rosegarden::Track *, Rosegarden::Event *);

    // called after the event has been removed from the track,
    // and just before it is deleted:
    virtual void eventRemoved(Rosegarden::Track *, Rosegarden::Event *);

    // called from the start of the track dtor:
    virtual void trackDeleted(Rosegarden::Track *);


protected:

    Rosegarden::Track    &m_track;
    NotationElementList* m_notationElements;

    NotationElementList::iterator findEvent(Rosegarden::Event *);
};

#endif


