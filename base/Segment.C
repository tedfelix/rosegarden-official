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

#include "Track.h"

Track::Track(unsigned int nbBars, unsigned int startIdx)
    : multiset<Event*, EventCmp>(),
    m_startIdx(startIdx),
    m_nbBars(nbBars)
{
    unsigned int initialTime = m_startIdx;
    
    // fill up with whole-note rests
    //
    for (unsigned int i = 0; i < nbBars; ++i) {
        Event *e = new Event;
        e->setType("rest");
        e->setDuration(384); // TODO : get rid of this magic number
        e->setAbsoluteTime(initialTime);
        insert(e);
        initialTime += 384; // btw, it comes from xmlstorableevent.cpp
    }
}

Track::~Track()
{
    // delete content
    for(iterator it = begin(); it != end(); ++it)
        delete (*it);
}

unsigned int
Track::getNbBars() const
{
    //!!! No, not really!

    const_iterator lastEl = end();
    --lastEl;
    unsigned int nbBars = ((*lastEl)->getAbsoluteTime() +
                           (*lastEl)->getDuration()) / 384;

    return nbBars;
}

