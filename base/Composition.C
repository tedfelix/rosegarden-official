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

#include "Composition.h"
#include "Segment.h"
#include "FastVector.h"

#include <iostream>


namespace Rosegarden 
{

const std::string Composition::BarEventType = "bar";
const PropertyName Composition::BarNumberProperty = "BarNumber";


Composition::Composition()
    : m_referenceSegment(0),
      m_tempo(120),
      m_position(0),
      m_barPositionsNeedCalculating(true)
{
    // empty
}

Composition::~Composition()
{
    clear();
}

void Composition::swap(Composition& c)
{
    // ugh.

    Composition *that = &c;

    double tp = this->m_tempo;
    this->m_tempo = that->m_tempo;
    that->m_tempo = tp;

    m_referenceSegment.swap(c.m_referenceSegment);

    m_segments.swap(c.m_segments);

    for (segmentcontainer::iterator i = that->m_segments.begin();
	 i != that->m_segments.end(); ++i) {
	(*i)->removeObserver(this);
	(*i)->addObserver(that);
	(*i)->setComposition(that);
    }

    for (segmentcontainer::iterator i = this->m_segments.begin();
	 i != this->m_segments.end(); ++i) {
	(*i)->removeObserver(that);
	(*i)->addObserver(this);
	(*i)->setComposition(this);
    }

    this->m_barPositionsNeedCalculating = true;
    that->m_barPositionsNeedCalculating = true;
    
}

Composition::iterator
Composition::addSegment(Segment *segment)
{
    if (!segment) return end();
    
    std::pair<iterator, bool> res = m_segments.insert(segment);
    segment->addObserver(this);
    segment->setComposition(this);

    return res.first;
}

void
Composition::deleteSegment(Composition::iterator i)
{
    if (i == end()) return;

    Segment *p = (*i);
    p->removeObserver(this);
    p->setComposition(0);

    delete p;
    m_segments.erase(i);
}

bool
Composition::deleteSegment(Segment *p)
{
    iterator i = find(begin(), end(), p);
    if (i == end()) return false;
    
    deleteSegment(i);
    return true;
}

timeT
Composition::getDuration() const
{
    unsigned int maxDuration = 0;

    for (segmentcontainer::const_iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

        unsigned int segmentTotal = (*i)->getDuration() + (*i)->getStartIndex();
        
        if (segmentTotal > maxDuration) {
            maxDuration = segmentTotal;
        }
    }
    
    return maxDuration;
}

void
Composition::clear()
{
    for (segmentcontainer::iterator i = m_segments.begin();
        i != m_segments.end(); ++i) {
        (*i)->removeObserver(this);
	(*i)->setComposition(0);
        delete (*i);
    }
    m_segments.erase(begin(), end());
    m_referenceSegment.erase(m_referenceSegment.begin(), m_referenceSegment.end());
}

Segment::iterator
Composition::addNewBar(timeT time, int barNo) const
{
//    std::cerr << "Composition::addNewBar" << std::endl;
    Event *e = new Event(BarEventType);
    e->setAbsoluteTime(time);
    e->set<Int>(BarNumberProperty, barNo);
    return m_referenceSegment.insert(e);
}

void
Composition::calculateBarPositions() const
{
//    std::cerr << "Composition::calculateBarPositions" << std::endl;

    if (!m_barPositionsNeedCalculating) return;

    Segment &t = m_referenceSegment;
    Segment::iterator i;

    FastVector<timeT> segments;
    FastVector<timeT> segmentTimes;

    FastVector<Segment::iterator> baritrs;

    for (i = t.begin(); i != t.end(); ++i) {
	if ((*i)->isa(BarEventType)) baritrs.push_back(i);
	else {
	    segments.push_back((*i)->getAbsoluteTime());
	    segmentTimes.push_back(TimeSignature(**i).getBarDuration());
	}
    }

    // Take out the old bar events, 'cos we're about to recalculate them
    for (int j = 0; j < baritrs.size(); ++j) {
	t.erase(baritrs[j]);
    }

//    std::cerr << "have " << t.size() << " non-bars in ref segment" << std::endl;

    bool segment0isTimeSig = true;
    if (segments.size() == 0 || segments[0] != 0) {
	segments.push_front(0);
	segmentTimes.push_front(TimeSignature().getBarDuration());
	segment0isTimeSig = false;
    }    

    timeT duration = getDuration();
    segments.push_back(duration);

    int barNo = 0;

    for (int s = 0; s < segments.size() - 1; ++s) {

	timeT start = segments[s], finish = segments[s+1];
	timeT time;

	if (s > 0 || segment0isTimeSig) {
	    start += segmentTimes[s];
	    ++barNo;
	}

//	std::cerr << "segment " << s << ": start " << start << ", finish " << finish << std::endl;

	for (time = start; time < finish; time += segmentTimes[s]) {
	    addNewBar(time, barNo++);
//            std::cerr << "added bar at " << time << std::endl;
	}

	if (s == segments.size() - 1 && time != duration) {
            addNewBar(time, barNo++);
//            std::cerr << "added final bar at " << time << std::endl;
        }            
    }

    m_barPositionsNeedCalculating = false;
//    std::cerr << "Composition::calculateBarPositions ending" << std::endl;

//    std::cerr << "Reference segment contains:" << std::endl;
//    for (i = t.begin(); i != t.end(); ++i) {
//	(*i)->dump(std::cerr);
//    }
}

int
Composition::getBarNumber(timeT t, bool truncate) const
{
    calculateBarPositions();

//    std::cerr << "Composition::getBarNumber: time is " << t
//	      << " (my duration is " << getDuration() << ")" << endl;

    Segment::iterator i = m_referenceSegment.findTime(t);
    long n = 0;

    if (i == m_referenceSegment.end()) {

	n = m_referenceSegment.size() - 1;

	if (!truncate) {
	    TimeSignature sig = getTimeSignatureAt(t);
	    n += (t - m_referenceSegment.getDuration()) / sig.getBarDuration();
	}
	    
    } else {

	int slack = 0;
	if ((*i)->getAbsoluteTime() != t) slack = -1;

	while (!(*i)->isa(BarEventType)) {
	    if (i == m_referenceSegment.begin()) break;
	    ++slack;
	    --i;
	}

	(*i)->get<Int>(BarNumberProperty, n);
	n += slack;
    }

//    std::cerr << "Composition::getBarNumber: returning " << n << endl;

    return (int)n;
    
//    int n = distance(m_referenceSegment.begin(), i);
//    if (i != m_referenceSegment.end() && (*i)->getAbsoluteTime() == t) return n;
//    else if (n > 0) return n - 1;
//    else return n;
}

timeT
Composition::getBarStart(timeT t) const
{
    calculateBarPositions();

    Segment::iterator i = m_referenceSegment.findTime(t);
    if (i != m_referenceSegment.end() && (*i)->getAbsoluteTime() == t) return t;
    if (i != m_referenceSegment.begin()) --i;
    if (i == m_referenceSegment.end()) return 0;

    return (*i)->getAbsoluteTime();
}

timeT
Composition::getBarEnd(timeT t) const
{
    calculateBarPositions();

    Segment::iterator i = m_referenceSegment.findTime(t);

//    cerr << "Composition::getBarEnd: asked for " << t << ", found an iterator at " << (i == m_referenceSegment.end() ? -1 : (*i)->getAbsoluteTime()) << endl;
//    Segment::iterator j = m_referenceSegment.end();
//    if (j != m_referenceSegment.begin()) {
//	--j;
//	cerr << "Composition::getBarEnd: final item in ref segment is at "
//	     << (*j)->getAbsoluteTime() << endl;
//    } else {
//	cerr << "Composition::getBarEnd: ref segment is empty" << endl;
//    }

    if (i == m_referenceSegment.end() || ++i == m_referenceSegment.end()) {
	timeT d = m_referenceSegment.getDuration();
	return d + getTimeSignatureAt(d).getBarDuration();
    }

    return (*i)->getAbsoluteTime();
}

std::pair<timeT, timeT>
Composition::getBarRange(timeT t) const
{
    calculateBarPositions();

    timeT start, finish;

    Segment::iterator i = m_referenceSegment.findTime(t);
    Segment::iterator j(i);
    ++j;

    if (i != m_referenceSegment.end() && (*i)->getAbsoluteTime() == t) {

	// t is itself the start of a bar
	start = t;

    } else {

	// t is in the middle of a bar, so rewind to its start
	if (i != m_referenceSegment.begin()) { --i; --j; }

	if (i == m_referenceSegment.end()) {
	    start = 0; 
	} else {
	    start = (*i)->getAbsoluteTime();
	}
    }

    if (j == m_referenceSegment.end()) {
	finish = m_referenceSegment.getDuration();
	finish += getTimeSignatureAt(finish).getBarDuration();
    } else {
	finish = (*j)->getAbsoluteTime();
    }

    return std::pair<timeT, timeT>(start, finish);
}


std::pair<timeT, timeT>
Composition::getBarRange(int n, bool truncate) const
{
    calculateBarPositions();

    Segment::iterator i = m_referenceSegment.begin();

    timeT start = 0, finish = start;
    
//    cerr << "Composition::getBarRange(" << n << ", " << truncate << ")" << endl;

    TimeSignature timeSignature;

    int m = 0;

    if (i != m_referenceSegment.end()) {

	for (; m <= n; ++m) {

	    start = (*i)->getAbsoluteTime();

//	    cerr << "Composition::getBarRange: looking at time " << start << endl;

	    if ((*i)->isa(TimeSignature::EventType)) {
		timeSignature = TimeSignature(**i);
	    }

	    finish = start + timeSignature.getBarDuration();
	    ++i;
	    if (i == m_referenceSegment.end()) break;
	}
    }

//    cerr << "Composition::getBarRange: start = " << start << ", finish = " << finish << endl;

    if (!truncate &&
	(m < n || finish == start)) { // reached end of composition too soon

//	cerr << "Composition::getBarRange: making it up (n = " << n << ", m = " << m << ")" << endl;

	timeT d = timeSignature.getBarDuration();
	if (m < n) start += d * (n - m);
	finish = start + d;
    }

//    cerr << "Composition::getBarRange: start = " << start << ", finish = " << finish << endl;

    return std::pair<timeT, timeT>(start, finish);
}

TimeSignature
Composition::getTimeSignatureAt(timeT t) const
{
    TimeSignature timeSig;
    (void)getTimeSignatureAt(t, timeSig);
    return timeSig;
}

timeT
Composition::getTimeSignatureAt(timeT t, TimeSignature &timeSig) const
{
    // We explicitly don't want to call calculateBarPositions(),
    // because we don't need the bars to be recalculated -- we only
    // want to look at the time signatures, not the bar events

    Segment::iterator i = m_referenceSegment.findTime(t);

    timeSig = TimeSignature();
    timeT sigTime = 0;

    if (i != m_referenceSegment.begin() &&
	(i == m_referenceSegment.end() ||
	 ((*i)->getAbsoluteTime() > t) ||
	 !(*i)->isa(TimeSignature::EventType))) {
	--i;
    }

    if (i == m_referenceSegment.end() ||
	!(*i)->isa(TimeSignature::EventType)) {
	return sigTime;
    }

    timeSig = TimeSignature(**i);
    return (*i)->getAbsoluteTime();
}


void Composition::eventAdded(const Segment *s, Event *e)
{
    // we only need to recalculate if we insert something after the
    // former end of the composition, or add a time sig to the
    // reference segment

    if (s == &m_referenceSegment ||
	(e->getAbsoluteTime() + e->getDuration() >
	 m_referenceSegment.getDuration())) {

	m_barPositionsNeedCalculating = true;
    }
}


void Composition::eventRemoved(const Segment *s, Event *e)
{
    // we only need to recalculate if we insert something after the
    // former end of the composition, or add a time sig to the
    // reference segment

    if (s == &m_referenceSegment ||
	(e->getAbsoluteTime() + e->getDuration() >=
	 m_referenceSegment.getDuration())) {

	m_barPositionsNeedCalculating = true;
    }
}



}
