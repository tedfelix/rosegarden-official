
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
#include <string>
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

    
    struct BarPosition
    {
        iterator start;       // i.e. event following barline
        bool fixed;           // user-supplied new-bar or timesig event?
        bool correct;         // false if preceding bar has incorrect duration
        
        BarPosition(iterator istart, bool ifixed, bool icorrect) :
            start(istart), fixed(ifixed), correct(icorrect) { }
    };

    typedef std::vector<BarPosition> BarPositionList;


    // Note that these should be non-persistent properties, because
    // the code that writes out XML converts them to
    // <group>...</group> instead of writing them as explicit
    // properties of the events:
    
    static const std::string BeamedGroupIdPropertyName;
    static const std::string BeamedGroupTypePropertyName;

    timeT getStartIndex() const         { return m_startIdx; }
    void  setStartIndex(timeT i);

    unsigned int getInstrument() const         { return m_instrument; }
    void         setInstrument(unsigned int i) { m_instrument = i; }

    TimeSignature getTimeSigAtEnd(timeT &absTimeOfSig) const;

    unsigned int getNbTimeSteps() const;
    void         setNbTimeSteps(unsigned int);

    /**
     * calculate suitable positions for the bar lines, taking into
     * account any changes of time signature during the piece. 
     * the results can be retrieved with getBarPositions()
     */
    void calculateBarPositions();

    /**
     * returns the set of bar positions calculated by the last call to
     * calculateBarPositions().  No guarantee these are still valid.
     */
    const BarPositionList &getBarPositions() const { return m_barPositions; }

    /**
     * deletes the Event
     */
    void erase(iterator pos);
    void erase(iterator from, iterator to);

    /**
     * Erase the Event and only this one
     * Returns true if the event was found and erased,
     * false otherwise.
     */
    bool eraseSingle(Event*);

    /**
     * If possible, collapse the event with the following or previous
     * one.
     *
     * @return true if collapse was done
     *
     * collapseForward is set to true if * the collapse was with the
     * following element, false if it was * with the previous one
     *
     * collapsedEvent will point to the deleted Event (so we can
     * easily find the corresponding ViewElements to delete), or null
     * if no event was deleted
     */
    bool collapse(Event*, bool& collapseForward, Event*& collapsedEvent);

    /**
     * Returns an iterator pointing to that specific element,
     * end() otherwise
     */
    iterator findSingle(Event*);

    /**
     * Returns an iterator pointing to the next contiguous element of
     * the same type (note or rest) as the one passed as argument, if
     * any. Returns end() otherwise.
     *
     * (for instance if the argument points to a note and the next
     * element is a rest, end() will be returned)
     */
    iterator findContiguousNext(iterator);

    /**
     * Returns an iterator pointing to the previous contiguous element
     * of the same type (note or rest) as the one passed as argument,
     * if any. Returns end() otherwise.
     *
     * (for instance if the argument points to a note and the previous
     * element is a rest, end() will be returned)
     */
    iterator findContiguousPrevious(iterator);

    /**
     * Returns true if both Events can be collapsed (into a single one)
     */
    bool canCollapse(iterator, iterator);
    
    /**
     * Returns an event group id
     * The id is guaranteed to be unique within the track
     */
    int getNextGroupId() const;

    /**
     * Expands (splits) events in the [from, to[ interval into 
     * tied events of duration baseDuration + events of duration R,
     * with R being equal to the events' initial duration minus baseDuration
     *
     * lastInsertedEvent will point to the last inserted event
     *
     * The events in [from, to[ must all be at the same absolute time
     *
     */
    bool expandIntoTie(iterator from, iterator to,
                       timeT baseDuration, iterator& lastInsertedEvent);

    /**
     * Expands (splits) the event pointed by i into an event of duration
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
    bool expandIntoTie(iterator i, timeT baseDuration,
                       iterator& lastInsertedEvent);

    /**
     * Same as expandIntoTie(), but for an Event which hasn't
     * been inserted into a track yet. It will be expanded and
     * inserted along with the new generated event.
     * lastInsertedEvent will point to the 2nd event (the generated
     * one)
     *
     * Note that even if the expansion is not possible, the
     * Event will still be inserted.
     */
    bool expandAndInsertEvent(Event*, timeT baseDuration,
                              iterator& lastInsertedEvent);

    /**
     * Returns the range [start, end[ of events which are at absoluteTime
     */
    void getTimeSlice(timeT absoluteTime, iterator &start, iterator &end);

    /**
     * Returns if the note is part of a chord
     * e.g. if there are more notes at the same absolute time
     */
    bool noteIsInChord(Event *note);

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

    static bool checkExpansionValid(timeT maxDuration, timeT minDuration);

    void addNewBar(iterator start, bool fixed, bool correct) {
        m_barPositions.push_back(BarPosition(start, fixed, correct));
    }

    timeT m_startIdx;
    unsigned int m_instrument;

    mutable int m_groupId;
    BarPositionList m_barPositions;
};

}


#endif
