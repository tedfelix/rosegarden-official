
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

class TimeSignature;

/**
 * This class owns the Events its items are pointing at
 */
class Track : public std::multiset<Event*, Event::EventCmp>
{
public:
    Track(unsigned int nbTimeSteps = 384, unsigned int startIdx = 0,
          unsigned int stepsPerBar = 384);
    ~Track();

    unsigned int getStartIndex() const         { return m_startIdx; }
    void         setStartIndex(unsigned int i);

    unsigned int getInstrument() const         { return m_instrument; }
    void         setInstrument(unsigned int i) { m_instrument = i; }

    TimeSignature getTimeSigAtEnd() const;

    unsigned int getNbTimeSteps() const;
    void setNbTimeSteps(unsigned int);

    struct TrackCmp
    {
        bool operator()(const Track* a, const Track* b) const 
        {
            if (a->getInstrument() == b->getInstrument())
                return a->getStartIndex() < b->getStartIndex();

            return a->getInstrument() < b->getInstrument();
        }
    };

protected:
    unsigned int m_startIdx;
    unsigned int m_instrument;
};

}


#endif
