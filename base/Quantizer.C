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
    makePropertyNames();
}

Quantizer::Quantizer(const StandardQuantization &sq,
		     std::string source,
		     std::string target) :
    m_type(sq.type), m_unit(sq.unit), m_maxDots(sq.maxDots),
    m_source(source), m_target(target)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
    makePropertyNames();
}    

Quantizer::Quantizer(const Quantizer &q,
		     std::string source,
		     std::string target) :
    m_type(q.m_type), m_unit(q.m_unit), m_maxDots(q.m_maxDots),
    m_source(source), m_target(target)
{
    makePropertyNames();
}
   
Quantizer::Quantizer(const Quantizer &q) :
    m_type(q.m_type), m_unit(q.m_unit), m_maxDots(q.m_maxDots),
    m_source(q.m_source), m_target(q.m_target)
{
    makePropertyNames();
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
    makePropertyNames();
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

timeT
Quantizer::UnitQuantizer::quantize(int unit, int, timeT duration, timeT) const
{
    if (duration != 0) {
	timeT low = (duration / unit) * unit;
	timeT high = low + unit;
	if (low > 0 && (high - duration > duration - low)) duration = low;
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
Quantizer::quantize(Segment *s,
		    Segment::iterator from, Segment::iterator to) const
{
    switch (m_type) {

    case UnitQuantize:
	quantize(s, from, to, UnitQuantizer(), UnitQuantizer());
	break;

    case NoteQuantize:
	quantize(s, from, to, UnitQuantizer(), NoteQuantizer());
	break;

    case LegatoQuantize:
	// Legato quantization is relatively slow (hence the name?) and
	// with the minimal unit it's equivalent to note quantization

	if (m_unit == Note(Note::Shortest).getDuration()) {
	    quantize(s, from, to, UnitQuantizer(), NoteQuantizer());
	} else {
	    quantize(s, from, to, UnitQuantizer(), LegatoQuantizer());
	}
	break;
    }
}

void
Quantizer::fixQuantizedValues(Segment *s, Segment::iterator from,
			      Segment::iterator to) const
{
    //!!! Twice as slow as it needs to be.

    quantize(s, from, to);

    for (Segment::iterator nextFrom = from; from != to; from = nextFrom) {

	++nextFrom;

	if (m_target != RawEventData) {
	    timeT t = getFromTarget(*from, AbsoluteTimeValue);
	    timeT d = getFromTarget(*from, DurationValue);
	    Event *e = new Event(**from, t, d);
	    s->erase(from);
	    m_toInsert.push_back(e);
	}
    }

    insertNewEvents(s);
}


timeT
Quantizer::getQuantizedDuration(Event *e) const
{
    if (m_target == RawEventData) {
	return e->getDuration();
    } else {
	if (e->has(m_targetProperties[DurationValue])) {
	    return e->get<Int>(m_targetProperties[DurationValue]);
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
	if (e->has(m_targetProperties[AbsoluteTimeValue])) {
	    return e->get<Int>(m_targetProperties[AbsoluteTimeValue]);
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
Quantizer::quantize(Segment *s, Segment::iterator from, Segment::iterator to,
		    const SingleQuantizer &aq, const SingleQuantizer &dq) const
{
    timeT excess = 0;
    bool legato = (m_type == LegatoQuantize);

    assert(m_toInsert.size() == 0);

    timeT fromTime = 0, toTime = 0;
    bool haveFromTime = false;

    // For the moment, legato quantization always uses the minimum
    // unit (rather than the potentially large legato unit) for the
    // absolute time.  Ideally we'd be able to specify both
    // separately.

    timeT absTimeQuantizeUnit =
	legato ? Note(Note::Shortest).getDuration() : m_unit;

    for (Segment::iterator nextFrom = from ; from != to; from = nextFrom) {

	++nextFrom;
//	cerr << "From is at " << std::distance(s->begin(), from) << " from start, nextFrom is at " << std::distance(s->begin(), nextFrom) << "; from's time is " << (*from)->getAbsoluteTime() << ", nextFrom's is " << (nextFrom == s->end() ? -1 : (*nextFrom)->getAbsoluteTime()) << endl;

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

	if (!haveFromTime) fromTime = qAbsoluteTime;
	toTime = qAbsoluteTime;

	setToTarget(s, from, qAbsoluteTime, qDuration);
//	cerr << "After set, from is at " << std::distance(s->begin(), from) << " from start, nextFrom is at " << std::distance(s->begin(), nextFrom) << endl;
    }
    
    insertNewEvents(s);
    if (haveFromTime) s->normalizeRests(fromTime, toTime);
}


timeT
Quantizer::findFollowingRestDuration(Segment::iterator from,
				     Segment::iterator to) const
{
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
Quantizer::unquantize(Segment *s,
		      Segment::iterator from, Segment::iterator to) const
{
    assert(m_toInsert.size() == 0);

    for (Segment::iterator nextFrom = from; from != to; from = nextFrom) {
	++nextFrom;

	if (m_target == RawEventData) {
	    setToTarget(s, from,
			getFromSource(*from, AbsoluteTimeValue),
			getFromSource(*from, DurationValue));
	    
	} else {
	    removeTargetProperties(*from);
	}
    }
    
    insertNewEvents(s);
}

timeT
Quantizer::getFromSource(Event *e, ValueType v) const
{
//    cerr << "Quantizer::getFromSource: source is \"" << m_source << "\"" << endl;

    if (m_source == RawEventData) {

	if (v == AbsoluteTimeValue) return e->getAbsoluteTime();
	else return e->getDuration();

    } else {

	// We need to write the source from the target if the
	// source doesn't exist (and the target does)

	bool haveSource = e->has(m_sourceProperties[v]);
	bool haveTarget = ((m_target == RawEventData) ||
			   (e->has(m_targetProperties[v])));
	timeT t = 0;

	if (!haveSource && haveTarget) {
	    t = getFromTarget(e, v);
	    e->setMaybe<Int>(m_sourceProperties[v], t);
	    return t;
	}

	e->get<Int>(m_sourceProperties[v], t);
	return t;
    }
}

timeT
Quantizer::getFromTarget(Event *e, ValueType v) const
{
    if (m_target == RawEventData) {

	if (v == AbsoluteTimeValue) return e->getAbsoluteTime();
	else return e->getDuration();

    } else {

	timeT value = 0;
	e->get<Int>(m_targetProperties[v], value);
	return value;
    }
}

void
Quantizer::setToTarget(Segment *s, Segment::iterator i,
		       timeT absTime, timeT duration) const
{
//    cerr << "Quantizer::setToTarget: target is \"" << m_target << "\", absTime is " << absTime << ", duration is " << duration << " (unit is " << m_unit << ", original values are absTime " << (*i)->getAbsoluteTime() << ", duration " << (*i)->getDuration() << ")" << endl;

    if (m_target == RawEventData) {

	timeT st = 0, sd = 0;
	bool haveSt = false, haveSd = false;
	if (m_source != RawEventData) {
	    haveSt = (*i)->get<Int>(m_sourceProperties[AbsoluteTimeValue], st);
	    haveSd = (*i)->get<Int>(m_sourceProperties[DurationValue],	   sd);
	}

	Event *e = new Event(**i, absTime, duration);

	if (haveSt) e->setMaybe<Int>(m_sourceProperties[AbsoluteTimeValue],st);
	if (haveSd) e->setMaybe<Int>(m_sourceProperties[DurationValue],    sd);

	s->erase(i);
	m_toInsert.push_back(e);

    } else {

	(*i)->setMaybe<Int>(m_targetProperties[AbsoluteTimeValue], absTime);
	(*i)->setMaybe<Int>(m_targetProperties[DurationValue], duration);
    }
}

void
Quantizer::removeProperties(Event *e) const
{
    if (m_source != RawEventData) {
	e->unset(m_sourceProperties[AbsoluteTimeValue]);
	e->unset(m_sourceProperties[DurationValue]);
    }

    if (m_target != RawEventData) {
	e->unset(m_targetProperties[AbsoluteTimeValue]);
	e->unset(m_targetProperties[DurationValue]);
    }	
}

void
Quantizer::removeTargetProperties(Event *e) const
{
    if (m_target != RawEventData) {
	e->unset(m_targetProperties[AbsoluteTimeValue]);
	e->unset(m_targetProperties[DurationValue]);
    }	
}

void
Quantizer::makePropertyNames()
{
    if (m_source != RawEventData) {
	m_sourceProperties[AbsoluteTimeValue] = m_source + "AbsoluteTimeSource";
	m_sourceProperties[DurationValue]     = m_source + "DurationSource";
    }

    if (m_target != RawEventData) {
	m_targetProperties[AbsoluteTimeValue] = m_target + "AbsoluteTimeTarget";
	m_targetProperties[DurationValue]     = m_target + "DurationTarget";
    }
}	

void
Quantizer::insertNewEvents(Segment *s) const
{
    for (int i = 0; i < m_toInsert.size(); ++i) {
	s->insert(m_toInsert[i]);
    }
    m_toInsert.clear();
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
