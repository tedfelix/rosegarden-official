
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
#include "NotationTypes.h" // for TimeSignature

namespace Rosegarden 
{

/**
 * Track is the container for a set of Events that are all played on
 * the same instrument.  Each event has an absolute starting time,
 * which is used as the index within the track.  Multiple events may
 * have the same absolute time.
 * 
 * (For example, chords are represented simply as a sequence of notes
 * that share a starting time.  The Track can contain counterpoint --
 * notes that overlap, rather than starting and ending together -- but
 * in practice it's probably too hard to display so we should make
 * more than one Track if we want to represent true counterpoint.)
 *
 * The Track is capable of doing various operations including
 * inserting and removing Events, splitting notes and rests,
 * calculating bar positions, etc.  It is aware of Note, Rest and
 * TimeSignature events and of BeamedGroup properties, but has no
 * awareness of any other specifically notation-related events
 * such as Key or Clef.
 *
 * The Track owns the Events its items are pointing at.
 */

class Track : public std::multiset<Event*, Event::EventCmp>
{
public:
    Track(unsigned int nbTimeSteps = 0, timeT startIdx = 0,
          unsigned int stepsPerBar = 384);
    ~Track();

    
    struct BarPosition
    {
        timeT start;          // absolute time of event following barline
        bool fixed;           // user-supplied new-bar or timesig event?
        bool correct;         // false if preceding bar has incorrect duration
        TimeSignature timeSignature;  // time sig of following bar
        
        BarPosition(timeT istart, bool ifixed, bool icorrect,
                    const TimeSignature &itimesig) :
            start(istart), fixed(ifixed), correct(icorrect),
            timeSignature(itimesig) { }

        bool operator<(const BarPosition &bp) const {
            return start < bp.start;
        }
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

    /**
     * Equivalent to getBarPositions()[getBarNumber(end())].timeSignature,
     * but slower and capable of working even if calculateBarPositions has
     * not been called lately.
     */
    //!!! This may be obsolete, but we'll think about that more later
    TimeSignature getTimeSigAtEnd(timeT &absTimeOfSig) const;

    unsigned int getNbTimeSteps() const;
    void         setNbTimeSteps(unsigned int);

    /**
     * Calculates suitable positions for the bar lines, taking into
     * account any changes of time signature during the piece. 
     * The results can be retrieved with getBarPositions().
     */
    void calculateBarPositions();

    /**
     * Returns the set of bar positions calculated by the last call to
     * calculateBarPositions().  No guarantee these are still valid.
     */
    const BarPositionList &getBarPositions() const { return m_barPositions; }

    /**
     * Returns the set of bar positions calculated by the last call to
     * calculateBarPositions().  No guarantee these are still valid.
     */
    BarPositionList &getBarPositions() { return m_barPositions; }

    /** 
     * Returns the number of the bar (as an index into the
     * BarPositionList) that contains the given iterator.  You may
     * pass end() to get the number of the final bar, although looking
     * up the last item in the BarPositionList might be simpler.
     * Returned value will only be correct if calculateBarPositions
     * has been called since the track was last modified.
     */
    //!!! Untested.
    int getBarNumber(const iterator &i) const;
    int getBarNumber(const Event *e) const;

    /**
     * Erases a single Event
     */
    void erase(iterator pos);

    /**
     * Erases a set of Events
     */
    void erase(iterator from, iterator to);

    /**
     * Looks up an Event and if it finds it, erases it.
     * @return true if the event was found and erased, false otherwise.
     */
    bool eraseSingle(Event*);

    /**
     * If possible, collapses the event with the following or previous
     * one.
     *
     * @return true if collapse was done
     *
     * collapseForward is set to true if the collapse was with the
     * following element, false if it was with the previous one
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

    void addNewBar(timeT start, bool fixed, bool correct, 
                   const TimeSignature &timesig) {
        m_barPositions.push_back(BarPosition(start, fixed, correct, timesig));
    }

    timeT m_startIdx;
    unsigned int m_instrument;

    mutable int m_groupId;
    BarPositionList m_barPositions;
};

}


#endif
