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

#ifndef _SEGMENT_NOTATION_HELPER_H_
#define _SEGMENT_NOTATION_HELPER_H_

#include "Segment.h"

namespace Rosegarden 
{

class SegmentNotationHelper : protected SegmentHelper
{
public:
    SegmentNotationHelper(Segment &t) : SegmentHelper(t) { }
    virtual ~SegmentNotationHelper();

    SegmentHelper::segment;

    
    /**
     * Looks for another note immediately following the one pointed to
     * by the given iterator, and (if matchPitch is true) of the same
     * pitch, and returns an iterator pointing to that note.  Returns
     * end() if there is no such note.
     * 
     * The notes are considered "adjacent" if the quantized start
     * time of one matches the quantized end time of the other, unless
     * allowOverlap is true in which case overlapping notes are also
     * considered adjacent so long as one does not completely enclose
     * the other.
     */
    iterator getNextAdjacentNote(iterator i,
				 bool matchPitch = true,
				 bool allowOverlap = true);


    /**
     * Looks for another note immediately preceding the one pointed to
     * by the given iterator, and (if matchPitch is true) of the same
     * pitch, and returns an iterator pointing to that note.  Returns
     * end() if there is no such note.
     *
     * rangeStart gives a bound to the distance that will be scanned
     * to find events -- no event with starting time earlier than that
     * will be considered.  (This method has no other way to know when
     * to stop scanning; potentially the very first note in the segment
     * could turn out to be adjacent to the very last one.)
     * 
     * The notes are considered "adjacent" if the quantized start
     * time of one matches the quantized end time of the other, unless
     * allowOverlap is true in which case overlapping notes are also
     * considered adjacent so long as one does not completely enclose
     * the other.
     */
    iterator getPreviousAdjacentNote(iterator i,
				     timeT rangeStart = 0,
				     bool matchPitch = true,
				     bool allowOverlap = true);


    /**
     * Checks whether it's reasonable to expand (split) a single event
     * of duration a+b into two events of durations a and b, for some
     * working definition of "reasonable".
     *
     * You should pass note-quantized durations into this method
     */
    bool isExpandValid(timeT a, timeT b);


    /**
     * Expands (splits) events in the [from, to[ interval into 
     * tied events of duration baseDuration + events of duration R,
     * with R being equal to the events' initial duration minus baseDuration
     *
     * The events in [from, to[ must all be at the same absolute time
     * 
     * Does not check "reasonableness" of expansion first
     *
     * @return iterator pointing at the last inserted event
     */
    iterator expandIntoTie(iterator from, iterator to, timeT baseDuration);


    /**
     * Expands (splits) events in the same timeslice as that pointed
     * to by i into tied events of duration baseDuration + events of
     * duration R, with R being equal to the events' initial duration
     * minus baseDuration
     *
     * Does not check "reasonableness" of expansion first
     *
     * @return iterator pointing at the last inserted event
     */
    iterator expandIntoTie(iterator i, timeT baseDuration);


    /**
     * Returns true if Events of durations a and b can reasonably be
     * collapsed into a single one of duration a+b, for some
     * definition of "reasonably".  For use by collapseIfValid
     *
     * You should pass note-quantized durations into this method
     */
    bool isCollapseValid(timeT a, timeT b);

    /**
     * If possible, collapses the event with the following or previous
     * one.
     *
     * @return true if collapse was done, false if it wasn't reasonable
     *
     * collapseForward is set to true if the collapse was with the
     * following element, false if it was with the previous one
     */
    bool collapseIfValid(Event*, bool& collapseForward);

    /**
     * Inserts a note, doing all the clever split/merge stuff as
     * appropriate.  Requires segment to be in a composition.  Returns
     * iterator pointing to last event inserted (there may be more
     * than one, as note may have had to be split)
     *
     * This method will only work correctly if there is a note or
     * rest event already starting at absoluteTime.
     */
    iterator insertNote(timeT absoluteTime, Note note, int pitch,
			Accidental explicitAccidental);

    /**
     * Inserts a rest, doing all the clever split/merge stuff as
     * appropriate.  Requires segment to be in a composition.  
     * Returns iterator pointing to last event inserted (there
     * may be more than one, as rest may have had to be split)
     *
     * This method will only work correctly if there is a note or
     * rest event already starting at absoluteTime.
     */
    iterator insertRest(timeT absoluteTime, Note note);

    /**
     * Insert a clef.
     * Returns iterator pointing to clef.
     */
    iterator insertClef(timeT absoluteTime, Clef clef);

    /**
     * Deletes a note, doing all the clever split/merge stuff as
     * appropriate.  Requires segment to be in a composition.  
     */
    void deleteNote(Event *e, bool collapseRest = false);

    /**
     * Deletes a rest, doing all the clever split/merge stuff as
     * appropriate.  Requires segment to be in a composition.  
     *
     * @return whether the rest could be deleted -- a rest can only
     * be deleted if there's a suitable rest next to it to merge it
     * with.
     */
    bool deleteRest(Event *e);

    /**
     * Deletes an event. If the event is a note or a rest, calls
     * deleteNote or deleteRest.
     *
     * @return whether the event was deleted (always true, unless the
     * event is a rest).
     *
     * @see deleteRest, deleteNote
     */
    bool deleteEvent(Event *e);

    /**
     * Check whether a note or rest event has a duration that can be
     * represented by a single note-type.  (If not, the code that's
     * doing the check might wish to split the event.)
     *
     * If dots is specified, a true value will only be returned if the
     * best-fit note has no more than that number of dots.  e.g. if
     * dots = 0, only notes that are viable without the use of dots
     * will be acceptable.  The default is whatever the segment's
     * quantizer considers acceptable (probably either 1 or 2 dots).
     */
    bool isViable(Event *e, int dots = -1) {
        return isViable(e->getDuration(), dots);
    }

    /**
     * Check whether a duration can be represented by a single
     * note-type.  (If not, the code that's doing the check might wish
     * to split the duration.)
     *
     * If dots is specified, a true value will only be returned if the
     * best-fit note has no more than that number of dots.  e.g. if
     * dots = 0, only notes that are viable without the use of dots
     * will be acceptable.  The default is whatever the segment's
     * quantizer considers acceptable (probably either 1 or 2 dots).
     */
    bool isViable(timeT duration, int dots = -1);

    
    /**
     * Given an iterator pointing to a rest, split that rest up
     * according to the durations returned by TimeSignature's
     * getDurationListForInterval     
     */
    void makeRestViable(iterator i);

    
    /**
     * Given an iterator pointing to a note, split that note up into
     * tied notes of viable lengths (longest possible viable duration
     * first, then longest possible viable component of remainder &c).
     *
     * Only splits a single note; unlike expandIntoTie, this does
     * not by default split all notes at a given timeslice.
     */
    void makeNoteViable(iterator i);


    /**
     * Give all events between the start of the timeslice containing
     * from and the start of the timeslice containing to the same new
     * group id and the given type
     */
    void makeBeamedGroup(timeT from, timeT to, std::string type);

    /**
     * Give all events between the start of the timeslice containing
     * from and the start of the timeslice containing to the same new
     * group id and the given type
     */
    void makeBeamedGroup(iterator from, iterator to, std::string type);


    /**
     * Divide the notes between the start of the bar containing
     * from and the end of the bar containing to up into sensible
     * beamed groups and give each group the right group properties
     * using makeBeamedGroup.  Requires segment to be in a composition.
     */
    void autoBeam(timeT from, timeT to, std::string type);

    /**
     * Divide the notes between the start of the bar containing
     * from and the end of the bar containing to up into sensible
     * beamed groups and give each group the right group properties
     * using makeBeamedGroup.  Requires segment to be in a composition.
     */
    void autoBeam(iterator from, iterator to, std::string type);


    /**
     * Clear the group id and group type from all events between the
     * start of the timeslice containing from and the start of the
     * timeslice containing to
     */
    void unbeam(timeT from, timeT to);

    /**
     * Clear the group id and group type from all events between the
     * start of the timeslice containing from and the start of the
     * timeslice containing to
     */
    void unbeam(iterator from, iterator to);


    /**
     * Guess which clef a section of music is supposed to be in,
     * ignoring any clef events actually found in the section.
     */
    Clef guessClef(iterator from, iterator to);
    

    /**
     * Remove all rests starting at \a time for \a duration,
     * splitting the last rest if needed.
     *
     * If there's an event which is not a rest in this interval,
     * return false.
     *
     * Used for Event pasting.
     */ 
    bool removeRests(timeT time, timeT duration);


    /**
     * For each series of contiguous rests found between the start and
     * end time, replace the series of rests with another series of
     * the same duration but composed of the theoretically "correct"
     * rest durations to fill the gap in the current time signature.
     */
    void normalizeRests(timeT startTime, timeT endTime);


    /**
     * For each series of contiguous rests found between the start and
     * end time, replace the series of rests with another series of
     * the same duration but composed of the longest possible valid
     * rest plus the remainder
     */
    void collapseRestsAggressively(timeT startTime, timeT endTime);


    /**
     * Locate the given event and, if it's a note, collapse it with
     * any following adjacent note of the same pitch, so long as its
     * start time is before the the given limit.  Does not care
     * whether the resulting note is viable.
     *
     * Returns true if a collapse happened, false if no collapse
     * or event not found
     */
    bool collapseNoteAggressively(Event *, timeT rangeEnd);


    /**
     * Note-quantize the Segment and set the NoteType and NoteDots
     * properties on note and rest events.  Only works when the
     * Segment is in a Composition.
     */
    void quantize();

    
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


    /// for use by insertNote and insertRest
    iterator insertSomething(iterator position, int duration, int pitch,
			     bool isRest, bool tiedBack,
			     Accidental explicitAcc);

    /// for use by insertSomething
    iterator insertSingleSomething(iterator position, int duration, int pitch,
				   bool isRest, bool tiedBack, Accidental);

    /// for use by insertSingleSomething
    void setInsertedNoteGroup(Event *e, iterator i);

    /// for use by makeBeamedGroup
    void makeBeamedGroupAux(iterator from, iterator to, std::string type);

    /// for use by unbeam
    void unbeamAux(iterator from, iterator to);

    /// for use by autoBeam

    void autoBeamBar(iterator from, iterator to, TimeSignature timesig,
                     std::string type);

    void autoBeamBar(iterator from, iterator to, timeT average,
                     timeT minimum, timeT maximum, std::string type);

    /// used by autoBeamAux (duplicate of private method in Segment)
    bool hasEffectiveDuration(iterator i);

    typedef void (SegmentNotationHelper::*Reorganizer)(timeT, timeT,
						       std::vector<Event *>&);

    void reorganizeRests(timeT, timeT, Reorganizer);

    /// for use by normalizeRests
    void normalizeContiguousRests(timeT, timeT, std::vector<Event *>&);

    /// for use by collapseRestsAggressively
    void mergeContiguousRests(timeT, timeT, std::vector<Event *>&);
};

}

#endif
