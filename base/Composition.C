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

#include "Composition.h"
#include "Segment.h"
#include "FastVector.h"

#include <iostream>
#include <iomanip>

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

#undef DEBUG_BAR_STUFF
#undef DEBUG_TEMPO_STUFF 


namespace Rosegarden 
{

const std::string Composition::BarEventType = "bar";
const PropertyName Composition::BarNumberProperty = "BarNumber";
const PropertyName Composition::BarHasTimeSigProperty = "BarHasTimeSig";

const std::string Composition::TempoEventType = "tempo";
const PropertyName Composition::TempoProperty = "BeatsPerHour";
const PropertyName Composition::TempoTimestampSecProperty = "TimestampSec";
const PropertyName Composition::TempoTimestampUsecProperty = "TimestampUsec";


bool
Composition::ReferenceSegmentEventCmp::operator()(const Event &e1,
						  const Event &e2) const
{
    if (e1.getAbsoluteTime() >= 0 &&
	e2.getAbsoluteTime() >= 0) {
	return e1 < e2;
    } else {
	RealTime r1 = getTempoTimestamp(&e1);
	RealTime r2 = getTempoTimestamp(&e2);
	return r1 < r2;
    }
}

bool
Composition::ReferenceSegmentEventCmp::operator()(const Event *e1,
						  const Event *e2) const
{
    return operator()(*e1, *e2);
}

Composition::ReferenceSegment::ReferenceSegment(std::string eventType) :
    m_eventType(eventType)
{
    // nothing
}

Composition::ReferenceSegment::~ReferenceSegment()
{
    clear();
}

void
Composition::ReferenceSegment::clear()
{
    for (iterator it = begin(); it != end(); ++it) delete (*it);
    Impl::erase(begin(), end());
}

timeT
Composition::ReferenceSegment::getDuration() const
{
    const_iterator i = end();
    if (i == begin()) return 0;
    --i;
    return (*i)->getAbsoluteTime() + (*i)->getDuration();
}

void
Composition::ReferenceSegment::swap(Composition::ReferenceSegment &r)
{
    Impl temp;

    // inefficient palaver.  must reorganise usage of Composition
    // so as not to require swap()

    for (iterator i = begin(); i != end(); ++i) {
	temp.push_back(*i);
    }
    Impl::erase(begin(), end());

    for (iterator i = r.begin(); i != r.end(); ++i) {
	push_back(new Event(**i));
    }
    r.clear();

    for (iterator i = temp.begin(); i != temp.end(); ++i) {
	r.push_back(*i);
    }
    temp.erase(temp.begin(), temp.end());
}
	

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::find(Event *e)
{
    return std::lower_bound
	(begin(), end(), e, ReferenceSegmentEventCmp());
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::insert(Event *e)
{
    if (!e->isa(m_eventType)) throw Event::BadType();

    iterator i = find(e);

    if (i != end() && (*i)->getAbsoluteTime() == e->getAbsoluteTime()) {

	Event *old = (*i);
	(*i) = e;
	delete old;
	return i;

    } else {
	return Impl::insert(i, e);
    }
}

void
Composition::ReferenceSegment::erase(Event *e)
{
    iterator i = find(e);
    if (i != end()) Impl::erase(i);
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::findTime(timeT t)
{
    Event dummy;
    dummy.setAbsoluteTime(t);
    dummy.setSubOrdering(MIN_SUBORDERING);
    return find(&dummy);
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::findRealTime(RealTime t)
{
    Event dummy;
    dummy.setAbsoluteTime(-1);
    dummy.setSubOrdering(MIN_SUBORDERING);
    setTempoTimestamp(&dummy, t);
    return find(&dummy);
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::findNearestTime(timeT t)
{
    iterator i = findTime(t);
    if (i == end() || (*i)->getAbsoluteTime() > t) {
	if (i == begin()) return end();
	else --i;
    }
    return i;
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::findNearestRealTime(RealTime t)
{
    iterator i = findRealTime(t);
    if (i == end() || (getTempoTimestamp(*i) > t)) {
	if (i == begin()) return end();
	else --i;
    }
    return i;
}



Composition::Composition() :
    m_recordTrack(0),
    m_barSegment(BarEventType),
    m_timeSigSegment(TimeSignature::EventType),
    m_tempoSegment(TempoEventType),
    m_position(0),
    m_defaultTempo(120.0),
    m_startMarker(0),
    m_endMarker(0),
    m_barPositionsNeedCalculating(true),
    m_tempoTimestampsNeedCalculating(true)
{
    // nothing else
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

    m_barSegment.swap(c.m_barSegment);
    m_timeSigSegment.swap(c.m_timeSigSegment);
    m_tempoSegment.swap(c.m_tempoSegment);

    // swap tracks and instruments
    //
    m_tracks.swap(c.m_tracks);
    m_instruments.swap(c.m_instruments);

    // swap all the segments
    //
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
    
    this->m_tempoTimestampsNeedCalculating = true;
    that->m_tempoTimestampsNeedCalculating = true;
}

Composition::iterator
Composition::addSegment(Segment *segment)
{
    std::cerr << "Composition::addSegment: segment is " << segment
	      << ", with track " << segment->getTrack() << " and start index "
	      << segment->getStartIndex() << "; currently have " << m_segments.size() << " segments"
	      << endl;

    if (!segment) return end();
    
    std::pair<iterator, bool> res = m_segments.insert(segment);
    segment->addObserver(this);
    segment->setComposition(this);

    std::cerr << "Composition::addSegment: added segment, now have "
	      << m_segments.size() << " segments" << endl;

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

bool
Composition::setSegmentStartIndex(Segment *s, timeT t)
{
    iterator i = find(begin(), end(), s);
    if (i == end()) return false;

    m_segments.erase(i);
    s->setStartIndex(t);
    m_segments.insert(s);
    return true;
}

bool
Composition::setSegmentTrack(Segment *s, unsigned int track)
{
    iterator i = find(begin(), end(), s);
    if (i == end()) return false;

    m_segments.erase(i);
    s->setTrack(track);
    m_segments.insert(s);
    return true;
}

timeT
Composition::getDuration() const
{
    timeT maxDuration = 0;

    for (segmentcontainer::const_iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

	timeT segmentTotal = (*i)->getEndIndex();
        
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
    m_barSegment.clear();
    m_timeSigSegment.clear();
    m_tempoSegment.clear();
}

Composition::ReferenceSegment::iterator
Composition::addNewBar(timeT time, timeT duration,
		       int barNo, bool hasTimeSig) const
{
#ifdef DEBUG_BAR_STUFF
    std::cerr << "Composition::addNewBar" << std::endl;
#endif
    Event *e = new Event(BarEventType);
    e->setAbsoluteTime(time);
    e->setDuration(duration);
    e->set<Int>(BarNumberProperty, barNo);
    e->set<Bool>(BarHasTimeSigProperty, hasTimeSig);
    return m_barSegment.insert(e);
}

void
Composition::calculateBarPositions() const
{
#ifdef DEBUG_BAR_STUFF
//    std::cerr << "Composition::calculateBarPositions" << std::endl;
#endif

    if (!m_barPositionsNeedCalculating) return;

    ReferenceSegment &t = m_timeSigSegment;
    ReferenceSegment::iterator i;

    FastVector<timeT> sections;
    FastVector<timeT> sectionTimes;

    for (i = t.begin(); i != t.end(); ++i) {

#ifdef DEBUG_BAR_STUFF
//		std::cerr << "Pushing time sig duration "
//			  << TimeSignature(**i).getBarDuration()
//			  << " at time "
//			  << (*i)->getAbsoluteTime() << std::endl;
#endif
	sections.push_back((*i)->getAbsoluteTime());
	sectionTimes.push_back(TimeSignature(**i).getBarDuration());
    }

    // Take out the old bar events, 'cos we're about to recalculate them
    m_barSegment.clear();

#ifdef DEBUG_BAR_STUFF
//    std::cerr << "have " << t.size() << " non-bars in ref segment" << std::endl;
#endif

    bool section0isTimeSig = true;
    if (sections.size() == 0 || sections[0] != 0) {
	sections.push_front(0);
	sectionTimes.push_front(TimeSignature().getBarDuration());
	section0isTimeSig = false;
    }    

    timeT duration = getDuration();
    if (sections.size() == 0 || sections[sections.size()-1] != duration) {
	sections.push_back(duration);
	sectionTimes.push_back(sectionTimes[sectionTimes.size()-1]);
    }

    int barNo = 0;

    for (int s = 0; s < sections.size() - 1; ++s) {

	timeT start = sections[s], finish = sections[s+1];
	timeT time;

#ifdef DEBUG_BAR_STUFF
//	std::cerr << "section " << s << ": start " << start << ", finish " << finish << std::endl;
#endif
	
	bool haveTimeSig = (s > 0 || section0isTimeSig);

	for (time = start; time < finish; time += sectionTimes[s]) {
	    addNewBar(time, sectionTimes[s],
		      barNo++, haveTimeSig);
	    haveTimeSig = false;
#ifdef DEBUG_BAR_STUFF
//            std::cerr << "added bar at " << time << std::endl;
#endif
	}
    }

    m_barPositionsNeedCalculating = false;
#ifdef DEBUG_BAR_STUFF
//    std::cerr << "Composition::calculateBarPositions ending" << std::endl;

    std::cerr << "Time sig segment contains:" << std::endl;
    for (i = t.begin(); i != t.end(); ++i) {
	(*i)->dump(std::cerr);
    }

    std::cerr << "Bar segment contains:" << std::endl;
    for (i = m_barSegment.begin(); i != m_barSegment.end(); ++i) {
	(*i)->dump(std::cerr);
    }
#endif
}

int
Composition::getNbBars() const
{
    calculateBarPositions();
    return m_barSegment.size();
}

int
Composition::getBarNumber(timeT t, bool truncate) const
{
    calculateBarPositions();

    int barNo;

    ReferenceSegment::iterator i = m_barSegment.findNearestTime(t);

    if (i == m_barSegment.end()) {
	// precedes any bar lines...?
	barNo = t / getTimeSignatureAt(t).getBarDuration();

    } else {

	barNo = (int)(*i)->get<Int>(BarNumberProperty);

#ifdef DEBUG_BAR_STUFF
	std::cerr << "found " << barNo << " at " << (*i)->getAbsoluteTime() << std::endl;
#endif

	if (!truncate && t >= m_barSegment.getDuration()) {
	    TimeSignature sig = getTimeSignatureAt(t);
	    barNo = m_barSegment.size() +
		(t - m_barSegment.getDuration()) / sig.getBarDuration();
	}
    }

#ifdef DEBUG_BAR_STUFF
    std::cerr << "Composition::getBarNumber: returning " << barNo
	 << " for " << t << " (m_barSegment.getDuration() is "
	 << m_barSegment.getDuration() << ", size() is "
	 << m_barSegment.size() << ")" << std::endl;
#endif

    return barNo;

}

timeT
Composition::getBarStart(timeT t) const
{
    return getBarRange(t).first;
}

timeT
Composition::getBarEnd(timeT t) const
{
    return getBarRange(t).second;
}

std::pair<timeT, timeT>
Composition::getBarRange(timeT t) const
{
    calculateBarPositions();

    timeT start, finish;
    ReferenceSegment::iterator i = m_barSegment.findNearestTime(t);

    if (i == m_barSegment.end()) {
	start = 0;
	finish = getTimeSignatureAt(t).getBarDuration();
    } else {
	start = (*i)->getAbsoluteTime();
	finish = start + (*i)->getDuration();
    }

#ifdef DEBUG_BAR_STUFF
    std::cerr << "Composition::getBarRange for " << t  << ": range is " << start
	 << " -> " << finish << std::endl;
#endif

    return std::pair<timeT, timeT>(start, finish);
}


std::pair<timeT, timeT>
Composition::getBarRange(int n, bool truncate) const
{
    calculateBarPositions();

    timeT start, barDuration;

    if (n >= m_barSegment.size() && truncate && m_barSegment.size() > 0) {
	n = m_barSegment.size() - 1;
    }

    if (n < m_barSegment.size()) {
	start = m_barSegment[n]->getAbsoluteTime();
	barDuration = m_barSegment[n]->getDuration();

#ifdef DEBUG_BAR_STUFF
    std::cerr << "Composition::getBarRange[A] for " << n << ": range is " << start
	 << " -> " << (start + barDuration) << std::endl;
#endif

	return std::pair<timeT, timeT>(start, start + barDuration);
    }
    
    ReferenceSegment::iterator i = m_barSegment.end();
    
    if (i == m_barSegment.begin()) { // no bars at all

	barDuration = getTimeSignatureAt(0).getBarDuration();

	if (truncate) {
	    start = 0;
	} else {
	    start = n * barDuration;
	}

    } else {

	// truncate must be false, otherwise the first conditional
	// in this function would have been fired

	--i;
	start = (*i)->getAbsoluteTime();
	barDuration = (*i)->getDuration();
	start += (n - (*i)->get<Int>(BarNumberProperty)) * barDuration;
    }

#ifdef DEBUG_BAR_STUFF
    std::cerr << "Composition::getBarRange[B] for " << n << ": range is " << start
	 << " -> " << (start + barDuration) << std::endl;
#endif

    return std::pair<timeT, timeT>(start, start + barDuration);
}

void
Composition::addTimeSignature(timeT t, TimeSignature timeSig)
{
    m_timeSigSegment.insert(timeSig.getAsEvent(t));
    m_barPositionsNeedCalculating = true;
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
    ReferenceSegment::iterator i = m_timeSigSegment.findNearestTime(t);

    if (i == m_timeSigSegment.end()) {
	timeSig = TimeSignature();
	return 0;
    } else {
	timeSig = TimeSignature(**i);
	return (*i)->getAbsoluteTime();
    }
}

TimeSignature
Composition::getTimeSignatureInBar(int barNo, bool &isNew) const
{
    if (barNo > m_barSegment.size()) barNo = m_barSegment.size() - 1;
    if (barNo < 0) {
	isNew = false;
	return TimeSignature();
    }

    isNew = m_barSegment[barNo]->get<Bool>(BarHasTimeSigProperty);
    return getTimeSignatureAt(m_barSegment[barNo]->getAbsoluteTime());
}

int
Composition::getTimeSignatureCount() const
{
    return m_timeSigSegment.size();
}

std::pair<timeT, TimeSignature>
Composition::getTimeSignatureChange(int n) const
{
    return std::pair<timeT, TimeSignature>
	(m_timeSigSegment[n]->getAbsoluteTime(),
	 TimeSignature(*m_timeSigSegment[n]));
}

void
Composition::removeTimeSignature(int n)
{
    m_timeSigSegment.erase(m_timeSigSegment[n]);
    m_barPositionsNeedCalculating = true;
}


double
Composition::getTempoAt(timeT t) const
{
    ReferenceSegment::iterator i = m_tempoSegment.findNearestTime(t);
    if (i == m_tempoSegment.end()) return m_defaultTempo;

    double tempo = (double)((*i)->get<Int>(TempoProperty)) / 60.0;

#ifdef DEBUG_TEMPO_STUFF
    std::cerr << "Composition: Found tempo " << tempo << " at " << t << std::endl;
#endif
    return tempo;
}

void
Composition::addTempo(timeT time, double tempo)
{
    Event *tempoEvent = new Event(TempoEventType);
    tempoEvent->setAbsoluteTime(time);
    tempoEvent->set<Int>(TempoProperty, (long)(tempo * 60));
    m_tempoSegment.insert(tempoEvent);
    m_tempoTimestampsNeedCalculating = true;

#ifdef DEBUG_TEMPO_STUFF
    std::cerr << "Composition: Added tempo " << tempo << " at " << time << std::endl;
#endif
}

void
Composition::addRawTempo(timeT time, int tempo)
{
    Event *tempoEvent = new Event(TempoEventType);
    tempoEvent->setAbsoluteTime(time);
    tempoEvent->set<Int>(TempoProperty, tempo);
    m_tempoSegment.insert(tempoEvent);
    m_tempoTimestampsNeedCalculating = true;

#ifdef DEBUG_TEMPO_STUFF
    std::cerr << "Composition: Added tempo " << tempo << " at " << time << std::endl;
#endif
}
int
Composition::getTempoChangeCount() const
{
    return m_tempoSegment.size();
}

std::pair<timeT, long>
Composition::getRawTempoChange(int n) const
{
    return std::pair<timeT, long>
	(m_tempoSegment[n]->getAbsoluteTime(),
	 m_tempoSegment[n]->get<Int>(TempoProperty));
}

void
Composition::removeTempoChange(int n)
{
    m_tempoSegment.erase(m_tempoSegment[n]);
    m_tempoTimestampsNeedCalculating = true;
}


RealTime
Composition::getElapsedRealTime(timeT t) const
{
    calculateTempoTimestamps();

    ReferenceSegment::iterator i = m_tempoSegment.findNearestTime(t);
    if (i == m_tempoSegment.end()) {
	return time2RealTime(t, m_defaultTempo);
    }

    RealTime elapsed = getTempoTimestamp(*i) +
	time2RealTime(t - (*i)->getAbsoluteTime(),
		      (double)((*i)->get<Int>(TempoProperty)) / 60.0);
/*
    cerr << "Composition::getElapsedRealTime: " << t << " -> "
	 << elapsed << endl;
*/
    return elapsed;
}

timeT
Composition::getElapsedTimeForRealTime(RealTime t) const
{
    calculateTempoTimestamps();

    ReferenceSegment::iterator i = m_tempoSegment.findNearestRealTime(t);
    if (i == m_tempoSegment.end()) {
	return realTime2Time(t, m_defaultTempo);
    }

    timeT elapsed = (*i)->getAbsoluteTime() +
	realTime2Time(t - getTempoTimestamp(*i),
		      (double)((*i)->get<Int>(TempoProperty)) / 60.0);

#ifdef DEBUG_TEMPO_STUFF
    //!!! temporary calculations of error
    static int doError = true;
    if (doError) {
	doError = false;
	RealTime cfReal = getElapsedRealTime(elapsed);
	timeT cfTimeT = getElapsedTimeForRealTime(cfReal);
	doError = true;
	cerr << "getElapsedTimeForRealTime: " << t << " -> "
	     << elapsed << " (error " << (cfReal - t)
	     << " or " << (cfTimeT - elapsed) << ", tempo "
	     << (*i)->getAbsoluteTime() << ":"
	     << ((double)((*i)->get<Int>(TempoProperty)) / 60.0) << ")" << endl;
    }
#endif
    return elapsed;
}

void
Composition::calculateTempoTimestamps() const
{
    if (!m_tempoTimestampsNeedCalculating) return;

    timeT lastTimeT = 0;
    RealTime lastRealTime;
    double tempo = m_defaultTempo;

#ifdef DEBUG_TEMPO_STUFF
    std::cerr << "Composition::calculateTempoTimestamps: Tempo events are:" << std::endl;
#endif

    for (ReferenceSegment::iterator i = m_tempoSegment.begin();
	 i != m_tempoSegment.end(); ++i) {

	RealTime myTime = lastRealTime +
	    time2RealTime((*i)->getAbsoluteTime() - lastTimeT, tempo);

	setTempoTimestamp(*i, myTime);

#ifdef DEBUG_TEMPO_STUFF
	(*i)->dump(cerr);
#endif

	lastRealTime = myTime;
	lastTimeT = (*i)->getAbsoluteTime();
	tempo = (double)((*i)->get<Int>(TempoProperty)) / 60.0;
    }

    m_tempoTimestampsNeedCalculating = false;
}	

RealTime
Composition::time2RealTime(timeT t, double tempo) const
{
    double factor = Note(Note::Crotchet).getDuration() * tempo;
    long sec = (long)((60.0 * (double)t) / factor);
    t -= realTime2Time(RealTime(sec, 0), tempo);
    return RealTime(sec, (long)((6e7L * (double)t) / factor));
}

timeT
Composition::realTime2Time(RealTime rt, double tempo) const
{
    double factor = Note(Note::Crotchet).getDuration() * tempo;
    double t = ((double)rt.sec * factor) / 60.0;
    t += ((double)rt.usec * factor) / 6e7L;
    return (timeT)t;
}

RealTime
Composition::getTempoTimestamp(const Event *e) 
{
    long sec = 0, usec = 0;
    e->get<Int>(TempoTimestampSecProperty, sec);
    e->get<Int>(TempoTimestampUsecProperty, usec);
    return RealTime(sec, usec);
}

void
Composition::setTempoTimestamp(Event *e, RealTime t)
{
    e->setMaybe<Int>(TempoTimestampSecProperty, t.sec);
    e->setMaybe<Int>(TempoTimestampUsecProperty, t.usec);
}

void
Composition::setPosition(timeT position)
{
    m_position = position;
}

void Composition::eventAdded(const Segment *, Event *e)
{
    // we only need to recalculate bars if we insert something after
    // the former end of the composition

    if (e->getAbsoluteTime() + e->getDuration() >
	m_barSegment.getDuration()) {
	m_barPositionsNeedCalculating = true;
    }
}


void Composition::eventRemoved(const Segment *, Event *e)
{
    // we only need to recalculate bars if we remove something at the
    // former end of the composition

    if (e->getAbsoluteTime() + e->getDuration() >=
	m_barSegment.getDuration()) {

	m_barPositionsNeedCalculating = true;
    }
}


// Insert an Instrument into the Composition
//
//
void Composition::addInstrument(Instrument *inst)
{
    // For the moment just insert it - but we should probably
    // check indexes first.
    //
    // 
    m_instruments[inst->getID()] = inst;
}

// Insert a Track representation into the Composition
//
void Composition::addTrack(Track *track)
{
    m_tracks[track->getID()] = track;
}


void Composition::deleteTrack(const int &track)
{
     trackiterator titerator = m_tracks.find(track);

     delete ((*titerator).second);
     m_tracks.erase(titerator);
}

void Composition::deleteInstrument(const int &instrument)
{
     instrumentiterator iiterator = m_instruments.find(instrument);

     delete ((*iiterator).second);
     m_instruments.erase(iiterator);
}

// Export the Composition as XML, also iterates through
// Instruments, Tracks and any further sub-objects
//
//
std::string Composition::toXmlString()
{
    std::stringstream composition;

    composition << "<composition recordtrack=\"";
    composition << m_recordTrack;
    composition << "\" pointer=\"" << m_position;
    composition << "\" defaultTempo=\"";
    composition << std::setiosflags(std::ios::fixed)
                << std::setprecision(4) << m_defaultTempo;
    composition << "\">" << std::endl << std::endl;

    for (instrumentiterator iit = getInstruments()->begin();
                            iit != getInstruments()->end();
                            iit++ )
    {
        composition << "  " << (*iit).second->toXmlString() << std::endl;
    }

    composition << std::endl;

    for (trackiterator tit = getTracks()->begin();
                        tit != getTracks()->end();
                        tit++ )
    {
        composition << "  " << (*tit).second->toXmlString() << std::endl;
    }

    composition << std::endl;

    for (ReferenceSegment::iterator i = m_timeSigSegment.begin();
	 i != m_timeSigSegment.end(); ++i) {

	// Might be nice just to stream the events, but that's
	// normally done by XmlStorableEvent in gui/ at the
	// moment.  Still, this isn't too much of a hardship

	composition << "  <timesignature time=\"" << (*i)->getAbsoluteTime()
		    << "\" numerator=\""
		    << (*i)->get<Int>(TimeSignature::NumeratorPropertyName)
		    << "\" denominator=\""
		    << (*i)->get<Int>(TimeSignature::DenominatorPropertyName)
		    << "\"/>" << std::endl;
    }

    composition << std::endl;

    for (ReferenceSegment::iterator i = m_tempoSegment.begin();
	 i != m_tempoSegment.end(); ++i) {

	composition << "  <tempo time=\"" << (*i)->getAbsoluteTime()
		    << "\" bph=\""
		    << (*i)->get<Int>(TempoProperty) << "\"/>" << std::endl;
    }

    composition << std::endl;

    composition << "</composition>" << std::ends;

    return composition.str();
}

void
Composition::clearTracks()
{
    trackiterator it = m_tracks.begin();

    for (; it != m_tracks.end(); it++)
        delete ((*it).second);

    m_tracks.erase(m_tracks.begin(), m_tracks.end());
}

void
Composition::clearInstruments()
{
    instrumentiterator it = m_instruments.begin();
 
    for (; it != m_instruments.end(); it++)
        delete((*it).second);

    m_instruments.erase(m_instruments.begin(), m_instruments.end());
}


}


