
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


timeT
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
    return duration;
}


void
Quantizer::quantizeByNote(Track::iterator from,
                          Track::iterator to,
                          int maxDots) 
{
    for ( ; from != to; ++from) quantizeByNote(*from, maxDots);
}


timeT
Quantizer::quantizeByNote(Event *e, int maxDots)
{
    timeT duration = quantizeByUnit(e, Note(Note::Shortest).getDuration());
    if (duration == 0 && e->getDuration() == 0) return 0;

    Note note(quantizeByNote(duration, maxDots));

    e->setMaybe<Int>(NoteDurationProperty, duration);
    e->setMaybe<Int>(Note::NoteType, note.getNoteType());
    e->setMaybe<Int>(Note::NoteDots, note.getDots());

    return duration;
}


Note
Quantizer::quantizeByNote(timeT &duration, int maxDots)
{
    Note  shortNote = Note::getNearestNote(duration, maxDots);
    timeT shortTime = shortNote.getDuration();
    if   (shortTime == duration) return shortNote;

    Note  longNote(shortNote);

    if ((shortNote.getDots() > 0 ||
         shortNote.getNoteType() == Note::Shortest)) { // can't dot that

        if (shortNote.getNoteType() == Note::Longest) {
            duration = shortTime;
            return shortNote;
        }

        longNote = Note(shortNote.getNoteType() + 1, 0);
    } else {
        longNote = Note(shortNote.getNoteType(), 1);
    }

    timeT longTime = longNote.getDuration();
    if (longTime - duration < duration - shortTime) {
        duration = longTime;
        return longNote;
    }

    duration = shortTime;
    return shortNote;
}


}
