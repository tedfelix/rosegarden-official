
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

#include <set>
#include "Event.h"

namespace Rosegarden 
{
    
/**
 * This class owns the Events its items are pointing at
 */
class Track : public std::multiset<Event*, Event::EventCmp>
{
public:
    Track(unsigned int nbBars = 0, unsigned int startIdx = 0);
    ~Track();

    unsigned int getStartIndex() const         { return m_startIdx; }
    void         setStartIndex(unsigned int i);

    unsigned int getInstrument() const         { return m_instrument; }
    void         setInstrument(unsigned int i) { m_instrument = i; }

    unsigned int getNbBars() const;
    void setNbBars(unsigned int);

protected:
    unsigned int m_startIdx;
    unsigned int m_nbBars;
    unsigned int m_instrument;
};

struct TrackCompare
{
    bool operator()(const Track* a, const Track* b) const 
    {
        if (a->getInstrument() == b->getInstrument())
            return a->getStartIndex() < b->getStartIndex();

        return a->getInstrument() < b->getInstrument();
    }

};

}


#endif

