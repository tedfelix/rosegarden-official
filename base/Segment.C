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

#include "Segment.h"
#include "NotationTypes.h"
#include "Quantizer.h"
#include "BaseProperties.h"
#include "Composition.h"

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
    m_track(0),
    m_type(segmentType),
    m_label(getNewUntitledLabel()),
    m_endMarkerTime(0),
    m_id(0),
    m_audioFileID(0),
    m_audioStartIdx(0),
    m_audioEndIdx(0),
    m_repeating(false),
    m_quantizer(new Quantizer("SegmentQ", Quantizer::RawEventData)),
    m_quantize(false),
    m_transpose(0),
    m_delay(0)
{
}

Segment::Segment(const Segment &segment):
    std::multiset<Event*, Event::EventCmp>(),
    m_composition(0), // Composition should decide what's in it and what's not
    m_startTime(segment.getStartTime()),
    m_track(segment.getTrack()),
    m_type(segment.getType()),
    m_label(segment.getLabel()),
    m_endMarkerTime(segment.m_endMarkerTime ?
		    new timeT(*segment.m_endMarkerTime) : 0),
    m_id(0),
    m_audioFileID(segment.getAudioFileID()),
    m_audioStartIdx(segment.getAudioStartTime()),
    m_audioEndIdx(segment.getAudioEndTime()),
    m_repeating(segment.isRepeating()),
    m_quantizer(new Quantizer(segment.getQuantizer())),
    m_quantize(segment.hasQuantization()),
    m_transpose(segment.getTranspose()),
    m_delay(segment.getDelay())
{
    for (iterator it = segment.begin(); it != segment.end(); ++it)
        insert(new Event(**it));
}


Segment::~Segment()
{
    // delete content
    for (iterator it = begin(); it != end(); ++it) delete (*it);

    delete m_endMarkerTime;
    delete m_quantizer;
}

std::string Segment::getNewUntitledLabel()
{
    std::string label = "untitled ";

    static unsigned int untitledSegCount = 0;
    char tmp[256];
    sprintf(tmp, " #%d", untitledSegCount);
    label += tmp;
    ++untitledSegCount;

    return label;
}

void Segment::setStartTime(timeT idx)
{
    int idxDiff = idx - m_startTime;

    // reset the time of all events.  can't just setAbsoluteTime on these,
    // partly 'cos we're not allowed, partly 'cos it might screw up the
    // quantizer (which is why we're not allowed)

    // still, this is rather unsatisfactory
    
    FastVector<Event *> events;

    for (iterator i = begin(); i != end(); ++i) {
	events.push_back(new Event(**i, (*i)->getAbsoluteTime() + idxDiff));
    }

    erase(begin(), end());

    for (int i = 0; i < events.size(); ++i) {
	insert(events[i]);
    }
        
    m_startTime = idx;
}


timeT Segment::getFirstEventTime() const
{
    iterator i = begin();
    if (i == end()) return getEndTime();
    return (*i)->getAbsoluteTime();
}


void Segment::recalculateStartTime()
{
    m_startTime = getFirstEventTime();
}


timeT Segment::getDuration() const
{
    //!!! hmm. return "real" end time (with another method for end
    // marker time) or return end marker time? consider the various
    // uses of this method in this file and elsewhere

    const_iterator lastEl = end();
    if (lastEl == begin()) return 0;
    --lastEl;

    return (*lastEl)->getAbsoluteTime() +
           (*lastEl)->getDuration() -
           getStartTime();
}


void Segment::setDuration(timeT d)
{
    timeT currentDuration = getDuration();

    cerr << "Segment::setDuration() : current = " << currentDuration
         << " - new : " << d << endl;
    
    if (d > currentDuration) {

	delete m_endMarkerTime;
	m_endMarkerTime = 0;
	fillWithRests(getStartTime() + d);

    } else { // shrink

	if (!m_endMarkerTime) m_endMarkerTime = new timeT;
	*m_endMarkerTime = d;
    }
}

timeT Segment::getEndMarker() const
{
    timeT endTime = getEndTime();
    if (m_endMarkerTime)
	endTime = std::min(endTime, *m_endMarkerTime);
    if (m_composition)
	endTime = std::min(endTime, m_composition->getEndMarker());
    return endTime;
}

void Segment::erase(iterator pos)
{
    Event *e = *pos;

    assert(e);

    std::multiset<Event*, Event::EventCmp>::erase(pos);
    notifyRemove(e);
    updateRefreshStatuses(e->getAbsoluteTime(),
			  e->getAbsoluteTime() + e->getDuration());
    delete e;
}

void Segment::updateRefreshStatuses(timeT startTime, timeT endTime)
{
    for(unsigned int i = 0; i < m_refreshStatusArray.size(); ++i)
        m_refreshStatusArray.getRefreshStatus(i).push(startTime, endTime);
}


Segment::iterator Segment::insert(Event *e)
{
    assert(e);

    iterator i = std::multiset<Event*, Event::EventCmp>::insert(e);
    notifyAdd(e);
    updateRefreshStatuses(e->getAbsoluteTime(),
			  e->getAbsoluteTime() + e->getDuration());
    return i;
}


void Segment::erase(iterator from, iterator to)
{
    timeT startTime = 0, endTime = 0;
    if (from != end()) startTime = (*from)->getAbsoluteTime();
    if (to != end()) endTime = (*to)->getAbsoluteTime();

    // Not very efficient, but without an observer event for
    // multiple erase we can't do any better.

    // We can't do this :
    //
    // for (Segment::iterator i = from; i != to; ++i) { ... erase(i); }
    //
    // because erasing an iterator invalidates it.

    // So, gather the events which we'll have to delete
    // call notifyRemove() for each of them,
    // and finally call std::multiset<>::erase(from, to)

    for (Segment::iterator i = from; i != to; ++i) {
        Event *e = *i;
        notifyRemove(e);
        delete e;
    }
    
    std::multiset<Event*, Event::EventCmp>::erase(from, to);

    updateRefreshStatuses(startTime, endTime);
}


bool Segment::eraseSingle(Event* e)
{
    iterator elPos = findSingle(e);

    if (elPos != end()) {

        erase(elPos);
        return true;
            
    } else return false;
    
}


Segment::iterator Segment::findSingle(Event* e) const
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


Segment::iterator Segment::findTime(timeT t) const
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
    if (t > getEndTime()) t = getEndTime();
    return getComposition()->getBarEndForTime(t);
}


int Segment::getNextId() const
{
    return m_id++;
}


void Segment::fillWithRests(timeT endTime, bool permitQuantize)
{
    fillWithRests(getEndTime(), endTime, permitQuantize);
}

void Segment::fillWithRests(timeT startTime,
			    timeT endTime, bool permitQuantize)
{
    TimeSignature ts;
    timeT sigTime = 0;

    if (getComposition()) {
	sigTime = getComposition()->getTimeSignatureAt(startTime, ts);
    }

    timeT restDuration = endTime - startTime;

    if (permitQuantize) {
	restDuration =
	    (getComposition() ? 
	     getComposition()->getBasicQuantizer()->quantizeDuration(restDuration) :
	     Quantizer().quantizeDuration(restDuration));
    }
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
	Event *e = new Event(Note::EventRestType, acc, *i);
	insert(e);
	acc += *i;
    }
}

void
Segment::normalizeRests(timeT startTime, timeT endTime, bool permitQuantize)
{
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

    // First stage: erase all existing rests in this range.

//    cerr << "Segment::normalizeRests " << startTime << " -> "
//	 << endTime << endl;

    iterator ia = findNearestTime(startTime);
    if (ia == end()) ia = begin();
    if (ia == end()) { // the segment is empty
	fillWithRests(endTime);
	return;
    }

    iterator ib = findTime(endTime);
    if (ib != end()) endTime = (*ib)->getAbsoluteTime();

    // If there's a rest preceding the start time, with no notes
    // between us and it, and if it doesn't have precisely the
    // right duration, then we need to normalize it too
    iterator scooter = ia;
    while (scooter-- != begin()) {
	if ((*scooter)->isa(Note::EventRestType)) {
	    if ((*scooter)->getAbsoluteTime() + (*scooter)->getDuration() !=
		startTime) {
		startTime = (*scooter)->getAbsoluteTime();
//		cerr << "Scooting back to " << startTime << endl;
		ia = scooter;
	    }
	    break;
	} else if ((*scooter)->getDuration() > 0) {
	    break;
	}
    }

    for (iterator i = ia, j = i; i != ib && i != end(); i = j) {
	++j;
	if ((*i)->isa(Note::EventRestType)) erase(i);
    }

    // Second stage: find the gaps that need to be filled with
    // rests.  We don't mind about the case where two simultaneous
    // notes end at different times -- we're only interested in
    // the one ending sooner.  Each time an event ends, we start
    // a candidate gap.
    
    // Re-find this, as it might have been erased
    ia = findTime(startTime);
    if (ib != end()) ++ib;
    
    std::vector<std::pair<timeT, timeT> > gaps;
    timeT lastNoteEnds = startTime;
    iterator i = ia;

    for (; i != ib && i != end(); ++i) {

	if (!(*i)->isa(Note::EventType)) continue;

	timeT thisNoteStarts = (*i)->getAbsoluteTime();

	if (thisNoteStarts > lastNoteEnds) {
	    gaps.push_back(std::pair<timeT, timeT>
			   (lastNoteEnds,
			    thisNoteStarts - lastNoteEnds));
	}
	lastNoteEnds = thisNoteStarts + (*i)->getDuration();
    }

    if (endTime > lastNoteEnds) {
	gaps.push_back(std::pair<timeT, timeT>
		       (lastNoteEnds, endTime - lastNoteEnds));
    }

    timeT duration;

    for (unsigned int gi = 0; gi < gaps.size(); ++gi) {

        startTime = gaps[gi].first;
	duration = gaps[gi].second;

	fillWithRests(startTime, startTime + duration, permitQuantize);
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
Segment::setQuantizeLevel(const StandardQuantization &q)
{
    Quantizer newQ(q, "SegmentQ", Quantizer::RawEventData);

    if (newQ != *m_quantizer) {
	*m_quantizer = newQ;
	if (m_quantize) m_quantizer->quantize(this, begin(), end());
    }
}

void
Segment::setQuantizeLevel(const Quantizer &q)
{
    Quantizer newQ(q, "SegmentQ", Quantizer::RawEventData);

    if (newQ != *m_quantizer) {
	*m_quantizer = newQ;
	if (m_quantize) m_quantizer->quantize(this, begin(), end());
    }
}

const Quantizer &
Segment::getQuantizer() const
{
    return *m_quantizer;
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
	    return m_composition->getDuration();
	}
    }
    return getEndTime();
}


void Segment::notifyAdd(Event *e) const
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventAdded(this, e);
    }
}

 
void Segment::notifyRemove(Event *e) const
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventRemoved(this, e);
    }
}



SegmentHelper::~SegmentHelper() { }


void SegmentRefreshStatus::push(timeT from, timeT to)
{
    if (!needsRefresh()) { // don't do anything subtle - just erase the old data

        m_from = from;
        m_to = to;

    } else { // accumulate on what was already there

        if (from < m_from) m_from = from;
        if (to > m_to) m_to = to;

    }

    if (m_to < m_from) m_from = m_to;

    setNeedsRefresh(true);
}



 
}
