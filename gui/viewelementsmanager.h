
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
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class ViewElementsManager
{
public: 
    ViewElementsManager(Rosegarden::Track&);
    ~ViewElementsManager();

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
     * Wrap Event in a ViewElement if it doesn't have one already
     */
    void insert(Rosegarden::Event*);

    // overload these for each ViewElement type
    void insert(NotationElement*);
    void erase(NotationElementList::iterator);

    Rosegarden::Track& getTrack() { return m_track; }

protected:

    Rosegarden::Track    &m_track;
    NotationElementList* m_notationElements;
    
};

#endif
