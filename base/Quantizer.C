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
#include "NotationTypes.h"

#include <iostream>
#include <cstdio> // for sprintf

namespace Rosegarden {

const std::string Quantizer::DefaultPropertyNamePrefix = "DefaultQ";

Quantizer::Quantizer(std::string propertyNamePrefix,
		     QuantizationType type,
		     timeT unit, int maxDots) :
    m_type(type), m_unit(unit), m_maxDots(maxDots),
    m_absoluteTimeProperty(propertyNamePrefix + "AbsoluteTime"),
    m_durationProperty(propertyNamePrefix + "Duration")
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

Quantizer::Quantizer(const StandardQuantization &sq,
		     std::string propertyNamePrefix) :
    m_type(sq.type), m_unit(sq.unit), m_maxDots(sq.maxDots),
    m_absoluteTimeProperty(propertyNamePrefix + "AbsoluteTime"),
    m_durationProperty(propertyNamePrefix + "Duration")
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}    

Quantizer::Quantizer(const Quantizer &q) :
    m_type(q.m_type), m_unit(q.m_unit), m_maxDots(q.m_maxDots),
    m_absoluteTimeProperty(q.m_absoluteTimeProperty),
    m_durationProperty(q.m_durationProperty)
{
    // nothing else
}

Quantizer &
Quantizer::operator=(const Quantizer &q)
{
    if (&q == this) return *this;
    m_type = q.m_type;
    m_unit = q.m_unit;
    m_maxDots = q.m_maxDots;
    m_absoluteTimeProperty = q.m_absoluteTimeProperty;
    m_durationProperty = q.m_durationProperty;
    return *this;
}

bool
Quantizer::operator==(const Quantizer &q) const
{
    return
	(m_type == q.m_type) &&
	(m_unit == q.m_unit) &&
	(m_maxDots == q.m_maxDots) &&
	(m_absoluteTimeProperty == q.m_absoluteTimeProperty) &&
	(m_durationProperty == q.m_durationProperty);
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
Quantizer::quantize(Segment::iterator from, Segment::iterator to) const
{
    switch (m_type) {

    case UnitQuantize:
	quantize(from, to, UnitQuantizer(), UnitQuantizer());
	break;

    case NoteQuantize:
	quantize(from, to, UnitQuantizer(), NoteQuantizer());
	break;

    case LegatoQuantize:
	// Legato quantization is relatively slow (hence the name?) and
	// with the minimal unit it's equivalent to note quantization

	if (m_unit == Note(Note::Shortest).getDuration()) {
	    quantize(from, to, UnitQuantizer(), NoteQuantizer());
	} else {
	    quantize(from, to, UnitQuantizer(), LegatoQuantizer());
	}
	break;
    }
}

void
Quantizer::fixQuantizedValues(Segment::iterator from,
			      Segment::iterator to) const
{
    quantize(from, to);

    for (; from != to; ++from) {
	if ((*from)->has(getAbsoluteTimeProperty())) {
	    (*from)->setAbsoluteTime((*from)->get<Int>
				     (getAbsoluteTimeProperty()));
	}
	if ((*from)->has(getDurationProperty())) {
	    (*from)->setDuration((*from)->get<Int>
				 (getDurationProperty()));
	}
	unquantize(*from);
    }
}


timeT
Quantizer::getQuantizedDuration(Event *e) const
{
    long d;
    if (e->get<Int>(getDurationProperty(), d)) return (timeT)d;
    else return quantizeDuration(e->getDuration());
}


timeT
Quantizer::getQuantizedAbsoluteTime(Event *e) const
{
    long d;
    if (e->get<Int>(getAbsoluteTimeProperty(), d)) return (timeT)d;
    else return quantizeAbsoluteTime(e->getAbsoluteTime());
}


timeT 
Quantizer::quantizeAbsoluteTime(timeT absoluteTime) const
{
    timeT d = 0;

    switch (m_type) {

    case UnitQuantize:
	d = UnitQuantizer().quantize(m_unit, m_maxDots, absoluteTime, 0);
	break;

    case NoteQuantize:
	d = UnitQuantizer().quantize(m_unit, m_maxDots, absoluteTime, 0);
	break;

    case LegatoQuantize:
	d = UnitQuantizer().quantize(Note(Note::Shortest).getDuration(),
					m_maxDots, absoluteTime, 0);
	break;
    }

    return d;
}


timeT 
Quantizer::quantizeDuration(timeT duration) const
{
    timeT d = 0;

    switch (m_type) {

    case UnitQuantize:
	d = UnitQuantizer().quantize(m_unit, m_maxDots, duration, 0);
	break;

    case NoteQuantize:
    case LegatoQuantize:
	d = NoteQuantizer().quantize(m_unit, m_maxDots, duration, 0);
	break;
    }

    return d;
}


void
Quantizer::quantize(Segment::iterator from, Segment::iterator to,
		    const SingleQuantizer &aq, const SingleQuantizer &dq) const
{
    timeT excess = 0;
    bool legato = (m_type == LegatoQuantize);
    
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

	(*from)->setMaybe<Int>(getAbsoluteTimeProperty(), qAbsoluteTime);
	(*from)->setMaybe<Int>(getDurationProperty(), qDuration);
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
Quantizer::unquantize(Segment::iterator from, Segment::iterator to) const
{
    for (; from != to; ++from) unquantize(*from);
}

void
Quantizer::unquantize(Event *e) const
{
    e->unset(getAbsoluteTimeProperty());
    e->unset(getDurationProperty());
}


std::vector<StandardQuantization>
StandardQuantization::getStandardQuantizations()
{
    std::vector<StandardQuantization> v;
    char buf[100];

    for (Note::Type nt = Note::Semibreve; nt >= Note::Shortest; --nt) {

	int i1 = (nt < Note::Quaver ? 1 : 0);
	for (int i = 0; i <= i1; ++i) {

	    string noteName = Note(nt).getReferenceName();
	    if (i) noteName = string("3-") + noteName;
	    
	    int divisor = (1 << (Note::Semibreve - nt));
	    if (i) divisor = divisor * 3 / 2;
	    sprintf(buf, "1/%d", divisor);

	    timeT unit = Note(Note::Semibreve).getDuration() / divisor;
	    
	    v.push_back(StandardQuantization(Quantizer::UnitQuantize,
					     unit, 2, buf, noteName));
	}
    }

    return v;
}

}
