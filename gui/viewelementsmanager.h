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

#ifndef VIEWELEMENTSMANAGER_H
#define VIEWELEMENTSMANAGER_H

#include "Track.h"
#include "Event.h"
#include "notationelement.h"

/**
 *
 * ViewElementsManager manages the relationship between Track/Event
 * and NotationElementList/NotationElement objects.  For most
 * purposes the only interesting method is getNotationElementList.
 * 
 */
class ViewElementsManager : public Rosegarden::TrackObserver
{
public: 
    ViewElementsManager(Rosegarden::Track&);
    virtual ~ViewElementsManager();

    /**
     * Create a new NotationElementList wrapping all Events in the
     * track, or return the previously created one
     */
    NotationElementList* getNotationElementList();

    /**
     * Create a new NotationElementList wrapping Events in the
     * [from, to[ interval, or return the previously created one
     * (even if passed new arguments)
     */
    NotationElementList* getNotationElementList
    (Rosegarden::Track::iterator from,
     Rosegarden::Track::iterator to);

    /**
     * Wrap Event in a ViewElement if it doesn't have one already, and
     * insert it in a ViewElements list.
     *
     * If insertInTrack is true, insert the Event itself in the
     * wrapped Track as well.  In this case behaviour is equivalent to
     * simply inserting in the track instead of calling this method,
     * so you should usually only use this method if you don't want
     * the event to appear in the underlying track.
     */
    void insert(Rosegarden::Event*, bool insertInTrack = false);

    /**
     * Erase the element pointed to by iterator.  If eraseFromTrack is
     * true, erase the Event from the wrapped Track as well.  In this
     * case behaviour is equivalent to simply erasing from the track
     * instead of calling this method, so you should usually only use
     * this method if you know that the event is not in the Track.
     */
    void erase(NotationElementList::iterator, bool eraseFromTrack = false);

    /// Return the Track which is wrapped by this object 
    Rosegarden::Track& getTrack() { return m_track; }

    /// Return the Track which is wrapped by this object 
    const Rosegarden::Track& getTrack() const { return m_track; }


    // TrackObserver methods:

    /**
     * TrackObserver method - called after the event has been added to
     * the track
     */
    virtual void eventAdded(const Rosegarden::Track *, Rosegarden::Event *);

    /**
     * TrackObserver method - called after the event has been removed
     * from the track, and just before it is deleted
     */
    virtual void eventRemoved(const Rosegarden::Track *, Rosegarden::Event *);


protected:
    Rosegarden::Track    &m_track;
    NotationElementList* m_notationElements;

    NotationElementList::iterator findEvent(Rosegarden::Event *);
};

#endif


