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

#ifndef _SEGMENT_PERFORMANCE_HELPER_H_
#define _SEGMENT_PERFORMANCE_HELPER_H_

#include "Segment.h"
#include "Composition.h" // for RealTime

namespace Rosegarden 
{

class SegmentPerformanceHelper : protected SegmentHelper
{
public:
    SegmentPerformanceHelper(Segment &t) : SegmentHelper(t) { }
    virtual ~SegmentPerformanceHelper();

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
     * Returns the duration of the note event pointed to by i, taking
     * into account any ties the note may have.
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

private:
    timeT getDurationWithTupling(Event *e);
};

}

#endif
