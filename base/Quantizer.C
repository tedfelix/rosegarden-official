
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

#include "Quantizer.h"

namespace Rosegarden {

const std::string Quantizer::DurationProperty = "QuantizedDuration";
const std::string Quantizer::NoteDurationProperty = "QuantizedNoteDuration";

void
Quantizer::quantizeByUnit(Track::iterator from,
                          Track::iterator to,
                          int unit) 
{
    for ( ; from != to; ++from) quantizeByUnit(*from, unit);
}


void
Quantizer::quantizeByUnit(Event *e, int unit)
{
    timeT duration = e->getDuration();

    if (duration != 0) {
        timeT low = (duration / unit) * unit;
        timeT high = low + unit;
        if (high - duration > duration - low) duration = low;
        else duration = high;
    }

    e->setMaybe<Int>(DurationProperty, duration);
}


void
Quantizer::quantizeByNote(Track::iterator from,
                          Track::iterator to,
                          int maxDots) 
{
    for ( ; from != to; ++from) quantizeByNote(*from, maxDots);
}


void
Quantizer::quantizeByNote(Event *e, int maxDots)
{
    quantizeByUnit(e, Note(Note::Shortest).getDuration());

    timeT duration = e->getDuration();
    if (duration == 0) return;

    timeT low, high;
    Note lowNote(Note::Shortest, false), highNote(lowNote);

    quantizeByNote(duration, maxDots, low, lowNote, high, highNote);

    Note &note(lowNote);

    if (high - duration > duration - low) {
        duration = low;
    } else {
        duration = high;
        note = highNote;
    }

    e->setMaybe<Int>(NoteDurationProperty, duration);
    e->setMaybe<Int>(Note::NoteType, note.getNoteType());
    e->setMaybe<Int>(Note::NoteDots, note.getDots());
}


void
Quantizer::quantizeByNote(timeT duration, int maxDots,
                          timeT &low,  Note &lowNote,
                          timeT &high, Note &highNote)
{
    lowNote = Note::getNearestNote(duration, maxDots);

    low  = lowNote.getDuration();
    high = 1000000; //!!!

    try {
	if (lowNote.getDots() > 0 ||
            lowNote.getNoteType() == Note::Shortest) { // can't dot that

	    highNote = Note(lowNote.getNoteType() + 1, 0);

	    if (highNote.getDuration() > duration)
                high = highNote.getDuration();

	} else {

	    highNote = Note(lowNote.getNoteType(), 1);

	    if (highNote.getDuration() > duration)
                high = highNote.getDuration();
	}

    } catch (Note::BadType) {
        // lowNote is already the longest there is
    }
    
    return;
}


}
