
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

#ifndef _TRACK_H_
#define _TRACK_H_

#include <multiset.h>
#include "Event.h"

/**
 * This class owns the Events its items are pointing at
 */
class Track : public multiset<Event*, EventCmp>
{
public:
    Track(unsigned int nbBars = 0, unsigned int startIdx = 0);
    ~Track();

    unsigned int getStartIndex() const         { return m_startIdx; }
    void         setStartIndex(unsigned int i) { m_startIdx = i; }

    unsigned int getNbBars() const;
    
protected:
    unsigned int m_startIdx;
    unsigned int m_nbBars;
};

#endif

