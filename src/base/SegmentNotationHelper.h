/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENT_NOTATION_HELPER_H
#define RG_SEGMENT_NOTATION_HELPER_H

#include "base/Segment.h"

namespace Rosegarden 
{

class SegmentNotationHelper : protected SegmentHelper
{
public:
    SegmentNotationHelper(Segment &t) : SegmentHelper(t) { }
    ~SegmentNotationHelper() override;

    using SegmentHelper::segment;

    /**
     * Set the NOTE_TYPE and NOTE_DOTS properties on the events
     * in the segment.  If startTime and endTime are equal, operates
     * on the whole segment.
     */
    void setNotationProperties(timeT startTime = 0, timeT endTime = 0);
    
    /**
     * Return the notation absolute time plus the notation duration.
     */
    timeT getNotationEndTime(Event *e);

    /**
     * Return an iterator pointing at the first event in the segment
     * to have an absolute time of t or later.  (Most of the time, the
     * non-notation absolute times should be used as reference
     * timings; this and the next function are provided for
     * completeness, but in most cases if you're about to call them
     * you should ask yourself why.)
     */
    iterator findNotationAbsoluteTime(timeT t);

    /**
     * Return an iterator pointing at the last event in the segment
     * to have an absolute time of t or earlier.  (Most of the time,
     * the non-notation absolute times should be used as reference
     * timings; this and the previous function are provided for
     * completeness, but in most cases if you're about to call them
     * you should ask yourself why.)
     */
    iterator findNearestNotationAbsoluteTime(timeT t);

    
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
    iterator findContiguousNext(iterator);

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
    iterator findContiguousPrevious(iterator);

    /**
     * Returns true if the iterator points at a note in a chord
     * e.g. if there are more notes at the same absolute time
     */
    bool noteIsInChord(Event *note);

    /**
     * Returns an iterator pointing to the note that this one is tied
     * with, in the forward direction if goForwards or back otherwise.
     * Returns end() if none.
     *
     * Untested and probably marked-for-expiry -- prefer
     * SegmentPerformanceHelper::getTiedNotes
     */
    iterator getNoteTiedWith(Event *note, bool goForwards);


    /**
     * Checks whether it's reasonable to split a single event
     * of duration a+b into two events of durations a and b, for some
     * working definition of "reasonable".
     *
     * You should pass note-quantized durations into this method
     */
    bool isSplitValid(timeT a, timeT b);


    /**
     * Splits events in the [from, to[ interval into 
     * tied events of duration baseDuration + events of duration R,
     * with R being equal to the events' initial duration minus baseDuration
     *
     * The events in [from, to[ must all be at the same absolute time
     * 
     * Does not check "reasonableness" of expansion first
     *
     * Events may be notes or rests (rests will obviously not be tied)
     *
     * @return iterator pointing at the last inserted event.  Also
     * modifies from to point at the first split event (the original
     * iterator would have been invalidated).
     */
    iterator splitIntoTie(iterator &from, iterator to, timeT baseDuration);


    /**
     * Splits (splits) events in the same timeslice as that pointed
     * to by i into tied events of duration baseDuration + events of
     * duration R, with R being equal to the events' initial duration
     * minus baseDuration
     *
     * Does not check "reasonableness" of expansion first
     *
     * Events may be notes or rests (rests will obviously not be tied)
     *
     * @return iterator pointing at the last inserted event.  Also
     * modifies i to point at the first split event (the original
     * iterator would have been invalidated).
     */
    iterator splitIntoTie(iterator &i, timeT baseDuration);


    /**
     * Returns true if Events of durations a and b can reasonably be
     * collapsed into a single one of duration a+b, for some
     * definition of "reasonably".  For use by collapseRestsIfValid
     *
     * You should pass note-quantized durations into this method
     */
    bool isCollapseValid(timeT a, timeT b);

    /**
     * If possible, collapses the rest event with the following or
     * previous one.
     *
     * @return true if collapse was done, false if it wasn't reasonable
     *
     * collapseForward is set to true if the collapse was with the
     * following element, false if it was with the previous one
     */
    bool collapseRestsIfValid(Event*, bool& collapseForward);

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
     * Inserts a note, doing all the clever split/merge stuff as
     * appropriate.  Requires segment to be in a composition.  Returns
     * iterator pointing to last event inserted (there may be more
     * than one, as note may have had to be split)
     *
     * This method will only work correctly if there is a note or
     * rest event already starting at the model event's absoluteTime.
     *
     * Passing a model event has the advantage over the previous
     * method of allowing additional properties to be supplied.  The
     * model event will be copied but not itself used; the caller
     * continues to own it and should release it after return.
     */
    iterator insertNote(Event *modelEvent);

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
     * Insert a symbol.
     * Returns iterator pointing to symbol.
     */
    iterator insertSymbol(timeT absoluteTime, Symbol symbol);

    /**
     * Insert a key.
     * Returns iterator pointing to key.
     */
    iterator insertKey(timeT absoluteTime, Key key);

    /**
     * Insert a text event.
     * Returns iterator pointing to text event.
     */
    iterator insertText(timeT absoluteTime, Text text);

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
    bool deleteEvent(Event *e, bool collapseRest = false);

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
     * Split a note or rest up into tied notes or shorter rests of
     * viable lengths (longest possible viable duration first, then
     * longest possible viable component of remainder &c).  Also
     * optionally split a note or rest at barlines -- this is
     * actually the most common user-visible use of this function.
     *
     * Note: no checks performed on validity of noteItr, use caution.
     *
     * Returns the original *noteItr Event if no split and tie occurs
     * or returns the first new event inserted if split and tie occurs
     */
    Event * makeThisNoteViable(iterator noteItr, bool splitAtBars = true);

    /**
     * Split notes and rests up into tied notes or shorter rests of
     * viable lengths (longest possible viable duration first, then
     * longest possible viable component of remainder &c).  Also
     * optionally splits notes and rests at barlines -- this is
     * actually the most common user-visible use of this function.
     */
    void makeNotesViable(iterator i, iterator j, bool splitAtBars = true);


    /**
     * As above but given a range in time rather than iterators.
     */
    void makeNotesViable(timeT startTime, timeT endTime,
                         bool splitAtBars = true);


    /**
     * Give all events between the start of the timeslice containing
     * from and the start of the timeslice containing to the same new
     * group id and the given type.
     *
     * Do not use this for making tuplet groups, unless the events
     * in the group already have the other tuplet properties or you
     * intend to add those yourself.  Use makeTupletGroup instead.
     */
    void makeBeamedGroup(timeT from, timeT to, std::string type);

    /**
     * Give all events between the start of the timeslice containing
     * from and the start of the timeslice containing to the same new
     * group id and the given type.
     *
     * Do not use this for making tuplet groups, unless the events
     * in the group already have the other tuplet properties or you
     * intend to add those yourself.  Use makeTupletGroup instead.
     */
    void makeBeamedGroup(iterator from, iterator to, std::string type);

    /**
     * Give all events between from and to the same new group id and
     * the given type.
     *
     * Use makeBeamedGroup for normal notes.  This function is usually
     * used for groups of grace notes, which are equal in time and
     * distinguished by subordering.
     *
     * Do not use this for making tuplet groups, unless the events
     * in the group already have the other tuplet properties or you
     * intend to add those yourself.
     */
    void makeBeamedGroupExact(iterator from, iterator to, std::string type);


    /**
     * Make a beamed group of tuplet type, whose tuplet properties are
     * specified as "(untupled-count) notes of duration (unit) played
     * in the time of (tupled-count)".  For example, a quaver triplet 
     * group could be specified with untupled = 3, tupled = 2, unit =
     * (the duration of a quaver).
     *
     * The group will start at the beginning of the timeslice containing
     * the time t, and will be constructed by compressing the appropriate
     * number of following notes into the tuplet time, and filling the
     * space that this compression left behind (after the group) with
     * rests.  The results may be unexpected if overlapping events are
     * present.
     */
    void makeTupletGroup(timeT t, int untupled, int tupled, timeT unit);


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
     * Removes all rests starting at \a time for \a duration,
     * splitting the last rest if needed.
     *
     * Modifies duration to the actual duration of the series
     * of rests that has been changed by this action (i.e. if
     * the last rest was split, duration will be extended to
     * include the second half of this rest).  This is intended
     * to be of use when calculating the extents of a command
     * for undo/refresh purposes.
     *
     * If there's an event which is not a rest in this interval,
     * returns false and sets duration to the maximum duration
     * that would have succeeded.
     *
     * If testOnly is true, does not actually remove any rests;
     * just checks whether the rests can be removed and sets
     * duration and the return value appropriately.
     *
     * (Used for Event pasting.)
     */ 
    bool removeRests(timeT time, timeT &duration, bool testOnly = false);


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
     * start time is before the the given limit and both notes behave
     * the same way in ornament expansion.  Does not care whether the
     * resulting note is viable.
     *
     * Returns an iterator pointing to the event that replaced the
     * original one if a collapse happened, segment.end() if no
     * collapse or event not found
     */
    iterator collapseNoteAggressively(Event *, timeT rangeEnd);


    
    std::pair<Event *, Event *> splitPreservingPerformanceTimes(Event *e,
                                                                timeT q1);

    /**
     * Look for examples of overlapping notes within the given range,
     * and split each into chords with some tied notes.
     */
    void deCounterpoint(timeT startTime, timeT endTime);

    /**
     * A rather specialised function: Add a slur to every beamed group.
     * If legatoOnly is true, add a slur only to those beamed groups
     * in which every note except the last has a tenuto mark already
     * (and remove that mark).
     * This is basically intended as a post-notation-quantization-auto-
     * beam step.
     */
    void autoSlur(timeT startTime, timeT endTime, bool legatoOnly);

    
protected:
    const Quantizer &basicQuantizer();
    const Quantizer &notationQuantizer();

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
    iterator insertSomething(iterator position, int duration,
                             Event *modelEvent, bool tiedBack);

    /// for use by insertSomething
    iterator insertSingleSomething(iterator position, int duration,
                                   Event *modelEvent, bool tiedBack);

    /// for use by insertSingleSomething
    void setInsertedNoteGroup(Event *e, iterator i);

    /// for use by makeBeamedGroup
    void makeBeamedGroupAux(iterator from, iterator to, std::string type,
			    bool groupGraces);

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

    /// find border of tupled
    int findBorderTuplet(iterator, iterator &start, iterator &end);
};

}

#endif
