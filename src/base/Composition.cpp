/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Composition]"

#include "Composition.h"

#include "misc/Debug.h"
#include "base/Segment.h"
#include "base/SegmentLinker.h"
#include "FastVector.h"
#include "base/BaseProperties.h"
#include "base/Profiler.h"
#include "BasicQuantizer.h"
#include "NotationQuantizer.h"
#include "base/AudioLevel.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <typeinfo>
#include <iterator>
#include <map>
#include <limits.h>

#include <sstream>

using std::cerr;
using std::endl;

//#define DEBUG_BAR_STUFF 1
//#define DEBUG_TEMPO_STUFF 1


namespace Rosegarden 
{

const PropertyName Composition::NoAbsoluteTimeProperty = "NoAbsoluteTime";
const PropertyName Composition::BarNumberProperty = "BarNumber";

const std::string Composition::TempoEventType = "tempo";
const PropertyName Composition::TempoProperty = "Tempo";
const PropertyName Composition::TargetTempoProperty = "TargetTempo";
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
Composition::ReferenceSegment::insertEvent(Event *e)
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
Composition::ReferenceSegment::eraseEvent(Event *e)
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
    m_notationSpacing(100),
    m_selectedTrackId(0),
    m_timeSigSegment(TimeSignature::EventType),
    m_tempoSegment(TempoEventType),
    m_barPositionsNeedCalculating(true),
    m_tempoTimestampsNeedCalculating(true),
    m_basicQuantizer(new BasicQuantizer()),
    m_notationQuantizer(new NotationQuantizer()),
    m_position(0),
    m_defaultTempo(getTempoForQpm(120.0)),
    m_minTempo(0),
    m_maxTempo(0),
    m_startMarker(0),
    m_endMarker(getBarRange(m_defaultNbBars).first),
    m_autoExpand(false),
    m_loopStart(0),
    m_loopEnd(0),
    m_playMetronome(false),
    m_recordMetronome(true),
    m_nextTriggerSegmentId(0)
{
    // nothing else
}

Composition::~Composition()
{
    if (!m_observers.empty()) {
        cerr << "Warning: Composition::~Composition() with " << m_observers.size()
             << " observers still extant" << endl;
        cerr << "Observers are:";
        for (ObserverSet::const_iterator i = m_observers.begin();
             i != m_observers.end(); ++i) {
            cerr << " " << (void *)(*i);
            cerr << " [" << typeid(**i).name() << "]";
        }
        cerr << endl;
    }

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
        distributeVerses();
        notifySegmentAdded(segment);
    }
    
    return res;
}

Composition::iterator
Composition::weakAddSegment(Segment *segment)
{
    if (!segment) return end();
    clearVoiceCaches();
    
    iterator res = m_segments.insert(segment);
    segment->setComposition(this);

    return res;
}

void
Composition::deleteSegment(Composition::iterator i)
{
    if (i == end()) return;
    clearVoiceCaches();

    Segment *p = (*i);
    p->setComposition(nullptr);

    m_segments.erase(i);
    distributeVerses();
    notifySegmentRemoved(p);

    // ??? If this delete occurs during playback, we may get a crash later in
    //     MappedBufMetaIterator::fetchEvents().
    delete p;

    updateRefreshStatuses();
}

bool
Composition::deleteSegment(Segment *segment)
{
    iterator i = findSegment(segment);
    if (i == end()) return false;

    deleteSegment(i);
    return true;
}

bool
Composition::detachSegment(Segment *segment)
{
    bool res = weakDetachSegment(segment);

    if (res) {
        distributeVerses();
        notifySegmentRemoved(segment);
        updateRefreshStatuses();
    }

    return res;
}

bool
Composition::weakDetachSegment(Segment *segment)
{
    iterator i = findSegment(segment);
    if (i == end()) return false;
    clearVoiceCaches();
    
    segment->setComposition(nullptr);
    m_segments.erase(i);

    return true;
}

// Add every segment in SegmentMultiSet
// @author Tom Breton (Tehom)
void
Composition::addAllSegments(SegmentMultiSet segments)
{
    for (SegmentMultiSet::iterator i = segments.begin();
         i != segments.end();
         ++i)
        { addSegment(*i); }
}

void
Composition::addAllSegments(SegmentVec segments)
{
    for (SegmentVec::iterator i = segments.begin();
         i != segments.end();
         ++i)
        { addSegment(*i); }
}

// Detach every segment in SegmentMultiSet
// @author Tom Breton (Tehom)
void
Composition::detachAllSegments(SegmentMultiSet segments)
{
    for (SegmentMultiSet::iterator i = segments.begin();
         i != segments.end();
         ++i)
        { detachSegment(*i); }    
}

void
Composition::detachAllSegments(SegmentVec segments)
{
    for (SegmentVec::iterator i = segments.begin();
         i != segments.end();
         ++i)
        { detachSegment(*i); }    
}

bool
Composition::contains(const Segment *s)
{
    iterator i = findSegment(s);
    return (i != end());
}

Composition::iterator
Composition::findSegment(const Segment *s)
{
    iterator i = m_segments.lower_bound(const_cast<Segment*>(s));

    while (i != end()) {
        if (*i == s) break;
        if ((*i)->getStartTime() > s->getStartTime()) return end();
        ++i;
    }

    return i;
}

void Composition::setSegmentStartTime(Segment *segment, timeT startTime)
{
    Profiler profiler("Composition::setSegmentStartTime");
    // remove the segment from the multiset
    iterator i = findSegment(segment);
    if (i == end()) return;

    clearVoiceCaches();
    
    m_segments.erase(i);

    segment->setStartTimeDataMember(startTime);

    // re-add it
    m_segments.insert(segment);
}

void
Composition::clearVoiceCaches()
{
//    std::cerr << "Composition::clearVoiceCaches" << std::endl;
    m_trackVoiceCountCache.clear();
    m_segmentVoiceIndexCache.clear();
}

void
Composition::rebuildVoiceCaches() const
{
//    std::cerr << "Composition::rebuildVoiceCaches" << std::endl;
    Profiler profiler("Composition::rebuildVoiceCaches");

    // slow
    
    m_trackVoiceCountCache.clear();
    m_segmentVoiceIndexCache.clear();

    for (trackcontainer::const_iterator tci = m_tracks.begin();
         tci != m_tracks.end(); ++tci) {

        TrackId tid = tci->first;
        
        std::multimap<timeT, Segment *> ends;
        
        for (const_iterator i = begin(); i != end(); ++i) {
            if ((*i)->getTrack() != tid) continue;
            timeT t0 = (*i)->getStartTime();
            timeT t1 = (*i)->getRepeatEndTime();
//            std::cerr << "track " << tid << " segment " << *i
//                      << " " << t0 << " to " << t1 << std::endl;
            int index = 0;
            std::multimap<timeT, Segment *>::iterator ei = ends.end();
            std::set<int> used;
            while (ei != ends.begin()) {
                --ei;
                if (ei->first <= t0) break;
                used.insert(m_segmentVoiceIndexCache[ei->second]);
            }
            if (!used.empty()) {
                for (index = 0; ; ++index) {
                    if (used.find(index) == used.end()) break;
                }
            }
            m_segmentVoiceIndexCache[*i] = index;
            if (index >= m_trackVoiceCountCache[tid]) {
//                std::cerr << "count to " << index + 1 << std::endl;
                m_trackVoiceCountCache[tid] = index + 1;
            }
            ends.insert(std::multimap<timeT, Segment *>::value_type(t1, *i));
        }
    }
}

int
Composition::getMaxContemporaneousSegmentsOnTrack(TrackId track) const
{
    Profiler profiler("Composition::getMaxContemporaneousSegmentsOnTrack");

    if (m_trackVoiceCountCache.empty()) {
        rebuildVoiceCaches();
    }

    int count = m_trackVoiceCountCache[track];
//    std::cerr << "max contemporaneous on track " << track << " = " << count << std::endl;
    return count;
}

int
Composition::getSegmentVoiceIndex(const Segment *segment) const
{
    if (m_segmentVoiceIndexCache.empty()) {
        rebuildVoiceCaches();
    }

    return m_segmentVoiceIndexCache[segment];
}

void
Composition::resetLinkedSegmentRefreshStatuses()
{
    std::set<const SegmentLinker *> linkers;
    for (iterator itr = begin(); itr != end(); ++itr) {
        Segment *segment = *itr;
        if (segment->isLinked()) {
            SegmentLinker *linker = segment->getLinker();
            std::set<const SegmentLinker *>::const_iterator finder = 
                                                           linkers.find(linker);
            if (finder == linkers.end()) {
                linker->clearRefreshStatuses();
                linkers.insert(linker);
            }
        }
    }
}

TriggerSegmentRec *
Composition::addTriggerSegment(Segment *s, int pitch, int velocity)
{
    TriggerSegmentId id = m_nextTriggerSegmentId;
    return addTriggerSegment(s, id, pitch, velocity);
}

TriggerSegmentRec *
Composition::addTriggerSegment(Segment *s, TriggerSegmentId id, int pitch, int velocity)
{
    TriggerSegmentRec *rec = getTriggerSegmentRec(id);
    if (rec) return nullptr;
    rec = new TriggerSegmentRec(id, s, pitch, velocity);
    m_triggerSegments.insert(rec);
    s->setComposition(this);
    if (m_nextTriggerSegmentId <= id) m_nextTriggerSegmentId = id + 1;
    return rec;
}

void
Composition::deleteTriggerSegment(TriggerSegmentId id)
{
    TriggerSegmentRec dummyRec(id, nullptr);
    triggersegmentcontaineriterator i = m_triggerSegments.find(&dummyRec);
    if (i == m_triggerSegments.end()) return;
    (*i)->getSegment()->setComposition(nullptr);
    delete (*i)->getSegment();
    delete *i;
    m_triggerSegments.erase(i);
}

void
Composition::detachTriggerSegment(TriggerSegmentId id)
{
    TriggerSegmentRec dummyRec(id, nullptr);
    triggersegmentcontaineriterator i = m_triggerSegments.find(&dummyRec);
    if (i == m_triggerSegments.end()) return;
    (*i)->getSegment()->setComposition(nullptr);
    delete *i;
    m_triggerSegments.erase(i);
}

void
Composition::clearTriggerSegments()
{
    for (triggersegmentcontaineriterator i = m_triggerSegments.begin();
         i != m_triggerSegments.end(); ++i) {
        delete (*i)->getSegment();
        delete *i;
    }
    m_triggerSegments.clear();
}

int
Composition::getTriggerSegmentId(Segment *s)
{
    for (triggersegmentcontaineriterator i = m_triggerSegments.begin();
         i != m_triggerSegments.end(); ++i) {
        if ((*i)->getSegment() == s) return (*i)->getId();
    }
    return -1;
}

Segment *
Composition::getTriggerSegment(TriggerSegmentId id)
{
    TriggerSegmentRec *rec = getTriggerSegmentRec(id);
    if (!rec) return nullptr;
    return rec->getSegment();
}    

TriggerSegmentRec *
Composition::getTriggerSegmentRec(TriggerSegmentId id)
{
    TriggerSegmentRec dummyRec(id, nullptr);
    triggersegmentcontaineriterator i = m_triggerSegments.find(&dummyRec);
    if (i == m_triggerSegments.end()) return nullptr;
    return *i;
}

TriggerSegmentRec *
Composition::getTriggerSegmentRec(Event* e)
{
    if (!e->has(BaseProperties::TRIGGER_SEGMENT_ID))
        { return nullptr; }

    const int id = e->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID);
    return getTriggerSegmentRec(id);
}

TriggerSegmentId
Composition::getNextTriggerSegmentId() const
{
    return m_nextTriggerSegmentId;
}

void
Composition::setNextTriggerSegmentId(TriggerSegmentId id)
{
    m_nextTriggerSegmentId = id;
}

void
Composition::updateTriggerSegmentReferences()
{
    std::map<TriggerSegmentId, TriggerSegmentRec::SegmentRuntimeIdSet> refs;

    for (iterator i = begin(); i != end(); ++i) {
        for (Segment::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {
            if ((*j)->has(BaseProperties::TRIGGER_SEGMENT_ID)) {
                TriggerSegmentId id =
                    (*j)->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID);
                refs[id].insert((*i)->getRuntimeId());
            }
        }
    }
    
    for (std::map<TriggerSegmentId,
                  TriggerSegmentRec::SegmentRuntimeIdSet>::iterator i = refs.begin();
         i != refs.end(); ++i) {
        TriggerSegmentRec *rec = getTriggerSegmentRec(i->first);
        if (rec) rec->setReferences(i->second);
    }
}


timeT
Composition::getDuration() const
{
    timeT maxDuration = 0;

    for (SegmentMultiSet::const_iterator i = m_segments.begin();
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
    clearVoiceCaches();
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
    clearMarkers();
    clearTriggerSegments();

    m_timeSigSegment.clear();
    m_tempoSegment.clear();
    m_defaultTempo = getTempoForQpm(120.0);
    m_minTempo = 0;
    m_maxTempo = 0;
    m_loopStart = 0;
    m_loopEnd = 0;
    m_position = 0;
    m_startMarker = 0;
    m_endMarker = getBarRange(m_defaultNbBars).first;
    m_selectedTrackId = 0;
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

    if (getStartMarker() < 0) {
        if (!t.empty() && (*t.begin())->getAbsoluteTime() <= 0) {
            barDuration = TimeSignature(**t.begin()).getBarDuration();
        }
        lastBarNo = getStartMarker() / barDuration;
        lastSigTime = getStartMarker();
#ifdef DEBUG_BAR_STUFF
        cerr << "Composition::calculateBarPositions: start marker = " << getStartMarker() << ", so initial bar number = " << lastBarNo << endl;
#endif
    }

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


#ifdef RWSTD_NO_CLASS_PARTIAL_SPEC
  template <class itr> inline int distance(itr first, itr last)
  {
    int n = 0;
    distance(first, last, n);
    return n;
  }
#else
  using std::distance;
#endif
    

int
Composition::addTimeSignature(timeT t, TimeSignature timeSig)
{
#ifdef DEBUG_BAR_STUFF
    cerr << "Composition::addTimeSignature(" << t << ", " << timeSig.getNumerator() << "/" << timeSig.getDenominator() << ")" << endl;
#endif

    ReferenceSegment::iterator i =
        m_timeSigSegment.insertEvent(timeSig.getAsEvent(t));
    m_barPositionsNeedCalculating = true;

    updateRefreshStatuses();
    notifyTimeSignatureChanged();

    return distance(m_timeSigSegment.begin(), i);
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
    return int(m_timeSigSegment.size());
}

int
Composition::getTimeSignatureNumberAt(timeT t) const
{
    ReferenceSegment::iterator i = getTimeSignatureAtAux(t);
    if (i == m_timeSigSegment.end()) return -1;
    else return distance(m_timeSigSegment.begin(), i);
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
    m_timeSigSegment.eraseEvent(m_timeSigSegment[n]);
    m_barPositionsNeedCalculating = true;
    updateRefreshStatuses();
    notifyTimeSignatureChanged();
}


tempoT
Composition::getTempoAtTime(timeT t) const
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
            return getTempoAtTime(0);
        }
        else return m_defaultTempo;
    }
    
    tempoT tempo = (tempoT)((*i)->get<Int>(TempoProperty));

    if ((*i)->has(TargetTempoProperty)) {

        tempoT target = (tempoT)((*i)->get<Int>(TargetTempoProperty));
        ReferenceSegment::iterator j = i;
        ++j;

        if (target > 0 || (target == 0 && j != m_tempoSegment.end())) {

            timeT t0 = (*i)->getAbsoluteTime();
            timeT t1 = (j != m_tempoSegment.end() ?
                        (*j)->getAbsoluteTime() : getEndMarker());

            if (t1 < t0) return tempo;

            if (target == 0) {
                target = (tempoT)((*j)->get<Int>(TempoProperty));
            }

            // tempo ramps are linear in 1/tempo
            double s0 = 1.0 / double(tempo);
            double s1 = 1.0 / double(target);
            double s = s0 + (t - t0) * ((s1 - s0) / (t1 - t0));

            tempoT result = tempoT((1.0 / s) + 0.01);

#ifdef DEBUG_TEMPO_STUFF
            cerr << "Composition: Calculated tempo " << result << " at " << t << endl;
#endif

            return result;
        }
    }

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition: Found tempo " << tempo << " at " << t << endl;
#endif
    return tempo;
}

int
Composition::addTempoAtTime(timeT time, tempoT tempo, tempoT targetTempo)
{
    // If there's an existing tempo at this time, the ReferenceSegment
    // object will remove the duplicate, but we have to ensure that
    // the minimum and maximum tempos are updated if necessary.

    bool fullTempoUpdate = false;

    int n = getTempoChangeNumberAt(time);
    if (n >= 0) {
        std::pair<timeT, tempoT> tc = getTempoChange(n);
        if (tc.first == time) {
            if (tc.second == m_minTempo || tc.second == m_maxTempo) {
                fullTempoUpdate = true;
            } else {
                std::pair<bool, tempoT> tr = getTempoRamping(n);
                if (tr.first &&
                    (tr.second == m_minTempo || tr.second == m_maxTempo)) {
                    fullTempoUpdate = true;
                }
            }
        }
    }

    Event *tempoEvent = new Event(TempoEventType, time);
    tempoEvent->set<Int>(TempoProperty, tempo);

    if (targetTempo >= 0) {
        tempoEvent->set<Int>(TargetTempoProperty, targetTempo);
    }

    ReferenceSegment::iterator i = m_tempoSegment.insertEvent(tempoEvent);

    if (fullTempoUpdate) {

        updateExtremeTempos();

    } else {

        if (tempo < m_minTempo || m_minTempo == 0) m_minTempo = tempo;
        if (targetTempo > 0 && targetTempo < m_minTempo) m_minTempo = targetTempo;

        if (tempo > m_maxTempo || m_maxTempo == 0) m_maxTempo = tempo;
        if (targetTempo > 0 && targetTempo > m_maxTempo) m_maxTempo = targetTempo;
    }

    m_tempoTimestampsNeedCalculating = true;
    updateRefreshStatuses();

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition: Added tempo " << tempo << " at " << time << endl;
#endif
    notifyTempoChanged();

    return distance(m_tempoSegment.begin(), i);
}

int
Composition::getTempoChangeCount() const
{
    return int(m_tempoSegment.size());
}

int
Composition::getTempoChangeNumberAt(timeT t) const
{
    ReferenceSegment::iterator i = m_tempoSegment.findNearestTime(t);
    if (i == m_tempoSegment.end()) return -1;
    else return distance(m_tempoSegment.begin(), i);
}

std::pair<timeT, tempoT>
Composition::getTempoChange(int n) const
{
    return std::pair<timeT, tempoT>
        (m_tempoSegment[n]->getAbsoluteTime(),
         tempoT(m_tempoSegment[n]->get<Int>(TempoProperty)));
}

std::pair<bool, tempoT>
Composition::getTempoRamping(int n, bool calculate) const
{
    tempoT target = -1;
    if (m_tempoSegment[n]->has(TargetTempoProperty)) {
        target = m_tempoSegment[n]->get<Int>(TargetTempoProperty);
    }
    bool ramped = (target >= 0);
    if (target == 0) {
        if (calculate) {
            if (int(m_tempoSegment.size()) > n+1) {
                target = m_tempoSegment[n+1]->get<Int>(TempoProperty);
            }
        }
    }
    if (target < 0 || (calculate && (target == 0))) {
        target = m_tempoSegment[n]->get<Int>(TempoProperty);
    }
    return std::pair<bool, tempoT>(ramped, target);
}

void
Composition::removeTempoChange(int n)
{
    tempoT oldTempo = m_tempoSegment[n]->get<Int>(TempoProperty);
    tempoT oldTarget = -1;

    if (m_tempoSegment[n]->has(TargetTempoProperty)) {
        oldTarget = m_tempoSegment[n]->get<Int>(TargetTempoProperty);
    }

    m_tempoSegment.eraseEvent(m_tempoSegment[n]);
    m_tempoTimestampsNeedCalculating = true;

    if (oldTempo == m_minTempo ||
        oldTempo == m_maxTempo ||
        (oldTarget > 0 && oldTarget == m_minTempo) ||
        (oldTarget > 0 && oldTarget == m_maxTempo)) {
        updateExtremeTempos();
    }

    updateRefreshStatuses();
    notifyTempoChanged();
}

void
Composition::updateExtremeTempos()
{
    m_minTempo = 0;
    m_maxTempo = 0;
    for (ReferenceSegment::iterator i = m_tempoSegment.begin();
         i != m_tempoSegment.end(); ++i) {
        tempoT tempo = (*i)->get<Int>(TempoProperty);
        tempoT target = -1;
        if ((*i)->has(TargetTempoProperty)) {
            target = (*i)->get<Int>(TargetTempoProperty);
        }
        if (tempo < m_minTempo || m_minTempo == 0) m_minTempo = tempo;
        if (target > 0 && target < m_minTempo) m_minTempo = target;
        if (tempo > m_maxTempo || m_maxTempo == 0) m_maxTempo = tempo;
        if (target > 0 && target > m_maxTempo) m_maxTempo = target;
    }
    if (m_minTempo == 0) {
        m_minTempo = m_defaultTempo;
        m_maxTempo = m_defaultTempo;
    }
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

    RealTime elapsed;

    tempoT target = -1;
    timeT nextTempoTime = t;

    if (!getTempoTarget(i, target, nextTempoTime)) target = -1;

    if (target > 0) {
        elapsed = getTempoTimestamp(*i) +
            time2RealTime(t - (*i)->getAbsoluteTime(),
                          tempoT((*i)->get<Int>(TempoProperty)),
                          nextTempoTime - (*i)->getAbsoluteTime(),
                          target);
    } else {
        elapsed = getTempoTimestamp(*i) +
            time2RealTime(t - (*i)->getAbsoluteTime(),
                          tempoT((*i)->get<Int>(TempoProperty)));
    }

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition::getElapsedRealTime: " << t << " -> "
         << elapsed << " (last tempo change at " << (*i)->getAbsoluteTime() << ")" << endl;
#endif

    return elapsed;
}

timeT
Composition::getElapsedTimeForRealTime(RealTime t) const
{
    calculateTempoTimestamps();

    ReferenceSegment::iterator i = m_tempoSegment.findNearestRealTime(t);
    if (i == m_tempoSegment.end()) {
        i = m_tempoSegment.begin();
        if (t >= RealTime::zeroTime ||
            (i == m_tempoSegment.end() || (*i)->getAbsoluteTime() > 0)) {
            return realTime2Time(t, m_defaultTempo);
        }
    }

    timeT elapsed;

    tempoT target = -1;
    timeT nextTempoTime = 0;
    if (!getTempoTarget(i, target, nextTempoTime)) target = -1;

    if (target > 0) {
        elapsed = (*i)->getAbsoluteTime() +
            realTime2Time(t - getTempoTimestamp(*i),
                          (tempoT)((*i)->get<Int>(TempoProperty)),
                          nextTempoTime - (*i)->getAbsoluteTime(),
                          target);
    } else {
        elapsed = (*i)->getAbsoluteTime() +
            realTime2Time(t - getTempoTimestamp(*i),
                          (tempoT)((*i)->get<Int>(TempoProperty)));
    }

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
             << (tempoT)((*i)->get<Int>(TempoProperty)) << ")" << endl;
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

    tempoT tempo = m_defaultTempo;
    tempoT target = -1;

#ifdef DEBUG_TEMPO_STUFF
    cerr << "Composition::calculateTempoTimestamps: Tempo events are:" << endl;
#endif

    for (ReferenceSegment::iterator i = m_tempoSegment.begin();
         i != m_tempoSegment.end(); ++i) {

        RealTime myTime;

        if (target > 0) {
            myTime = lastRealTime +
                time2RealTime((*i)->getAbsoluteTime() - lastTimeT, tempo,
                              (*i)->getAbsoluteTime() - lastTimeT, target);
        } else {
            myTime = lastRealTime +
                time2RealTime((*i)->getAbsoluteTime() - lastTimeT, tempo);
        }

        setTempoTimestamp(*i, myTime);

#ifdef DEBUG_TEMPO_STUFF
        (*i)->dump(cerr);
#endif

        lastRealTime = myTime;
        lastTimeT = (*i)->getAbsoluteTime();
        tempo = tempoT((*i)->get<Int>(TempoProperty));

        target = -1;
        timeT nextTempoTime = 0;
        if (!getTempoTarget(i, target, nextTempoTime)) target = -1;
    }

    m_tempoTimestampsNeedCalculating = false;
}

#ifdef DEBUG_TEMPO_STUFF
static int DEBUG_silence_recursive_tempo_printout = 0;
#endif

RealTime
Composition::time2RealTime(timeT t, tempoT tempo) const
{
    static timeT cdur = Note(Note::Crotchet).getDuration();

    double dt = (double(t) * 100000 * 60) / (double(tempo) * cdur);

    int sec = int(dt);
    int nsec = int((dt - sec) * 1000000000);
   
    RealTime rt(sec, nsec);

#ifdef DEBUG_TEMPO_STUFF
    if (!DEBUG_silence_recursive_tempo_printout) {
        cerr << "Composition::time2RealTime: t " << t << ", sec " << sec << ", nsec "
             << nsec << ", tempo " << tempo
             << ", cdur " << cdur << ", dt " << dt << ", rt " << rt << endl;
        DEBUG_silence_recursive_tempo_printout = 1;
        timeT ct = realTime2Time(rt, tempo);
        timeT et = t - ct;
        RealTime ert = time2RealTime(et, tempo);
        cerr << "cf. realTime2Time(" << rt << ") -> " << ct << " [err " << et << " (" << ert << "?)]" << endl;
        DEBUG_silence_recursive_tempo_printout=0;
    }
#endif

    return rt;
}

RealTime
Composition::time2RealTime(timeT time, tempoT tempo,
                           timeT targetTime, tempoT targetTempo) const
{
    static timeT cdur = Note(Note::Crotchet).getDuration();

    // The real time elapsed at musical time t, in seconds, during a
    // smooth tempo change from "tempo" at musical time zero to
    // "targetTempo" at musical time "targetTime", is
    // 
    //           2 
    //     at + t (b - a)
    //          ---------
    //             2n
    // where
    // 
    // a is the initial tempo in seconds per tick
    // b is the target tempo in seconds per tick
    // n is targetTime in ticks

    if (targetTime == 0 || targetTempo == tempo) {
        return time2RealTime(time, targetTempo);
    }

    double a = (100000 * 60) / (double(tempo) * cdur);
    double b = (100000 * 60) / (double(targetTempo) * cdur);
    double t = time;
    double n = targetTime;
    double result = (a * t) + (t * t * (b - a)) / (2 * n);

    int sec = int(result);
    int nsec = int((result - sec) * 1000000000);
   
    RealTime rt(sec, nsec);

#ifdef DEBUG_TEMPO_STUFF
    if (!DEBUG_silence_recursive_tempo_printout) {
        cerr << "Composition::time2RealTime[2]: time " << time << ", tempo "
             << tempo << ", targetTime " << targetTime << ", targetTempo "
             << targetTempo << ": rt " << rt << endl;
        DEBUG_silence_recursive_tempo_printout = 1;
//        RealTime nextRt = time2RealTime(targetTime, tempo, targetTime, targetTempo);
        timeT ct = realTime2Time(rt, tempo, targetTime, targetTempo);
        cerr << "cf. realTime2Time: rt " << rt << " -> " << ct << endl;
        DEBUG_silence_recursive_tempo_printout=0;
    }
#endif

    return rt;
}

timeT
Composition::realTime2Time(RealTime rt, tempoT tempo) const
{
    static timeT cdur = Note(Note::Crotchet).getDuration();

    double tsec = (double(rt.sec) * cdur) * (tempo / (60.0 * 100000.0));
    double tnsec = (double(rt.nsec) * cdur) * (tempo / 100000.0);

    double dt = tsec + (tnsec / 60000000000.0);
    timeT t = (timeT)(dt + (dt < 0 ? -1e-6 : 1e-6));

#ifdef DEBUG_TEMPO_STUFF
    if (!DEBUG_silence_recursive_tempo_printout) {
        cerr << "Composition::realTime2Time: rt.sec " << rt.sec << ", rt.nsec "
             << rt.nsec << ", tempo " << tempo
             << ", cdur " << cdur << ", tsec " << tsec << ", tnsec " << tnsec << ", dt " << dt << ", t " << t << endl;
        DEBUG_silence_recursive_tempo_printout = 1;
        RealTime crt = time2RealTime(t, tempo);
        RealTime ert = rt - crt;
        timeT et = realTime2Time(ert, tempo);
        cerr << "cf. time2RealTime(" << t << ") -> " << crt << " [err " << ert << " (" << et << "?)]" << endl;
        DEBUG_silence_recursive_tempo_printout = 0;
    }
#endif

    return t;
}

timeT
Composition::realTime2Time(RealTime rt, tempoT tempo,
                           timeT targetTime, tempoT targetTempo) const
{
    static timeT cdur = Note(Note::Crotchet).getDuration();

    // Inverse of the expression in time2RealTime above.
    // 
    // The musical time elapsed at real time t, in ticks, during a
    // smooth tempo change from "tempo" at real time zero to
    // "targetTempo" at real time "targetTime", is
    // 
    //          2na (+/-) sqrt((2nb)^2 + 8(b-a)tn)
    //       -  ----------------------------------
    //                       2(b-a)
    // where
    // 
    // a is the initial tempo in seconds per tick
    // b is the target tempo in seconds per tick
    // n is target real time in ticks

    if (targetTempo == tempo) return realTime2Time(rt, tempo);

    double a = (100000 * 60) / (double(tempo) * cdur);
    double b = (100000 * 60) / (double(targetTempo) * cdur);
    double t = double(rt.sec) + double(rt.nsec) / 1e9;
    double n = targetTime;

    double term1 = 2.0 * n * a;
    double term2 = (2.0 * n * a) * (2.0 * n * a) + 8 * (b - a) * t * n;

    if (term2 < 0) { 
        // We're screwed, but at least let's not crash
        std::cerr << "ERROR: Composition::realTime2Time: term2 < 0 (it's " << term2 << ")" << std::endl;
#ifdef DEBUG_TEMPO_STUFF
        std::cerr << "rt = " << rt << ", tempo = " << tempo << ", targetTime = " << targetTime << ", targetTempo = " << targetTempo << std::endl;
        std::cerr << "n = " << n << ", b = " << b << ", a = " << a << ", t = " << t <<std::endl;
        std::cerr << "that's sqrt( (" << ((2.0*n*a*2.0*n*a)) << ") + "
                  << (8*(b-a)*t*n) << " )" << endl;

        std::cerr << "so our original expression was " << rt << " = "
                  << a << "t + (t^2 * (" << b << " - " << a << ")) / " << 2*n << std::endl;
#endif

        return realTime2Time(rt, tempo);
    }

    double term3 = sqrt(term2);

    // We only want the positive root
    if (term3 > 0) term3 = -term3;

    double result = - (term1 + term3) / (2 * (b - a));

#ifdef DEBUG_TEMPO_STUFF
    std::cerr << "Composition::realTime2Time:" <<endl;
    std::cerr << "n = " << n << ", b = " << b << ", a = " << a << ", t = " << t <<std::endl;
    std::cerr << "+/-sqrt(term2) = " << term3 << std::endl;
    std::cerr << "result = " << result << endl;
#endif

    return long(result + 0.1);
}

// @param A RealTime
// @param The same time in TimeT
// @param A target tempo to ramp to.  For now, this parameter is
// ignored and ramping is not supported.
// @returns A tempo that effects this relationship.  
// @author Tom Breton (Tehom)
tempoT
Composition::timeRatioToTempo(RealTime &realTime,
                                  timeT beatTime, tempoT)
{
    static const timeT cdur = Note(Note::Crotchet).getDuration();
    const double beatsPerMinute = realTime.toPerMinute();
    const double qpm = beatsPerMinute * double(beatTime) / double(cdur);
    tempoT averageTempo = Composition::getTempoForQpm(qpm);
    return averageTempo;
}

bool
Composition::getTempoTarget(ReferenceSegment::const_iterator i,
                            tempoT &target,
                            timeT &targetTime) const
{
    target = -1;
    targetTime = 0;
    bool have = false;

    if ((*i)->has(TargetTempoProperty)) {
        target = (*i)->get<Int>(TargetTempoProperty);
        if (target >= 0) {
            ReferenceSegment::const_iterator j(i);
            if (++j != m_tempoSegment.end()) {
                if (target == 0) target = (*j)->get<Int>(TempoProperty);
                targetTime = (*j)->getAbsoluteTime();
            } else {
                targetTime = getEndMarker();
                if (targetTime < (*i)->getAbsoluteTime()) {
                    target = -1;
                }
            }
            if (target > 0) have = true;
        }
    }

    return have;
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
Composition::getMusicalTimeForAbsoluteTime(timeT absTime,
                                           int &bar, int &beat,
                                           int &fraction, int &remainder)
{
    bar = getBarNumber(absTime);

    TimeSignature timeSig = getTimeSignatureAt(absTime);
    timeT barStart = getBarStart(bar);
    timeT beatDuration = timeSig.getBeatDuration();
    beat = (absTime - barStart) / beatDuration + 1;

    remainder = (absTime - barStart) % beatDuration;
    timeT fractionDuration = Note(Note::Shortest).getDuration();
    fraction = remainder / fractionDuration;
    remainder = remainder % fractionDuration;
}

void
Composition::getMusicalTimeForDuration(timeT absTime, timeT duration,
                                       int &bars, int &beats,
                                       int &fractions, int &remainder)
{
    TimeSignature timeSig = getTimeSignatureAt(absTime);
    timeT barDuration = timeSig.getBarDuration();
    timeT beatDuration = timeSig.getBeatDuration();

    bars = duration / barDuration;
    beats = (duration % barDuration) / beatDuration;
    remainder = (duration % barDuration) % beatDuration;
    timeT fractionDuration = Note(Note::Shortest).getDuration();
    fractions = remainder / fractionDuration;
    remainder = remainder % fractionDuration;
}

timeT
Composition::getAbsoluteTimeForMusicalTime(int bar, int beat,
                                           int fraction, int remainder)
{
    timeT t = getBarStart(bar - 1);
    TimeSignature timesig = getTimeSignatureAt(t);
    t += (beat-1) * timesig.getBeatDuration();
    t += Note(Note::Shortest).getDuration() * fraction;
    t += remainder;
    return t;
}

timeT
Composition::getDurationForMusicalTime(timeT absTime,
                                       int bars, int beats,
                                       int fractions, int remainder)
{
    TimeSignature timeSig = getTimeSignatureAt(absTime);
    timeT barDuration = timeSig.getBarDuration();
    timeT beatDuration = timeSig.getBeatDuration();
    timeT t = bars * barDuration + beats * beatDuration + fractions *
        Note(Note::Shortest).getDuration() + remainder;
    return t;
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

Track* Composition::getTrackById(TrackId track) const
{
    trackconstiterator i = m_tracks.find(track);

    if (i != m_tracks.end())
        return (*i).second;

    RG_WARNING << "Composition::getTrackById("
              << track << ") - WARNING - track id not found, this is probably a BUG "
              << __FILE__ << ":" << __LINE__;
    NOTATION_DEBUG << "Available track ids are:";
    for (trackconstiterator i = m_tracks.begin(); i != m_tracks.end(); ++i) {
        NOTATION_DEBUG << (int)(*i).second->getId();
    }

    return nullptr;
}

bool
Composition::haveTrack(TrackId track) const
{
    trackconstiterator i = m_tracks.find(track);
    return (i != m_tracks.end());
}

#if 0
// unused
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
        for (SegmentMultiSet::const_iterator i = m_segments.begin();
             i != m_segments.end(); ++i) 
        {
            if ((*i)->getTrack() == oldId) (*i)->setTrack(newId);
        }

        checkSelectedAndRecordTracks();
        updateRefreshStatuses();
        notifyTrackChanged(getTrackById(newId));
    }
    else
        std::cerr << "Composition::resetTrackIdAndPosition - "
                  << "can't move track " << oldId << " to " << newId 
                  << std::endl;
}
#endif

InstrumentId Composition::getSelectedInstrumentId() const
{
    if (m_selectedTrackId == NO_TRACK)
        return NoInstrument;

    Track *track = getTrackById(m_selectedTrackId);

    if (!track)
        return NoInstrument;

    return track->getInstrument();
}

void Composition::setSelectedTrack(TrackId trackId)
{
    m_selectedTrackId = trackId;

    // SequenceManager needs to update ControlBlock for auto thru routing
    // to work.
    notifySelectedTrackChanged();
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

#if 0
// unused
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
        notifyTrackDeleted(track);
    }
    
}
#endif

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

    ((*it).second)->setOwningComposition(nullptr);

    m_tracks.erase(it);
    updateRefreshStatuses();
    checkSelectedAndRecordTracks();

    return true;
}

void Composition::checkSelectedAndRecordTracks()
{
    // reset m_selectedTrackId and m_recordTrack to the next valid track id
    // if the track they point to has been deleted

    if (m_tracks.find(m_selectedTrackId) == m_tracks.end()) {

        m_selectedTrackId = getClosestValidTrackId(m_selectedTrackId);

        // SequenceManager needs to update ControlBlock for auto thru routing
        // to work.
        notifySelectedTrackChanged();
        
    }

    // For each record track
    for (recordtrackcontainer::iterator i = m_recordTracks.begin();
         i != m_recordTracks.end(); ) {
        // Increment before use.  This way deleting the element does not
        // invalidate the iterator.
        recordtrackcontainer::iterator j = i++;

        // If the track is no longer in the composition
        if (m_tracks.find(*j) == m_tracks.end()) {
            // Remove it from the list of record tracks.
            m_recordTracks.erase(j);
        }
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

void
Composition::setTrackRecording(TrackId trackId, bool recording)
{
    if (recording) {
        m_recordTracks.insert(trackId);
    } else {
        m_recordTracks.erase(trackId);
    }

    Track *track = getTrackById(trackId);

    if (!track)
        return;

    track->setArmed(recording);
}

bool
Composition::isTrackRecording(TrackId track) const
{
    return m_recordTracks.find(track) != m_recordTracks.end();
}

bool
Composition::isInstrumentRecording(InstrumentId instrumentID) const
{
    // For each Track in the Composition
    // ??? Performance: LINEAR SEARCH
    //     I see no easy fix.  Each Instrument would need to keep a list
    //     of the Tracks it is on.  Or something equally complicated.
    for (Composition::trackcontainer::const_iterator ti =
             m_tracks.begin();
         ti != m_tracks.end();
         ++ti) {
        Track *track = ti->second;

        // If this Track has this Instrument
        if (track->getInstrument() == instrumentID) {
            if (isTrackRecording(track->getId())) {
                // Only one Track can be armed per Instrument.  Regardless,
                // we only need to know that a Track is in record mode.
                return true;
            }
        }
    }

    return false;
}

// Export the Composition as XML, also iterates through
// Tracks and any further sub-objects
//
//
std::string Composition::toXmlString() const
{
    std::stringstream composition;

    composition << "<composition recordtracks=\"";
    for (recordtrackiterator i = m_recordTracks.begin();
         i != m_recordTracks.end(); ) {
        composition << *i;
        if (++i != m_recordTracks.end()) {
            composition << ",";
        }
    }
    composition << "\" pointer=\"" << m_position;
    composition << "\" defaultTempo=\"";
    composition << std::setiosflags(std::ios::fixed)
                << std::setprecision(4)
                << getTempoQpm(m_defaultTempo);
    composition << "\" compositionDefaultTempo=\"";
    composition << m_defaultTempo;

    if (m_loopStart != m_loopEnd)
    {
        composition << "\" loopstart=\"" << m_loopStart;
        composition << "\" loopend=\"" << m_loopEnd;
    }

    composition << "\" startMarker=\"" << m_startMarker;
    composition << "\" endMarker=\"" << m_endMarker;

    if (m_autoExpand)
        composition << "\" autoExpand=\"" << m_autoExpand;

    composition << "\" selected=\"" << m_selectedTrackId;
    composition << "\" playmetronome=\"" << m_playMetronome;
    composition << "\" recordmetronome=\"" << m_recordMetronome;
    composition << "\" nexttriggerid=\"" << m_nextTriggerSegmentId;

    // Place the number of the current pan law in the composition tag.
    int panLaw = AudioLevel::getPanLaw();
    composition << "\" panlaw=\"" << panLaw;

    composition << "\" notationspacing=\"" << m_notationSpacing;

    composition << "\">" << endl << endl;

    composition << endl;

    for (trackconstiterator tit = getTracks().begin();
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
        (*i)->get<Bool>(TimeSignature::IsHiddenPropertyName, hidden);
        if (hidden) composition << " hidden=\"true\"";

        bool hiddenBars = false;
        (*i)->get<Bool>(TimeSignature::HasHiddenBarsPropertyName, hiddenBars);
        if (hiddenBars) composition << " hiddenbars=\"true\"";

        composition << "/>" << endl;
    }

    composition << endl;

    for (ReferenceSegment::iterator i = m_tempoSegment.begin();
         i != m_tempoSegment.end(); ++i) {

        tempoT tempo = tempoT((*i)->get<Int>(TempoProperty));
        tempoT target = -1;
        if ((*i)->has(TargetTempoProperty)) {
            target = tempoT((*i)->get<Int>(TargetTempoProperty));
        }
        composition << "  <tempo time=\"" << (*i)->getAbsoluteTime()
                    << "\" bph=\"" << ((tempo * 6) / 10000)
                    << "\" tempo=\"" << tempo;
        if (target >= 0) {
            composition << "\" target=\"" << target;
        }
        composition << "\"/>" << endl;
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
    composition << "</composition>";

    return composition.str();
}

void
Composition::clearTracks()
{
    trackiterator it = m_tracks.begin();

    for (; it != m_tracks.end(); ++it)
        delete ((*it).second);

    m_tracks.erase(m_tracks.begin(), m_tracks.end());
}

Track*
Composition::getTrackByPosition(int position) const
{
    trackconstiterator it = m_tracks.begin();

    for (; it != m_tracks.end(); ++it)
    {
        if ((*it).second->getPosition() == position)
            return (*it).second;
    }

    return nullptr;

}

int
Composition::getTrackPositionById(TrackId id) const
{
    Track *track = getTrackById(id);
    if (!track) return -1;
    return track->getPosition();
}


Rosegarden::TrackId
Composition::getNewTrackId() const
{
    // Re BR #1070325: another track deletion problem
    // Formerly this was returning the count of tracks currently in
    // existence -- returning a duplicate ID if some had been deleted
    // from the middle.  Let's find one that's really available instead.

    TrackId highWater = 0;

    trackconstiterator it = m_tracks.begin();

    for (; it != m_tracks.end(); ++it)
    {
        if ((*it).second->getId() >= highWater)
            highWater = (*it).second->getId() + 1;
    }

    return highWater;
}

bool
Composition::hasTrack(InstrumentId instrumentId) const
{
    // We don't return the TrackId since an Instrument can be on more than
    // one Track.  That would require a std::vector<TrackId>.

    // For each Track
    for (trackcontainer::const_iterator trackIter =
                 m_tracks.begin();
         trackIter != m_tracks.end();
         ++trackIter) {

        // If this Track is using the Instrument
        if (trackIter->second->getInstrument() == instrumentId)
            return true;

    }

    return false;
}

// Get all the segments that the same instrument plays that plays
// segment s.
// @return a SegmentMultiSet that includes s itself.
SegmentMultiSet
Composition::getInstrumentSegments(Segment *s, timeT t) const
{
    SegmentMultiSet segments;
    InstrumentId Instrument = getInstrumentId(s);

    const SegmentMultiSet& allSegments = getSegments();
    for (Composition::iterator i = allSegments.begin();
         i != allSegments.end();
         ++i)
        {
            if (((*i)->getStartTime() < t) &&
                (getInstrumentId(*i) == Instrument))
                { segments.insert(*i); }
        }
    return segments;
}

void
Composition::enforceArmRule(const Track *track)
{
    // No more than one armed track per instrument.

    if (!track->isArmed())
        return;

    recordtrackcontainer recordTracks = getRecordTracks();

    // For each track that is armed for record
    for (recordtrackcontainer::const_iterator i =
            recordTracks.begin();
            i != recordTracks.end(); ++i) {

        const TrackId otherTrackId = *i;
        Track *otherTrack = getTrackById(otherTrackId);

        if (!otherTrack)
            continue;
        if (otherTrack == track)
            continue;

        // If this track is using the same instrument, unarm it.
        if (otherTrack->getInstrument() == track->getInstrument()) {
            setTrackRecording(otherTrackId, false);
            notifyTrackChanged(otherTrack);
        }
    }
}

void
Composition::notifySegmentAdded(Segment *s) const
{
    // If there is an earlier repeating segment on the same track, we
    // need to notify the change of its repeat end time

    for (const_iterator i = begin(); i != end(); ++i) {

        if (((*i)->getTrack() == s->getTrack())
            && ((*i)->isRepeating())
            && ((*i)->getStartTime() < s->getStartTime())) {

            notifySegmentRepeatEndChanged(*i, (*i)->getRepeatEndTime());
        }
    }

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->segmentAdded(this, s);
    }
}

 
void
Composition::notifySegmentRemoved(Segment *s) const
{
    // If there is an earlier repeating segment on the same track, we
    // need to notify the change of its repeat end time

    for (const_iterator i = begin(); i != end(); ++i) {

        if (((*i)->getTrack() == s->getTrack())
            && ((*i)->isRepeating())
            && ((*i)->getStartTime() < s->getStartTime())) {

            notifySegmentRepeatEndChanged(*i, (*i)->getRepeatEndTime());
        }
    }

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
Composition::notifySegmentRepeatEndChanged(Segment *s, timeT t) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->segmentRepeatEndChanged(this, s, t);
    }
}

void
Composition::notifySegmentEventsTimingChanged(Segment *s, timeT delay, RealTime rtDelay) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->segmentEventsTimingChanged(this, s, delay, rtDelay);
    }
}

void
Composition::notifySegmentTransposeChanged(Segment *s, int transpose) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->segmentTransposeChanged(this, s, transpose);
    }
}

void
Composition::notifySegmentTrackChanged(Segment *s, TrackId oldId, TrackId newId) const
{
    // If there is an earlier repeating segment on either the
    // origin or destination track, we need to notify the change
    // of its repeat end time

    for (const_iterator i = begin(); i != end(); ++i) {

        if (((*i)->getTrack() == oldId || (*i)->getTrack() == newId)
            && ((*i)->isRepeating())
            && ((*i)->getStartTime() < s->getStartTime())) {

            notifySegmentRepeatEndChanged(*i, (*i)->getRepeatEndTime());
        }
    }

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->segmentTrackChanged(this, s, newId);
    }
}

void
Composition::notifySegmentStartChanged(Segment *s, timeT t)
{
    // not ideal, but best way to ensure track heights are recomputed:
    clearVoiceCaches();
    updateRefreshStatuses();

    // If there is an earlier repeating segment on the same track, we
    // need to notify the change of its repeat end time
    // (Copied from notifySegmentTrackChanged())
    for (const_iterator i = begin(); i != end(); ++i) {
        if (((*i)->getTrack() == s->getTrack())
            && ((*i)->isRepeating())
            && ((*i)->getStartTime() < s->getStartTime())) {

            notifySegmentRepeatEndChanged(*i, (*i)->getRepeatEndTime());
        }
    }

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->segmentStartChanged(this, s, t);
    }
}    

void
Composition::notifySegmentEndMarkerChange(Segment *s, bool shorten)
{
    // not ideal, but best way to ensure track heights are recomputed:
    clearVoiceCaches();
    updateRefreshStatuses();
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->segmentEndMarkerChanged(this, s, shorten);
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
Composition::notifyTrackChanged(Track *t)
{
    enforceArmRule(t);

    for (ObserverSet::const_iterator i = m_observers.begin();
            i != m_observers.end(); ++i) {
        (*i)->trackChanged(this, t);
    }
}

void
Composition::notifyTracksDeleted(std::vector<TrackId> trackIds) const
{
    //RG_DEBUG << "Composition::notifyTracksDeleted() notifying" << m_observers.size() << "observers";

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->tracksDeleted(this, trackIds);
    }
}

void
Composition::notifyTracksAdded(std::vector<TrackId> trackIds) const
{
    //RG_DEBUG << "Composition::notifyTracksAdded() notifying" << m_observers.size() << "observers";

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->tracksAdded(this, trackIds);
    }
}

void
Composition::notifyTrackSelectionChanged(TrackId t) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->trackSelectionChanged(this, t);
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
Composition::notifyTempoChanged() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->tempoChanged(this);
    }
}

void
Composition::notifySelectedTrackChanged() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->selectedTrackChanged(this);
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

#if 0
// unused
bool
Composition::isMarkerAtPosition(Rosegarden::timeT time) const
{
    markerconstiterator it = m_markers.begin();

    for (; it != m_markers.end(); ++it)
        if ((*it)->getTime() == time) return true;

    return false;
}
#endif

void
Composition::setSegmentColourMap(Rosegarden::ColourMap &newmap)
{
    m_segmentColourMap = newmap;

    updateRefreshStatuses();
}

void
Composition::setGeneralColourMap(Rosegarden::ColourMap &newmap)
{
    m_generalColourMap = newmap;

    updateRefreshStatuses();
}

void
Composition::distributeVerses()
{
    typedef std::map<int, SegmentMultiSet> SegmentMap;
    SegmentMap tracks;
    SegmentMap repeats;

    // Sort segments by track ID
    for (iterator i = begin(); i != end(); ++i) {
        Segment* s = *i;
        tracks[s->getTrack()].insert(s);
    }

    // Work track after track
    for (SegmentMap::iterator i = tracks.begin(); i != tracks.end(); ++i) {

        // Reset all verse indexes and look for repeated segments
        repeats.clear();
        for (SegmentMultiSet::iterator j = i->second.begin(); j != i->second.end(); ++j) {
             Segment* s = *j;
             s->setVerse(0);
             if (s->isPlainlyLinked()) {
                 repeats[s->getLinker()->getSegmentLinkerId()].insert(s);
            }
        }

        // Set verse indexes where needed
        for (SegmentMap::iterator j = repeats.begin(); j != repeats.end(); ++j) {
            int verse = 0;
            for (SegmentMultiSet::iterator k = j->second.begin(); k != j->second.end(); ++k) {
                Segment* s = *k;
                s->setVerse(verse++);
            }
        }
    }
}

void
Composition::dump(std::ostream& out, bool) const
{
    out << "Composition segments : " << endl;

    for (const_iterator i = begin(); i != end(); ++i) {

        Segment* s = *i;

        out << "Segment start : " << s->getStartTime() << " - end : " << s->getEndMarkerTime()
            << " - repeating : " << s->isRepeating()
            << " - track id : " << s->getTrack()
            << " - label : " << s->getLabel()
//            << " - verse : " << s->getVerse()
            << endl;
        
    }
    
}



}


