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
#include <sstream>


namespace Rosegarden 
{

const std::string Composition::BarEventType = "bar";
const PropertyName Composition::BarNumberProperty = "BarNumber";

const std::string Composition::TempoEventType = "tempo";
const PropertyName Composition::TempoProperty = "BeatsPerHour";


Composition::Composition() :
    m_recordTrack(0),
    m_referenceSegment(0),
    m_barCount(0),
    m_position(0),
    m_currentTempo(120.0),
    m_defaultTempo(120.0),
    m_barPositionsNeedCalculating(true)
{
    // nothing
}

Composition::~Composition()
{
    clear();
}

void Composition::swap(Composition& c)
{
    // ugh.

    Composition *that = &c;

    double tp = this->m_defaultTempo;
    this->m_defaultTempo = that->m_defaultTempo;
    that->m_defaultTempo = tp;

    tp = this->m_currentTempo;
    this->m_currentTempo = that->m_currentTempo;
    that->m_currentTempo = tp;

    m_referenceSegment.swap(c.m_referenceSegment);

    m_segments.swap(c.m_segments);

    // swap tracks and instruments
    //
    m_tracks.swap(c.m_tracks);
    m_instruments.swap(c.m_instruments);

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

    FastVector<timeT> sections;
    FastVector<timeT> sectionTimes;

    FastVector<Segment::iterator> baritrs;

    for (i = t.begin(); i != t.end(); ++i) {

	if ((*i)->isa(BarEventType)) {

	    baritrs.push_back(i);

	} else if ((*i)->isa(TimeSignature::EventType)) {

	    if (sections.size() > 0 &&
		sections[sections.size()-1] == (*i)->getAbsoluteTime()) {
		std::cerr << "Ignoring time sig at "
			  << (*i)->getAbsoluteTime()
			  << " because we already have it" << endl;
	    } else {
		std::cerr << "Pushing time sig duration "
			  << TimeSignature(**i).getBarDuration()
			  << " at time "
			  << (*i)->getAbsoluteTime() << endl;
		sections.push_back((*i)->getAbsoluteTime());
		sectionTimes.push_back(TimeSignature(**i).getBarDuration());
	    }
	}
    }

    // Take out the old bar events, 'cos we're about to recalculate them
    for (int j = 0; j < baritrs.size(); ++j) {
	t.erase(baritrs[j]);
    }

//    std::cerr << "have " << t.size() << " non-bars in ref segment" << std::endl;

    bool section0isTimeSig = true;
    if (sections.size() == 0 || sections[0] != 0) {
	sections.push_front(0);
	sectionTimes.push_front(TimeSignature().getBarDuration());
	section0isTimeSig = false;
    }    

    timeT duration = getDuration();
    sections.push_back(duration);

    int barNo = 0;

    for (int s = 0; s < sections.size() - 1; ++s) {

	timeT start = sections[s], finish = sections[s+1];
	timeT time;

	if (s > 0 || section0isTimeSig) {
	    start += sectionTimes[s];
	    ++barNo;
	}

//	std::cerr << "section " << s << ": start " << start << ", finish " << finish << std::endl;

	for (time = start; time < finish; time += sectionTimes[s]) {
	    addNewBar(time, barNo++);
//            std::cerr << "added bar at " << time << std::endl;
	}

	if (s == sections.size() - 1 && time != duration) {
            addNewBar(time, barNo++);
//            std::cerr << "added final bar at " << time << std::endl;
        }            
    }

    m_barCount = barNo - 1;
    if (m_barCount < 0) m_barCount = 0; // don't count the final bar line

    m_barPositionsNeedCalculating = false;
//    std::cerr << "Composition::calculateBarPositions ending" << std::endl;

//    std::cerr << "Reference segment contains:" << std::endl;
//    for (i = t.begin(); i != t.end(); ++i) {
//	(*i)->dump(std::cerr);
//    }
}

int
Composition::getNbBars() const
{
    calculateBarPositions();
    return m_barCount;
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

	n = m_barCount;

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

void
Composition::addTimeSignature(timeT t, TimeSignature timeSig)
{
    // Remove any existing time sigs at time t before we insert the
    // new one

    calculateBarPositions();

    Segment::iterator i = m_referenceSegment.findTime(t);
    std::vector<Segment::iterator> timeSigs;

    while (i != m_referenceSegment.end() && (*i)->getAbsoluteTime() == t) {
	if ((*i)->isa(TimeSignature::EventType)) timeSigs.push_back(i);
	++i;
    }

    for (unsigned int j = 0; j < timeSigs.size(); ++j) {
	m_referenceSegment.erase(timeSigs[j]);
    }
    
    m_referenceSegment.insert(timeSig.getAsEvent(t));
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

double
Composition::getTempoAt(timeT t) const
{
    Segment::iterator i = m_referenceSegment.findTime(t);

    for (;;) {

	if (i != m_referenceSegment.end() && (*i)->isa(TempoEventType)) {
	    return (double)((*i)->get<Int>(TempoProperty)) / 60.0;
	}
	
	if (i == m_referenceSegment.begin()) return m_defaultTempo;
	--i;
    }
}

void
Composition::setPosition(timeT position)
{
    if (m_position == position) return;

    if (position > m_position) {

	//!!! some optimisation still available here

	// may be quicker to get j by incrementing from i, if position is
	// fairly close to m_position
	Segment::iterator i = m_referenceSegment.findTime(m_position);
	Segment::iterator j = m_referenceSegment.findTime(position);

	bool useJ = (j != m_referenceSegment.end() &&
		     (*j)->getAbsoluteTime() == position);

	while (i != j || useJ) {

	    if ((*i)->isa(TempoEventType)) {
		m_currentTempo = (double)((*i)->get<Int>(TempoProperty)) / 60.0;
	    }

	    if (useJ && (i == j)) break;
	    ++i;
	}

    } else {

	m_currentTempo = getTempoAt(position);
    }

    m_position = position;
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


// Insert an Instrument into the Composition
//
//
void Composition::addInstrument(const Instrument &inst)
{
    // For the moment just insert it - but we should probably
    // check indexes first.
    //
    // 
    m_instruments[inst.getID()] = inst;
}

// Insert a Track representation into the Composition
//
void Composition::addTrack(const Track &track)
{
    m_tracks[track.getID()] = track;
}


void Composition::deleteTrack(const int &track)
{
     trackiterator titerator = m_tracks.find(track);

     m_tracks.erase(titerator);
}

void Composition::deleteInstrument(const int &instrument)
{
     instrumentiterator iiterator = m_instruments.find(instrument);

     m_instruments.erase(iiterator);
}

// Export the Composition as XML, also iterates through
// Instruments, Tracks and any further sub-objects
//
//
string Composition::toXmlString()
{
    stringstream composition;

    composition << "<composition recordtrack=\"";
    composition << m_recordTrack;
    composition << "\" pointer=\"" << m_position;
    composition << "\" tempo=\"" << m_currentTempo;
    composition << "\">" << endl << endl;

    for (instrumentiterator iit = getInstruments()->begin();
                            iit != getInstruments()->end();
                            iit++ )
    {
        composition << "  " << (*iit).second.toXmlString() << endl;
    }

    composition << endl;

    for (trackiterator tit = getTracks()->begin();
                        tit != getTracks()->end();
                        tit++ )
    {
        composition << "  " << (*tit).second.toXmlString() << endl;
    }

    composition << endl;
    composition << "</composition>" << ends;

    return composition.str();
}



}


