// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include "Segment.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "Composition.h"
#include "Quantizer.h"
#include "Profiler.h"

#include <iostream>
#include <algorithm>
#include <iterator>
#include <cstdio>

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;
    


Segment::Segment(SegmentType segmentType, timeT startTime) :
    std::multiset<Event*, Event::EventCmp>(),
    m_composition(0),
    m_startTime(startTime),
    m_endMarkerTime(0),
    m_endTime(startTime),
    m_track(0),
    m_type(segmentType),
    m_colourIndex(0),
    m_id(0),
    m_audioFileId(0),
    m_audioStartTime(0, 0),
    m_audioEndTime(0, 0),
    m_repeating(false),
    m_quantizer(new BasicQuantizer()),
    m_quantize(false),
    m_transpose(0),
    m_delay(0),
    m_realTimeDelay(0, 0),
    m_clefKeyList(0)
{
}

Segment::Segment(const Segment &segment):
    std::multiset<Event*, Event::EventCmp>(),
    m_composition(0), // Composition should decide what's in it and what's not
    m_startTime(segment.getStartTime()),
    m_endMarkerTime(segment.m_endMarkerTime ?
		    new timeT(*segment.m_endMarkerTime) : 0),
    m_endTime(segment.getEndTime()),
    m_track(segment.getTrack()),
    m_type(segment.getType()),
    m_label(segment.getLabel()),
    m_colourIndex(segment.getColourIndex()),
    m_id(0),
    m_audioFileId(segment.getAudioFileId()),
    m_audioStartTime(segment.getAudioStartTime()),
    m_audioEndTime(segment.getAudioEndTime()),
    m_repeating(segment.isRepeating()),
    m_quantizer(new BasicQuantizer(segment.m_quantizer->getUnit(),
				   segment.m_quantizer->getDoDurations())),
    m_quantize(segment.hasQuantization()),
    m_transpose(segment.getTranspose()),
    m_delay(segment.getDelay()),
    m_realTimeDelay(segment.getRealTimeDelay()),
    m_clefKeyList(0)
{
    for (iterator it = segment.begin();
	 segment.isBeforeEndMarker(it); ++it) {
        insert(new Event(**it));
    }
}

Segment::~Segment()
{
    notifySourceDeletion();

    if (m_observers.size() > 0) {
	cerr << "Warning: Segment::~Segment() with " << m_observers.size()
	     << " observers still extant" << endl;
    }

    if (m_composition) m_composition->detachSegment(this);

    if (m_clefKeyList) {
	// don't delete contents of m_clefKeyList: the pointers
	// are just aliases for events in the main segment
	delete m_clefKeyList;
    }

    // delete content
    for (iterator it = begin(); it != end(); ++it) delete (*it);

    delete m_endMarkerTime;
}


void
Segment::setTrack(TrackId id)
{
    Composition *c = m_composition;
    if (c) c->weakDetachSegment(this); // sets m_composition to 0
    m_track = id;
    if (c) {
        c->weakAddSegment(this);
        c->updateRefreshStatuses();
    }
}

timeT
Segment::getStartTime() const
{
    return m_startTime;
}

timeT
Segment::getEndMarkerTime() const
{
    timeT endTime;

    if (m_endMarkerTime) {
	endTime = *m_endMarkerTime;
    } else {
	endTime = getEndTime();
    }

    if (m_composition) {
	endTime = std::min(endTime, m_composition->getEndMarker());
    }

    return endTime;
}

timeT
Segment::getEndTime() const
{
    if (m_type == Audio && m_composition) {
	RealTime startRT = m_composition->getElapsedRealTime(m_startTime);
	RealTime endRT = startRT - m_audioStartTime + m_audioEndTime;
	return m_composition->getElapsedTimeForRealTime(endRT);
    } else {
	return m_endTime;
    }
}

void
Segment::setStartTime(timeT t)
{
    int dt = t - m_startTime;
    if (dt == 0) return;

    // reset the time of all events.  can't just setAbsoluteTime on these,
    // partly 'cos we're not allowed, partly 'cos it might screw up the
    // quantizer (which is why we're not allowed)

    // still, this is rather unsatisfactory
    
    FastVector<Event *> events;

    for (iterator i = begin(); i != end(); ++i) {
	Event *e = new Event(**i, (*i)->getAbsoluteTime() + dt);
	e->setNotationAbsoluteTime((*i)->getNotationAbsoluteTime() + dt);
	e->setNotationDuration((*i)->getNotationDuration());
	events.push_back(e);
    }

    erase(begin(), end());
    if (m_endMarkerTime) *m_endMarkerTime += dt;
    m_endTime += dt;

    if (m_composition) m_composition->setSegmentStartTime(this, t);
    else m_startTime = t;

    for (int i = 0; i < events.size(); ++i) {
	insert(events[i]);
    }
}

void
Segment::setEndMarkerTime(timeT t)
{
    if (t < m_startTime) t = m_startTime;

    if (m_type == Audio) {
	if (m_composition) {
	    m_audioEndTime = m_audioStartTime +
		m_composition->getRealTimeDifference(m_startTime, t);
	}
    } else {

	timeT endTime = getEndTime();
	bool shorten = (t < getEndMarkerTime());

	if (t > endTime) {
	    fillWithRests(endTime, t);
	} else {
	    // only need to do this if we aren't inserting or
	    // deleting any actual events
	    updateRefreshStatuses(t, endTime);
	}

	if (m_endMarkerTime) *m_endMarkerTime = t;
	else m_endMarkerTime = new timeT(t);
	if (m_type != Audio) notifyEndMarkerChange(shorten);
    }
}

void
Segment::setEndTime(timeT t)
{
    timeT endTime = getEndTime();
    if (t < m_startTime) t = m_startTime;

    if (m_type == Audio) {
	setEndMarkerTime(t);
    } else {
	if (t < endTime) {
	    erase(findTime(t), end());
	    endTime = getEndTime();
	    if (m_endMarkerTime && endTime < *m_endMarkerTime) {
		*m_endMarkerTime = endTime;
		notifyEndMarkerChange(true);
	    }
	} else if (t > endTime) {
	    fillWithRests(endTime, t);
	}
    }
}

Segment::iterator 
Segment::getEndMarker() const
{
    if (m_endMarkerTime) {
	return findTime(*m_endMarkerTime);
    } else {
	return end();
    }
}

bool
Segment::isBeforeEndMarker(iterator i) const
{ 
    if (i == end()) return false;

    timeT absTime = (*i)->getAbsoluteTime();
    timeT endTime = getEndMarkerTime();

    return ((absTime <  endTime) ||
	    (absTime == endTime && (*i)->getDuration() == 0));
}

void
Segment::clearEndMarker()
{
    delete m_endMarkerTime;
    m_endMarkerTime = 0;
    if (m_type != Audio) notifyEndMarkerChange(false);
}

const timeT *
Segment::getRawEndMarkerTime() const
{
    return m_endMarkerTime;
}


void
Segment::updateRefreshStatuses(timeT startTime, timeT endTime)
{
    for(unsigned int i = 0; i < m_refreshStatusArray.size(); ++i)
        m_refreshStatusArray.getRefreshStatus(i).push(startTime, endTime);
}


Segment::iterator
Segment::insert(Event *e)
{
    assert(e);

    timeT t0 = e->getAbsoluteTime();
    timeT t1 = t0 + e->getDuration();

    if (t0 < m_startTime ||
	(begin() == end() && t0 > m_startTime)) {

        if (m_composition) m_composition->setSegmentStartTime(this, t0);
	else m_startTime = t0;
    }

    if (t1 > m_endTime ||
	begin() == end()) {
	m_endTime = t1;
    }

    iterator i = std::multiset<Event*, Event::EventCmp>::insert(e);
    notifyAdd(e);
    updateRefreshStatuses(e->getAbsoluteTime(),
			  e->getAbsoluteTime() + e->getDuration());
    return i;
}


void
Segment::updateEndTime()
{
    m_endTime = m_startTime;
    for (iterator i = begin(); i != end(); ++i) {
	timeT t = (*i)->getAbsoluteTime() + (*i)->getDuration();
	if (t > m_endTime) m_endTime = t;
    }
}


void
Segment::erase(iterator pos)
{
    Event *e = *pos;

    assert(e);

    timeT t0 = e->getAbsoluteTime();
    timeT t1 = t0 + e->getDuration();

    std::multiset<Event*, Event::EventCmp>::erase(pos);
    notifyRemove(e);
    delete e;
    updateRefreshStatuses(t0, t1);

    if (t0 == m_startTime && begin() != end()) {
	timeT startTime = (*begin())->getAbsoluteTime();
        if (m_composition) m_composition->setSegmentStartTime(this, startTime);
	else m_startTime = startTime;
    }
    if (t1 == m_endTime) {
	updateEndTime();
    }
}


void
Segment::erase(iterator from, iterator to)
{
    timeT startTime = 0, endTime = 0;
    if (from != end()) startTime = (*from)->getAbsoluteTime();
    if (to != end()) endTime = (*to)->getAbsoluteTime() + (*to)->getDuration();

    // Not very efficient, but without an observer event for
    // multiple erase we can't do any better.

    for (Segment::iterator i = from; i != to; ) {

	Segment::iterator j(i);
        ++j;

	Event *e = *i;
	assert(e);

	std::multiset<Event*, Event::EventCmp>::erase(i);
	notifyRemove(e);
	delete e;

	i = j;
    }

    if (startTime == m_startTime && begin() != end()) {
	timeT startTime = (*begin())->getAbsoluteTime();
        if (m_composition) m_composition->setSegmentStartTime(this, startTime);
	else m_startTime = startTime;
    }

    if (endTime == m_endTime) {
	updateEndTime();
    }

    updateRefreshStatuses(startTime, endTime);
}


bool
Segment::eraseSingle(Event* e)
{
    iterator elPos = findSingle(e);

    if (elPos != end()) {

        erase(elPos);
        return true;
            
    } else return false;
    
}


Segment::iterator
Segment::findSingle(Event* e) const
{
    iterator res = end();

    std::pair<iterator, iterator> interval = equal_range(e);

    for(iterator i = interval.first; i != interval.second; ++i) {
        if (*i == e) {
            res = i;
            break;
        }
    }
    return res;
}


Segment::iterator
Segment::findTime(timeT t) const
{
    Event dummy("dummy", t, 0, MIN_SUBORDERING);
    return lower_bound(&dummy);
}


Segment::iterator
Segment::findNearestTime(timeT t) const
{
    iterator i = findTime(t);
    if (i == end() || (*i)->getAbsoluteTime() > t) {
	if (i == begin()) return end();
	else --i;
    }
    return i;
}


timeT
Segment::getBarStartForTime(timeT t) const
{
    if (t < getStartTime()) t = getStartTime();
    return getComposition()->getBarStartForTime(t);
}


timeT
Segment::getBarEndForTime(timeT t) const
{
    if (t > getEndMarkerTime()) t = getEndMarkerTime();
    return getComposition()->getBarEndForTime(t);
}


int Segment::getNextId() const
{
    return m_id++;
}


void
Segment::fillWithRests(timeT endTime)
{
    fillWithRests(getEndTime(), endTime);
}

void
Segment::fillWithRests(timeT startTime, timeT endTime)
{
    if (startTime < m_startTime) {
        if (m_composition) m_composition->setSegmentStartTime(this, startTime);
	else m_startTime = startTime;
    }

    TimeSignature ts;
    timeT sigTime = 0;

    if (getComposition()) {
	sigTime = getComposition()->getTimeSignatureAt(startTime, ts);
    }

    timeT restDuration = endTime - startTime;
    if (restDuration <= 0) return;

/*
    cerr << "Segment(" << this << ")::fillWithRests: endTime "
	 << endTime << ", startTime " << startTime << ", composition "
	 << (getComposition() ? "exists" : "does not exist") << ", sigTime "
	 << sigTime << ", timeSig duration " << ts.getBarDuration() << ", restDuration " << restDuration << endl;
*/

    DurationList dl;
    ts.getDurationListForInterval(dl, restDuration, startTime - sigTime);

    timeT acc = startTime;

    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {
	Event *e = new Event(Note::EventRestType, acc, *i,
			     Note::EventRestSubOrdering);
	insert(e);
	acc += *i;
    }
}

void
Segment::normalizeRests(timeT startTime, timeT endTime)
{
    Profiler profiler("Segment::normalizeRests");

    if (startTime < m_startTime) {
        if (m_composition) m_composition->setSegmentStartTime(this, startTime);
	else m_startTime = startTime;
    }

    //!!! Need to remove the rests then relocate the start time
    // and get the notation end time for the nearest note before that
    // (?)

    //!!! We need to insert rests at fictitious unquantized times that
    //are broadly correct, so as to maintain ordering of notes and
    //rests in the unquantized segment.  The quantized times should go
    //in notation-prefix properties.

    // Preliminary: If there are any time signature changes between
    // the start and end times, consider separately each of the sections
    // they divide the range up into.

    Composition *composition = getComposition();
    if (composition) {
	int timeSigNo = composition->getTimeSignatureNumberAt(startTime);
	if (timeSigNo < composition->getTimeSignatureCount() - 1) {
	    timeT nextSigTime =
		composition->getTimeSignatureChange(timeSigNo + 1).first;
	    if (nextSigTime < endTime) {
		normalizeRests(startTime, nextSigTime);
		normalizeRests(nextSigTime, endTime);
		return;
	    }
	}
    }

    // First stage: erase all existing non-tupleted rests in this range.

    /*
    cerr << "Segment::normalizeRests " << startTime << " -> "
	 << endTime << endl;
         */

    timeT segmentEndTime = m_endTime;

    iterator ia = findNearestTime(startTime);
    if (ia == end()) ia = begin();
    if (ia == end()) { // the segment is empty
	fillWithRests(endTime);
	return;
    }

    iterator ib = findTime(endTime);
    if (ib == end()) {
	if (ib != begin()) {
	    --ib;
	    // if we're pointing at the real-end-time of the last event,
	    // use its notation-end-time instead
	    if (endTime == (*ib)->getAbsoluteTime() + (*ib)->getDuration()) {
		endTime =
		    (*ib)->getNotationAbsoluteTime() +
		    (*ib)->getNotationDuration();
	    }
	    ++ib;
	}
    } else {
	endTime = (*ib)->getNotationAbsoluteTime();
    }

    // If there's a rest preceding the start time, with no notes
    // between us and it, and if it doesn't have precisely the
    // right duration, then we need to normalize it too

    //!!! needs modification for new scheme

    iterator scooter = ia;
    while (scooter-- != begin()) {
//	if ((*scooter)->isa(Note::EventRestType)) { //!!! experimental
	if ((*scooter)->getDuration() > 0) { 
	    if ((*scooter)->getAbsoluteTime() + (*scooter)->getDuration() !=
		startTime) {
		startTime = (*scooter)->getAbsoluteTime();
//		cerr << "Scooting back to " << startTime << endl;
		ia = scooter;
	    }
	    break;
/*!!!
	} else if ((*scooter)->getDuration() > 0) {
	    break;
*/
	}
    }

    for (iterator i = ia, j = i; i != ib && i != end(); i = j) {
	++j;
	if ((*i)->isa(Note::EventRestType) &&
	    !(*i)->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE)) {
	    erase(i);
	}
    }

    // It's possible we've just removed all the events between here
    // and the end of the segment, if they were all rests.  Check.

    if (endTime < segmentEndTime && m_endTime < segmentEndTime) {
	endTime = segmentEndTime;
    }

    // Second stage: find the gaps that need to be filled with
    // rests.  We don't mind about the case where two simultaneous
    // notes end at different times -- we're only interested in
    // the one ending sooner.  Each time an event ends, we start
    // a candidate gap.

    std::vector<std::pair<timeT, timeT> > gaps;

    timeT lastNoteStarts = startTime;
    timeT lastNoteEnds = startTime;
    
    // Re-find this, as it might have been erased
    ia = findNearestTime(startTime);

    if (ia == end()) {
	// already have good lastNoteStarts, lastNoteEnds
	ia = begin();
    } else {
	lastNoteStarts = (*ia)->getNotationAbsoluteTime();
	lastNoteEnds = lastNoteStarts;
    }

    if (ib != end()) {
	//!!! This and related code really need to get a quantized
	// absolute time of a note event that has the same unquantized
	// time as ib, not necessarily of ib itself... or else the
	// quantizer needs to set the quantized times of all non-note
	// events that happen at the same unquantized time as a note
	// event to the same as that of the note event... yeah, that's
	// probably the right thing
	endTime = (*ib)->getNotationAbsoluteTime();

	// was this just a nasty hack?
	++ib;
    }

    iterator i = ia;

    for (; i != ib && i != end(); ++i) {

	// if we have any rests remaining in this area, treat them
	// as "hard" rests (they had tuplet data, so we don't want to
	// disturb them)
	if (!((*i)->isa(Note::EventType) || (*i)->isa(Note::EventRestType))) {
	    continue;
	}

	timeT thisNoteStarts = (*i)->getNotationAbsoluteTime();

        /*
	cerr << "scanning: thisNoteStarts " << thisNoteStarts
	     << ", lastNoteStarts " << lastNoteStarts
	     << ", lastNoteEnds " << lastNoteEnds << endl;
             */

	if (thisNoteStarts < lastNoteEnds &&
	    thisNoteStarts > lastNoteStarts) { //!!! experimental
	    gaps.push_back(std::pair<timeT, timeT>
			   (lastNoteStarts,
			    thisNoteStarts - lastNoteStarts));
	}

	if (thisNoteStarts > lastNoteEnds) {
	    gaps.push_back(std::pair<timeT, timeT>
			   (lastNoteEnds,
			    thisNoteStarts - lastNoteEnds));
	}

	lastNoteStarts = thisNoteStarts;
	lastNoteEnds = thisNoteStarts + (*i)->getNotationDuration();
    }

    if (endTime > lastNoteEnds) {
	gaps.push_back(std::pair<timeT, timeT>
		       (lastNoteEnds, endTime - lastNoteEnds));
    }

    timeT duration;

    for (unsigned int gi = 0; gi < gaps.size(); ++gi) {

	//cerr << "gap " << gi << ": " << gaps[gi].first << " -> " << gaps[gi].second << endl;

        startTime = gaps[gi].first;
	duration = gaps[gi].second;

	fillWithRests(startTime, startTime + duration);
    }
}



void Segment::getTimeSlice(timeT absoluteTime, iterator &start, iterator &end)
    const
{
    Event dummy("dummy", absoluteTime, 0, MIN_SUBORDERING);

    // No, this won't work -- we need to include things that don't
    // compare equal because they have different suborderings, as long
    // as they have the same times
  
//    std::pair<iterator, iterator> res = equal_range(&dummy);

//    start = res.first;
//    end = res.second;

    // Got to do this instead:

    start = end = lower_bound(&dummy);

    while (end != this->end() &&
	   (*end)->getAbsoluteTime() == (*start)->getAbsoluteTime())
	++end;
}

void
Segment::setQuantization(bool quantize)
{
    if (m_quantize != quantize) {
	m_quantize = quantize;
	if (m_quantize) {
	    m_quantizer->quantize(this, begin(), end());
	} else {
	    m_quantizer->unquantize(this, begin(), end());
	}
    }
}

bool
Segment::hasQuantization() const
{
    return m_quantize;
}

void
Segment::setQuantizeLevel(timeT unit)
{
    if (m_quantizer->getUnit() == unit) return;

    m_quantizer->setUnit(unit);
    if (m_quantize) m_quantizer->quantize(this, begin(), end());
}

const BasicQuantizer *const
Segment::getQuantizer() const
{
    return m_quantizer;
}


void
Segment::setRepeating(bool value)
{
    m_repeating = value;
    if (m_composition) {
        m_composition->updateRefreshStatuses();
        m_composition->notifySegmentRepeatChanged(this, value);
    }
}

void
Segment::setLabel(const std::string &label)
{
    m_label = label; 
    if (m_composition) m_composition->updateRefreshStatuses();
}

bool
Segment::ClefKeyCmp::operator()(const Event *e1, const Event *e2) const
{
    if (e1->getType() == e2->getType()) return Event::EventCmp()(e1, e2);
    else return e1->getType() < e2->getType();
}

Clef
Segment::getClefAtTime(timeT time) const
{
    if (!m_clefKeyList) return Clef();

    Event ec(Clef::EventType, time);
    ClefKeyList::iterator i = m_clefKeyList->lower_bound(&ec);

    while (i == m_clefKeyList->end() ||
	   (*i)->getAbsoluteTime() > time ||
	   (*i)->getType() != Clef::EventType) {

	if (i == m_clefKeyList->begin()) return Clef();
	--i;
    }

    try {
	return Clef(**i);
    } catch (const Exception &e) {
	std::cerr << "Segment::getClefAtTime(" << time
		  << "): bogus clef in ClefKeyList: event dump follows:"
		  << std::endl;
	(*i)->dump(std::cerr);
	return Clef();
    }
}

Key
Segment::getKeyAtTime(timeT time) const
{
    if (!m_clefKeyList) return Key();

    Event ek(Key::EventType, time);
    ClefKeyList::iterator i = m_clefKeyList->lower_bound(&ek);

    while (i == m_clefKeyList->end() ||
	   (*i)->getAbsoluteTime() > time ||
	   (*i)->getType() != Key::EventType) {

	if (i == m_clefKeyList->begin()) return Key();
	--i;
    }

    try {
	return Key(**i);
    } catch (const Exception &e) {
	std::cerr << "Segment::getClefAtTime(" << time
		  << "): bogus key in ClefKeyList: event dump follows:"
		  << std::endl;
	(*i)->dump(std::cerr);
	return Key();
    }
}

timeT
Segment::getRepeatEndTime() const
{
    if (m_repeating && m_composition) {
	Composition::iterator i(m_composition->findSegment(this));
	assert(i != m_composition->end());
	++i;
	if (i != m_composition->end() && (*i)->getTrack() == getTrack()) {
	    return (*i)->getStartTime();
	} else {
            return m_composition->getEndMarker();
	}
    }
    return getEndMarkerTime();
}


void
Segment::notifyAdd(Event *e) const
{
    if (e->isa(Clef::EventType) || e->isa(Key::EventType)) {
	if (!m_clefKeyList) m_clefKeyList = new ClefKeyList;
	m_clefKeyList->insert(e);
    }

    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventAdded(this, e);
    }
}

 
void
Segment::notifyRemove(Event *e) const
{
    if (m_clefKeyList && (e->isa(Clef::EventType) || e->isa(Key::EventType))) {
	ClefKeyList::iterator i = m_clefKeyList->find(e);
	if (i != m_clefKeyList->end()) {
	    m_clefKeyList->erase(i);
	}
    }
    
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventRemoved(this, e);
    }
}
 

void
Segment::notifyEndMarkerChange(bool shorten) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->endMarkerTimeChanged(this, shorten);
    }
}


void
Segment::notifySourceDeletion() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->segmentDeleted(this);
    }
}


void
Segment::setColourIndex(const unsigned int input)
{
    m_colourIndex = input;
    updateRefreshStatuses(getStartTime(), getEndTime());
    if (m_composition) m_composition->updateRefreshStatuses();
}


SegmentHelper::~SegmentHelper() { }


void 
SegmentRefreshStatus::push(timeT from, timeT to)
{
    if (!needsRefresh()) { // don't do anything subtle - just erase the old data

        m_from = from;
        m_to = to;

    } else { // accumulate on what was already there

        if (from < m_from) m_from = from;
        if (to > m_to) m_to = to;

    }

    if (m_to < m_from) std::swap(m_from, m_to);

    setNeedsRefresh(true);
}



 
}
