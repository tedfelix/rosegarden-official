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

#include "SegmentMatrixHelper.h"

namespace Rosegarden 
{

Segment::iterator SegmentMatrixHelper::insertNote(Event* e)
{
    iterator i, j;

    segment().getTimeSlice(e->getAbsoluteTime(), i, j);

    timeT duration = e->getDuration();

    while (i != end() && (*i)->getDuration() == 0) ++i;

    if (i == end()) {
	return insertSingleSomething(i, e);
    }

    // If there's a rest at the insertion position, merge it with any
    // following rests, if available, until we have at least the
    // duration of the new note.
    collapseRestsForInsert(i, duration);

    timeT existingDuration = (*i)->getDuration();

    if (duration == existingDuration) {

        // 1. If the new note or rest is the same length as an
        // existing note or rest at that position, chord the existing
        // note or delete the existing rest and insert.

	cerr << "Durations match; doing simple insert" << endl;

    } else if (duration < existingDuration) {

        cerr << "Found rest, splitting" << endl;
        iterator last = expandIntoTie(i, duration);

        // Recover viability for the second half of any split rest

        if (last != end() && !isViable(*last, 1)) {
            makeRestViable(last);
        }

    } else if (duration > existingDuration) {
        // Do nothing - should never happen

    }
    
    return insertSingleSomething(i, e);
    
}


Segment::iterator
SegmentMatrixHelper::insertSingleSomething(iterator i, Event* e)
{
    timeT time;
    bool eraseI = false;

    if (i == end()) {
	time = segment().getDuration();
    } else {
	time = (*i)->getAbsoluteTime();
	if ((*i)->isa(Note::EventRestType)) eraseI = true;
    }

//     e->setAbsoluteTime(time);
//     e->setDuration(duration);

//     if (!isRest) {
//         e->set<Int>(PITCH, pitch);
//         if (acc != Accidentals::NoAccidental) {
//             e->set<String>(ACCIDENTAL, acc);
//         }
//         setInsertedNoteGroup(e, i);
//     }

    if (eraseI) erase(i);

    return insert(e);
}


}
