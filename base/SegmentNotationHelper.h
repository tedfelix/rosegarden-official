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

#ifndef _TRACK_NOTATION_HELPER_H_
#define _TRACK_NOTATION_HELPER_H_

#include "Track.h"

namespace Rosegarden 
{

class TrackNotationHelper : protected TrackHelper
{
public:
    TrackNotationHelper(Track &t) : TrackHelper(t) { }
    virtual ~TrackNotationHelper();


    // Note that these should be non-persistent properties, because
    // the code that writes out XML converts them to
    // <group>...</group> instead of writing them as explicit
    // properties of the events:
    
    static const PropertyName BeamedGroupIdPropertyName;
    static const PropertyName BeamedGroupTypePropertyName;
    static const PropertyName BeamedGroupTupledLengthPropertyName;
    static const PropertyName BeamedGroupTupledCountPropertyName;
    static const PropertyName BeamedGroupUntupledLengthPropertyName;


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
     */

    //!!! maybe needs a more specific name -- doesn't always collapse,
    //only if the collapsed notes make a single note of meaningful
    //duration

    bool collapse(Event*, bool& collapseForward);

    /**
     * Inserts a note, doing all the clever split/merge stuff as
     * appropriate.  Requires up-to-date bar position list.
     *
     * This method will only work correctly if there is a note or
     * rest event already starting at absoluteTime.
     */
    void insertNote(timeT absoluteTime, Note note, int pitch,
                    Accidental explicitAccidental);

    /**
     * Inserts a rest, doing all the clever split/merge stuff as
     * appropriate.  Requires up-to-date bar position list.
     *
     * This method will only work correctly if there is a note or
     * rest event already starting at absoluteTime.
     */
    void insertRest(timeT absoluteTime, Note note);

    /**
     * Insert a clef
     */
    void insertClef(timeT absoluteTime, Clef clef);

    /**
     * Deletes a note, doing all the clever split/merge stuff as
     * appropriate.  Requires up-to-date bar position list.
     */
    void deleteNote(Event *e, bool collapseRest = false);

    /**
     * Deletes a rest, doing all the clever split/merge stuff as
     * appropriate.  Requires up-to-date bar position list.
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
     * will be acceptable.  The default is whatever the track's
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
     * will be acceptable.  The default is whatever the track's
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
    void makeBeamedGroup(iterator from, iterator to, std::string type);


    /**
     * Divide the notes in the interval [from, to[ up into sensible
     * beamed groups and give each group the right group properties
     * using makeBeamedGroup.  Requires up-to-date bar position list.
     */
    void autoBeam(iterator from, iterator to, std::string type);


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
     * Note-quantize the Track and set the NoteType and NoteDots
     * properties on note and rest events.  Only works when the
     * Track is in a Composition.
     */
    void quantize();

    
private:

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
    void insertSomething(iterator position, int duration, int pitch,
			 bool isRest, bool tiedBack, Accidental explicitAcc);

    /// for use by insertSomething
    iterator insertSingleSomething(iterator position, int duration, int pitch,
				   bool isRest, bool tiedBack, Accidental);

    /// for use by insertSingleSomething
    void setInsertedNoteGroup(Event *e, iterator i);

    /// for use by autoBeam

    void autoBeamBar(iterator from, iterator to, TimeSignature timesig,
                     std::string type);

    void autoBeamBar(iterator from, iterator to, timeT average,
                     timeT minimum, timeT maximum, std::string type);

    /// used by autoBeamAux (duplicate of private method in Track)
    bool hasEffectiveDuration(iterator i);
};

}

#endif
