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

#include "Composition.h"
#include "Segment.h"
#include "FastVector.h"
#include "Quantizer.h"

#include <iostream>
#include <iomanip>
#include <algorithm>

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

using std::cerr;
using std::endl;

//#define DEBUG_BAR_STUFF 1
//#define DEBUG_TEMPO_STUFF 1


namespace Rosegarden 
{

const PropertyName Composition::NoAbsoluteTimeProperty = "NoAbsoluteTime";
const PropertyName Composition::BarNumberProperty = "BarNumber";

const std::string Composition::TempoEventType = "tempo";
const PropertyName Composition::TempoProperty = "BeatsPerHour";
const PropertyName Composition::TempoTimestampProperty = "TimestampSec";


bool
Composition::ReferenceSegmentEventCmp::operator()(const Event &e1,
						  const Event &e2) const
{
    if (e1.has(NoAbsoluteTimeProperty) ||
	e2.has(NoAbsoluteTimeProperty)) {
	RealTime r1 = getTempoTimestamp(&e1);
	RealTime r2 = getTempoTimestamp(&e2);
	return r1 < r2;
    } else {
	return e1 < e2;
    }
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

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::find(Event *e)
{
    return std::lower_bound
	(begin(), end(), e, ReferenceSegmentEventCmp());
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::insert(Event *e)
{
    if (!e->isa(m_eventType)) {
	throw Event::BadType(std::string("event in ReferenceSegment"),
			     m_eventType, e->getType(), __FILE__, __LINE__);
    }

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
    Event dummy("dummy", t, 0, MIN_SUBORDERING);
    return find(&dummy);
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::findRealTime(RealTime t)
{
    Event dummy("dummy", 0, 0, MIN_SUBORDERING);
    dummy.set<Bool>(NoAbsoluteTimeProperty, true);
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



int Composition::m_defaultNbBars = 100;

Composition::Composition() :
    m_recordTrack(0),
    m_solo(false),   // default is not soloing
    m_selectedTrack(0),
    m_timeSigSegment(TimeSignature::EventType),
    m_tempoSegment(TempoEventType),
    m_barPositionsNeedCalculating(true),
    m_tempoTimestampsNeedCalculating(true),
    m_basicQuantizer(new BasicQuantizer()),
    m_notationQuantizer(new NotationQuantizer()),
    m_position(0),
    m_defaultTempo(120.0),
    m_startMarker(0),
    m_endMarker(getBarRange(m_defaultNbBars).second),
    m_loopStart(0),
    m_loopEnd(0),
    m_playMetronome(false),
    m_recordMetronome(true)
{
    // nothing else
}

Composition::~Composition()
{
    notifySourceDeletion();
    clear();
    delete m_basicQuantizer;
    delete m_notationQuantizer;
}

Composition::iterator
Composition::addSegment(Segment *segment)
{
    iterator res = weakAddSegment(segment);

    if (res != end()) {
        updateRefreshStatuses();
	notifySegmentAdded(segment);
    }
    
    return res;
}

Composition::iterator
Composition::weakAddSegment(Segment *segment)
{
    if (!segment) return end();
    
    iterator res = m_segments.insert(segment);
    segment->setComposition(this);

    return res;
}

void
Composition::deleteSegment(Composition::iterator i)
{
    if (i == end()) return;

    Segment *p = (*i);
    p->setComposition(0);

    m_segments.erase(i);
    notifySegmentRemoved(p);
    delete p;

    updateRefreshStatuses();
}

bool
Composition::deleteSegment(Segment *segment)
{
    iterator i = find(begin(), end(), segment);
    if (i == end()) return false;

    deleteSegment(i);
    return true;
}

bool
Composition::detachSegment(Segment *segment)
{
    bool res = weakDetachSegment(segment);

    if (res) {
        notifySegmentRemoved(segment);
        updateRefreshStatuses();
    }

    return res;
}

bool
Composition::weakDetachSegment(Segment *segment)
{
    iterator i = find(begin(), end(), segment);
    if (i == end()) return false;
    
    segment->setComposition(0);
    m_segments.erase(i);

    return true;
}

bool
Composition::contains(const Segment *s)
{
    iterator i = find(begin(), end(), s);
    return (i != end());
}

Composition::iterator
Composition::findSegment(Segment *s)
{
    return find(begin(), end(), s);
}

Composition::const_iterator
Composition::findSegment(const Segment *s)
{
    return find(begin(), end(), s);
}

void Composition::setSegmentStartTime(Segment *segment, timeT startTime)
{
    // remove the segment from the multiset
    iterator i = find(begin(), end(), segment);
    if (i == end()) return;
    
    m_segments.erase(i);

    segment->setStartTimeDataMember(startTime);

    // re-add it
    m_segments.insert(segment);    
}

timeT
Composition::getDuration() const
{
    timeT maxDuration = 0;

    for (segmentcontainer::const_iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

	timeT segmentTotal = (*i)->getEndTime();

        if (segmentTotal > maxDuration) {
            maxDuration = segmentTotal;
        }
    }
    
    return maxDuration;
}

void
Composition::setStartMarker(const timeT &sM)
{
    m_startMarker = sM;
    updateRefreshStatuses();
}

void
Composition::setEndMarker(const timeT &eM)
{
    bool shorten = (eM < m_endMarker);
    m_endMarker = eM;
    updateRefreshStatuses();
    notifyEndMarkerChange(shorten);
}

void
Composition::clear()
{
    while (m_segments.size() > 0) {
	deleteSegment(begin());
    }

    clearTracks();

    m_timeSigSegment.clear();
    m_tempoSegment.clear();
    m_loopStart = 0;
    m_loopEnd = 0;
    m_position = 0;
    m_startMarker = 0;
    m_endMarker = getBarRange(m_defaultNbBars).second;
    m_solo = false;
    m_selectedTrack = 0;
    updateRefreshStatuses();
}

void
Composition::calculateBarPositions() const
{
    if (!m_barPositionsNeedCalculating) return;

#ifdef DEBUG_BAR_STUFF
    cerr << "Composition::calculateBarPositions" << endl;
#endif

    ReferenceSegment &t = m_timeSigSegment;
    ReferenceSegment::iterator i;

    timeT lastBarNo = 0;
    timeT lastSigTime = 0;
    timeT barDuration = TimeSignature().getBarDuration();

    for (i = t.begin(); i != t.end(); ++i) {

	timeT myTime = (*i)->getAbsoluteTime();
	int n = (myTime - lastSigTime) / barDuration;

	// should only happen for first time sig, when it's at time < 0:
	if (myTime < lastSigTime) --n;

	// would there be a new bar here anyway?
	if (barDuration * n + lastSigTime == myTime) { // yes
	    n += lastBarNo;
	} else { // no
	    n += lastBarNo + 1;
	}

#ifdef DEBUG_BAR_STUFF
	cerr << "Composition::calculateBarPositions: bar " << n
		  << " at " << myTime << endl;
#endif

	(*i)->set<Int>(BarNumberProperty, n);

	lastBarNo = n;
	lastSigTime = myTime;
	barDuration = TimeSignature(**i).getBarDuration();
    }

    m_barPositionsNeedCalculating = false;
}

int
Composition::getNbBars() const
{
    calculateBarPositions();
    
    // the "-1" is a small kludge to deal with the case where the
    // composition has a duration that's an exact number of bars
    int bars = getBarNumber(getDuration() - 1) + 1;

#ifdef DEBUG_BAR_STUFF
    cerr << "Composition::getNbBars: returning " << bars << endl;
#endif
    return bars;
}

int
Composition::getBarNumber(timeT t) const
{
    calculateBarPositions();
    ReferenceSegment::iterator i = m_timeSigSegment.findNearestTime(t);
    int n;
    
    if (i == m_timeSigSegment.end()) { // precedes any time signatures
	
	timeT bd = TimeSignature().getBarDuration();
	if (t < 0) { // see comment in getTimeSignatureAtAux
	    i = m_timeSigSegment.begin();
	    if (i != m_timeSigSegment.end() && (*i)->getAbsoluteTime() <= 0) {
		bd = TimeSignature(**i).getBarDuration();
	    }
	}

	n = t / bd;
	if (t < 0) {
	    // negative bars should be rounded down, except where
	    // the time is on a barline in which case we already
	    // have the right value (i.e. time -1920 is bar -1,
	    // but time -3840 is also bar -1, in 4/4)
	    if (n * bd != t) --n;
	}

    } else {
	
	n = (*i)->get<Int>(BarNumberProperty);
	timeT offset = t - (*i)->getAbsoluteTime();
	n += offset / TimeSignature(**i).getBarDuration();
    }

#ifdef DEBUG_BAR_STUFF
    cerr << "Composition::getBarNumber(" << t << "): returning " << n << endl;
#endif
    return n;
}


std::pair<timeT, timeT>
Composition::getBarRangeForTime(timeT t) const
{
    return getBarRange(getBarNumber(t));
}


std::pair<timeT, timeT>
Composition::getBarRange(int n) const
{
    calculateBarPositions();

    Event dummy("dummy", 0);
    dummy.set<Int>(BarNumberProperty, n);

    ReferenceSegment::iterator j = std::lower_bound
	(m_timeSigSegment.begin(), m_timeSigSegment.end(),
	 &dummy, BarNumberComparator());
    ReferenceSegment::iterator i = j;

    if (i == m_timeSigSegment.end() || (*i)->get<Int>(BarNumberProperty) > n) {
	if (i == m_timeSigSegment.begin()) i = m_timeSigSegment.end();
	else --i;
    } else ++j; // j needs to point to following barline

    timeT start, finish;

    if (i == m_timeSigSegment.end()) { // precedes any time sig changes

	timeT barDuration = TimeSignature().getBarDuration();
	if (n < 0) { // see comment in getTimeSignatureAtAux
	    i = m_timeSigSegment.begin();
	    if (i != m_timeSigSegment.end() && (*i)->getAbsoluteTime() <= 0) {
		barDuration = TimeSignature(**i).getBarDuration();
	    }
	}

	start = n * barDuration;
	finish = start + barDuration;

#ifdef DEBUG_BAR_STUFF
    cerr << "Composition::getBarRange[1]: bar " << n << ": (" << start
	      << " -> " << finish << ")" << endl;
#endif

    } else {
	
	timeT barDuration = TimeSignature(**i).getBarDuration();
	start = (*i)->getAbsoluteTime() +
	    (n - (*i)->get<Int>(BarNumberProperty)) * barDuration;
	finish = start + barDuration;

#ifdef DEBUG_BAR_STUFF
    cerr << "Composition::getBarRange[2]: bar " << n << ": (" << start
	      << " -> " << finish << ")" << endl;
#endif
    } 

    // partial bar
    if (j != m_timeSigSegment.end() && finish > (*j)->getAbsoluteTime()) {
	finish = (*j)->getAbsoluteTime();
#ifdef DEBUG_BAR_STUFF
    cerr << "Composition::getBarRange[3]: bar " << n << ": (" << start
	      << " -> " << finish << ")" << endl;
#endif
    }

    return std::pair<timeT, timeT>(start, finish);
}
    

int
Composition::addTimeSignature(timeT t, TimeSignature timeSig)
{
    ReferenceSegment::iterator i =
	m_timeSigSegment.insert(timeSig.getAsEvent(t));
    m_barPositionsNeedCalculating = true;

    updateRefreshStatuses();
    notifyTimeSignatureChanged();

    return std::distance(m_timeSigSegment.begin(), i);
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
    ReferenceSegment::iterator i = getTimeSignatureAtAux(t);

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
    isNew = false;
    timeT t = getBarRange(barNo).first;

    ReferenceSegment::iterator i = getTimeSignatureAtAux(t);

    if (i == m_timeSigSegment.end()) return TimeSignature();
    if (t == (*i)->getAbsoluteTime()) isNew = true;

    return TimeSignature(**i);
}

Composition::ReferenceSegment::iterator
Composition::getTimeSignatureAtAux(timeT t) const
{
    ReferenceSegment::iterator i = m_timeSigSegment.findNearestTime(t);
    
    // In negative time, if there's no time signature actually defined
    // prior to the point of interest then we use the next time
    // signature after it, so long as it's no later than time zero.
    // This is the only rational way to deal with count-in bars where
    // the correct time signature otherwise won't appear until we hit
    // bar zero.

    if (t < 0 && i == m_timeSigSegment.end()) {
	i = m_timeSigSegment.begin();
	if (i != m_timeSigSegment.end() && (*i)->getAbsoluteTime() > 0) {
	    i  = m_timeSigSegment.end();
	}
    }
    
    return i;
}

int
Composition::getTimeSignatureCount() const
{
    return m_timeSigSegment.size();
}

int
Composition::getTimeSignatureNumberAt(timeT t) const
{
    ReferenceSegment::iterator i = getTimeSignatureAtAux(t);
    if (i == m_timeSigSegment.end()) return -1;
    else return std::distance(m_timeSigSegment.begin(), i);
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
    updateRefreshStatuses();
    notifyTimeSignatureChanged();
}


double
Composition::getTempoAt(timeT t) const
{
    ReferenceSegment::iterator i = m_tempoSegment.findNearestTime(t);

    // In negative time, if there's no tempo event actually defined
    // prior to the point of interest then we use the next one after
    // it, so long as it's no later than time zero.  This is the only
    // rational way to deal with count-in bars where the correct
    // tempo otherwise won't appear until we hit bar zero.  See also
    // getTimeSignatureAt

    if (i == m_tempoSegment.end()) {
	if (t < 0) {
#ifdef DEBUG_TEMPO_STUFF
	    cerr << "Composition: Negative time " << t << " for tempo, using 0" << endl;
#endif
	    return getTempoAt(0);
	}
	else return m_defaultTempo;
    }
    
    double tempo = (double)((*i)->get<Int>(TempoProperty)) / 60.0;

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition: Found tempo " << tempo << " at " << t << endl;
#endif
    return tempo;
}

int
Composition::addTempo(timeT time, double tempo)
{
    Event *tempoEvent = new Event(TempoEventType, time);
    tempoEvent->set<Int>(TempoProperty, (long)(tempo * 60));

    ReferenceSegment::iterator i = m_tempoSegment.insert(tempoEvent);

    m_tempoTimestampsNeedCalculating = true;
    updateRefreshStatuses();

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition: Added tempo " << tempo << " at " << time << endl;
#endif
    notifyTempoChanged();

    return std::distance(m_tempoSegment.begin(), i);
}

int
Composition::addRawTempo(timeT time, int tempo)
{
    Event *tempoEvent = new Event(TempoEventType, time);
    tempoEvent->set<Int>(TempoProperty, tempo);

    ReferenceSegment::iterator i = m_tempoSegment.insert(tempoEvent);

    updateRefreshStatuses();
    m_tempoTimestampsNeedCalculating = true;

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition: Added tempo " << tempo << " at " << time << endl;
#endif
    notifyTempoChanged();

    return std::distance(m_tempoSegment.begin(), i);
}

int
Composition::getTempoChangeCount() const
{
    return m_tempoSegment.size();
}

int
Composition::getTempoChangeNumberAt(timeT t) const
{
    ReferenceSegment::iterator i = m_tempoSegment.findNearestTime(t);
    if (i == m_tempoSegment.end()) return -1;
    else return std::distance(m_tempoSegment.begin(), i);
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
    updateRefreshStatuses();
}


RealTime
Composition::getElapsedRealTime(timeT t) const
{
    calculateTempoTimestamps();

    ReferenceSegment::iterator i = m_tempoSegment.findNearestTime(t);
    if (i == m_tempoSegment.end()) {
	i = m_tempoSegment.begin();
	if (t >= 0 ||
	    (i == m_tempoSegment.end() || (*i)->getAbsoluteTime() > 0)) {
	    return time2RealTime(t, m_defaultTempo);
	}
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
	i = m_tempoSegment.begin();
	if (t >= RealTime(0, 0) ||
	    (i == m_tempoSegment.end() || (*i)->getAbsoluteTime() > 0)) {
	    return realTime2Time(t, m_defaultTempo);
	}
    }

    timeT elapsed = (*i)->getAbsoluteTime() +
	realTime2Time(t - getTempoTimestamp(*i),
		      (double)((*i)->get<Int>(TempoProperty)) / 60.0);

#ifdef DEBUG_TEMPO_STUFF
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
    cerr << "Composition::calculateTempoTimestamps: Tempo events are:" << endl;
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
Composition::time2RealTime(timeT tsec, double tempo) const
{
    double factor = Note(Note::Crotchet).getDuration() * tempo;
    long sec = (long)((60.0 * (double)tsec) / factor);
    timeT tusec = tsec - realTime2Time(RealTime(sec, 0), tempo);
    long usec = (long)((6e7L * (double)tusec) / factor);

    RealTime rt(sec, usec);

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition::time2RealTime: sec " << sec << ", usec "
	 << usec << ", tempo " << tempo
	 << ", factor " << factor << ", tsec " << tsec << ", tusec " << tusec << ", rt " << rt << endl;
#endif

    return rt;
}

timeT
Composition::realTime2Time(RealTime rt, double tempo) const
{
    double factor = Note(Note::Crotchet).getDuration() * tempo;
    double tsec = ((double)rt.sec * factor) / 60.0;
    double tusec = ((double)rt.usec * factor);

    double t = tsec + (tusec / 6e7L);

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition::realTime2Time: rt.sec " << rt.sec << ", rt.usec "
	 << rt.usec << ", tempo " << tempo
	 << ", factor " << factor << ", tsec " << tsec << ", tusec " << tusec << ", t " << t << endl;
#endif

    return (timeT)t;
}

RealTime
Composition::getTempoTimestamp(const Event *e) 
{
    RealTime res;
    e->get<RealTimeT>(TempoTimestampProperty, res);
    return res;
}

void
Composition::setTempoTimestamp(Event *e, RealTime t)
{
    e->setMaybe<RealTimeT>(TempoTimestampProperty, t);
}

void
Composition::setPosition(timeT position)
{
    m_position = position;
}

void Composition::setPlayMetronome(bool value)
{
    m_playMetronome = value;
    notifyMetronomeChanged();
}

void Composition::setRecordMetronome(bool value)
{
    m_recordMetronome = value;
    notifyMetronomeChanged();
}



#ifdef TRACK_DEBUG
// track debug convenience function
//
static void dumpTracks(Composition::trackcontainer& tracks)
{
    Composition::trackiterator it = tracks.begin();
    for (; it != tracks.end(); ++it) {
        std::cerr << "tracks[" << (*it).first << "] = "
                  << (*it).second << std::endl;
    }
}
#endif

Track* Composition::getTrackById(TrackId track)
{
    trackiterator i = m_tracks.find(track);

    if (i != m_tracks.end())
        return (*i).second;

    std:: cerr << "Composition::getTrackById("
               << track << ") - WARNING - track id not found, this is probably a BUG "
               << __FILE__ << ":" << __LINE__ << std::endl;

    return 0;
}

// Move a track object to a new id and position in the container -
// used when deleting and undoing deletion of tracks.
//
//
void Composition::resetTrackIdAndPosition(TrackId oldId, TrackId newId,
                                          int position)
{
    trackiterator titerator = m_tracks.find(oldId);

    if (titerator != m_tracks.end())
    {
        // detach old track
        Track *track = (*titerator).second;
        m_tracks.erase(titerator);

        // set new position and 
        track->setId(newId);
        track->setPosition(position);
        m_tracks[newId] = track;

        // modify segment mappings
        //
        for (segmentcontainer::const_iterator i = m_segments.begin();
             i != m_segments.end(); ++i) 
        {
            if ((*i)->getTrack() == oldId) (*i)->setTrack(newId);
        }

        checkSelectedAndRecordTracks();
        updateRefreshStatuses();
    }
    else
        std::cerr << "Composition::resetTrackIdAndPosition - "
                  << "can't move track " << oldId << " to " << newId 
                  << std::endl;
}

void Composition::setSelectedTrack(TrackId track)
{
    m_selectedTrack = track;
    notifySoloChanged();
}

void Composition::setSolo(bool value)
{
    m_solo = value;
    notifySoloChanged();
}

// Insert a Track representation into the Composition
//
void Composition::addTrack(Track *track)
{
    // make sure a track with the same id isn't already there
    //
    if (m_tracks.find(track->getId()) == m_tracks.end()) {

        m_tracks[track->getId()] = track;
        track->setOwningComposition(this);
        updateRefreshStatuses();

    } else {
        std::cerr << "Composition::addTrack("
                  << track << "), id = " << track->getId()
                  << " - WARNING - track id already present "
                  << __FILE__ << ":" << __LINE__ << std::endl;
        // throw Exception("track id already present");
    }
}


void Composition::deleteTrack(Rosegarden::TrackId track)
{
    trackiterator titerator = m_tracks.find(track);

    if (titerator == m_tracks.end()) {

        std::cerr << "Composition::deleteTrack : no track of id " << track << std::endl;
        throw Exception("track id not found");

    } else {

        delete ((*titerator).second);
        m_tracks.erase(titerator);
        checkSelectedAndRecordTracks();
        updateRefreshStatuses();
    }
    
}

bool Composition::detachTrack(Rosegarden::Track *track)
{
    trackiterator it = m_tracks.begin();

    for (; it != m_tracks.end(); ++it)
    {
        if ((*it).second == track)
            break;
    }

    if (it == m_tracks.end()) {
        std::cerr << "Composition::detachTrack() : no such track " << track << std::endl;
        throw Exception("track id not found");
        return false;
    }

    ((*it).second)->setOwningComposition(0);

    m_tracks.erase(it);
    updateRefreshStatuses();
    checkSelectedAndRecordTracks();

    return true;
}

void Composition::checkSelectedAndRecordTracks()
{
    // reset m_selectedTrack and m_recordTrack to the next valid track id
    // if the track they point to has been deleted

    if (m_tracks.find(m_selectedTrack) == m_tracks.end()) {

        m_selectedTrack = getClosestValidTrackId(m_selectedTrack);
        
    }

    if (m_tracks.find(m_recordTrack) == m_tracks.end()) {

        m_recordTrack = getClosestValidTrackId(m_recordTrack);
        
    }

}

TrackId
Composition::getClosestValidTrackId(TrackId id) const
{
    long diff = LONG_MAX;
    TrackId closestValidTrackId = 0;

    for (trackcontainer::const_iterator i = getTracks().begin();
	 i != getTracks().end(); ++i) {

        long cdiff = labs(i->second->getId() - id);

	if (cdiff < diff) {
            diff = cdiff;
	    closestValidTrackId = i->second->getId();

	} else break; // std::map is sorted, so if the diff increases, we're passed closest valid id

    }

    return closestValidTrackId;
}
 
TrackId
Composition::getMinTrackId() const
{
    if (getTracks().size() == 0) return 0;
        
    trackcontainer::const_iterator i = getTracks().begin();
    return i->first;
}

TrackId
Composition::getMaxTrackId() const
{
    if (getTracks().size() == 0) return 0;

    trackcontainer::const_iterator i = getTracks().end();
    --i;
    
    return i->first;
}


// Export the Composition as XML, also iterates through
// Tracks and any further sub-objects
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

    if (m_loopStart != m_loopEnd)
    {
        composition << "\" loopstart=\"" << m_loopStart;
        composition << "\" loopend=\"" << m_loopEnd;
    }

    composition << "\" startMarker=\"" << m_startMarker;
    composition << "\" endMarker=\"" << m_endMarker;

    // Add the Solo if set
    //
    if (m_solo)
        composition << "\" solo=\"" << m_solo;

    composition << "\" selected=\"" << m_selectedTrack;

    composition << "\">" << endl << endl;

    composition << endl;

    for (trackiterator tit = getTracks().begin();
         tit != getTracks().end();
         ++tit)
        {
            if ((*tit).second)
                composition << "  " << (*tit).second->toXmlString() << endl;
        }

    composition << endl;

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
		    << "\"";

	bool common = false;
	(*i)->get<Bool>(TimeSignature::ShowAsCommonTimePropertyName, common);
	if (common) composition << " common=\"true\"";

	bool hidden = false;
	(*i)->get<Bool>(TimeSignature::IsHiddenPropertyName, common);
	if (hidden) composition << " hidden=\"true\"";

	composition << "/>" << endl;
    }

    composition << endl;

    for (ReferenceSegment::iterator i = m_tempoSegment.begin();
	 i != m_tempoSegment.end(); ++i) {

	composition << "  <tempo time=\"" << (*i)->getAbsoluteTime()
		    << "\" bph=\""
		    << (*i)->get<Int>(TempoProperty) << "\"/>" << endl;
    }

    composition << endl;

    composition << "<metadata>" << endl
		<< m_metadata.toXmlString() << endl
		<< "</metadata>" << endl << endl;

    composition << "<markers>" << endl;
    for (markerconstiterator mIt = m_markers.begin();
         mIt != m_markers.end(); ++mIt)
    {
        composition << (*mIt)->toXmlString();
    }
    composition << "</markers>" << endl;
                

#if (__GNUC__ < 3)
    composition << "</composition>" << std::ends;
#else
    composition << "</composition>";
#endif

    return composition.str();
}

void
Composition::clearTracks()
{
    trackiterator it = m_tracks.begin();

    for (; it != m_tracks.end(); it++)
        delete ((*it).second);

    m_tracks.erase(m_tracks.begin(), m_tracks.end());
    updateRefreshStatuses();
}

Track*
Composition::getTrackByPosition(int position)
{
    trackiterator it = m_tracks.begin();

    for (; it != m_tracks.end(); it++)
    {
        if ((*it).second->getPosition() == position)
            return (*it).second;
    }

    return 0;

}

Rosegarden::TrackId
Composition::getNewTrackId() const
{
    if (m_tracks.size() == 0) return 0;
    return m_tracks.size();
}


void
Composition::notifySegmentAdded(Segment *s) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->segmentAdded(this, s);
    }
}

 
void
Composition::notifySegmentRemoved(Segment *s) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->segmentRemoved(this, s);
    }
}

void
Composition::notifySegmentRepeatChanged(Segment *s, bool repeat) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->segmentRepeatChanged(this, s, repeat);
    }
}

void
Composition::notifyEndMarkerChange(bool shorten) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->endMarkerTimeChanged(this, shorten);
    }
}

void
Composition::notifyTrackChanged(Track *t) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->trackChanged(this, t);
    }
}

void
Composition::notifyMetronomeChanged() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->metronomeChanged(this);
    }
}

void
Composition::notifyTimeSignatureChanged() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->timeSignatureChanged(this);
    }
}

void
Composition::notifySoloChanged() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->soloChanged(this, isSolo(), getSelectedTrack());
    }
}

void
Composition::notifyTempoChanged() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->tempoChanged(this);
    }
}


void
Composition::notifySourceDeletion() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->compositionDeleted(this);
    }
}


void breakpoint()
{
    //std::cerr << "breakpoint()\n";
}

// Just empty out the markers
void
Composition::clearMarkers()
{
    markerconstiterator it = m_markers.begin();

    for (; it != m_markers.end(); ++it)
    {
        delete *it;
    }

    m_markers.clear();
}

void 
Composition::addMarker(Rosegarden::Marker *marker)
{
    m_markers.push_back(marker);
    updateRefreshStatuses();
}

bool
Composition::detachMarker(Rosegarden::Marker *marker)
{
    markeriterator it = m_markers.begin();

    for (; it != m_markers.end(); ++it)
    {
        if (*it == marker)
        {
            m_markers.erase(it);
            updateRefreshStatuses();
            return true;
        }
    }

    return false;
}

bool
Composition::isMarkerAtPosition(Rosegarden::timeT time) const
{
    markerconstiterator it = m_markers.begin();

    for (; it != m_markers.end(); ++it)
        if ((*it)->getTime() == time) return true;

    return false;
}

void
Composition::setSegmentColourMap(Rosegarden::ColourMap &newmap)
{
    m_segmentColourMap = newmap;

    updateRefreshStatuses();
}




}


