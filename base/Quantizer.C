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
const PropertyName Quantizer::LegatoDurationProperty = "QuantizedLegatoDuration";

Quantizer::SingleQuantizer::~SingleQuantizer() { }
Quantizer::UnitQuantizer::~UnitQuantizer() { }
Quantizer::NoteQuantizer::~NoteQuantizer() { }
Quantizer::LegatoQuantizer::~LegatoQuantizer() { }

timeT
Quantizer::UnitQuantizer::quantize(int unit, int, timeT duration, timeT) const
{
    if (duration != 0) {
	timeT low = (duration / unit) * unit;
	timeT high = low + unit;
	if (high - duration > duration - low) duration = low;
	else duration = high;
    }

    return duration;
}

timeT
Quantizer::NoteQuantizer::quantize(int unit, int maxDots,
				   timeT duration, timeT) const
{
//    cerr << "NoteQuantizer::quantize: unit is " << unit << ", duration is " << duration << endl;

    duration = UnitQuantizer().quantize(unit, maxDots, duration, 0);
    Note shortNote = Note::getNearestNote(duration, maxDots);

    timeT shortTime = shortNote.getDuration();
    if (shortTime == duration) {
//	cerr << "returning(1) " << shortTime << endl;
	return shortTime;
    }

    Note longNote(shortNote);

    if (shortTime < unit) { // original duration probably quantized to zero

//	cerr << "shortTime is " << shortTime << endl;
	longNote = Note::getNearestNote(unit, maxDots);
//	cerr << "longTime is " << longNote.getDuration() << endl;

    } else if ((shortNote.getDots() > 0 ||
		shortNote.getNoteType() == Note::Shortest)) { // can't dot that

	if (shortNote.getNoteType() == Note::Longest) {
//	    cerr << "returning(2) " << shortTime << endl;
	    return shortTime;
	}

	longNote = Note(shortNote.getNoteType() + 1, 0);

    } else {

        longNote = Note(shortNote.getNoteType(), 1);
    }

    timeT longTime = longNote.getDuration();

    if (shortTime < unit || 
	(longTime - duration < duration - shortTime)) {
//	cerr << "returning(3) " << longTime << endl;
	return longTime;
    } else {
//	cerr << "returning(4) " << shortTime << endl;
	return shortTime;
    }
};

timeT
Quantizer::LegatoQuantizer::quantize(int unit, int maxDots, timeT duration,
				     timeT followingRestDuration) const
{
//    cerr << "LegatoQuantizer::quantize: followingRestDuration is " << followingRestDuration << endl;

    if (followingRestDuration > 0) {

	timeT possibleDuration = NoteQuantizer().quantize
	    (unit, maxDots, duration, 0);

	if (possibleDuration > duration) {
	    if (possibleDuration - duration <= followingRestDuration) {
		return possibleDuration;
	    } else {
		return NoteQuantizer().quantize
		    (Note(Note::Shortest).getDuration(),
		     maxDots, duration + followingRestDuration, 0);
	    }
	}
    }

    return NoteQuantizer().quantize(Note(Note::Shortest).getDuration(),
				    maxDots, duration, 0);
}


void
Quantizer::quantize(Track::iterator from, Track::iterator to,
		    const SingleQuantizer &aq, const SingleQuantizer &dq,
		    PropertyName durationProperty, bool legato) const
{
    timeT excess = 0;

    for ( ; from != to; ++from) {

	timeT absoluteTime   = (*from)->getAbsoluteTime();
	timeT duration	     = (*from)->getDuration();
	timeT qDuration	     = 0;

	// wacky legato-stylee big-unit quantization doesn't interest us here;
	// we'd need a separate unit for it if it did
	timeT qAbsoluteTime  =
	    aq.quantize(Note(Note::Shortest).getDuration(),
			m_maxDots, absoluteTime, 0);

	if ((*from)->isa(Note::EventType)) {

	    timeT followingRestDuration = 0;
	    if (legato) {
		followingRestDuration = findFollowingRestDuration(from, to);
	    }

	    qDuration = dq.quantize
		(m_unit, m_maxDots, duration, followingRestDuration);
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

		qDuration = dq.quantize(m_unit, m_maxDots, duration, 0);
	    }
	} else continue;//!!!


	//!!! should skip this for non-note/rest events, but can't
	//while we still have the next two methods working the way
	//they do -- really want to drop per-Event quantization
	//altogether, because it's inconsistent

	(*from)->setMaybe<Int>(AbsoluteTimeProperty, qAbsoluteTime);
	(*from)->setMaybe<Int>(durationProperty, qDuration);
    }
}


timeT
Quantizer::findFollowingRestDuration(Track::iterator from,
				     Track::iterator to) const
{
    Track::iterator j(from);
    timeT nextTime = (*j)->getAbsoluteTime() + (*j)->getDuration();

    while (j != to && (*j)->getAbsoluteTime() < nextTime) ++j;
    if (j == to) return 0;

    if (j != from && (*j)->isa(Note::EventRestType)) {
	return (*j)->getDuration() + findFollowingRestDuration(j, to);
    }

    return 0;
}


void
Quantizer::quantizeByUnit(Track::iterator from, Track::iterator to) const
{
    quantize(from, to,
	     UnitQuantizer(), UnitQuantizer(), DurationProperty, false);
}

timeT
Quantizer::quantizeByUnit(timeT duration) const
{
    return UnitQuantizer().quantize(m_unit, m_maxDots, duration, 0);
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
    quantize(from, to,
	     UnitQuantizer(), NoteQuantizer(), NoteDurationProperty, false);

/*!!!

 Need an option that quantizes note durations with a minimum unit, but
 quantizes with the given unit if and only if there's enough rest space
 following to take up the slack (the real meaning of legato!)

 We could also use a "fixNoteQuantizedValues" type of function that
 could be used explicitly by a user to state that (across a particular
 selection) they want quantized values to be saved as the normal ones,
 with the file.

*/
}

void
Quantizer::quantizeLegato(Track::iterator from, Track::iterator to) const
{
    quantize(from, to,
	     UnitQuantizer(), LegatoQuantizer(), LegatoDurationProperty, true);
}

timeT 
Quantizer::quantizeByNote(timeT duration) const
{
    return NoteQuantizer().quantize(m_unit, m_maxDots, duration, 0);
}


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
    e->unset(LegatoDurationProperty);
    e->unset(AbsoluteTimeProperty);
}


}
