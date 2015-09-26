/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Quantizer.h"
#include "base/BaseProperties.h"
#include "base/NotationTypes.h"
#include "Selection.h"
#include "Composition.h"
#include "Sets.h"
#include "base/Profiler.h"

#include <QtGlobal>

#include <iostream>
#include <cmath>
#include <cstdio> // for sprintf
#include <ctime>

using std::cout;
using std::cerr;
using std::endl;

//#define DEBUG_NOTATION_QUANTIZER 1

namespace Rosegarden {

Quantizer::Quantizer(std::string source,
		     std::string target) :
    m_source(source), m_target(target)
{
    makePropertyNames();
}


Quantizer::Quantizer(std::string target) :
    m_target(target)
{
    if (target == RawEventData) {
	m_source = GlobalSource;
    } else {
	m_source = RawEventData;
    }

    makePropertyNames();
}


Quantizer::~Quantizer()
{
    // nothing
}

void
Quantizer::quantize(Segment *s) const
{
    quantize(s, s->begin(), s->getEndMarker());
}

void
Quantizer::quantize(Segment *s,
		    Segment::iterator from,
		    Segment::iterator to) const
{
    Q_ASSERT(m_toInsert.size() == 0);

    m_normalizeRegion.first =
        (from != s->end() ? (*from)->getAbsoluteTime() : s->getStartTime());
    m_normalizeRegion.second =
        (to != s->end() ? (*to)->getAbsoluteTime() : s->getEndTime());

    quantizeRange(s, from, to);

    insertNewEvents(s);
}

void
Quantizer::quantize(EventSelection *selection)
{
    Q_ASSERT(m_toInsert.size() == 0);

    Segment &segment = selection->getSegment();

    m_normalizeRegion.first = segment.getStartTime();
    m_normalizeRegion.second = segment.getEndTime();

    // Attempt to handle non-contiguous selections.

    // We have to be a bit careful here, because the rest-
    // normalisation that's carried out as part of a quantize
    // process is liable to replace the event that follows
    // the quantized range.  (moved here from editcommands.cpp)

    EventSelection::RangeList ranges(selection->getRanges());

    // So that we can retrieve a list of new events we cheat and stop
    // the m_toInsert vector from being cleared automatically.  Remember
    // to turn it back on.
    //

    EventSelection::RangeList::iterator r = ranges.end();
    while (r-- != ranges.begin()) {

/*
	cerr << "Quantizer: quantizing range ";
	if (r->first == segment.end()) {
	    cerr << "end";
	} else {
	    cerr << (*r->first)->getAbsoluteTime();
	}
	cerr << " to ";
	if (r->second == segment.end()) {
	    cerr << "end";
	} else {
	    cerr << (*r->second)->getAbsoluteTime();
	}
	cerr << endl;
*/

        quantizeRange(&segment, r->first, r->second);
    }

    // Push the new events to the selection
    for (int i = 0; i < int(m_toInsert.size()); ++i) {
        if (m_toInsert[i]->getAbsoluteTime() < segment.getEndTime()) {
            // Select only the events inside the segment.
            selection->addEvent(m_toInsert[i]);
        }
    }

    // and then to the segment
    insertNewEvents(&segment);
}


void
Quantizer::fixQuantizedValues(Segment *s,
			      Segment::iterator from,
			      Segment::iterator to) const
{
    Q_ASSERT(m_toInsert.size() == 0);

    quantize(s, from, to);

    if (m_target == RawEventData) return;

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


timeT
Quantizer::getQuantizedDuration(const Event *e) const
{
    if (m_target == RawEventData) {
	return e->getDuration();
    } else if (m_target == NotationPrefix) {
	return e->getNotationDuration();
    } else {
	timeT d = e->getDuration();
	e->get<Int>(m_targetProperties[DurationValue], d);
	return d;
    }
}

timeT
Quantizer::getQuantizedAbsoluteTime(const Event *e) const
{
    if (m_target == RawEventData) {
	return e->getAbsoluteTime();
    } else if (m_target == NotationPrefix) {
	return e->getNotationAbsoluteTime();
    } else {
	timeT t = e->getAbsoluteTime();
	e->get<Int>(m_targetProperties[AbsoluteTimeValue], t);
	return t;
    }
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
Quantizer::quantizeRange(Segment *s,
			 Segment::iterator from,
			 Segment::iterator to) const
{
    //!!! It is vital that ordering is maintained after quantization.
    // That is, an event whose absolute time quantizes to a time t must
    // appear in the original segment before all events whose times
    // quantize to greater than t.  This means we must quantize the
    // absolute times of non-note events as well as notes.

    // We don't need to worry about quantizing rests, however; they're
    // only used for notation and will be explicitly recalculated when
    // the notation quantization values change.

    for (Segment::iterator nextFrom = from; from != to; from = nextFrom) {

	++nextFrom;
	quantizeSingle(s, from);
    }
}
    
void
Quantizer::unquantize(Segment *s,
		      Segment::iterator from,
		      Segment::iterator to) const
{
    Q_ASSERT(m_toInsert.size() == 0);

    for (Segment::iterator nextFrom = from; from != to; from = nextFrom) {
	++nextFrom;

	if (m_target == RawEventData || m_target == NotationPrefix) {
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
    Q_ASSERT(m_toInsert.size() == 0);

    Segment *s = &selection->getSegment();

    Rosegarden::EventSelection::eventcontainer::iterator it
        = selection->getSegmentEvents().begin();

    for (; it != selection->getSegmentEvents().end(); it++) {

	if (m_target == RawEventData || m_target == NotationPrefix) {

            Segment::iterator from = s->findSingle(*it);
            Segment::iterator to = s->findSingle(*it);
	    setToTarget(s, from,
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
    Profiler profiler("Quantizer::getFromSource");

//    cerr << "Quantizer::getFromSource: source is \"" << m_source << "\"" << endl;

    if (m_source == RawEventData) {

	if (v == AbsoluteTimeValue) return e->getAbsoluteTime();
	else return e->getDuration();

    } else if (m_source == NotationPrefix) {

	if (v == AbsoluteTimeValue) return e->getNotationAbsoluteTime();
	else return e->getNotationDuration();

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
    Profiler profiler("Quantizer::getFromTarget");

    if (m_target == RawEventData) {

	if (v == AbsoluteTimeValue) return e->getAbsoluteTime();
	else return e->getDuration();

    } else if (m_target == NotationPrefix) {

	if (v == AbsoluteTimeValue) return e->getNotationAbsoluteTime();
	else return e->getNotationDuration();

    } else {
	timeT value;
	if (v == AbsoluteTimeValue) value = e->getAbsoluteTime();
	else value = e->getDuration();
	e->get<Int>(m_targetProperties[v], value);
	return value;
    }
}

void
Quantizer::setToTarget(Segment *s, Segment::iterator i,
		       timeT absTime, timeT duration) const
{
    Profiler profiler("Quantizer::setToTarget");

    //cerr << "Quantizer::setToTarget: target is \"" << m_target << "\", absTime is " << absTime << ", duration is " << duration << " (unit is " << m_unit << ", original values are absTime " << (*i)->getAbsoluteTime() << ", duration " << (*i)->getDuration() << ")" << endl;

    timeT st = 0, sd = 0;
    bool haveSt = false, haveSd = false;
    if (m_source != RawEventData && m_target == RawEventData) {
	haveSt = (*i)->get<Int>(m_sourceProperties[AbsoluteTimeValue], st);
	haveSd = (*i)->get<Int>(m_sourceProperties[DurationValue],     sd);
    }
    
    Event *e;
    if (m_target == RawEventData) {
	e = new Event(**i, absTime, duration);
    } else if (m_target == NotationPrefix) {
	// Setting the notation absolute time on an event without
	// recreating it would be dodgy, just as setting the absolute
	// time would, because it could change the ordering of events
	// that are already being referred to in ViewElementLists,
	// preventing us from locating them in the ViewElementLists
	// because their ordering would have silently changed
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "Quantizer: setting " << absTime << " to notation absolute time and "
	     << duration << " to notation duration"
	     << endl;
#endif
	e = new Event(**i, (*i)->getAbsoluteTime(), (*i)->getDuration(),
		      (*i)->getSubOrdering(), absTime, duration);
    } else {
	e = *i;
	e->clearNonPersistentProperties();
    }

    if (m_target == NotationPrefix) {
	timeT normalizeStart = std::min(absTime, (*i)->getAbsoluteTime());
	timeT normalizeEnd = std::max(absTime + duration,
				      (*i)->getAbsoluteTime() +
				      (*i)->getDuration()) + 1;

	if (m_normalizeRegion.first != m_normalizeRegion.second) {
	    normalizeStart = std::min(normalizeStart, m_normalizeRegion.first);
	    normalizeEnd = std::max(normalizeEnd, m_normalizeRegion.second);
	}
	
	m_normalizeRegion = std::pair<timeT, timeT>
	    (normalizeStart, normalizeEnd);
    }
    
    if (haveSt) e->setMaybe<Int>(m_sourceProperties[AbsoluteTimeValue],st);
    if (haveSd) e->setMaybe<Int>(m_sourceProperties[DurationValue],    sd);
    
    if (m_target != RawEventData && m_target != NotationPrefix) {
	e->setMaybe<Int>(m_targetProperties[AbsoluteTimeValue], absTime);
	e->setMaybe<Int>(m_targetProperties[DurationValue], duration);
    } else {
	s->erase(i);
	m_toInsert.push_back(e);
    }

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "m_toInsert.size() is now " << m_toInsert.size() << endl;
#endif
}

void
Quantizer::removeProperties(Event *e) const
{
    if (m_source != RawEventData) {
	e->unset(m_sourceProperties[AbsoluteTimeValue]);
	e->unset(m_sourceProperties[DurationValue]);
    }

    if (m_target != RawEventData && m_target != NotationPrefix) {
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
    if (m_source != RawEventData && m_source != NotationPrefix) {
	m_sourceProperties[AbsoluteTimeValue] = m_source + "AbsoluteTimeSource";
	m_sourceProperties[DurationValue]     = m_source + "DurationSource";
    }

    if (m_target != RawEventData && m_target != NotationPrefix) {
	m_targetProperties[AbsoluteTimeValue] = m_target + "AbsoluteTimeTarget";
	m_targetProperties[DurationValue]     = m_target + "DurationTarget";
    }
}	

void
Quantizer::insertNewEvents(Segment *s) const
{
    size_t sz = m_toInsert.size();

    timeT startTime = m_normalizeRegion.first;
    timeT endTime = m_normalizeRegion.second;
    timeT minTime = (sz > 0 ? endTime : 0);
    timeT maxTime = (sz > 0 ? startTime : 0);

    for (size_t i = 0; i < sz; ++i) {

	timeT myTime = m_toInsert[i]->getAbsoluteTime();
	timeT myDur  = m_toInsert[i]->getDuration();

        if (endTime > 0 && myTime >= endTime) {
            cerr << "Quantizer::insertNewEvents(): ignoring event outside the segment at "
                 << myTime << endl;
            continue;
        }

        if (myTime < minTime) minTime = myTime;
        if (myTime + myDur > maxTime) maxTime = myTime + myDur;

	s->insert(m_toInsert[i]);
    }

    if (minTime < startTime) {
        minTime = startTime;
    } else if (minTime > startTime) {
        timeT barStart = s->getBarStartForTime(minTime);

        if (barStart <= startTime) {
            minTime = startTime;
        } else if (minTime != barStart) {
            minTime = barStart;
        } else {
            // Maybe some events are quantized from the previous bar.
            minTime = std::max(startTime, s->getBarStartForTime(barStart - 1));
        }
    }

    if (endTime > 0) {
        if (maxTime > endTime) {
            maxTime = endTime;
        } else if (maxTime < endTime) {
            timeT barEnd = s->getBarEndForTime(maxTime);

            if (barEnd >= endTime) {
                maxTime = endTime;
            } else {
                // Maybe some quantized events occupied two bars.
                maxTime = std::min(endTime, s->getBarEndForTime(barEnd + 1));
            }
        }
    }

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "Quantizer::insertNewEvents: sz is " << sz
	      << ", minTime " << minTime << ", maxTime " << maxTime
	      << endl;
#endif

    if (m_target == NotationPrefix || m_target == RawEventData) {

	if (m_normalizeRegion.first == m_normalizeRegion.second) {
	    if (sz > 0) {
		s->normalizeRests(minTime, maxTime);
	    }
	} else {
	    s->normalizeRests(minTime, maxTime);
	    m_normalizeRegion = std::pair<timeT, timeT>(0, 0);
	}
    }
		
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "Quantizer: calling normalizeRests("
		  << minTime << ", " << maxTime << ")" << endl;
#endif

    m_toInsert.clear();
}




}
