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

#include "Quantizer.h"
#include "BaseProperties.h"

#include <iostream>

namespace Rosegarden {

const PropertyName Quantizer::AbsoluteTimeProperty = "QuantizedAbsoluteTime";
const PropertyName Quantizer::DurationProperty	   = "QuantizedDuration";
const PropertyName Quantizer::NoteDurationProperty = "QuantizedNoteDuration";
const PropertyName Quantizer::LegatoDurationProperty = "QuantizedLegatoDuration";

Quantizer::Quantizer(int unit, int maxDots) :
    m_unit(unit), m_maxDots(maxDots)
{
    if (unit < 0) setUnit(Note(Note::Shortest));

    if (m_maxDots != 1 && m_maxDots != 2) {
	std::cerr << "Quantizer::Quantizer: WARNING: m_maxDots = " << m_maxDots
	     << std::endl;
	// dump core, please
	char *myString = (char *)1;
	std::cerr << "myString -> " << myString << std::endl;
    }
}

Quantizer::~Quantizer()
{
    // nothing
}

Quantizer::SingleQuantizer::~SingleQuantizer() { }
Quantizer::UnitQuantizer::~UnitQuantizer() { }
Quantizer::NoteQuantizer::~NoteQuantizer() { }
Quantizer::LegatoQuantizer::~LegatoQuantizer() { }

timeT
Quantizer::SingleQuantizer::getDuration(Event *e) const
{
    return e->getDuration();
}

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
//    cerr << "NoteQuantizer::quantize: unit is " << unit << ", duration is " << duration << std::endl;

    duration = UnitQuantizer().quantize(unit, maxDots, duration, 0);
    Note shortNote = Note::getNearestNote(duration, maxDots);

    timeT shortTime = shortNote.getDuration();
    if (shortTime == duration) {
//	cerr << "returning(1) " << shortTime << std::endl;
	return shortTime;
    }

    Note longNote(shortNote);

    if (shortTime < unit) { // original duration probably quantized to zero

//	cerr << "shortTime is " << shortTime << std::endl;
	longNote = Note::getNearestNote(unit, maxDots);
//	cerr << "longTime is " << longNote.getDuration() << std::endl;

    } else if ((shortNote.getDots() > 0 ||
		shortNote.getNoteType() == Note::Shortest)) { // can't dot that

	if (shortNote.getNoteType() == Note::Longest) {
//	    cerr << "returning(2) " << shortTime << std::endl;
	    return shortTime;
	}

	longNote = Note(shortNote.getNoteType() + 1, 0);

    } else {

        longNote = Note(shortNote.getNoteType(), 1);
    }

    timeT longTime = longNote.getDuration();

    if (shortTime < unit || 
	(longTime - duration < duration - shortTime)) {
//	cerr << "returning(3) " << longTime << std::endl;
	return longTime;
    } else {
//	cerr << "returning(4) " << shortTime << std::endl;
	return shortTime;
    }
};

timeT
Quantizer::NoteQuantizer::getDuration(Event *e) const
{
    long nominalDuration;
    if (e->get<Int>(BaseProperties::TUPLET_NOMINAL_DURATION, nominalDuration)) {
	return nominalDuration;
    } else {
	return e->getDuration();
    }
}

timeT
Quantizer::LegatoQuantizer::quantize(int unit, int maxDots, timeT duration,
				     timeT followingRestDuration) const
{
//    cerr << "LegatoQuantizer::quantize: followingRestDuration is " << followingRestDuration << std::endl;

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
Quantizer::quantize(Segment::iterator from, Segment::iterator to,
		    const SingleQuantizer &aq, const SingleQuantizer &dq,
		    PropertyName durationProperty, bool legato) const
{
    timeT excess = 0;

    // For the moment, legato quantization always uses the minimum
    // unit (rather than the potentially large legato unit) for the
    // absolute time.  Ideally we'd be able to specify both
    // separately.

    timeT absTimeQuantizeUnit =
	legato ? Note(Note::Shortest).getDuration() : m_unit;

    for ( ; from != to; ++from) {

	timeT absoluteTime   = (*from)->getAbsoluteTime();
	timeT duration	     = dq.getDuration(*from);
	timeT qDuration	     = 0;

	timeT qAbsoluteTime  =
	    aq.quantize(absTimeQuantizeUnit, m_maxDots, absoluteTime, 0);

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
	} else continue;

	(*from)->setMaybe<Int>(AbsoluteTimeProperty, qAbsoluteTime);
	(*from)->setMaybe<Int>(durationProperty, qDuration);
    }
}


timeT
Quantizer::findFollowingRestDuration(Segment::iterator from,
				     Segment::iterator to) const
{
    Segment::iterator j(from);
    timeT nextTime = (*j)->getAbsoluteTime() + (*j)->getDuration();

    while (j != to && (*j)->getAbsoluteTime() < nextTime) ++j;
    if (j == to) return 0;

    if (j != from && (*j)->isa(Note::EventRestType)) {
	return (*j)->getDuration() + findFollowingRestDuration(j, to);
    }

    return 0;
}


void
Quantizer::quantizeByUnit(Segment::iterator from, Segment::iterator to) const
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
Quantizer::quantizeByNote(Segment::iterator from, Segment::iterator to) const
{
    quantize(from, to,
	     UnitQuantizer(), NoteQuantizer(), NoteDurationProperty, false);
}

void
Quantizer::quantizeLegato(Segment::iterator from, Segment::iterator to) const
{
    // Legato quantization is relatively slow (hence the name?) and
    // with the minimal unit it's equivalent to note quantization

    if (m_unit == Note(Note::Shortest).getDuration()) {
	quantize(from, to,
		 UnitQuantizer(), NoteQuantizer(),
		 LegatoDurationProperty, false);
    } else {
	quantize(from, to,
		 UnitQuantizer(), LegatoQuantizer(),
		 LegatoDurationProperty, true);
    }
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


void
Quantizer::fixUnitQuantizedValues(Segment::iterator from,
				  Segment::iterator to) const
{
    quantizeByUnit(from, to);

    for (; from != to; ++from) {
	if ((*from)->has(AbsoluteTimeProperty)) {
	    (*from)->setAbsoluteTime((*from)->get<Int>(AbsoluteTimeProperty));
	}
	if ((*from)->has(DurationProperty)) {
	    (*from)->setDuration((*from)->get<Int>(DurationProperty));
	}
	unquantize(*from);
    }
}


void
Quantizer::fixNoteQuantizedValues(Segment::iterator from,
				  Segment::iterator to) const
{
    quantizeByNote(from, to);

    for (; from != to; ++from) {
	if ((*from)->has(AbsoluteTimeProperty)) {
	    (*from)->setAbsoluteTime((*from)->get<Int>(AbsoluteTimeProperty));
	}
	if ((*from)->has(NoteDurationProperty)) {
	    (*from)->setDuration((*from)->get<Int>(NoteDurationProperty));
	}
	unquantize(*from);
    }
}


void
Quantizer::fixLegatoQuantizedValues(Segment::iterator from,
				  Segment::iterator to) const
{
    quantizeLegato(from, to);

    for (; from != to; ++from) {
	if ((*from)->has(AbsoluteTimeProperty)) {
	    (*from)->setAbsoluteTime((*from)->get<Int>(AbsoluteTimeProperty));
	}
	if ((*from)->has(LegatoDurationProperty)) {
	    (*from)->setDuration((*from)->get<Int>(LegatoDurationProperty));
	}
	unquantize(*from);
    }
}


void Quantizer::unquantize(Event *e) const
{
    e->unset(DurationProperty);
    e->unset(NoteDurationProperty);
    e->unset(LegatoDurationProperty);
    e->unset(AbsoluteTimeProperty);
}


}
