// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4
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

#include "Track.h"
#include "Event.h"
#include "NotationTypes.h"
#include "RefreshStatus.h"
#include "RealTime.h"

namespace Rosegarden 
{

class SegmentRefreshStatus : public RefreshStatus
{
public:
    SegmentRefreshStatus() : m_from(0), m_to(0) {}

    void push(timeT from, timeT to);

    timeT from() { return m_from; }
    timeT to()   { return m_to; }

protected:
    timeT m_from;
    timeT m_to;
};


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
class BasicQuantizer;
class Composition;

class Segment : public std::multiset<Event*, Event::EventCmp>
{
public:
    /// A Segment contains either Internal representation or Audio
    typedef enum {
        Internal,
        Audio
    } SegmentType;

    /**
     * Construct a Segment of a given type with a given formal starting time.
     */
    Segment(SegmentType segmentType = Internal,
            timeT startTime = 0);
    /*
     * Copy constructor
     */
    Segment(const Segment &);

    virtual ~Segment();


    //////
    //
    // BASIC SEGMENT ATTRIBUTES

    /**
     * Get the Segment type (Internal or Audio)
     */
    SegmentType getType() const { return m_type; }

    /**
     * Note that a Segment does not have to be in a Composition;
     * if it isn't, this will return zero
     */
    Composition *getComposition() const {
	return m_composition;
    }

    /**
     * Get the track number this Segment is associated with.
     */
    TrackId getTrack() const { return m_track; }

    /**
     * Set the track number this Segment is associated with.
     */
    void setTrack(TrackId i);

    // label
    //
    void setLabel(const std::string &label) { m_label = label; }
    std::string getLabel() const { return m_label; }

    /**
     * Returns a numeric id of some sort
     * The id is guaranteed to be unique within the segment, but not to
     * have any other interesting properties
     */
    int getNextId() const;


    //////
    //
    // TIME & DURATION VALUES

    /**
     * Return the start time of the Segment.  For a non-audio
     * Segment, this is the start time of the first event in it.
     */
    timeT getStartTime() const;

    /**
     * Return the nominal end time of the Segment.  This must
     * be the same as or earlier than the getEndTime() value.
     * The return value will not necessarily be that last set
     * with setEndMarkerTime, as if there is a Composition its
     * end marker will also be used for clipping.
     */
    timeT getEndMarkerTime() const;

    /**
     * Return the time of the end of the last event stored in the
     * Segment.  This time may be outside the audible/editable
     * range of the Segment, depending on the location of the end
     * marker.
     */
    timeT getEndTime() const;

    /**
     * Shift the start time of the Segment by moving the start
     * times of all the events in the Segment.
     */
    void setStartTime(timeT);

    /**
     * Set the end marker (nominal end time) of this Segment.
     * 
     * If the given time is later than the current end of the
     * Segment's storage, extend the Segment by filling it with
     * rests; if earlier, simply move the end marker.  The end
     * marker time may not precede the start time.
     */
    void setEndMarkerTime(timeT);

    /**
     * Set the end time of the Segment.
     * 
     * If the given time is later than the current end of the
     * Segment's storage, extend the Segment by filling it with
     * rests; if earlier, shorten it by throwing away events as
     * necessary (though do not truncate any events) and also move
     * the end marker to the given time.  The end time may not
     * precede the start time.
     *
     * Note that simply inserting an event beyond the end of the
     * Segment will also change the end time, although it does
     * not fill with rests in the desirable way.
     *
     * Consider using setEndMarkerTime in preference to this.
     */
    void setEndTime(timeT);

    /**
     * Return an iterator pointing to the nominal end of the
     * Segment.  This may be earlier than the end() iterator.
     */
    iterator getEndMarker() const;

    /**
     * Return true if the given iterator points earlier in the
     * Segment than the nominal end marker.  You can use this
     * as an extent test in code such as
     * 
     *  while (segment.isBeforeEndMarker(my_iterator)) {
     *      // ...
     *      ++my_iterator;
     *  }
     *
     * It is not generally safe to write
     *
     *  while (my_iterator != segment.getEndMarker()) {
     *      // ...
     *      ++my_iterator;
     *  }
     * 
     * as the loop will not terminate if my_iterator's initial
     * value is already beyond the end marker.  (Also takes the
     * Composition's end marker into account.)
     */
    bool isBeforeEndMarker(iterator) const;

    /**
     * Remove the end marker, thus making the Segment end
     * at its storage end time (unless the Composition's
     * end marker is earlier).
     */
    void clearEndMarker();

    /**
     * Return the end marker in raw form, that is, a pointer to
     * its value or null if none is set.  Does not take the
     * composition's end marker into account.
     */
    const timeT *getRawEndMarkerTime() const;


    //////
    //
    // QUANTIZATION

    /**
     * Switch quantization on or off.
     */
    void setQuantization(bool quantize);

    /**
     * Find out whether quantization is on or off.
     */
    bool hasQuantization() const;

    /**
     * Set the quantization level.
     * (This does not switch quantization on, if it's currently off,
     * it only changes the level that will be used when it's next
     * switched on.)
     */
    void setQuantizeLevel(timeT unit);

    /**
     * Get the quantizer currently in (or not in) use.
     */
    const BasicQuantizer *const getQuantizer() const;



    //////
    //
    // EVENT MANIPULATION

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
     * Returns an iterator pointing to the first element starting at
     * or before the given absolute time (so returns end() if the
     * time precedes the first event, not if it follows the last one)
     */
    iterator findNearestTime(timeT time) const;


    //////
    //
    // ADVANCED, ESOTERIC, or PLAIN STUPID MANIPULATION

    /**
     * Returns the range [start, end[ of events which are at absoluteTime
     */
    void getTimeSlice(timeT absoluteTime, iterator &start, iterator &end) const;
    
    /**
     * Return the starting time of the bar that contains time t.  This
     * differs from Composition's bar methods in that it will truncate
     * to the start and end times of this Segment, and is guaranteed
     * to return the start time of a bar that is at least partially
     * within this Segment.
     * 
     * (See Composition for most of the generally useful bar methods.)
     */
    timeT getBarStartForTime(timeT t) const;

    /**
     * Return the ending time of the bar that contains time t.  This
     * differs from Composition's bar methods in that it will truncate
     * to the start and end times of this Segment, and is guaranteed
     * to return the end time of a bar that is at least partially
     * within this Segment.
     * 
     * (See Composition for most of the generally useful bar methods.)
     */
    timeT getBarEndForTime(timeT t) const;

    /**
     * Fill up the segment with rests, from the end of the last event
     * currently on the segment to the endTime given.  Actually, this
     * does much the same as setDuration does when it extends a segment,
     * although the endTime is absolute whereas the argument to
     * setDuration is relative to the start of the segment.
     */
    void fillWithRests(timeT endTime);

    /**
     * Fill up a section within a segment with rests, from the
     * startTime given to the endTime given.  This may be useful if
     * you have a pathological segment that contains notes already but
     * not rests, but it is is likely to be dangerous unless you're
     * quite careful about making sure the given range doesn't overlap
     * any notes.
     */
    void fillWithRests(timeT startTime, timeT endTime);

    /**
     * For each series of contiguous rests found between the start and
     * end time, replace the series of rests with another series of
     * the same duration but composed of the theoretically "correct"
     * rest durations to fill the gap, in the current time signature.
     * The start and end time should be the raw absolute times of the
     * events, not the notation-quantized versions, although the code
     * will use the notation quantizations if it finds them.
     */
    void normalizeRests(timeT startTime, timeT endTime);


    //////
    //
    // REPEAT, DELAY, TRANSPOSE

    // Is this Segment repeating?
    //
    bool isRepeating() const { return m_repeating; }
    void setRepeating(bool value);

    /**
     * If this Segment is repeating, calculate and return the time at
     * which the repeating stops.  This is the start time of the
     * following Segment on the same Track, if any, or else the end
     * time of the Composition.  (If this Segment does not repeat,
     * return the end time of the Segment.)
     */
    timeT getRepeatEndTime() const;

    timeT getDelay() const { return m_delay; }
    void setDelay(timeT delay) { m_delay = delay; }

    RealTime getRealTimeDelay() const { return m_realTimeDelay; }
    void setRealTimeDelay(RealTime delay) { m_realTimeDelay = delay; }

    int getTranspose() const { return m_transpose; }
    void setTranspose(const int &transpose) { m_transpose = transpose; }



    //////
    //
    // AUDIO

    // Get and set Audio file Id (see the AudioFileManager)
    //
    unsigned int getAudioFileId() const { return m_audioFileId; }
    void setAudioFileId(const unsigned int &id) { m_audioFileId = id; }

    // The audio start and end times tell us how far into
    // audio file "m_audioFileId" this Segment starts and
    // how far into the sample the Segment finishes.
    //
    void setAudioStartTime(const RealTime&time) { m_audioStartTime = time; }
    void setAudioEndTime(const RealTime &time) { m_audioEndTime = time; }
    RealTime getAudioStartTime() const { return m_audioStartTime; }
    RealTime getAudioEndTime() const { return m_audioEndTime; }

    //////
    //
    // MISCELLANEOUS

    /// Should only be called by Composition
    void setComposition(Composition *composition) {
	m_composition = composition;
    }

    /**
     * The compare class used by Composition
     */
    struct SegmentCmp
    {
        bool operator()(const Segment* a, const Segment* b) const 
        {
            if (a->getTrack() == b->getTrack())
                return a->getStartTime() < b->getStartTime();

            return a->getTrack() < b->getTrack();
        }
    };

    /**
     * An alternative compare class that orders by start time first
     */
    struct SegmentTimeCmp
    {
	bool operator()(const Segment *a, const Segment *b) const {
	    return a->getStartTime() < b->getStartTime();
	}
    };

    /// For use by SegmentObserver objects like Composition & ViewElementsManager
    void    addObserver(SegmentObserver *obs) { m_observers.insert(obs); }

    /// For use by SegmentObserver objects like Composition & ViewElementsManager
    void removeObserver(SegmentObserver *obs) { m_observers.erase (obs); }


    //////
    //
    // REFRESH STATUS

    // delegate part of the RefreshStatusArray API

    unsigned int getNewRefreshStatusId() {
	return m_refreshStatusArray.getNewRefreshStatusId();
    }

    SegmentRefreshStatus &getRefreshStatus(unsigned int id) {
	return m_refreshStatusArray.getRefreshStatus(id);
    }

    void updateRefreshStatuses(timeT startTime, timeT endTime);

private:
    Composition *m_composition; // owns me, if it exists

    timeT  m_startTime;
    timeT *m_endMarkerTime;     // points to end time, or null if none
    timeT  m_endTime;

    void updateEndTime();       // called after erase of item at end

    TrackId m_track;
    SegmentType m_type;         // identifies Segment type
    std::string m_label;        // segment label

    mutable int m_id; // not id of Segment, but a value for return by getNextId

    unsigned int m_audioFileId; // audio file ID (see AudioFileManager)
    RealTime m_audioStartTime;   // start time relative to start of audio file
    RealTime m_audioEndTime;     // end time relative to start of audio file

    bool m_repeating;           // is this segment repeating?

    BasicQuantizer *const m_quantizer;
    bool m_quantize;

    int m_transpose;            // all Events tranpose
    timeT m_delay;              // all Events delay
    RealTime m_realTimeDelay;   // all Events delay (the delays are cumulative)

    RefreshStatusArray<SegmentRefreshStatus> m_refreshStatusArray;

private: // stuff to support SegmentObservers

    typedef std::set<SegmentObserver *> ObserverSet;
    ObserverSet m_observers;

    void notifyAdd(Event *) const;
    void notifyRemove(Event *) const;
    void notifyEndMarkerChange(bool shorten) const;

private: // assignment operator not provided

    Segment &operator=(const Segment &);

};


class SegmentObserver
{
public:
    /**
     * Called after the event has been added to the segment
     */
    virtual void eventAdded(const Segment *, Event *) = 0;

    /**
     * Called after the event has been removed from the segment,
     * and just before it is deleted
     */
    virtual void eventRemoved(const Segment *, Event *) = 0;

    /**
     * Called after the segment's end marker time has been
     * changed
     */
    virtual void endMarkerTimeChanged(const Segment *, bool shorten) = 0;
};



// an abstract base

class SegmentHelper
{
protected:
    SegmentHelper(Segment &t) : m_segment(t) { }
    virtual ~SegmentHelper();

    typedef Segment::iterator iterator;

    Segment &segment() { return m_segment; }

    Segment::iterator begin() { return segment().begin(); }
    Segment::iterator end()   { return segment().end();   }

    bool isBeforeEndMarker(Segment::iterator i) {
	return segment().isBeforeEndMarker(i);
    }

    Segment::iterator insert(Event *e) { return segment().insert(e); }
    void erase(Segment::iterator i)    { segment().erase(i); }

private:
    Segment &m_segment;
};

}


#endif
