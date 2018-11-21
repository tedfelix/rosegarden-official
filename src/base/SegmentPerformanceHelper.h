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

#ifndef RG_SEGMENT_PERFORMANCE_HELPER_H
#define RG_SEGMENT_PERFORMANCE_HELPER_H

#include "base/Segment.h"
#include "Composition.h" // for RealTime

namespace Rosegarden 
{

class SegmentPerformanceHelper : protected SegmentHelper
{
public:
    SegmentPerformanceHelper(Segment &t) : SegmentHelper(t) { }
    ~SegmentPerformanceHelper() override;

    typedef std::vector<iterator> iteratorcontainer;

    /**
     * Returns a sequence of iterators pointing to the note events
     * that are tied with the given event.  If the given event is not
     * a note event or is not tied, its iterator will be the only one
     * in the sequence.  If the given event is tied but is not the
     * first in the tied chain, the returned sequence will be empty.
     */
    iteratorcontainer getTiedNotes(iterator i);

    /**
     * Returns two sequences of iterators pointing to the note events
     * that are grace notes, or host notes for grace notes, associated
     * with the given event, which is itself either a grace note or a
     * host note for a grace note.  The grace note iterators are
     * returned in the graceNotes sequence, and the host note
     * iterators in hostNotes.  isHostNote is set to true if the
     * given event is a host note, false otherwise.
     * 
     * If the given event is not a grace note, is a grace note with no
     * host note, or is a potential host note without any grace notes,
     * the sequences will both be empty and the function will return
     * false.
     */
    bool getGraceAndHostNotes(iterator i,
			      iteratorcontainer &graceNotes,
			      iteratorcontainer &hostNotes,
			      bool &isHostNote);

    /**
     * Returns the absolute time of the note event pointed to by i.
     */
    timeT getSoundingAbsoluteTime(iterator i);

    /**
     * Returns the duration of the note event pointed to by i, taking
     * into account any ties the note may have etc.
     * 
     * If the note is the first of two or more tied notes, this will
     * return the accumulated duration of the whole series of notes
     * it's tied to.
     * 
     * If the note is in a tied series but is not the first, this will
     * return zero, because the note's duration is presumed to have
     * been accounted for by a previous call to this method when
     * examining the first note in the tied series.
     * 
     * If the note is not tied, or if i does not point to a note
     * event, this will just return the duration of the event at i.
     *
     * This method may return an incorrect duration for any note
     * event that is tied but lacks a pitch property.  This is
     * expected behaviour; don't create tied notes without pitches.
     */
    timeT getSoundingDuration(iterator i);

    /**
     * Returns the absolute time of the event pointed to by i,
     * in microseconds elapsed since the start of the Composition.
     * This method exploits the Composition's getElapsedRealTime
     * method to take into account any tempo changes that appear
     * in the section of the composition preceding i.
     */
    RealTime getRealAbsoluteTime(iterator i);

    /**
     * Returns the duration of the note event pointed to by i,
     * in microseconds.  This takes into account the tempo in
     * force at i's position within the composition, as well as
     * any tempo changes occurring during the event at i.
     */
    RealTime getRealSoundingDuration(iterator i);

    /**
     * Return a sounding duration (estimated) and start time for the
     * note event pointed to by i.  If host is true, i is expected to
     * be the "host" note for one or more grace notes; if host is
     * false, i is expected to point to a grace note.  If the relevant
     * expectation is not met, this function returns false.  Otherwise
     * the sounding time and duration are returned through t and d and
     * the function returns true.
     */
    bool getGraceNoteTimeAndDuration(bool host, iterator i, timeT &t, timeT &d);
};

}

#endif
