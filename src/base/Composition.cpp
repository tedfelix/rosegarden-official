/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Composition]"
#define RG_NO_DEBUG_PRINT

#include "Composition.h"

#include "misc/Debug.h"
#include "base/Segment.h"
#include "base/SegmentLinker.h"
#include "base/BaseProperties.h"
#include "base/Profiler.h"
#include "BasicQuantizer.h"
#include "NotationQuantizer.h"
#include "base/AudioLevel.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>  // for std::distance
#include <map>
#include <set>
#include <sstream>

//#include <iostream>
//#include <typeinfo>
//#include <limits.h>

//#define DEBUG_BAR_STUFF 1
//#define DEBUG_TEMPO_STUFF 1


namespace Rosegarden
{


const PropertyName Composition::NoAbsoluteTimeProperty("NoAbsoluteTime");
const PropertyName Composition::BarNumberProperty("BarNumber");

const std::string Composition::TempoEventType("tempo");
const PropertyName Composition::TempoProperty("Tempo");
const PropertyName Composition::TargetTempoProperty("TargetTempo");
const PropertyName Composition::TempoTimestampProperty("TimestampSec");


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

Composition::ReferenceSegment::ReferenceSegment(const std::string& eventType) :
    m_eventType(eventType)
{
    // nothing
}

Composition::ReferenceSegment::~ReferenceSegment()
{
    clear();
}

Composition::ReferenceSegment::iterator Composition::ReferenceSegment::begin()
{
    return m_events.begin();
}

Composition::ReferenceSegment::const_iterator Composition::ReferenceSegment::begin() const
{
    return m_events.begin();
}

Composition::ReferenceSegment::iterator Composition::ReferenceSegment::end()
{
    return m_events.end();
}

Composition::ReferenceSegment::const_iterator Composition::ReferenceSegment::end() const
{
    return m_events.end();
}

Composition::ReferenceSegment::size_type Composition::ReferenceSegment::size() const
{
    return m_events.size();
}

bool Composition::ReferenceSegment::empty() const
{
    return m_events.empty();
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::erase(Composition::ReferenceSegment::iterator position)
{
    delete *position;
    return m_events.erase(position);
}

void Composition::ReferenceSegment::clear()
{
    for (iterator it = begin(); it != end(); ++it) delete (*it);
    m_events.clear();
}

Event* Composition::ReferenceSegment::operator[] (size_type n)
{
    return m_events[n];
}

const Event* Composition::ReferenceSegment::operator[] (size_type n) const
{
    return m_events[n];
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
    // Return the Event at or after e's time.
    // Note that lower_bound() does a binary search.
    return std::lower_bound(begin(), end(), e, ReferenceSegmentEventCmp());
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
        return m_events.insert(i, e);
    }
}

void
Composition::ReferenceSegment::eraseEvent(Event *e)
{
    iterator i = find(e);
    if (i != end()) erase(i);
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::findAtOrBefore(timeT t)
{
    if (m_events.empty())
        return end();

    // Find the Event at or after t.
    Event event("dummy", t, 0, MIN_SUBORDERING);
    // Use std::lower_bound() which does a binary search.
    // These tempo and time signature Segments tend to be really
    // small, so a binary search probably isn't much faster than linear.
    iterator i = std::lower_bound(
            begin(), end(), &event, ReferenceSegmentEventCmp());

    // Found an exact match, return it.
    if (i != end()  &&  (*i)->getAbsoluteTime() == t)
        return i;

    // If begin() is after, indicate no Event prior.
    if (i == begin())
        return end();

    // i is after, so return the previous which is before t.
    return i - 1;
}

Composition::ReferenceSegment::iterator
Composition::ReferenceSegment::findAtOrBefore(RealTime t)
{
    if (m_events.empty())
        return end();

    // Find the Event at or after t.
    Event tempEvent("dummy", 0, 0, MIN_SUBORDERING);
    tempEvent.set<Bool>(NoAbsoluteTimeProperty, true);
    setTempoTimestamp(&tempEvent, t);
    // Use std::lower_bound() which does a binary search.
    // These tempo and time signature Segments tend to be really
    // small, so a binary search probably isn't much faster than linear.
    iterator i = std::lower_bound(
            begin(), end(), &tempEvent, ReferenceSegmentEventCmp());

    // Found an exact match, return it.
    if (i != end()  &&  getTempoTimestamp(*i) == t)
        return i;

    // If begin() is after, indicate no Event prior.
    if (i == begin())
        return end();

    // i is after, so return the previous which is before t.
    return i - 1;
}

namespace
{
    constexpr int defaultNumberOfBars = 100;
}

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
    m_endMarker(getBarRange(defaultNumberOfBars).first),
    m_autoExpand(false),
    m_playMetronome(false),
    m_recordMetronome(true),
    m_nextTriggerSegmentId(0),
    m_editorFollowPlayback(true),
    m_mainFollowPlayback(true)
{
    // nothing else
}

Composition::~Composition()
{
    if (!m_observers.empty()) {
        RG_WARNING << "dtor: WARNING:" << m_observers.size() << "observers still extant:";
        for (const CompositionObserver *observer : m_observers) {
            RG_WARNING << "  " << static_cast<const void *>(observer) <<
                ":" << typeid(*observer).name();
        }
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
Composition::deleteSegment(Composition::iterator segmentIter)
{
    if (segmentIter == end())
        return;

    clearVoiceCaches();

    Segment *segment = (*segmentIter);

    segment->setComposition(nullptr);

    m_segments.erase(segmentIter);

    distributeVerses();

    notifySegmentRemoved(segment);

    // ??? If this delete occurs during playback, we may get a crash later in
    //     MappedBufMetaIterator::fetchEvents().
    delete segment;

    updateRefreshStatuses();
}

bool
Composition::deleteSegment(const Segment *segment)
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
Composition::addAllSegments(SegmentVector segments)
{
    for (SegmentVector::iterator i = segments.begin();
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
Composition::detachAllSegments(SegmentVector segments)
{
    for (SegmentVector::iterator i = segments.begin();
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
    m_trackVoiceCountCache.clear();
    m_segmentVoiceIndexCache.clear();
}

void
Composition::rebuildVoiceCaches() const
{
    Profiler profiler("Composition::rebuildVoiceCaches");

    // slow

    m_trackVoiceCountCache.clear();
    m_segmentVoiceIndexCache.clear();

    for (TrackMap::const_iterator tci = m_tracks.begin();
         tci != m_tracks.end(); ++tci) {

        TrackId tid = tci->first;

        std::multimap<timeT, Segment *> ends;

        for (const_iterator i = begin(); i != end(); ++i) {
            if ((*i)->getTrack() != tid) continue;
            timeT t0 = (*i)->getStartTime();
            timeT t1 = (*i)->getRepeatEndTime();
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

/* unused
void
Composition::deleteTriggerSegment(TriggerSegmentId id)
{
    TriggerSegmentRec dummyRec(id, nullptr);
    TriggerSegmentSet::iterator i = m_triggerSegments.find(&dummyRec);
    if (i == m_triggerSegments.end()) return;
    (*i)->getSegment()->setComposition(nullptr);
    delete (*i)->getSegment();
    delete *i;
    m_triggerSegments.erase(i);
}
*/

void
Composition::detachTriggerSegment(TriggerSegmentId id)
{
    TriggerSegmentRec triggerSegmentFind(id, nullptr);
    TriggerSegmentSet::const_iterator triggerSegmentIter =
            m_triggerSegments.find(&triggerSegmentFind);
    // Not found?  Bail.
    if (triggerSegmentIter == m_triggerSegments.end())
        return;

    TriggerSegmentRec * const triggerSegment = *triggerSegmentIter;
    Segment * const segment = triggerSegment->getSegment();

    // We must call this before erase() to make sure it can tell
    // we are deleting a trigger Segment.  If this order becomes
    // problematic, we might need to pass a triggeredSegment flag.
    notifySegmentRemoved(segment);

    segment->setComposition(nullptr);

    m_triggerSegments.erase(triggerSegmentIter);
    delete triggerSegment;
}

void
Composition::clearTriggerSegments()
{
    for (TriggerSegmentSet::iterator i = m_triggerSegments.begin();
         i != m_triggerSegments.end(); ++i) {
        delete (*i)->getSegment();
        delete *i;
    }
    m_triggerSegments.clear();
}

int
Composition::getTriggerSegmentId(const Segment *s) const
{
    for (TriggerSegmentSet::iterator i = m_triggerSegments.begin();
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
    TriggerSegmentSet::iterator i = m_triggerSegments.find(&dummyRec);
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

/* unused
TriggerSegmentId
Composition::getNextTriggerSegmentId() const
{
    return m_nextTriggerSegmentId;
}
*/

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
Composition::getDuration(bool withRepeats) const
{
    //RG_DEBUG << "getDuration()";

    // Check the cache.  This is an expensive operation.
    if (withRepeats) {
        if (!m_durationWithRepeatsDirty)
            return m_durationWithRepeats;
    } else {
        if (!m_durationWithoutRepeatsDirty)
            return m_durationWithoutRepeats;
    }

    //RG_DEBUG << "  cache miss";

    timeT maxDuration = 0;

    for (const Segment *segment : m_segments) {

        timeT segmentEnd = 0;

        if (withRepeats)
            segmentEnd = segment->getRepeatEndTime();
        else
            segmentEnd = segment->getEndTime();

        if (segmentEnd > maxDuration)
            maxDuration = segmentEnd;

    }

    // Update the cache.
    if (withRepeats) {
        m_durationWithRepeats = maxDuration;
        m_durationWithRepeatsDirty = false;
    } else {
        m_durationWithoutRepeats = maxDuration;
        m_durationWithoutRepeatsDirty = false;
    }

    return maxDuration;
}

void
Composition::invalidateDurationCache() const
{
    m_durationWithRepeatsDirty = true;
    m_durationWithoutRepeatsDirty = true;
}

void
Composition::setStartMarker(const timeT &sM)
{
    m_startMarker = sM;
    updateRefreshStatuses();
}

void
Composition::setEndMarker(const timeT &endMarker)
{
    const bool shorten = (endMarker < m_endMarker);

    m_endMarker = endMarker;

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
    m_loopMode = LoopOff;
    m_loopStart = 0;
    m_loopEnd = 0;
    m_position = 0;
    m_startMarker = 0;
    m_endMarker = getBarRange(defaultNumberOfBars).first;
    m_selectedTrackId = 0;
    updateRefreshStatuses();
}

void
Composition::calculateBarPositions() const
{
    if (!m_barPositionsNeedCalculating) return;

#ifdef DEBUG_BAR_STUFF
    RG_DEBUG << "calculateBarPositions()";
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
        RG_DEBUG << "calculateBarPositions(): start marker = " << getStartMarker() << ", so initial bar number = " << lastBarNo;
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
        RG_DEBUG << "calculateBarPositions(): bar " << n << " at " << myTime;
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
    RG_DEBUG << "getNbBars(): returning " << bars;
#endif
    return bars;
}

int
Composition::getBarNumber(timeT t) const
{
    calculateBarPositions();
    ReferenceSegment::iterator i = m_timeSigSegment.findAtOrBefore(t);
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
    RG_DEBUG << "getBarNumber(" << t << "): returning " << n;
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
    RG_DEBUG << "getBarRange(): [1] bar " << n << ": (" << start << " -> " << finish << ")";
#endif

    } else {

        timeT barDuration = TimeSignature(**i).getBarDuration();
        start = (*i)->getAbsoluteTime() +
            (n - (*i)->get<Int>(BarNumberProperty)) * barDuration;
        finish = start + barDuration;

#ifdef DEBUG_BAR_STUFF
    RG_DEBUG << "getBarRange(): [2] bar " << n << ": (" << start << " -> " << finish << ")";
#endif
    }

    // partial bar
    if (j != m_timeSigSegment.end() && finish > (*j)->getAbsoluteTime()) {
        finish = (*j)->getAbsoluteTime();
#ifdef DEBUG_BAR_STUFF
    RG_DEBUG << "getBarRange(): [3] bar " << n << ": (" << start << " -> " << finish << ")";
#endif
    }

    return std::pair<timeT, timeT>(start, finish);
}

int
Composition::addTimeSignature(timeT t, const TimeSignature& timeSig)
{
#ifdef DEBUG_BAR_STUFF
    RG_DEBUG << "addTimeSignature(" << t << ", " << timeSig.getNumerator() << "/" << timeSig.getDenominator() << ")";
#endif

    ReferenceSegment::iterator i =
        m_timeSigSegment.insertEvent(timeSig.getAsEvent(t));
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
    ReferenceSegment::iterator i = m_timeSigSegment.findAtOrBefore(t);

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
    m_timeSigSegment.eraseEvent(m_timeSigSegment[n]);
    m_barPositionsNeedCalculating = true;
    updateRefreshStatuses();
    notifyTimeSignatureChanged();
}


tempoT
Composition::getTempoAtTime(timeT t) const
{
    ReferenceSegment::iterator i = m_tempoSegment.findAtOrBefore(t);

    // In negative time, if there's no tempo event actually defined
    // prior to the point of interest then we use the next one after
    // it, so long as it's no later than time zero.  This is the only
    // rational way to deal with count-in bars where the correct
    // tempo otherwise won't appear until we hit bar zero.  See also
    // getTimeSignatureAt

    if (i == m_tempoSegment.end()) {
        if (t < 0) {
#ifdef DEBUG_TEMPO_STUFF
            RG_DEBUG << "getTempoAtTime(): Negative time " << t << " for tempo, using 0";
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
            RG_DEBUG << "getTempoAtTime(): Calculated tempo " << result << " at " << t;
#endif

            return result;
        }
    }

#ifdef DEBUG_TEMPO_STUFF
    RG_DEBUG << "getTempoAtTime(): Found tempo " << tempo << " at " << t;
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
    RG_DEBUG << "addTempoAtTime(): Added tempo " << tempo << " at " << time;
#endif
    notifyTempoChanged();

    return std::distance(m_tempoSegment.begin(), i);
}

int
Composition::getTempoChangeCount() const
{
    return int(m_tempoSegment.size());
}

int
Composition::getTempoChangeNumberAt(timeT t) const
{
    ReferenceSegment::iterator i = m_tempoSegment.findAtOrBefore(t);
    if (i == m_tempoSegment.end()) return -1;
    else return std::distance(m_tempoSegment.begin(), i);
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

bool
Composition::compareSignaturesAndTempos(const Composition &other) const
{
    if (getTimeSignatureCount() != other.getTimeSignatureCount())
        return false;

    // For each time signature
    for (int i = 0; i < getTimeSignatureCount(); ++i) {
        std::pair<timeT, TimeSignature> t1 =
            getTimeSignatureChange(i);
        std::pair<timeT, TimeSignature> t2 =
            other.getTimeSignatureChange(i);
        // If they do not match
        if (t1.first != t2.first || t1.second != t2.second)
            return false;
    }

    if (getTempoChangeCount() != other.getTempoChangeCount())
        return false;

    // For each tempo change
    for (int i = 0; i < getTempoChangeCount(); ++i) {
        std::pair<timeT, tempoT> t1 = getTempoChange(i);
        std::pair<timeT, tempoT> t2 = other.getTempoChange(i);
        // If they do not match
        if (t1.first != t2.first || t1.second != t2.second)
            return false;
    }

    // They match perfectly.
    return true;
}

#ifndef BUG1627
// Original version
RealTime
Composition::getElapsedRealTime(timeT t) const
{
    calculateTempoTimestamps();

    ReferenceSegment::iterator i = m_tempoSegment.findAtOrBefore(t);
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
    RG_DEBUG << "getElapsedRealTime(): " << t << " -> " << elapsed << " (last tempo change at " << (*i)->getAbsoluteTime() << ")";
#endif

    return elapsed;
}

#else
// Version with proposed fix for bug #1627
RealTime
Composition::getElapsedRealTime(timeT t) const
{
    calculateTempoTimestamps();

    // In case we have an anacrusis, make sure we have the proper
    // start time which could be negative.
    const timeT start = getStartMarker();
    const RealTime realStart = time2RealTime(start, m_defaultTempo);

    // Elapsed time is dependent on tempo changes.  Find the previous one.
    ReferenceSegment::iterator tempoIter = m_tempoSegment.findAtOrBefore(t);
    // None found?  We should probably use the default tempo.
    if (tempoIter == m_tempoSegment.end()) {
        // Try the first, if any.
        // ??? If present, this will be after t.  So it is useless.
        // ??? Make this a new firstTempoIter for clarity.
        tempoIter = m_tempoSegment.begin();

        // If the tempo segment is empty OR the first tempo change is
        // after the composition start OR t is after the composition start...
        // ??? We can get rid of the parens by reordering this.  However,
        //     we should probably reorder this for speed.  The first check
        //     should be the one that is most frequently true.  And the
        //     iter check should obviously be before dereferencing.
        // ??? This will probably always be true since t will always
        //     be at or after start.  Then again, what if the composition
        //     start is changed to something large and positive, thus
        //     cutting off the tempo changes?  Does that delete them?
        // ??? Previously, this checked against 0.  So it only detected
        //     anacrusis (negative start time).  With this new version,
        //     will it be affected by composition start bar which can be
        //     something other than 1?  The original code translates to
        //     this:
        //     If the tempo segment is empty OR the first tempo change
        //     is after the anacrusis OR t is after the anacrusis.
        if (t >= start ||
            (tempoIter == m_tempoSegment.end() ||  // tempo segment empty?
                 (*tempoIter)->getAbsoluteTime() > start)) {  // tempo change is after composition start?
            // Perform a simple pulses to seconds conversion using the
            // default tempo.
            RealTime rt = time2RealTime(t, m_defaultTempo);
            rt = rt - realStart;
            RG_DEBUG << "getElapsedRealTime 1" << t << rt;
            return rt;
        }

        // ??? To get here, we would need:
        //
        //       - t prior to the first tempo change (easy to do)
        //       - t prior to the start of the Composition
        //       - the first tempo change prior to the start of Composition
        //
        //     Is this possible?  Can we have an event and a tempo change
        //     prior to the start of the Composition?  So, set up the
        //     first condition, then move the Composition start past that
        //     point.  I have a feeling that might purge the events and
        //     the tempo change.

        // ??? Previously to get here we would need:
        //
        //       - t < 0 (within an anacrusis or some other reason)
        //       - At least one tempo change at time 0 or earlier
        //         (e.g. within an anacrusis).  I don't believe we
        //         support tempo changes at negative time, so it would
        //         have to be at time zero.
        //       - t prior to that first tempo change.
        //       - So given a negative t and tempo change at zero, we
        //         have what we need to exercise this.
        //
        //     Then what happens below in this case?

        // ??? I'm wondering if the original intent of t >= 0 was, "is
        //     t sensible".  But then anacrusis was added and t >= 0
        //     inadvertently became, "is t not within the anacrusis".
    }

    RealTime elapsed;

    tempoT target = -1;
    timeT nextTempoTime = t;

    if (!getTempoTarget(tempoIter, target, nextTempoTime)) target = -1;

    if (target > 0) {
        elapsed = getTempoTimestamp(*tempoIter) +
            time2RealTime(t - (*tempoIter)->getAbsoluteTime(),
                          tempoT((*tempoIter)->get<Int>(TempoProperty)),
                          nextTempoTime - (*tempoIter)->getAbsoluteTime(),
                          target);
    } else {
        elapsed = getTempoTimestamp(*tempoIter) +
            time2RealTime(t - (*tempoIter)->getAbsoluteTime(),
                          tempoT((*tempoIter)->get<Int>(TempoProperty)));
    }

#ifdef DEBUG_TEMPO_STUFF
    RG_DEBUG << "getElapsedRealTime(): " << t << " -> " << elapsed << " (last tempo change at " << (*tempoIter)->getAbsoluteTime() << ")";
#endif

    elapsed = elapsed - realStart;
    RG_DEBUG << "getElapsedRealTime 2" << t << elapsed;

    return elapsed;
}
#endif

#ifndef BUG1627
// Original version.
timeT
Composition::getElapsedTimeForRealTime(RealTime t) const
{
    calculateTempoTimestamps();

    ReferenceSegment::iterator i = m_tempoSegment.findAtOrBefore(t);
    if (i == m_tempoSegment.end()) {
        i = m_tempoSegment.begin();
        if (t >= RealTime::zero()  ||
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
        RG_DEBUG << "getElapsedTimeForRealTime(): " << t << " -> "
             << elapsed << " (error " << (cfReal - t)
             << " or " << (cfTimeT - elapsed) << ", tempo "
             << (*i)->getAbsoluteTime() << ":"
             << (tempoT)((*i)->get<Int>(TempoProperty)) << ")";
    }
#endif
    return elapsed;
}

#else
// Proposed fix for bug #1627.
timeT
Composition::getElapsedTimeForRealTime(RealTime t) const
{
    calculateTempoTimestamps();

    // if the composition does not start at bar 1 we must add the
    // start time here
    timeT start = getStartMarker();
    RealTime realStart = time2RealTime(start, m_defaultTempo);
    t = t + realStart;

    ReferenceSegment::iterator i = m_tempoSegment.findAtOrBefore(t);
    if (i == m_tempoSegment.end()) {
        i = m_tempoSegment.begin();
        if (t >= realStart ||
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
        RG_DEBUG << "getElapsedTimeForRealTime(): " << t << " -> "
             << elapsed << " (error " << (cfReal - t)
             << " or " << (cfTimeT - elapsed) << ", tempo "
             << (*i)->getAbsoluteTime() << ":"
             << (tempoT)((*i)->get<Int>(TempoProperty)) << ")";
    }
#endif
    return elapsed;
}
#endif

void
Composition::calculateTempoTimestamps() const
{
    if (!m_tempoTimestampsNeedCalculating) return;

    timeT lastTimeT = 0;
    RealTime lastRealTime;

    tempoT tempo = m_defaultTempo;
    tempoT target = -1;

#ifdef DEBUG_TEMPO_STUFF
    RG_DEBUG << "calculateTempoTimestamps(): Tempo events are:";
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
        RG_DEBUG << (*i);
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
Composition::time2RealTime(timeT t, tempoT tempo)
{
    static timeT cdur = Note(Note::Crotchet).getDuration();

    double dt = (double(t) * 100000 * 60) / (double(tempo) * cdur);

    int sec = int(dt);
    int nsec = int((dt - sec) * 1000000000);

    RealTime rt(sec, nsec);

#ifdef DEBUG_TEMPO_STUFF
    if (!DEBUG_silence_recursive_tempo_printout) {
        RG_DEBUG << "time2RealTime(): t " << t << ", sec " << sec << ", nsec "
             << nsec << ", tempo " << tempo
             << ", cdur " << cdur << ", dt " << dt << ", rt " << rt;
        DEBUG_silence_recursive_tempo_printout = 1;
        timeT ct = realTime2Time(rt, tempo);
        timeT et = t - ct;
        RealTime ert = time2RealTime(et, tempo);
        RG_DEBUG << "cf. realTime2Time(" << rt << ") -> " << ct << " [err " << et << " (" << ert << "?)]";
        DEBUG_silence_recursive_tempo_printout=0;
    }
#endif

    return rt;
}

RealTime
Composition::time2RealTime(timeT time, tempoT tempo,
                           timeT targetTime, tempoT targetTempo)
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
        RG_DEBUG << "time2RealTime(): [2] time " << time << ", tempo "
             << tempo << ", targetTime " << targetTime << ", targetTempo "
             << targetTempo << ": rt " << rt;
        DEBUG_silence_recursive_tempo_printout = 1;
//        RealTime nextRt = time2RealTime(targetTime, tempo, targetTime, targetTempo);
        timeT ct = realTime2Time(rt, tempo, targetTime, targetTempo);
        RG_DEBUG << "cf. realTime2Time: rt " << rt << " -> " << ct;
        DEBUG_silence_recursive_tempo_printout=0;
    }
#endif

    return rt;
}

timeT
Composition::realTime2Time(RealTime rt, tempoT tempo)
{
    static timeT cdur = Note(Note::Crotchet).getDuration();

    double tsec = (double(rt.sec) * cdur) * (tempo / (60.0 * 100000.0));
    double tnsec = (double(rt.nsec) * cdur) * (tempo / 100000.0);

    double dt = tsec + (tnsec / 60000000000.0);
    timeT t = (timeT)(dt + (dt < 0 ? -1e-6 : 1e-6));

#ifdef DEBUG_TEMPO_STUFF
    if (!DEBUG_silence_recursive_tempo_printout) {
        RG_DEBUG << "realTime2Time(): rt.sec " << rt.sec << ", rt.nsec "
             << rt.nsec << ", tempo " << tempo
             << ", cdur " << cdur << ", tsec " << tsec << ", tnsec " << tnsec << ", dt " << dt << ", t " << t;
        DEBUG_silence_recursive_tempo_printout = 1;
        RealTime crt = time2RealTime(t, tempo);
        RealTime ert = rt - crt;
        timeT et = realTime2Time(ert, tempo);
        RG_DEBUG << "cf. time2RealTime(" << t << ") -> " << crt << " [err " << ert << " (" << et << "?)]";
        DEBUG_silence_recursive_tempo_printout = 0;
    }
#endif

    return t;
}

timeT
Composition::realTime2Time(RealTime rt, tempoT tempo,
                           timeT targetTime, tempoT targetTempo)
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
        RG_WARNING << "realTime2Time(): ERROR: term2 < 0 (it's " << term2 << ")";
#ifdef DEBUG_TEMPO_STUFF
        RG_DEBUG << "rt = " << rt << ", tempo = " << tempo << ", targetTime = " << targetTime << ", targetTempo = " << targetTempo;
        RG_DEBUG << "n = " << n << ", b = " << b << ", a = " << a << ", t = " << t;
        RG_DEBUG << "that's sqrt( (" << ((2.0*n*a*2.0*n*a)) << ") + "
                  << (8*(b-a)*t*n) << " )";

        RG_DEBUG << "so our original expression was " << rt << " = "
                  << a << "t + (t^2 * (" << b << " - " << a << ")) / " << 2*n;
#endif

        return realTime2Time(rt, tempo);
    }

    double term3 = std::sqrt(term2);

    // We only want the positive root
    if (term3 > 0) term3 = -term3;

    double result = - (term1 + term3) / (2 * (b - a));

#ifdef DEBUG_TEMPO_STUFF
    RG_DEBUG << "realTime2Time():";
    RG_DEBUG << "n = " << n << ", b = " << b << ", a = " << a << ", t = " << t;
    RG_DEBUG << "+/-sqrt(term2) = " << term3;
    RG_DEBUG << "result = " << result;
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
Composition::timeRatioToTempo(const RealTime &realTime,
                                  timeT beatTime, tempoT)
{
    static const timeT cdur = Note(Note::Crotchet).getDuration();
    const double beatsPerMinute = 60 / realTime.toSeconds();
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
                                           int &fraction, int &remainder) const
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
                                       int &fractions, int &remainder) const
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
                                           int fraction, int remainder) const
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
                                       int fractions, int remainder) const
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
// cppcheck-suppress unusedFunction
static void dumpTracks(Composition::TrackMap& tracks)
{
    Composition::TrackMap::iterator it = tracks.begin();
    for (; it != tracks.end(); ++it) {
        RG_DEBUG << "tracks[" << (*it).first << "] = " << (*it).second;
    }
}
#endif

Track* Composition::getTrackById(TrackId track) const
{
    TrackMap::const_iterator i = m_tracks.find(track);

    if (i != m_tracks.end())
        return (*i).second;

    RG_WARNING << "getTrackById(" << track << "): WARNING: Track ID not found.";
    RG_WARNING << "  Available track ids are:";
    for (TrackMap::const_iterator i2 = m_tracks.begin(); i2 != m_tracks.end(); ++i2) {
        RG_WARNING << "    " << (int)(*i2).second->getId();
    }

    return nullptr;
}

bool
Composition::haveTrack(TrackId track) const
{
    TrackMap::const_iterator i = m_tracks.find(track);
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
    TrackMap::iterator titerator = m_tracks.find(oldId);

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
        RG_DEBUG << "resetTrackIdAndPosition() - "
                  << "can't move track " << oldId << " to " << newId;
}
#endif

InstrumentId Composition::getSelectedInstrumentId() const
{
    if (m_selectedTrackId == NoTrack)
        return NoInstrument;

    const Track *track = getTrackById(m_selectedTrackId);

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
        RG_DEBUG << "addTrack("
                  << track << "), id = " << track->getId()
                  << " - WARNING - track id already present "
                  << __FILE__ << ":" << __LINE__;
        // throw Exception("track id already present");
    }
}

#if 0
// unused
void Composition::deleteTrack(Rosegarden::TrackId track)
{
    TrackMap::iterator titerator = m_tracks.find(track);

    if (titerator == m_tracks.end()) {

        RG_DEBUG << "deleteTrack() : no track of id " << track;
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
    TrackMap::iterator it = m_tracks.begin();

    for (; it != m_tracks.end(); ++it)
    {
        if ((*it).second == track)
            break;
    }

    if (it == m_tracks.end()) {
        RG_DEBUG << "detachTrack() : no such track " << track;
        throw Exception("track id not found");
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
    for (TrackIdSet::iterator i = m_recordTracks.begin();
         i != m_recordTracks.end(); ) {
        // Increment before use.  This way deleting the element does not
        // invalidate the iterator.
        TrackIdSet::iterator j = i++;

        // If the track is no longer in the composition
        if (m_tracks.find(*j) == m_tracks.end()) {
            // Remove it from the list of record tracks.
            m_recordTracks.erase(j);
        }
    }
}

void Composition::refreshRecordTracks()
{
    m_recordTracks.clear();

    // For each Track
    for (const TrackMap::value_type &trackPair : m_tracks) {
        // Armed?  Add to m_recordTracks.
        if (trackPair.second->isArmed())
            m_recordTracks.insert(trackPair.first);
    }
}

TrackId
Composition::getClosestValidTrackId(TrackId id) const
{
    long diff = LONG_MAX;
    TrackId closestValidTrackId = 0;

    for (TrackMap::const_iterator i = getTracks().begin();
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

    TrackMap::const_iterator i = getTracks().begin();
    return i->first;
}

TrackId
Composition::getMaxTrackId() const
{
    if (getTracks().size() == 0) return 0;

    TrackMap::const_iterator i = getTracks().end();
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
    for (Composition::TrackMap::const_iterator ti =
             m_tracks.begin();
         ti != m_tracks.end();
         ++ti) {
        const Track *track = ti->second;

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
    bool first = true;
    // For each Track...
    for (const TrackMap::value_type &trackPair : m_tracks) {
        // If the Track isn't really armed, try the next.
        if (!trackPair.second->isReallyArmed())
            continue;

        // After the first, we need a comma.
        if (!first)
            composition << ",";
        else  // No longer the first
            first = false;

        composition << trackPair.first;
    }

    composition << "\" pointer=\"" << m_position;
    composition << "\" defaultTempo=\"";
    composition << std::setiosflags(std::ios::fixed)
                << std::setprecision(4)
                << getTempoQpm(m_defaultTempo);
    composition << "\" compositionDefaultTempo=\"";
    composition << m_defaultTempo;

    // Legacy looping
    if (m_loopStart != m_loopEnd)
    {
        composition << "\" loopstart=\"" << m_loopStart;
        composition << "\" loopend=\"" << m_loopEnd;
    }
    const bool isLooping = (m_loopMode == LoopOn);
    composition << "\" islooping=\"" << isLooping;

    // New looping
    composition << "\" loopmode=\"" << m_loopMode;
    composition << "\" loopstart2=\"" << m_loopStart;
    composition << "\" loopend2=\"" << m_loopEnd;

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

    composition << "\" editorfollowplayback=\"" << m_editorFollowPlayback;
    composition << "\" mainfollowplayback=\"" << m_mainFollowPlayback;

    composition << "\">" << std::endl << std::endl;

    composition << std::endl;

    for (TrackMap::const_iterator tit = getTracks().begin();
         tit != getTracks().end();
         ++tit)
        {
            if ((*tit).second)
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

        composition << "/>" << std::endl;
    }

    composition << std::endl;

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
        composition << "\"/>" << std::endl;
    }

    composition << std::endl;

    composition << "<metadata>" << std::endl
                << m_metadata.toXmlString() << std::endl
                << "</metadata>" << std::endl << std::endl;

    composition << "<markers>" << std::endl;
    for (MarkerVector::const_iterator mIt = m_markers.begin();
         mIt != m_markers.end(); ++mIt)
    {
        composition << (*mIt)->toXmlString();
    }
    composition << "</markers>" << std::endl;
    composition << "</composition>";

    return composition.str();
}

void
Composition::clearTracks()
{
    TrackMap::iterator it = m_tracks.begin();

    for (; it != m_tracks.end(); ++it)
        delete ((*it).second);

    m_tracks.erase(m_tracks.begin(), m_tracks.end());
}

Track*
Composition::getTrackByPosition(int position) const
{
    TrackMap::const_iterator it = m_tracks.begin();

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
    const Track *track = getTrackById(id);
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

    TrackMap::const_iterator it = m_tracks.begin();

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


    // For each track...
    for (const Composition::TrackMap::value_type &pair : m_tracks) {
        const Track *track = pair.second;
        // if this track is using the instrumentId, return true
        if (track->getInstrument() == instrumentId)
            return true;
    }

    return false;
}

// Get all the segments that the same instrument plays that plays
// segment s.
// @return a SegmentMultiSet that includes s itself.
/* unused
SegmentMultiSet
Composition::getInstrumentSegments(Segment *s, timeT t) const
{
    SegmentMultiSet segments;
    InstrumentId instrumentId = getInstrumentId(s);

    // For each Segment in the Composition...
    const SegmentMultiSet& allSegments = getSegments();
    for (Composition::iterator i = allSegments.begin();
         i != allSegments.end();
         ++i)
    {
        if (((*i)->getStartTime() < t)  &&
            (getInstrumentId(*i) == instrumentId)) {
            segments.insert(*i);
        }
    }

    return segments;
}
*/

void
Composition::enforceArmRule(const Track *track)
{
    // No more than one armed track per instrument.

    if (!track->isArmed())
        return;

    // For each track...
    for (const TrackMap::value_type &trackPair: m_tracks) {
        Track *otherTrack = trackPair.second;
        // Not armed?  Skip.
        // Use "isReallyArmed()" to make sure we check archived tracks as well.
        if (!otherTrack->isReallyArmed())
            continue;
        // Same Track?  Skip.
        if (otherTrack == track)
            continue;
        // Not using the same Instrument?  Skip.
        if (otherTrack->getInstrument() != track->getInstrument())
            continue;

        // We have found an armed Track using the same Instrument.
        // Unarm it.

        setTrackRecording(trackPair.first, false);
        notifyTrackChanged(otherTrack);
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
Composition::notifySegmentRemoved(Segment *segment) const
{
    // If not a trigger segment...
    if (getTriggerSegmentId(segment) == -1) {

        // If there is an earlier repeating segment on the same track, we
        // need to let observers know that the repeat end time has changed.

        // For each Segment...
        for (const_iterator segmentIter = begin();
             segmentIter != end();
             ++segmentIter) {
            Segment *currentSegment = *segmentIter;
            // If this Segment is on the same track and it repeats and
            // it is before the Segment being removed...
            if (currentSegment->getTrack() == segment->getTrack()  &&
                currentSegment->isRepeating()  &&
                currentSegment->getStartTime() < segment->getStartTime()) {
                notifySegmentRepeatEndChanged(
                        currentSegment,
                        currentSegment->getRepeatEndTime());
            }
        }

    }

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end();
         ++i) {
        (*i)->segmentRemoved(this, segment);
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
Composition::notifySegmentRepeatEndChanged
(Segment *segment, timeT repeatEndTime) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->segmentRepeatEndChanged(this, segment, repeatEndTime);
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

// cppcheck-suppress unusedFunction
void breakpoint()
{
    //RG_DEBUG << "breakpoint()";
}

// Just empty out the markers
void
Composition::clearMarkers()
{
    MarkerVector::const_iterator it = m_markers.begin();

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

    // Sort the markers.
    // ??? A std::set should be a little more efficient.
    std::sort(m_markers.begin(), m_markers.end(),
            [](const Marker *lhs, const Marker *rhs){ return lhs->getTime() < rhs->getTime(); });

    updateRefreshStatuses();
}

bool
Composition::detachMarker(const Rosegarden::Marker *marker)
{
    MarkerVector::iterator it = m_markers.begin();

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

Segment*
Composition::getSegmentByMarking(const QString& marking) const
{
    for (SegmentMultiSet::const_iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

        Segment* s = *i;
        if (s->getMarking() == marking) {
            return s;
        }
    }
    return nullptr;
}

#if 0
// unused
bool
Composition::isMarkerAtPosition(Rosegarden::timeT time) const
{
    MarkerVector::const_iterator it = m_markers.begin();

    for (; it != m_markers.end(); ++it)
        if ((*it)->getTime() == time) return true;

    return false;
}
#endif

void
Composition::setSegmentColourMap(const Rosegarden::ColourMap &newmap)
{
    m_segmentColourMap = newmap;

    updateRefreshStatuses();
}

void
Composition::setGeneralColourMap(const Rosegarden::ColourMap &newmap)
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
Composition::dump() const
{
    RG_DEBUG << "dump(): Composition segments";

    for (const_iterator i = begin(); i != end(); ++i) {
        const Segment *s = *i;

        RG_DEBUG << "Segment start : " << s->getStartTime()
                << " - end : " << s->getEndMarkerTime()
                << " - repeating : " << s->isRepeating()
                << " - track id : " << s->getTrack()
                << " - label : " << s->getLabel();
                //<< " - verse : " << s->getVerse()
        RG_DEBUG << *s;
    }
}

QString
Composition::makeTimeString(timeT midiTicks, TimeMode timeMode) const
{
    switch (timeMode) {

    case TimeMode::MusicalTime:
        {
            int bar;
            int beat;
            int fraction;
            int remainder;
            getMusicalTimeForAbsoluteTime(
                            midiTicks, bar, beat, fraction, remainder);
            // Humans prefer a 1-based bar number.
            ++bar;
            return QString("%1%2%3-%4%5-%6%7-%8%9")
                   .arg(bar / 100)
                   .arg((bar % 100) / 10)
                   .arg(bar % 10)
                   .arg(beat / 10)
                   .arg(beat % 10)
                   .arg(fraction / 10)
                   .arg(fraction % 10)
                   .arg(remainder / 10)
                   .arg(remainder % 10);
        }

    case TimeMode::RealTime:
        {
            const RealTime rt = getElapsedRealTime(midiTicks);
            return QString("%1").arg(rt.toText().c_str());
        }

    case TimeMode::RawTime:
        return QString("%1").arg(midiTicks);

    default:
        return "---";
    }
}

QString
Composition::makeDurationString(
        timeT time, timeT duration, TimeMode timeMode) const
{
    switch (Composition::TimeMode(timeMode)) {

    case Composition::TimeMode::MusicalTime:
        {
            int bar;
            int beat;
            int fraction;
            int remainder;

            getMusicalTimeForDuration(time, duration, bar, beat, fraction, remainder);

            return QString("%1%2%3-%4%5-%6%7-%8%9   ")
                   .arg(bar / 100)
                   .arg((bar % 100) / 10)
                   .arg(bar % 10)
                   .arg(beat / 10)
                   .arg(beat % 10)
                   .arg(fraction / 10)
                   .arg(fraction % 10)
                   .arg(remainder / 10)
                   .arg(remainder % 10);
        }

    case Composition::TimeMode::RealTime:
        {
            RealTime rt = getRealTimeDifference(time, time + duration);
            return QString("%1  ").arg(rt.toText().c_str());
        }

    case Composition::TimeMode::RawTime:
    default:
        return QString("%1  ").arg(duration);
    }
}

QVariant
Composition::makeTimeVariant(timeT midiTicks, TimeMode timeMode) const
{
    switch (timeMode) {

    case TimeMode::MusicalTime:
        {
            int bar;
            int beat;
            int fraction;
            int remainder;
            getMusicalTimeForAbsoluteTime(
                            midiTicks, bar, beat, fraction, remainder);
            // Humans prefer a 1-based bar number.
            ++bar;
            return QString("%1%2%3-%4%5-%6%7-%8%9")
                   .arg(bar / 100)
                   .arg((bar % 100) / 10)
                   .arg(bar % 10)
                   .arg(beat / 10)
                   .arg(beat % 10)
                   .arg(fraction / 10)
                   .arg(fraction % 10)
                   .arg(remainder / 10)
                   .arg(remainder % 10);
        }

    case TimeMode::RealTime:
        return getElapsedRealTime(midiTicks).toSeconds();

    case TimeMode::RawTime:
        return qlonglong(midiTicks);

    default:
        return "---";
    }
}


}
