// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
 * the same track.  Each event has an absolute starting time,
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
class Composition;

class Segment : public std::multiset<Event*, Event::EventCmp>
{
public:

    // A Segment contains either Internal representation or Audio
    //
    typedef enum {
        Internal,
        Audio
    } SegmentType;

    /**
     * Construct a Segment of a given type with a given formal starting time.
     */
    Segment(SegmentType segmentType = Internal,
            timeT startIdx = 0);

    virtual ~Segment();

    /**
     * Get the formal starting time of the Segment.  This is not
     * necessarily the same as the time of the first event in it.
     */
    timeT getStartIndex() const { return m_startIdx; }

    /**
     * Set the formal starting time of the Segment.  It is usually
     * VERY DANGEROUS to call this on a Segment that has been stored
     * in a Composition, because the Composition uses the start index
     * as a part of the ordering for Segments and if the index changes
     * the ordering may break.  If your Segment is already in a
     * Composition, use Composition::setSegmentStartIndex instead.
     */
    void setStartIndex(timeT i);

    /**
     * Return the effective duration of the segment.  This is the
     * time at which the final event ends relative to the start time
     * of the segment.
     */
    timeT getDuration() const;

    /**
     * Ensure that the duration of the segment reaches the given
     * time, by filling it with suitable rests if it needs
     * lengthening.
     * 
     * It is not strictly necessary to call setDuration to change the
     * duration of a segment -- the duration is always taken from the
     * time and duration of the final event, and events may be
     * inserted anywhere.  But for segments that may be rendered in
     * a score it's vital that the filling rests are present, so
     * in practice setDuration should always be used.
     */
    void setDuration(timeT);

    /**
     * Return the end time of the Segment.  This is the end time of
     * the final event.
     */
    timeT getEndIndex() const { return m_startIdx + getDuration(); }

    /**
     * Get the track number this Segment is associated with.
     */
    unsigned int getTrack() const { return m_track; }

    /**
     * Set the track number this Segment is associated with.  It is
     * usually VERY DANGEROUS to call this on a Segment that has been
     * stored in a Composition, because the Composition uses the track
     * number as part of the ordering for Segments and if the number
     * changes the ordering may break.  If your Segment is already in a
     * Composition, use Composition::setSegmentTrack instead.
     */
    void setTrack(unsigned int i) { m_track = i; }

    /**
     * Note that a Segment does not have to be in a Composition;
     * if it isn't, this will return zero
     */
    Composition *getComposition() const {
	return m_composition;
    }

    /// Should only be called by Composition
    void setComposition(Composition *composition) {
	m_composition = composition;
    }

    /// Only valid when in a Composition
    const Quantizer *getQuantizer() const;


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
     *
     * Note that if the iterator points to a note, the "contiguous"
     * iterator returned may point to a note that follows the first
     * one, overlaps with it, shares a starting time (i.e. they're
     * both in the same chord) or anything else.  "Contiguous" refers
     * only to their locations in the segment's event container,
     * which normally means what you expect for rests but not notes.
     * 
     * See also SegmentNotationHelper::getNextAdjacentNote.
     */
    iterator findContiguousNext(iterator) const;

    /**
     * Returns an iterator pointing to the previous contiguous element
     * of the same type (note or rest) as the one passed as argument,
     * if any. Returns end() otherwise.
     *
     * (for instance if the argument points to a note and the previous
     * element is a rest, end() will be returned)
     *
     * Note that if the iterator points to a note, the "contiguous"
     * iterator returned may point to a note that precedes the first
     * one, overlaps with it, shares a starting time (i.e. they're
     * both in the same chord) or anything else.  "Contiguous" refers
     * only to their locations in the segment's event container,
     * which normally means what you expect for rests but not notes.
     * 
     * See also SegmentNotationHelper::getPreviousAdjacentNote.
     */
    iterator findContiguousPrevious(iterator) const;
    
    /**
     * Return the starting time of the bar that contains time t.
     * This simply delegates to the Composition, and so can only work
     * if the Segment is in a Composition.  See Composition for some
     * more interesting time signature and bar related methods.
     */
    timeT getBarStart(timeT t) const;

    /**
     * Return the ending time of the bar that contains time t.
     * This simply delegates to the Composition, and so can only work
     * if the Segment is in a Composition.  See Composition for some
     * more interesting time signature and bar related methods.
     */
    timeT getBarEnd(timeT t) const;

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
     * Untested and probably marked-for-expiry -- prefer
     * SegmentPerformanceHelper::getTiedNotes
     */
    iterator getNoteTiedWith(Event *note, bool goForwards) const;

    /**
     * Fill up the segment with rests, from the end of the last event
     * currently on the segment to the endTime given.  Actually, this
     * does much the same as setDuration does when it extends a segment.
     * Hm.
     *
     * If permitQuantize is true, the rest duration may be rounded
     * before filling -- this could significantly simplify the
     * resulting score when (for example) interpreting a MIDI file.
     * permitQuantize should not be used if the precise duration of
     * the track will subsequently be of interest.
     *
     * If startTime is supplied, this can be used to fill up a
     * section within a segment (should you have a pathological
     * segment that contains notes already but not rests).  This
     * is likely to be dangerous unless you're quite careful 
     * about making sure the given range doesn't overlap any notes.
     * The default is to fill from the current end of the segment.
     */
    void fillWithRests(timeT endTime, bool permitQuantize = false,
		       timeT startTime = -1);

    /**
     * The compare class used by Composition
     */
    struct SegmentCmp
    {
        bool operator()(const Segment* a, const Segment* b) const 
        {
            if (a->getTrack() == b->getTrack())
                return a->getStartIndex() < b->getStartIndex();

            return a->getTrack() < b->getTrack();
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

    // Get and set Audio file ID (see the AudioFileManager)
    //
    unsigned int getAudioFileID() const { return m_audioFileID; }
    void setAudioFileID(const unsigned int &id) { m_audioFileID = id; }

    // Get the Segment type (Internal or Audio)
    //
    SegmentType getType() const { return m_type; }


    // The audio start and end indices tell us how far into
    // audio file "m_audioFileID" this Segment starts and
    // how far into the sample the Segment finishes.
    //
    // The absolute time this Segment finishes is:
    //
    //    audioEnd - audioStart + start of Segment
    //
    void setAudioStartIndex(const timeT& audioStart)
        { m_audioStartIdx = audioStart; }

    void setAudioEndIndex(const timeT & audioEnd)
        { m_audioEndIdx = audioEnd; }

    timeT getAudioStartIndex() const { return m_audioStartIdx; }
    timeT getAudioEndIndex() const { return m_audioEndIdx; }

private:
    timeT m_startIdx;
    unsigned int m_track;

    mutable int m_id;

    Composition *m_composition; // owns me, if it exists

    typedef std::set<SegmentObserver *> ObserverSet;
    ObserverSet m_observers;

    void notifyAdd(Event *) const;
    void notifyRemove(Event *) const;

private:
    Segment(const Segment &);
    Segment &operator=(const Segment &);

    unsigned int m_audioFileID; // audio file ID (see AudioFileManager)
    SegmentType m_type;         // identifies Segment type

    timeT m_audioStartIdx;      // how far into m_audioFileID our Segment starts
    timeT m_audioEndIdx;        // how far into m_audioFileID our Segment ends
};


class SegmentObserver
{
public:
    // called after the event has been added to the segment:
    virtual void eventAdded(const Segment *, Event *) = 0;

    // called after the event has been removed from the segment,
    // and just before it is deleted:
    virtual void eventRemoved(const Segment *, Event *) = 0;
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
