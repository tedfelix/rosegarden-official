// -*- c-basic-offset: 4 -*-


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

#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <set>
#include <string>

#include "Event.h"
#include "NotationTypes.h"

namespace Rosegarden 
{

/**
 * Segment is the container for a set of Events that are all played on
 * the same instrument.  Each event has an absolute starting time,
 * which is used as the index within the segment.  Multiple events may
 * have the same absolute time.
 * 
 * (For example, chords are represented simply as a sequence of notes
 * that share a starting time.  The Segment can contain counterpoint --
 * notes that overlap, rather than starting and ending together -- but
 * in practice it's probably too hard to display so we should make
 * more than one Segment if we want to represent true counterpoint.)
 *
 * If you want to carry out notation-related editing operations on
 * a Segment, take a look at SegmentNotationHelper.  If you want to play a
 * Segment, try SegmentPerformanceHelper for duration calculations.
 *
 * The Segment owns the Events its items are pointing at.
 */

class SegmentObserver;
class Quantizer;

class Segment : public std::multiset<Event*, Event::EventCmp>
{
public:
    Segment(timeT startIdx = 0);
    ~Segment();
    
    timeT getStartIndex() const { return m_startIdx; }
    void  setStartIndex(timeT i);

    timeT getDuration() const;
    void  setDuration(timeT); // fills up with rests when lengthening

    timeT getEndIndex() const { return m_startIdx + getDuration(); }

    unsigned int getInstrument() const         { return m_instrument; }
    void         setInstrument(unsigned int i) { m_instrument = i; }

    /**
     * Returns the segment storing Bar and TimeSignature events,
     * or null if none (which should be the case iff the Segment is
     * not contained in a Composition).
     */
    const Segment *getReferenceSegment() const {
	notifyReferenceSegmentRequested();
	return m_referenceSegment;
    }

    /// Should only be called by Composition
    void setReferenceSegment(const Segment *refsegment) {
	m_referenceSegment = refsegment;
    }

    /// Should only be called by Composition
    void setQuantizer(const Quantizer *q) { m_quantizer = q; }

    /// Only valid when in a Composition
    const Quantizer *getQuantizer() const { return m_quantizer; }

    /**
     * Returns an iterator onto the reference segment, pointing to the
     * last bar or time signature event before (or at) the absolute
     * time given.  Returns end() of reference segment if there are no
     * bar or time signature events before this time.
     *
     * Do not call this unless the segment is in a Composition.
     */
    iterator findBarAt(timeT) const;

    /**
     * Returns an iterator onto the reference segment, pointing to the
     * next bar or time signature event after (or at) the absolute
     * time given.  Returns end() of reference segment if there are no
     * bar or time signature events after this time.
     *
     * Do not call this unless the segment is in a Composition.
     */
    iterator findBarAfter(timeT) const;

    /**
     * Returns an iterator onto the reference segment, pointing to the
     * last time signature before (or at) the absolute time given.
     * Returns end() of reference segment if there was no time signature
     * before this time.
     * 
     * Do not call this unless the segment is in a Composition.
     */
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
     * It is safe to call this if the segment is not in a Composition;
     * in this case there can be no time signatures and the method
     * will return 0 and the default.
     */
    timeT findTimeSignatureAt(timeT, TimeSignature &) const;

    /**
     * Returns the time at which the bar containing the given time
     * starts.  Returns end-of-segment if the given time exceeds the
     * segment's length.
     *
     * Do not call this unless the segment is in a Composition.
     */
    timeT findBarStartTime(timeT) const;

    /**
     * Returns the time at which the bar containing the given time
     * ends.  Returns end-of-segment if the given time exceeds the
     * segment's length.
     *
     * Do not call this unless the segment is in a Composition.
     */
    timeT findBarEndTime(timeT) const;

    /**
     * Returns an iterator onto this segment, pointing to the first item
     * in the bar containing the given time.
     *
     * Do not call this unless the segment is in a Composition.
     */
    iterator findStartOfBar(timeT) const;

    /**
     * Returns an iterator onto this segment, pointing to the first item
     * in the bar following the one containing the given time.
     *
     * Do not call this unless the segment is in a Composition.
     */
    iterator findStartOfNextBar(timeT) const;

    /**
     * Returns the time signature in effect at the end of the segment,
     * without referring to the bar position data.  Inefficient unless
     * the time signature changes very close to the end of the segment.
     * 
     * Does not require the reference segment to exist (i.e. okay if
     * segment is not in a Composition).
     */
//!!!    TimeSignature getTimeSigAtEnd(timeT &absTimeOfSig);

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
     * The id is guaranteed to be unique within the segment, but not to
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
     * Returns an iterator pointing to the note that this one is tied
     * with, in the forward direction if goForwards or back otherwise.
     * Returns end() if none.
     *
     * Untested
     */
    iterator getNoteTiedWith(Event *note, bool goForwards) const;

    /**
     * Fill up the segment with rests, from the end of the last event
     * currently on the segment to the endTime given.  Actually, this
     * does much the same as setDuration does when it extends a segment.
     * Hm.
     */
    void fillWithRests(timeT endTime);

    /**
     * The compare class used by Composition
     */
    struct SegmentCmp
    {
        bool operator()(const Segment* a, const Segment* b) const 
        {
            if (a->getInstrument() == b->getInstrument())
                return a->getStartIndex() < b->getStartIndex();

            return a->getInstrument() < b->getInstrument();
        }
    };

    /**
     * An alternative compare class that orders by start time first
     */
    struct SegmentTimeCmp
    {
	bool operator()(const Segment *a, const Segment *b) const {
	    return a->getStartIndex() < b->getStartIndex();
	}
    };

    /// For use by SegmentObserver objects like Composition & ViewElementsManager
    void    addObserver(SegmentObserver *obs) { m_observers.insert(obs); }

    /// For use by SegmentObserver objects like Composition & ViewElementsManager
    void removeObserver(SegmentObserver *obs) { m_observers.erase (obs); }

private:
    timeT m_startIdx;
    unsigned int m_instrument;

    mutable int m_id;

    /// contains bar position data etc. I do not own this
    const Segment *m_referenceSegment;

    typedef std::set<SegmentObserver *> ObserverSet;
    ObserverSet m_observers;

    const Quantizer *m_quantizer;

    void notifyAdd(Event *) const;
    void notifyRemove(Event *) const;
    void notifyReferenceSegmentRequested() const;

    // cache to optimise otherwise-disgusting-inefficiency in fillWithRests
    // that seriously affects MIDI file loading for files in which notes
    // are generally truncated somewhat (hence lots of rests)
//!!!    TimeSignature m_timeSigAtEnd;
//!!!    timeT m_timeSigTime;
//!!!    void invalidateTimeSigAtEnd() { m_timeSigTime = -1; }
//!!!    void findTimeSigAtEnd();

private:
    Segment(const Segment &);
    Segment &operator=(const Segment &);
};


class SegmentObserver
{
public:
    // called after the event has been added to the segment:
    virtual void eventAdded(const Segment *, Event *) = 0;

    // called after the event has been removed from the segment,
    // and just before it is deleted:
    virtual void eventRemoved(const Segment *, Event *) = 0;

    // probably only of interest to Composition
    virtual void referenceSegmentRequested(const Segment *) { }
};


// an abstract base

class SegmentHelper
{
protected:
    SegmentHelper(Segment &t) : m_segment(t) { }
    virtual ~SegmentHelper();

    typedef Segment::iterator iterator;

    Segment &segment() { return m_segment; }
    const Quantizer &quantizer() { return *(segment().getQuantizer()); }

    Segment::iterator begin() { return segment().begin(); }
    Segment::iterator end()   { return segment().end();   }

    Segment::iterator insert(Event *e) { return segment().insert(e); }
    void erase(Segment::iterator i)    { segment().erase(i); }

private:
    Segment &m_segment;
};

}


#endif
