
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
    Track(unsigned int nbTimeSteps = 0, timeT startIdx = 0,
          unsigned int stepsPerBar = 384);
    ~Track();

    timeT getStartIndex() const         { return m_startIdx; }
    void  setStartIndex(timeT i);

    unsigned int getInstrument() const         { return m_instrument; }
    void         setInstrument(unsigned int i) { m_instrument = i; }

    TimeSignature getTimeSigAtEnd() const;

    unsigned int getNbTimeSteps() const;
    void         setNbTimeSteps(unsigned int);

    /**
     * Returns an event group id
     * The id is guaranteed to be unique within the track
     */
    int getNextGroupId() const;

    /**
     * Expands events in the [from, to[ interval into 
     * events of duration baseDuration + events of duration R,
     * with R being equal to the events' initial duration minus baseDuration
     *
     * The events in [from, to[ must all be at the same absolute time
     */
    bool expandIntoGroup(iterator from, iterator to,
                         timeT baseDuration);

    /**
     * Expands the event pointed by i into an event of duration
     * baseDuration + an event of duration R, with R being equal to the
     * event's initial duration minus baseDuration
     *
     * This can work only if, given D = max(i->duration, baseDuration)
     * and d = min(i->duration, baseDuration)
     * one of the following is true :
     * D = 2*d
     * D = 4*d
     * D = 4*d/3
     */
    bool expandIntoGroup(iterator i,
                         timeT baseDuration);
    

    /**
     * The compare class used by Composition
     */
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
    timeT m_startIdx;
    unsigned int m_instrument;

    mutable int m_groupId;
};

}


#endif
