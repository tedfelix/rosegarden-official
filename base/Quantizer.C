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

#include "Quantizer.h"

#include <iostream>

namespace Rosegarden {

const PropertyName Quantizer::AbsoluteTimeProperty = "QuantizedAbsoluteTime";
const PropertyName Quantizer::DurationProperty	   = "QuantizedDuration";
const PropertyName Quantizer::NoteDurationProperty = "QuantizedNoteDuration";

Quantizer::SingleQuantizer::~SingleQuantizer() { }
Quantizer::UnitQuantizer::~UnitQuantizer() { }
Quantizer::NoteQuantizer::~NoteQuantizer() { }

timeT Quantizer::UnitQuantizer::quantize(int unit, int, timeT duration) const
{
    if (duration != 0) {
	timeT low = (duration / unit) * unit;
	timeT high = low + unit;
	if (high - duration > duration - low) duration = low;
	else duration = high;
    }

    return duration;
}

timeT Quantizer::NoteQuantizer::quantize(int unit, int maxDots,
					 timeT duration) const
{
    duration = UnitQuantizer().quantize(unit, maxDots, duration);
    Note shortNote = Note::getNearestNote(duration, maxDots);

    timeT shortTime = shortNote.getDuration();
    if   (shortTime == duration) return shortTime;

    Note  longNote(shortNote);

    if ((shortNote.getDots() > 0 ||
         shortNote.getNoteType() == Note::Shortest)) { // can't dot that

        if (shortNote.getNoteType() == Note::Longest) {
            return shortTime;
        }

        longNote = Note(shortNote.getNoteType() + 1, 0);
    } else {
        longNote = Note(shortNote.getNoteType(), 1);
    }

    timeT longTime = longNote.getDuration();
    if (shortTime < unit || 
	(longTime - duration < duration - shortTime)) {
	return longTime;
    } else {
	return shortTime;
    }
};


void
Quantizer::quantize(Track::iterator from, Track::iterator to,
		    const SingleQuantizer &aq, const SingleQuantizer &dq,
		    PropertyName durationProperty) const
{
    timeT excess = 0;

    for ( ; from != to; ++from) {

	timeT absoluteTime   = (*from)->getAbsoluteTime();
	timeT duration	     = (*from)->getDuration();
	timeT qAbsoluteTime  = aq.quantize(m_unit, m_maxDots, absoluteTime);
	timeT qDuration	     = 0;

	if ((*from)->isa(Note::EventType)) {

	    qDuration = dq.quantize(m_unit, m_maxDots, duration);
	    excess = (qAbsoluteTime + qDuration) - (absoluteTime + duration);

	} else if ((*from)->isa(Note::EventRestType)) {

	    if (excess >= duration) {
		
		qDuration = 0;
		excess -= duration;

	    } else {

		if (excess != 0) {
		    qAbsoluteTime += excess;
		    duration	  -= excess;
		}

		qDuration = dq.quantize(m_unit, m_maxDots, duration);
	    }
	}
	//!!! should skip this for non-note/rest events, but can't
	//while we still have the next two methods working the way
	//they do -- really want to drop per-Event quantization
	//altogether, because it's inconsistent

	(*from)->setMaybe<Int>(AbsoluteTimeProperty, qAbsoluteTime);
	(*from)->setMaybe<Int>(durationProperty, qDuration);
    }
}


void
Quantizer::quantizeByUnit(Track::iterator from, Track::iterator to) const
{
    quantize(from, to, UnitQuantizer(), UnitQuantizer(), DurationProperty);
}
/*

timeT
Quantizer::quantizeByUnit(Event *e) const
{
    timeT duration = quantizeByUnit(e->getDuration());
    e->setMaybe<Int>(m_durationProperty, duration);
    return duration;
}
*/

timeT
Quantizer::quantizeByUnit(timeT duration) const
{
    return UnitQuantizer().quantize(m_unit, m_maxDots, duration);
}

timeT
Quantizer::getUnitQuantizedDuration(Event *e) const
{
    long d;
    if (e->get<Int>(DurationProperty, d)) return (timeT)d;
    else return quantizeByUnit(e->getDuration());
}

timeT
Quantizer::getUnitQuantizedAbsoluteTime(Event *e) const
{
    long d;
    if (e->get<Int>(AbsoluteTimeProperty, d)) return (timeT)d;
    else return quantizeByUnit(e->getAbsoluteTime());
}


void
Quantizer::quantizeByNote(Track::iterator from, Track::iterator to) const
{
    quantize(from, to, UnitQuantizer(), NoteQuantizer(), NoteDurationProperty);
/*

    quantizeByUnit(from, to);

    for ( ; from != to; ++from) {

	timeT duration(getUnitQuantizedDuration(*from));
	Note note(requantizeByNote(duration));

	(*from)->setMaybe<Int>(m_noteDurationProperty, duration);
	(*from)->setMaybe<Int>(Note::NoteType, note.getNoteType());
	(*from)->setMaybe<Int>(Note::NoteDots, note.getDots());
    }
*/
#ifdef NOT_DEFINED

    timeT excess = 0;

    for ( ; from != to; ++from) {

	timeT duration = quantizeByUnit(*from);

	//!!! Obviously too much duplication here at the moment

/*!!! I think we should move much of this stuff up into
 quantizeByUnit, and do the excess sums using un-quantized values as a
 lot of the problem is with MIDI files where notes are "inaccurate" to
 degrees smaller than the note resolution.

 Then we need to find a way to tell the renderer to omit altogether
 any events with a quantized duration of zero.

 We could also use a "fixNoteQuantizedValues" type of function that
 could be used explicitly by a user to state that (across a particular
 selection) they want quantized values to be saved as the normal ones,
 with the file.

 What about the distinction between quantizing starts of notes versus
 note durations?
*/

	if ((*from)->isa(Note::EventType)) {

	    timeT prev = duration;
	    Note note(requantizeByNote(duration));
	    (*from)->setMaybe<Int>(m_noteDurationProperty, duration);
	    (*from)->setMaybe<Int>(Note::NoteType, note.getNoteType());
	    (*from)->setMaybe<Int>(Note::NoteDots, note.getDots());
	    excess = duration - prev;

	} else if ((*from)->isa(Note::EventRestType)) {

	    if (excess > duration) {
		
		(*from)->setMaybe<Int>(m_noteDurationProperty, 0);
		excess -= duration;

	    } else {

		if (excess != 0) duration -= excess;

		Note note(requantizeByNote(duration));
		(*from)->setMaybe<Int>(m_noteDurationProperty, duration);
		(*from)->setMaybe<Int>(Note::NoteType, note.getNoteType());
		(*from)->setMaybe<Int>(Note::NoteDots, note.getDots());

	    }
	}
    }
#endif
}
/*

timeT
Quantizer::quantizeByNote(Event *e) const
{
    timeT duration = quantizeByUnit(e);
//    if (duration == 0 && e->getDuration() == 0) return 0;

    Note note(requantizeByNote(duration));

    e->setMaybe<Int>(m_noteDurationProperty, duration);
    e->setMaybe<Int>(Note::NoteType, note.getNoteType());
    e->setMaybe<Int>(Note::NoteDots, note.getDots());

    return duration;
}
*/

timeT 
Quantizer::quantizeByNote(timeT duration) const
{
    return NoteQuantizer().quantize(m_unit, m_maxDots, duration);
}

/*
Note
Quantizer::requantizeByNote(timeT &duration) const
{
    Note shortNote = Note::getNearestNote(duration, m_maxDots);

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

*/

timeT
Quantizer::getNoteQuantizedDuration(Event *e) const
{
    long d;
    if (e->get<Int>(NoteDurationProperty, d)) return (timeT)d;
    else return quantizeByNote(e->getDuration());
}


void Quantizer::unquantize(Event *e) const
{
    e->unset(DurationProperty);
    e->unset(NoteDurationProperty);
    e->unset(AbsoluteTimeProperty);

    // should we do this?  not entirely sure, but probably
/*    e->unset(Note::NoteType);
      e->unset(Note::NoteDots);*/
}


}
