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

#include "Composition.h"
#include "Segment.h"
#include "FastVector.h"

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

// const unsigned int Composition::DefaultCountInBars = 2;


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

Composition::ReferenceSegment&
Composition::ReferenceSegment::operator=(const ReferenceSegment &seg)
{
    clear();

    for (iterator i = begin(); i != end(); ++i)
	this->push_back(new Event(**i));

    m_eventType = seg.getEventType();

    return (*this);
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



Composition::Composition() :
    m_recordTrack(0),
    m_solo(false),   // default is not soloing
    m_selectedTrack(0),
    m_timeSigSegment(TimeSignature::EventType),
    m_tempoSegment(TempoEventType),
    m_basicQuantizer(Quantizer::RawEventData, "BasicQ"),
    m_noteQuantizer(Quantizer::RawEventData, "NoteQ", Quantizer::NoteQuantize),
    m_legatoQuantizer(Quantizer::RawEventData, "LegatoQ", Quantizer::LegatoQuantize),
    m_position(0),
    m_defaultTempo(120.0),
    m_startMarker(0),
    m_endMarker(getBarRange(100).second), //!!! default end marker
    m_loopStart(0),
    m_loopEnd(0),
    m_barPositionsNeedCalculating(true),
    m_tempoTimestampsNeedCalculating(true),
//     m_countInBars(DefaultCountInBars),
    m_playMetronome(false),
    m_recordMetronome(true),
    m_needsRefresh(true)
{
    // nothing else
}

Composition::~Composition()
{
    clear();
}

Composition::Composition(const Composition &comp):
    XmlExportable(),
    m_recordTrack(comp.getRecordTrack()),
    m_solo(comp.isSolo()),
    m_selectedTrack(comp.getSelectedTrack()),
    m_timeSigSegment(comp.getTimeSigSegment()),
    m_tempoSegment(comp.getTempoSegment()),
    m_basicQuantizer(*comp.getBasicQuantizer()),
    m_noteQuantizer(*comp.getNoteQuantizer()),
    m_legatoQuantizer(*comp.getLegatoQuantizer()),
    m_position(comp.getPosition()),
    m_defaultTempo(comp.getDefaultTempo()),
    m_startMarker(comp.getStartMarker()),
    m_endMarker(comp.getEndMarker()),
    m_loopStart(comp.getLoopStart()),
    m_loopEnd(comp.getLoopEnd()),
    m_barPositionsNeedCalculating(true),
    m_tempoTimestampsNeedCalculating(true),
    m_playMetronome(comp.usePlayMetronome()),
    m_recordMetronome(comp.useRecordMetronome()),
    m_needsRefresh(true)
{
    m_tracks.clear();
    m_segments.clear();

    m_tracks = comp.getTracks();
    m_segments = comp.getSegments();

    for (segmentcontainer::iterator i = this->m_segments.begin();
	 i != this->m_segments.end(); ++i) {
	(*i)->setComposition(this);
    }
}

Composition&
Composition::operator=(const Composition &comp)
{

    m_tracks.clear();
    m_segments.clear();

    // Copy track information
    //
    trackcontainer tracks = comp.getTracks();
    for (trackcontainer::iterator i = tracks.begin(); i != tracks.end(); ++i) 
	m_tracks[i->first] = new Track(*(i->second));

    // Copy segments
    //
    segmentcontainer segments = comp.getSegments();
    for (segmentcontainer::iterator i = segments.begin();
         i != segments.end(); ++i)
	m_segments.insert(new Segment(**i));

    for (segmentcontainer::iterator i = this->m_segments.begin();
	 i != this->m_segments.end(); ++i) {
	(*i)->setComposition(this);
    }

    m_recordTrack = comp.getRecordTrack();
    m_solo = comp.isSolo();
    m_selectedTrack = comp.getSelectedTrack();

    m_timeSigSegment = comp.getTimeSigSegment();
    m_tempoSegment = comp.getTempoSegment();

    m_basicQuantizer = *comp.getBasicQuantizer();
    m_noteQuantizer = *comp.getNoteQuantizer();
    m_legatoQuantizer = *comp.getLegatoQuantizer();
    m_position = comp.getPosition();
    m_defaultTempo = comp.getDefaultTempo();
    m_startMarker = comp.getStartMarker();
    m_endMarker = comp.getEndMarker();
    m_loopStart = comp.getLoopStart();
    m_loopEnd = comp.getLoopEnd();
    m_barPositionsNeedCalculating = true;
    m_tempoTimestampsNeedCalculating = true;

    m_metadata = comp.getMetadata();

    m_playMetronome = comp.usePlayMetronome();
    m_recordMetronome = comp.useRecordMetronome();
    m_needsRefresh = true;

    return *this;
}



void Composition::swap(Composition& c)
{
    // ugh.

    //!!! some data members are not being swapped yet

    Composition *that = &c;

    double tp = this->m_defaultTempo;
    this->m_defaultTempo = that->m_defaultTempo;
    that->m_defaultTempo = tp;

    m_timeSigSegment.swap(c.m_timeSigSegment);
    m_tempoSegment.swap(c.m_tempoSegment);

    // swap tracks
    //
    m_tracks.swap(c.m_tracks);

    // swap all the segments
    //
    m_segments.swap(c.m_segments);

    for (segmentcontainer::iterator i = that->m_segments.begin();
	 i != that->m_segments.end(); ++i) {
	(*i)->setComposition(that);
    }

    for (segmentcontainer::iterator i = this->m_segments.begin();
	 i != this->m_segments.end(); ++i) {
	(*i)->setComposition(this);
    }

    this->m_barPositionsNeedCalculating = true;
    that->m_barPositionsNeedCalculating = true;
    
    this->m_tempoTimestampsNeedCalculating = true;
    that->m_tempoTimestampsNeedCalculating = true;
}

#ifdef NOT_DEFINED

void
Composition::mergeFrom(Composition &c, MergeType type)
{
    // We want to make new Segments from the Segments in the other
    // Composition, and we want to apply the time signatures and
    // tempos only if the merge type is MergeAtEnd (actually we should
    // have more merge types to select this capability more
    // accurately).  The new Segments always have to go in new Tracks,
    // but I'm not quite sure how best to create them, or to manage
    // the Studio side of things yet.  For the moment let's leave our
    // own Studio unchanged.

    // What we need to do is go through each of the other
    // Composition's Tracks, look for an instrument, and use our own
    // Studio's assignMidiProgramToInstrument on that instrument's
    // program and bank in order to get a "local" Instrument that we
    // can attach a new Track to.

    // But, we can't iterate through a Composition's Tracks and then
    // iterate through the Segments for each Track; we have to iterate
    // through the Tracks, create a new local Track for each, record
    // the offset of the first TrackId, then iterate through the
    // Segments placing each on the Track with the appropriately
    // offset TrackId.

    TrackId maxOldTrackId = getMaxTrackId();
    TrackId minNewTrackId = c.getMinTrackId();
    TrackId prevOrigId = 0;

    for (iterator i = c.begin(); i != c.end(); ++i) {

	TrackId origId = (*i)->getTrack();
	TrackId localId = id + maxOldTrackId - minNewTrackId;

	if (i == c.begin() || origId != prevOrigId) {
	    
	    // new track

	    Instrument *remoteInstrument = c.getTrackByIndex(origId)->getInstrument();
	    

	    Track *track = new Track(localId,
				     instrument,
				     getTracks()->size(),
				     name,
				     false);


    for (trackcontainer::iterator i = c.getTracks()->begin();
	 i != c.getTracks()->end(); ++i) {
	if (long((*i)->getId()) < minNewTrackId || minNewTrackId == -1) {
	    minNewTrackId = (*i)->getId();
	}
    }


#endif


void
Composition::setLegatoQuantizerDuration(timeT duration)
{
    Quantizer q(Quantizer::RawEventData, "LegatoQ",
		Quantizer::LegatoQuantize, duration);
    m_legatoQuantizer = q;
}
    

Composition::iterator
Composition::addSegment(Segment *segment)
{
    cerr << "Composition::addSegment: segment is " << segment
	      << ", with track " << segment->getTrack() << " and start index "
	      << segment->getStartTime() << "; currently have " << m_segments.size() << " segments"
	      << endl;

    if (!segment) return end();
    
    iterator res = m_segments.insert(segment);
    segment->setComposition(this);

    cerr << "Composition::addSegment: added segment, now have "
	      << m_segments.size() << " segments" << endl;

    updateRefreshStatuses();

    return res;
}

void
Composition::deleteSegment(Composition::iterator i)
{
    if (i == end()) return;

    Segment *p = (*i);
    p->setComposition(0);

    delete p;
    m_segments.erase(i);

    updateRefreshStatuses();
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
Composition::detachSegment(Segment *p)
{
    iterator i = find(begin(), end(), p);
    if (i == end()) return false;
    
    p->setComposition(0);
    m_segments.erase(i);

    updateRefreshStatuses();

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
Composition::clear()
{
    for (segmentcontainer::iterator i = m_segments.begin();
        i != m_segments.end(); ++i) {
	(*i)->setComposition(0);
        delete (*i);
    }
    m_segments.erase(begin(), end());
    m_timeSigSegment.clear();
    m_tempoSegment.clear();
    m_loopStart = 0;
    m_loopEnd = 0;
    m_position = 0;
    m_startMarker = 0;
    m_endMarker = 0;
    m_solo = false;
    m_selectedTrack = 0;
//     m_countInBars = DefaultCountInBars;
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
	i = m_timeSigSegment.findTime(t);
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


// Insert a Track representation into the Composition
//
void Composition::addTrack(Track *track)
{
    m_tracks[track->getId()] = track;
    updateRefreshStatuses();
}


void Composition::deleteTrack(const int &track)
{
     trackiterator titerator = m_tracks.find(track);

     delete ((*titerator).second);
     m_tracks.erase(titerator);
    updateRefreshStatuses();
}

TrackId
Composition::getMinTrackId() const
{
    long m = -1;

    for (trackcontainer::const_iterator i = getTracks().begin();
	 i != getTracks().end(); ++i) {
	if (long(i->second->getId()) < m || m == -1) {
	    m = i->second->getId();
	}
    }

    if (m == -1) return 0;
    else return TrackId(m);
}

TrackId
Composition::getMaxTrackId() const
{
    long m = -1;

    for (trackcontainer::const_iterator i = getTracks().begin();
	 i != getTracks().end(); ++i) {
	if (long(i->second->getId()) > m) {
	    m = i->second->getId();
	}
    }

    if (m == -1) return 0;
    else return TrackId(m);
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

    for (trackiterator tit = getTracks()->begin();
                        tit != getTracks()->end();
                        tit++ )
    {
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
		<< "</metadata>" << endl;

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
Composition::getTrackByPosition(TrackId position)
{
    trackiterator it = m_tracks.begin();

    for (; it != m_tracks.end(); it++)
    {
        if ((*it).second->getPosition() == position)
            return (*it).second;
    }

    return 0;

}

}


