// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4
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
#include "Selection.h"

#include <iostream>
#include <cstdio> // for sprintf

using std::cerr;
using std::endl;

namespace Rosegarden {

const std::string Quantizer::RawEventData = "";
const std::string Quantizer::DefaultTarget = "DefaultQ";
const std::string Quantizer::GlobalSource = "GlobalQ";

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
Quantizer::IdentityQuantizer::~IdentityQuantizer() { }
Quantizer::UnitQuantizer::~UnitQuantizer() { }
Quantizer::NoteQuantizer::~NoteQuantizer() { }
Quantizer::LegatoQuantizer::~LegatoQuantizer() { }

timeT
Quantizer::IdentityQuantizer::quantize(int, int, timeT t, timeT, bool) const
{
    return t;
}

timeT
Quantizer::UnitQuantizer::quantize(int unit, int, timeT t, timeT,
				   bool isAbsoluteTime) const
{
    if (t != 0) {
	timeT low = (t / unit) * unit;
	timeT high = low + unit;
	if ((low > 0 || isAbsoluteTime) &&
	    (high - t > t - low)) t = low;
	else t = high;
    }

    return t;
}

timeT
Quantizer::NoteQuantizer::quantize(int unit, int maxDots,
				   timeT t, timeT, bool) const
{
//    cerr << "NoteQuantizer::quantize: unit is " << unit << ", t is " << t << std::endl;

    //!!! We probably shouldn't quantize tuplets

    Note shortNote = Note::getNearestNote(t, maxDots);

    timeT shortTime = shortNote.getDuration();
    if (shortTime == t) {
//	cerr << "returning(1) " << shortTime << std::endl;
	return shortTime;
    }

    Note longNote(shortNote);

    if (shortTime < unit) { // original t probably quantized to zero

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

    // we should prefer to round up to a note with fewer dots rather
    // than down to one with more

    if (shortTime < unit || 
	((longNote.getDots() + 1) * (longTime - t) <
	 (shortNote.getDots() + 1) * (t - shortTime))) {
//	cerr << "returning(3) " << longTime << std::endl;
	return longTime;
    } else {
//	cerr << "returning(4) " << shortTime << std::endl;
	return shortTime;
    }
};

timeT
Quantizer::LegatoQuantizer::quantize(int unit, int maxDots, timeT t,
				     timeT followingRestDuration,
				     bool isAbsoluteTime) const
{
//    cerr << "LegatoQuantizer::quantize: followingRestDuration is " << followingRestDuration << std::endl;

    if (followingRestDuration > 0) {

	timeT possibleDuration = NoteQuantizer().quantize
	    (unit, maxDots, t, 0, isAbsoluteTime);

	if (possibleDuration > t) {
	    if (possibleDuration - t <= followingRestDuration) {
		return possibleDuration;
	    } else {
		return NoteQuantizer().quantize
		    (Note(Note::Shortest).getDuration(),
		     maxDots, t + followingRestDuration, 0,
		     isAbsoluteTime);
	    }
	}
    }

    return NoteQuantizer().quantize(Note(Note::Shortest).getDuration(),
				    maxDots, t, 0, isAbsoluteTime);
}


Quantizer::SingleQuantizer &
Quantizer::getDefaultAbsTimeQuantizer() const
{
    static SingleQuantizer *unitQuantizer = new UnitQuantizer();
    return *unitQuantizer;
}

Quantizer::SingleQuantizer &
Quantizer::getDefaultDurationQuantizer() const
{
    static SingleQuantizer *identityQuantizer = new IdentityQuantizer();
    static SingleQuantizer *unitQuantizer = new UnitQuantizer();
    static SingleQuantizer *noteQuantizer = new NoteQuantizer();
    static SingleQuantizer *legatoQuantizer = new LegatoQuantizer();

    switch (m_type) {

    case PositionQuantize:
	return *identityQuantizer;

    case UnitQuantize:
	return *unitQuantizer;

    case NoteQuantize:
	return *noteQuantizer;
	
    case LegatoQuantize:
	if (m_unit == Note(Note::Shortest).getDuration()) {
	    return *noteQuantizer;
	} else {
	    return *legatoQuantizer;
	}
    }

    return *unitQuantizer; // avoid compiler warnings
}


void
Quantizer::quantize(Segment *s,
		    Segment::iterator from, Segment::iterator to) const
{
    assert(m_toInsert.size() == 0);

    quantize(s, from, to,
	     getDefaultAbsTimeQuantizer(),
	     getDefaultDurationQuantizer());

    insertNewEvents(s);
}

void
Quantizer::quantize(EventSelection *selection)
{
    assert(m_toInsert.size() == 0);

    Segment &segment = selection->getSegment();

    // Attempt to handle non-contiguous selections.

    // We have to be a bit careful here, because the rest-
    // normalisation that's carried out as part of a quantize
    // process is liable to replace the event that follows
    // the quantized range.  (moved here from editcommands.cpp)

    typedef std::vector<std::pair<Segment::iterator,
                                  Segment::iterator> > RangeList;
    RangeList ranges;

    Segment::iterator i = segment.findTime(selection->getStartTime());
    Segment::iterator j = i;
    Segment::iterator k = segment.findTime(selection->getEndTime());

    if (j == segment.end()) {
	std::cerr << "j is at end" << endl;
    } else {
	std::cerr << "j is at " << (*j)->getAbsoluteTime() << std::endl;
    }

    if (k == segment.end()) {
	std::cerr << "k is at end" << endl;
    } else {
	std::cerr << "k is at " << (*k)->getAbsoluteTime() << std::endl;
    }

    while (j != k) {

//	std::cerr << "j is now at " << (*j)->getAbsoluteTime() << " with type " << (*j)->getType() << endl;

        for (j = i; j != k && selection->contains(*j); ++j);

        if (j != i) {
            ranges.push_back(RangeList::value_type(i, j));
	}

	for (i = j; i != k && !selection->contains(*i); ++i);
	j = i;
    }

    // So that we can retrieve a list of new events we cheat and stop
    // the m_toInsert vector from being cleared automatically.  Remember
    // to turn it back on.
    //

    RangeList::iterator r = ranges.end();
    while (r-- != ranges.begin()) {

	std::cerr << "Quantizer: quantizing range ";
	if (r->first == segment.end()) {
	    std::cerr << "end";
	} else {
	    std::cerr << (*r->first)->getAbsoluteTime();
	}
	std::cerr << " to ";
	if (r->second == segment.end()) {
	    std::cerr << "end";
	} else {
	    std::cerr << (*r->second)->getAbsoluteTime();
	}
	std::cerr << std::endl;

        quantize(&segment, r->first, r->second,
		 getDefaultAbsTimeQuantizer(),
		 getDefaultDurationQuantizer());
    }

    // Push the new events to the selection
    for (int i = 0; i < m_toInsert.size(); ++i) {
	selection->addEvent(m_toInsert[i]);
    }

    // and then to the segment
    insertNewEvents(&segment);
}


void
Quantizer::fixQuantizedValues(Segment *s, Segment::iterator from,
			      Segment::iterator to) const
{
    assert(m_toInsert.size() == 0);

    // Twice as slow as it needs to be, unless target is RawEventData

    quantize(s, from, to);

    if (m_target != RawEventData) {

	for (Segment::iterator nextFrom = from; from != to; from = nextFrom) {

	    ++nextFrom;

	    timeT t = getFromTarget(*from, AbsoluteTimeValue);
	    timeT d = getFromTarget(*from, DurationValue);
	    Event *e = new Event(**from, t, d);
	    s->erase(from);
	    m_toInsert.push_back(e);
	}

	insertNewEvents(s);
    }
}


timeT
Quantizer::getQuantizedDuration(Event *e) const
{
    if (m_target == RawEventData ||
	!(e->isa(Note::EventType) || e->isa(Note::EventRestType))) {
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
    return getDefaultAbsTimeQuantizer().quantize
	(m_unit, m_maxDots, absoluteTime, 0, true);
}


timeT 
Quantizer::quantizeDuration(timeT duration) const
{
    return getDefaultDurationQuantizer().quantize
	(m_unit, m_maxDots, duration, 0, false);
}


void
Quantizer::quantize(Segment *s, Segment::iterator from, Segment::iterator to,
		    const SingleQuantizer &aq, const SingleQuantizer &dq) const
{
    timeT excess = 0;
    bool legato = (m_type == LegatoQuantize);

    timeT absTimeQuantizeUnit = m_unit;

    if (legato) {

	int unitNoteType = Note::getNearestNote(m_unit).getNoteType();

	// For legato quantization, the unit specified is potentially
	// large (the point is to round up notes to this unit only if
	// there's enough space following them, so the unit can be
	// larger than it might otherwise be).  This means we need a
	// much shorter unit for the absolute-time part of the legato
	// quantization; perhaps ideally the user could specify this
	// too, but for now we hazard a guess at...

	if (unitNoteType > Note::Shortest + 2) unitNoteType -= 2;
	else unitNoteType = Note::Shortest;

	absTimeQuantizeUnit = Note(unitNoteType).getDuration();
    }

    for (Segment::iterator nextFrom = from ; from != to; from = nextFrom) {

	++nextFrom;
//	cerr << "From is at " << std::distance(s->begin(), from) << " from start, nextFrom is at " << std::distance(s->begin(), nextFrom) << "; from's time is " << (*from)->getAbsoluteTime() << ", nextFrom's is " << (nextFrom == s->end() ? -1 : (*nextFrom)->getAbsoluteTime()) << endl;

	timeT absoluteTime   = getFromSource(*from, AbsoluteTimeValue);
	timeT duration       = getFromSource(*from, DurationValue);

	timeT qDuration	     = 0;

	timeT qAbsoluteTime  =
	    aq.quantize(absTimeQuantizeUnit, m_maxDots, absoluteTime, 0, true);

	if ((*from)->isa(Note::EventType)) {

	    timeT followingRestDuration = 0;
	    if (legato) {
		followingRestDuration = findFollowingRestDuration(from, to);
	    }

	    qDuration = dq.quantize
		(m_unit, m_maxDots, duration, followingRestDuration, false);
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

		qDuration = dq.quantize(m_unit, m_maxDots, duration, 0, false);
	    }
	} else continue;

	setToTarget(s, from, qAbsoluteTime, qDuration);
    }
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

timeT
Quantizer::getUnquantizedAbsoluteTime(Event *e) const
{
    return getFromSource(e, AbsoluteTimeValue);
}

timeT
Quantizer::getUnquantizedDuration(Event *e) const
{
    return getFromSource(e, DurationValue);
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

void
Quantizer::unquantize(EventSelection *selection) const
{
    assert(m_toInsert.size() == 0);

    Rosegarden::EventSelection::eventcontainer::iterator it
        = selection->getSegmentEvents().begin();

    for (; it != selection->getSegmentEvents().end(); it++) {

	if (m_target == RawEventData) {

            Segment::iterator from = selection->getSegment().findSingle(*it);
            Segment::iterator to = selection->getSegment().findSingle(*it);
	    setToTarget(&selection->getSegment(), from,
			getFromSource(*from, AbsoluteTimeValue),
			getFromSource(*to, DurationValue));
	    
	} else {
	    removeTargetProperties(*it);
	}
    }
    
    insertNewEvents(&selection->getSegment());
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
    //cerr << "Quantizer::setToTarget: target is \"" << m_target << "\", absTime is " << absTime << ", duration is " << duration << " (unit is " << m_unit << ", original values are absTime " << (*i)->getAbsoluteTime() << ", duration " << (*i)->getDuration() << ")" << endl;

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
    unsigned int sz = m_toInsert.size();

    timeT minTime = 0, maxTime = 0;

    for (unsigned int i = 0; i < sz; ++i) {

	timeT myTime = m_toInsert[i]->getAbsoluteTime();
	timeT myDur  = m_toInsert[i]->getDuration();
	if (i == 0 || myTime < minTime) minTime = myTime;
	if (i == 0 || myTime + myDur > maxTime) maxTime = myTime + myDur;

	s->insert(m_toInsert[i]);
    }

    if (sz > 0 && m_target == RawEventData) {
	s->normalizeRests(minTime, maxTime);
    }

    m_toInsert.clear();
}


std::vector<StandardQuantization>
StandardQuantization::getStandardQuantizations()
{
    checkStandardQuantizations();
    return m_standardQuantizations;
}

void
StandardQuantization::checkStandardQuantizations()
{
    if (m_standardQuantizations.size() > 0) return;

    char buf[100];

    for (Note::Type nt = Note::Semibreve; nt >= Note::Shortest; --nt) {

	int i1 = (nt < Note::Quaver ? 1 : 0);
	for (int i = 0; i <= i1; ++i) {

	    std::string noteName = Note(nt).getReferenceName();
	    if (i) noteName = std::string("3-") + noteName;
	    
	    int divisor = (1 << (Note::Semibreve - nt));
	    if (i) divisor = divisor * 3 / 2;
	    sprintf(buf, "1/%d", divisor);
	    std::string name(buf);

	    sprintf(buf, "%s %s",
		    Note(nt).getEnglishName().c_str(),
		    (i ? "triplet" : ""));
	    std::string description(buf);

	    timeT unit = Note(Note::Semibreve).getDuration() / divisor;
	    
	    m_standardQuantizations.push_back
		(StandardQuantization
		 (Quantizer::PositionQuantize,
		  unit, 2, name, description, noteName));
	}
    }
}    

StandardQuantization *
StandardQuantization::getStandardQuantization(Segment *s)
{
    checkStandardQuantizations();
    timeT unit = -1;

    for (Segment::iterator i = s->begin(); s->isBeforeEndMarker(i); ++i) {
	
	if (!(*i)->isa(Rosegarden::Note::EventType)) continue;
	timeT myUnit = getUnitFor(*i);
	if (unit < 0 || myUnit < unit) unit = myUnit;
    }

    return getStandardQuantizationFor(unit);
}

StandardQuantization *
StandardQuantization::getStandardQuantization(EventSelection *s)
{
    checkStandardQuantizations();
    timeT unit = -1;

    if (!s) return 0;

    for (EventSelection::eventcontainer::iterator i =
	     s->getSegmentEvents().begin();
	 i != s->getSegmentEvents().end(); ++i) {
	
	if (!(*i)->isa(Rosegarden::Note::EventType)) continue;
	timeT myUnit = getUnitFor(*i);
	if (unit < 0 || myUnit < unit) unit = myUnit;
    }

    return getStandardQuantizationFor(unit);
}

timeT
StandardQuantization::getUnitFor(Event *e)
{	    
    timeT absTime = e->getAbsoluteTime();
    timeT myQuantizeUnit = 0;
    
    // m_quantizations is in descending order of duration;
    // stop when we reach one that divides into the note's time
    
    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
	if (absTime % m_standardQuantizations[i].unit == 0) {
	    myQuantizeUnit = m_standardQuantizations[i].unit;
	    break;
	}
    }

    return myQuantizeUnit;
}

StandardQuantization *
StandardQuantization::getStandardQuantizationFor(timeT unit)
{
    if (unit > 0) {
	for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
	    if (m_standardQuantizations[i].unit == unit) {
		return &m_standardQuantizations[i];
	    }
	}
    }

    return 0;
}    

std::vector<StandardQuantization>
StandardQuantization::m_standardQuantizations;

}
