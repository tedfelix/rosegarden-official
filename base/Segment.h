
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
#include "NotationTypes.h"

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

class TrackObserver;
class Quantizer;

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
    
    static const PropertyName BeamedGroupIdPropertyName;
    static const PropertyName BeamedGroupTypePropertyName;

    const Quantizer &getQuantizer() { return *m_quantizer; }

    timeT getStartIndex() const { return m_startIdx; }
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

    timeT getDuration() const;
    void  setDuration(timeT); // fills up with rests when lengthening

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
     * Inserts a single Event
     */
    iterator insert(Event *e);

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
     * Returns an event group id
     * The id is guaranteed to be unique within the track
     */
    int getNextGroupId() const;


    /**
     * Checks whether it's reasonable to expand (split) a single event
     * of duration a+b into two events of durations a and b, for some
     * working definition of "reasonable".
     *
     * Currently "reasonable" is: given D = max(a, b) and d = min(b, a)
     * one of the following is true :
     * D = 2*d
     * D = 4*d
     * D = 4*d/3 
     *
     * You should pass note-quantized durations into this method
     */
    bool isExpandValid(timeT a, timeT b);


    /**
     * Expands (splits) events in the [from, to[ interval into 
     * tied events of duration baseDuration + events of duration R,
     * with R being equal to the events' initial duration minus baseDuration
     *
     * lastInsertedEvent will point to the last inserted event
     *
     * The events in [from, to[ must all be at the same absolute time
     * 
     * Does not check "reasonableness" of expansion first
     */
    void expandIntoTie(iterator from, iterator to, timeT baseDuration);


    /**
     * Expands (splits) events in the same timeslice as that pointed
     * to by i into tied events of duration baseDuration + events of
     * duration R, with R being equal to the events' initial duration
     * minus baseDuration
     *
     * Does not check "reasonableness" of expansion first
     */
    void expandIntoTie(iterator i, timeT baseDuration);

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

    /*!!! Currently unused.

    bool expandAndInsertEvent(Event*, timeT baseDuration,
                              iterator& lastInsertedEvent);

    */


    /**
     * Returns true if Events of durations a and b can reasonably be
     * collapsed into a single one of duration a+b, for some
     * definition of "reasonably".
     *
     * You should pass note-quantized durations into this method
     */
    bool isCollapseValid(timeT a, timeT b);

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

    //!!! we can probably remove collapsedEvent now that
    //ViewElementsManager is a TrackObserver.  Besides, surely the
    //event would have been deleted (by erase()) before the code that
    //called this method was able to look at it?

    //!!! maybe needs a more specific name -- doesn't always collapse,
    //only if the collapsed notes make a single note of meaningful
    //duration

    bool collapse(Event*, bool& collapseForward, Event*& collapsedEvent);

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
     * Fill up the track with rests, from the end of the last event
     * currently on the track to the endTime given.
     */
    void fillWithRests(timeT endTime);

    /**
     * Inserts a note, doing all the clever split/merge stuff as
     * appropriate.  Requires up-to-date bar position list.
     */
//    void insertNote(iterator position, Note note, int pitch);
    void insertNote(timeT absoluteTime, Note note, int pitch);

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

    void    addObserver(TrackObserver *obs) { m_observers.insert(obs); }
    void removeObserver(TrackObserver *obs) { m_observers.erase (obs); }

protected:

    /**
     * Collapse multiple consecutive rests into one, in preparation
     * for insertion of a note (whose duration may exceed that of the
     * first rest) at the given position.  The resulting rest event
     * may have a duration that is not expressible as a single note
     * type, and may therefore require splitting again after the
     * insertion.
     *
     * Returns position at which the collapse ended (i.e. the first
     * uncollapsed event)
     */
    iterator collapseRestsForInsert(iterator firstRest, timeT desiredDuration);
    void insertNoteAux(iterator position, int duration, int pitch,
		       bool tiedBack);
    iterator insertSingleNote(iterator position, int duration, int pitch,
			      bool tiedBack);

    void addNewBar(timeT start, bool fixed, bool correct, 
                   const TimeSignature &timesig) {
        m_barPositions.push_back(BarPosition(start, fixed, correct, timesig));
    }

    timeT m_startIdx;
    unsigned int m_instrument;

    mutable int m_groupId;
    BarPositionList m_barPositions;

    typedef std::set<TrackObserver *> ObserverSet;
    ObserverSet m_observers;

    Quantizer *m_quantizer;

    void notifyAdd(Event *);
    void notifyRemove(Event *);
    void notifyTrackGone();
};

class TrackObserver
{
public:
    // called after the event has been added to the track:
    virtual void eventAdded(Track *, Event *) = 0;

    // called after the event has been removed from the track,
    // and just before it is deleted:
    virtual void eventRemoved(Track *, Event *) = 0;

    // called from the start of the track dtor:
    virtual void trackDeleted(Track *) = 0;
};


}


#endif
