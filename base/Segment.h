
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
    Track(timeT startIdx = 0);
    ~Track();

/*
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
*/

    const Quantizer &getQuantizer() const { return *m_quantizer; }

    timeT getStartIndex() const { return m_startIdx; }
    void  setStartIndex(timeT i);

    timeT getDuration() const;
    void  setDuration(timeT); // fills up with rests when lengthening

    timeT getEndIndex() const { return m_startIdx + getDuration(); }

    unsigned int getInstrument() const         { return m_instrument; }
    void         setInstrument(unsigned int i) { m_instrument = i; }

    /**
     * Calculates suitable positions for the bar lines, taking into
     * account any changes of time signature during the piece. 
     * The results can be retrieved with getBarPositions().
     */
    //!!! NEW -- called automatically by Composition when a time sig is inserted somewhere (anywhere)
/*
    void calculateBarPositions();
*/

    /**
     * Returns the set of bar positions calculated by the last call to
     * calculateBarPositions().  No guarantee these are still valid.
     */
/*
    const BarPositionList &getBarPositions() const { return m_barPositions; }
*/

    /**
     * Returns the set of bar positions calculated by the last call to
     * calculateBarPositions().  No guarantee these are still valid.
     */
/*
    BarPositionList &getBarPositions() { return m_barPositions; }
*/

    /** 
     * Returns the number of the bar (as an index into the
     * BarPositionList) that contains the given iterator.  You may
     * pass end() to get the number of the final bar, although looking
     * up the last item in the BarPositionList might be simpler.
     * Returned value will only be correct if calculateBarPositions
     * has been called since the track was last modified.
     */
/*
    int getBarNumber(const iterator &i) const;
    int getBarNumber(const Event *e) const;
*/

    /// Returns the track storing Bar and TimeSignature events, or null if none
    const Track *getReferenceTrack() const {
	notifyReferenceTrackRequested();
	return m_referenceTrack;
    }

    /// Should only be called by Composition
    void setReferenceTrack(const Track *reftrack) {
	m_referenceTrack = reftrack;
    }

    /**
     * Returns an iterator onto the reference track, pointing to the
     * last bar or time signature event before (or at) the absolute
     * time given.  Returns end() of reference track if there are no
     * bar or time signature events before this time.
     *
     * Do not call this unless the track is in a Composition.
     */
    //!!! untested
    iterator findBarAt(timeT) const;

    /**
     * Returns an iterator onto the reference track, pointing to the
     * last time signature before (or at) the absolute time given.
     * Returns end() of reference track if there was no time signature
     * before this time.
     * 
     * Do not call this unless the track is in a Composition.
     */
    //!!! untested
    iterator findTimeSignatureAt(timeT) const;

    /**
     * Returns the absolute time of the last time signature before (or
     * at) the absolute time given, and the time signature itself
     * through the reference argument.
     * 
     * Returns 0 and the default time signature if there was no time
     * signature before this time.  (You cannot distinguish between
     * this case and a real default time signature at time 0; use the
     * previous method if this matters to you.)
     * 
     * Do not call this unless the track is in a Composition.
     */
    //!!! untested
    timeT findTimeSignatureAt(timeT, TimeSignature &) const;

    /**
     * Returns the time at which the bar containing the given time
     * starts.  Returns -1 if the given time is somehow out of range.
     *
     * Do not call this unless the track is in a Composition.
     */
    //!!! untested
    timeT findBarStartTime(timeT) const;

    /**
     * Returns the time at which the bar containing the given time
     * ends.  Returns -1 if the given time is somehow out of range.
     *
     * Do not call this unless the track is in a Composition.
     */
    //!!! untested
    timeT findBarEndTime(timeT) const;

    /**
     * Returns an iterator onto this track, pointing to the first item
     * in the bar containing the given time.
     *
     * Do not call this unless the track is in a Composition.
     */
    //!!! untested
    iterator findStartOfBar(timeT) const;

    /**
     * Returns an iterator onto this track, pointing to the first item
     * in the bar following the one containing the given time.
     *
     * Do not call this unless the track is in a Composition.
     */
    //!!! untested
    iterator findStartOfNextBar(timeT) const;

    /**
     * Returns the time signature in effect at the end of the track,
     * without referring to the bar position data.  Inefficient unless
     * the time signature changes very close to the end of the track.
     * Does not require the reference track to exist.
     */
    TimeSignature getTimeSigAtEnd(timeT &absTimeOfSig) const;

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
    iterator findSingle(Event*) const;

    /**
     * Returns an iterator pointing to the first element starting at
     * or beyond the given absolute time
     */
    iterator findTime(timeT time) const;

    /**
     * Returns an iterator pointing to the next contiguous element of
     * the same type (note or rest) as the one passed as argument, if
     * any. Returns end() otherwise.
     *
     * (for instance if the argument points to a note and the next
     * element is a rest, end() will be returned)
     */
    iterator findContiguousNext(iterator) const;

    /**
     * Returns an iterator pointing to the previous contiguous element
     * of the same type (note or rest) as the one passed as argument,
     * if any. Returns end() otherwise.
     *
     * (for instance if the argument points to a note and the previous
     * element is a rest, end() will be returned)
     */
    iterator findContiguousPrevious(iterator) const;
    
    /**
     * Returns a numeric id of some sort
     * The id is guaranteed to be unique within the track, but not to
     * have any other interesting properties
     */
    int getNextId() const;

    /**
     * Returns the range [start, end[ of events which are at absoluteTime
     */
    void getTimeSlice(timeT absoluteTime, iterator &start, iterator &end) const;

    /**
     * Returns true if the iterator points at a note in a chord
     * e.g. if there are more notes at the same absolute time
     */
    bool noteIsInChord(Event *note) const;

    /**
     * Fill up the track with rests, from the end of the last event
     * currently on the track to the endTime given.  Actually, this
     * does much the same as setDuration does when it extends a track.
     * Hm.
     */
    void fillWithRests(timeT endTime);

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

    /**
     * An alternative compare class that orders by start time first
     */
    struct TrackTimeCmp
    {
	bool operator()(const Track *a, const Track *b) const {
	    return a->getStartIndex() < b->getStartIndex();
	}
    };

    /// For use by TrackObserver objects like Composition & ViewElementsManager
    void    addObserver(TrackObserver *obs) { m_observers.insert(obs); }

    /// For use by TrackObserver objects like Composition & ViewElementsManager
    void removeObserver(TrackObserver *obs) { m_observers.erase (obs); }

private:
    /// for use by calculateBarPositions
/*
    void addNewBar(timeT start, bool fixed,
                   timeT prevStart, TimeSignature tsig);

    /// used by calculateBarPositions
    bool hasEffectiveDuration(iterator i);
*/

    timeT m_startIdx;
    unsigned int m_instrument;

    mutable int m_id;

/*
    BarPositionList m_barPositions;
*/

    /// contains bar position data etc. I do not own this
    const Track *m_referenceTrack;

    typedef std::set<TrackObserver *> ObserverSet;
    ObserverSet m_observers;

    Quantizer *m_quantizer;

    void notifyAdd(Event *) const;
    void notifyRemove(Event *) const;
    void notifyReferenceTrackRequested() const;

private:
    Track(const Track &);
    Track &operator=(const Track &);
};


class TrackObserver
{
public:
    // called after the event has been added to the track:
    virtual void eventAdded(const Track *, Event *) = 0;

    // called after the event has been removed from the track,
    // and just before it is deleted:
    virtual void eventRemoved(const Track *, Event *) = 0;

    // probably only of interest to Composition
    virtual void referenceTrackRequested(const Track *) { }
};


// an abstract base

class TrackHelper
{
protected:
    TrackHelper(Track &t) : m_track(t) { }
    virtual ~TrackHelper();

    typedef Track::iterator iterator;

    Track &track() { return m_track; }
    const Quantizer &quantizer() { return track().getQuantizer(); }

    Track::iterator begin() { return track().begin(); }
    Track::iterator end()   { return track().end();   }

    Track::iterator insert(Event *e) { return track().insert(e); }
    void erase(Track::iterator i)    { track().erase(i); }

private:
    Track &m_track;
};

}


#endif
