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
#include "Track.h"
#include "FastVector.h"

#include <iostream>


namespace Rosegarden 
{

const std::string Composition::BarEventType = "bar";


Composition::Composition()
    : m_timeReference(0),
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

    m_timeReference.swap(c.m_timeReference);

    m_segments.swap(c.m_segments);

    for (segmentcontainer::iterator i = that->m_segments.begin();
	 i != that->m_segments.end(); ++i) {
	(*i)->removeObserver(this);
	(*i)->addObserver(that);
	(*i)->setReferenceSegment(&that->m_timeReference);
    }

    for (segmentcontainer::iterator i = this->m_segments.begin();
	 i != this->m_segments.end(); ++i) {
	(*i)->removeObserver(that);
	(*i)->addObserver(this);
	(*i)->setReferenceSegment(&this->m_timeReference);
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
    segment->setReferenceSegment(&m_timeReference);
    segment->setQuantizer(getQuantizer());

    return res.first;
}

void
Composition::deleteSegment(Composition::iterator i)
{
    if (i == end()) return;

    Segment *p = (*i);
    p->removeObserver(this);
    p->setReferenceSegment(0);

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
	(*i)->setReferenceSegment(0);
        delete (*i);
    }
    m_segments.erase(begin(), end());
    m_timeReference.erase(m_timeReference.begin(), m_timeReference.end());
}

Segment::iterator
Composition::addNewBar(timeT time)
{
//    std::cerr << "Composition::addNewBar" << std::endl;
    Event *e = new Event(BarEventType);
    e->setAbsoluteTime(time);
    return m_timeReference.insert(e);
}

void
Composition::calculateBarPositions()
{
//    std::cerr << "Composition::calculateBarPositions" << std::endl;

    if (!m_barPositionsNeedCalculating) return;

    Segment &t = m_timeReference;
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

    std::cerr << "have " << t.size() << " non-bars in ref segment" << std::endl;

    bool segment0isTimeSig = true;
    if (segments.size() == 0 || segments[0] != 0) {
	segments.push_front(0);
	segmentTimes.push_front(TimeSignature().getBarDuration());
	segment0isTimeSig = false;
    }    

    timeT duration = getDuration();
    segments.push_back(duration);

    for (int s = 0; s < segments.size() - 1; ++s) {

	timeT start = segments[s], finish = segments[s+1];
	timeT time;

	if (s > 0 || segment0isTimeSig) start += segmentTimes[s];

	std::cerr << "segment " << s << ": start " << start << ", finish " << finish << std::endl;

	for (time = start; time < finish; time += segmentTimes[s]) {
	    addNewBar(time);
            std::cerr << "added bar at " << time << std::endl;
	}

	if (s == segments.size() - 1 && time != duration) {
            addNewBar(time);
            std::cerr << "added final bar at " << time << std::endl;
        }            
    }

    m_barPositionsNeedCalculating = false;
    std::cerr << "Composition::calculateBarPositions ending" << std::endl;

    std::cerr << "Reference segment contains:" << std::endl;
    for (i = t.begin(); i != t.end(); ++i) {
	(*i)->dump(std::cerr);
    }
}

int
Composition::getBarNumber(timeT t)
{
    calculateBarPositions();

    Segment::iterator i = m_timeReference.findTime(t);
    return distance(m_timeReference.begin(), i);
}

timeT
Composition::getBarStart(timeT t)
{
    calculateBarPositions();

    Segment::iterator i = m_timeReference.findTime(t);
    if (i != m_timeReference.begin()) --i;
    if (i == m_timeReference.end()) return 0;

    return (*i)->getAbsoluteTime();
}

timeT
Composition::getBarEnd(timeT t)
{
    calculateBarPositions();

    Segment::iterator i = m_timeReference.findTime(t);
    if (i == m_timeReference.end() || ++i == m_timeReference.end()) {
        return m_timeReference.getDuration();
    }
    return (*i)->getAbsoluteTime();
}


std::pair<timeT, timeT>
Composition::getBarRange(int n, bool truncate)
{
    calculateBarPositions();

    Segment::iterator i = m_timeReference.begin();

    timeT start = 0, finish = start;
    
//    cerr << "Composition::getBarRange(" << n << ", " << truncate << ")" << endl;

    TimeSignature timeSignature;

    int m = 0;

    if (i != m_timeReference.end()) {

	for (; m <= n; ++m) {

	    start = (*i)->getAbsoluteTime();

//	    cerr << "Composition::getBarRange: looking at time " << start << endl;

	    if ((*i)->isa(TimeSignature::EventType)) {
		timeSignature = TimeSignature(**i);
	    }

	    finish = start + timeSignature.getBarDuration();
	    ++i;
	    if (i == m_timeReference.end()) break;
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


void Composition::eventAdded(const Segment *t, Event *e)
{
    // in theory this should only be true if we insert something after
    // the former end of the composition -- or add a time sig to the
    // reference segment
    m_barPositionsNeedCalculating = true;
}


void Composition::eventRemoved(const Segment *t, Event *e)
{
    // in theory this should only be true if we remove something at
    // the very end of the composition -- or remove a time sig from
    // the reference segment
    m_barPositionsNeedCalculating = true;
}


void Composition::referenceSegmentRequested(const Segment *)
{
    calculateBarPositions();
}


}
