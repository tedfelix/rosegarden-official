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

using std::cerr;
using std::endl;

namespace Rosegarden {

const std::string Quantizer::RawEventData = "";
const std::string Quantizer::DefaultTarget = "DefaultQ";

Quantizer::Quantizer(std::string source,
		     std::string target,
		     QuantizationType type,
		     timeT unit, int maxDots) :
    m_type(type), m_unit(unit), m_maxDots(maxDots),
    m_source(source), m_target(target)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

Quantizer::Quantizer(const StandardQuantization &sq,
		     std::string source,
		     std::string target) :
    m_type(sq.type), m_unit(sq.unit), m_maxDots(sq.maxDots),
    m_source(source), m_target(target)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}    

Quantizer::Quantizer(const Quantizer &q,
		     std::string source,
		     std::string target) :
    m_type(q.m_type), m_unit(q.m_unit), m_maxDots(q.m_maxDots),
    m_source(source), m_target(target)
{
    // nothing else
}
   
Quantizer::Quantizer(const Quantizer &q) :
    m_type(q.m_type), m_unit(q.m_unit), m_maxDots(q.m_maxDots),
    m_source(q.m_source), m_target(q.m_target)
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
    m_source = q.m_source;
    m_target = q.m_target;
    return *this;
}

bool
Quantizer::operator==(const Quantizer &q) const
{
    return
	(m_type == q.m_type) &&
	(m_unit == q.m_unit) &&
	(m_maxDots == q.m_maxDots) &&
	(m_source == q.m_source) &&
	(m_target == q.m_target);
}

Quantizer::~Quantizer()
{
    // nothing
}

Quantizer::SingleQuantizer::~SingleQuantizer() { }
Quantizer::UnitQuantizer::~UnitQuantizer() { }
Quantizer::NoteQuantizer::~NoteQuantizer() { }
Quantizer::LegatoQuantizer::~LegatoQuantizer() { }
/*!!!
timeT
Quantizer::SingleQuantizer::getDuration(Event *e) const
{
    return e->getDuration();
}
*/
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
/*!!!
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
*/
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

	if (m_target != RawEventData) {
	    timeT t = getFromTarget(*from, AbsoluteTimeValue);
	    timeT d = getFromTarget(*from, DurationValue);
	    (*from)->setAbsoluteTime(t);
	    (*from)->setDuration(d);
	}

	removeProperties(*from);
    }
}


timeT
Quantizer::getQuantizedDuration(Event *e) const
{
    if (m_target == RawEventData) {
	return e->getDuration();
    } else {
	if (e->has(m_target + "Duration")) {
	    return e->get<Int>(m_target + "Duration");
	} else {
	    return quantizeDuration(e->getDuration());
	}
    }
}


timeT
Quantizer::getQuantizedAbsoluteTime(Event *e) const
{
    if (m_target == RawEventData) {
	return e->getAbsoluteTime();
    } else {
	if (e->has(m_target + "AbsoluteTime")) {
	    return e->get<Int>(m_target + "AbsoluteTime");
	} else {
	    return quantizeAbsoluteTime(e->getAbsoluteTime());
	}
    }
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

//!!!	timeT absoluteTime   = (*from)->getAbsoluteTime();
//!!!	timeT duration	     = dq.getDuration(*from);
	timeT absoluteTime   = getFromSource(*from, AbsoluteTimeValue);
	timeT duration       = getFromSource(*from, DurationValue);

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

	setToTarget(*from, AbsoluteTimeValue, qAbsoluteTime);
	setToTarget(*from, DurationValue, qDuration);
//!!!	(*from)->setMaybe<Int>(getAbsoluteTimeProperty(), qAbsoluteTime);
//!!!	(*from)->setMaybe<Int>(getDurationProperty(), qDuration);
    }
}


timeT
Quantizer::findFollowingRestDuration(Segment::iterator from,
				     Segment::iterator to) const
{
    //!!! update to use getFromSource

    Segment::iterator j(from);
    timeT nextTime =
	getFromSource(*j, AbsoluteTimeValue) +
	getFromSource(*j, DurationValue);

    while (j != to && getFromSource(*j, AbsoluteTimeValue) < nextTime) ++j;
    if (j == to) return 0;

    if (j != from && (*j)->isa(Note::EventRestType)) {
	return getFromSource(*j, DurationValue) +
	    findFollowingRestDuration(j, to);
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
    if (m_target == RawEventData) {
	setToTarget(e, AbsoluteTimeValue, getFromSource(e, AbsoluteTimeValue));
	setToTarget(e, DurationValue,     getFromSource(e, DurationValue));

    } else {
	removeTargetProperties(e);
    }
}

timeT
Quantizer::getFromSource(Event *e, ValueType v) const
{
    cerr << "Quantizer::getFromSource: source is \"" << m_source << "\"" << endl;

    if (m_source == RawEventData) {

	if (v == AbsoluteTimeValue) return e->getAbsoluteTime();
	else return e->getDuration();

    } else {

	std::string tag(v == AbsoluteTimeValue ? "AbsoluteTime" : "Duration");

	// We need to write the source from the target if the
	// source doesn't exist, or if the backup value for the
	// target exists and doesn't match the current target
	// value

	//!!! All these "m_source + tag" things need dealing
	// with -- slow interning

	bool haveSource = e->has(m_source + tag);
	bool haveTarget =
	    ((m_target == RawEventData) || (e->has(m_target + tag)));

	if (haveSource) {
	    if (haveTarget) {
		timeT targetValueBackup = 0;
		if (e->get<Int>(m_source + tag + "TargetBackup",
				targetValueBackup)){
		    
		    timeT currentTargetValue = getFromTarget(e, v);
		    if (currentTargetValue != targetValueBackup) {
			e->setMaybe<Int>(m_source + tag, currentTargetValue);
			return currentTargetValue;
		    }
		}
	    }
	} else {
	    timeT currentTargetValue = getFromTarget(e, v);
	    e->setMaybe<Int>(m_source + tag, currentTargetValue);
	    return currentTargetValue;
	}

	return e->get<Int>(m_source + tag);
    }
}

timeT
Quantizer::getFromTarget(Event *e, ValueType v) const
{
    if (m_target == RawEventData) {

	if (v == AbsoluteTimeValue) return e->getAbsoluteTime();
	else return e->getDuration();

    } else {

	std::string tag(v == AbsoluteTimeValue ? "AbsoluteTime" : "Duration");

	timeT value = 0;
	e->get<Int>(m_target + tag, value);
	return value;
    }
}

void
Quantizer::setToSource(Event *e, ValueType v, timeT time) const
{
    if (m_source == RawEventData) {

	if (v == AbsoluteTimeValue) e->setAbsoluteTime(time);
	else e->setDuration(time);

    } else {

	std::string tag(v == AbsoluteTimeValue ? "AbsoluteTime" : "Duration");
	e->setMaybe<Int>(m_source + tag, time);
    }
}

void
Quantizer::setToTarget(Event *e, ValueType v, timeT time) const
{
    cerr << "Quantizer::setToTarget: target is \"" << m_target << "\", time is " << time << " (unit is " << m_unit << ", sort is " << (v == AbsoluteTimeValue ? "absolute time" : "duration") << ", original value is " << (v == AbsoluteTimeValue ? e->getAbsoluteTime() : e->getDuration()) << ")" << endl;

    std::string tag(v == AbsoluteTimeValue ? "AbsoluteTime" : "Duration");

    if (m_target == RawEventData) {
	
	if (v == AbsoluteTimeValue) e->setAbsoluteTime(time);
	else e->setDuration(time);

    } else {

	e->setMaybe<Int>(m_target + tag, time);
    }

    //!!! which things are setMaybes and which are sets?

    e->setMaybe<Int>(m_source + tag + "TargetBackup", time);
}

void
Quantizer::removeProperties(Event *e) const
{
    if (m_source != RawEventData) {
	e->unset(m_source + "AbsoluteTime");
	e->unset(m_source + "Duration");
	e->unset(m_source + "AbsoluteTimeTargetBackup");
	e->unset(m_source + "DurationTargetBackup");
    }

    if (m_target != RawEventData) {
	e->unset(m_target + "AbsoluteTime");
	e->unset(m_target + "Duration");
    }	
}

void
Quantizer::removeTargetProperties(Event *e) const
{
    if (m_target != RawEventData) {
	e->unset(m_target + "AbsoluteTime");
	e->unset(m_target + "Duration");
    }	
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
