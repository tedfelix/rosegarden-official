
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
Quantizer::quantizeByUnit(Track::iterator from, Track::iterator to) const
{
    for ( ; from != to; ++from) quantizeByUnit(*from);
}


timeT
Quantizer::quantizeByUnit(Event *e) const
{
    timeT duration = quantizeByUnit(e->getDuration());
    e->setMaybe<Int>(DurationProperty, duration);
    return duration;
}


timeT
Quantizer::quantizeByUnit(timeT duration) const
{
    if (duration != 0) {
        timeT low = (duration / m_unit) * m_unit;
        timeT high = low + m_unit;
        if (high - duration > duration - low) duration = low;
        else duration = high;
    }
    return duration;
}


timeT
Quantizer::getUnitQuantizedDuration(Event *e) const
{
    long d;
    if (e->get<Int>(DurationProperty, d)) return (timeT)d;
    else return quantizeByUnit(e);
}


void
Quantizer::quantizeByNote(Track::iterator from, Track::iterator to) const
{
    for ( ; from != to; ++from) quantizeByNote(*from);
}


timeT
Quantizer::quantizeByNote(Event *e) const
{
    timeT duration = quantizeByUnit(e);
    if (duration == 0 && e->getDuration() == 0) return 0;

    Note note(requantizeByNote(duration));

    e->setMaybe<Int>(NoteDurationProperty, duration);
    e->setMaybe<Int>(Note::NoteType, note.getNoteType());
    e->setMaybe<Int>(Note::NoteDots, note.getDots());

    return duration;
}


timeT 
Quantizer::quantizeByNote(timeT duration) const
{
    duration = quantizeByUnit(duration);
    (void)requantizeByNote(duration);
    return duration;
}


Note
Quantizer::requantizeByNote(timeT &duration) const
{
    Note  shortNote = Note::getNearestNote(duration, m_maxDots);
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


timeT
Quantizer::getNoteQuantizedDuration(Event *e) const
{
    long d;
    if (e->get<Int>(NoteDurationProperty, d)) return (timeT)d;
    else return quantizeByNote(e);
}


void Quantizer::unquantize(Event *e) const
{
    e->unset(DurationProperty);
    e->unset(NoteDurationProperty);

    // should we do this?  not entirely sure, but probably
    e->unset(Note::NoteType);
    e->unset(Note::NoteDots);
}


}
