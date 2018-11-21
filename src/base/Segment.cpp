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

#define RG_MODULE_STRING "[Segment]"

#include "base/Segment.h"
#include "base/NotationTypes.h"
#include "base/BaseProperties.h"
#include "Composition.h"
#include "BasicQuantizer.h"
#include "base/Profiler.h"
#include "base/SegmentLinker.h"
#include "document/DocumentGet.h"
#include "gui/general/GUIPalette.h"
//#include "misc/Debug.h"

#include <QtGlobal>

#include <iostream>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <typeinfo>

// YG: Only for debug (dumpObservers)
#include "gui/editors/notation/StaffHeader.h"

namespace Rosegarden
{
using std::cerr;
using std::endl;
using std::string;

//#define DEBUG_NORMALIZE_RESTS 1

static int g_runtimeSegmentId = 0;

Segment::Segment(SegmentType segmentType, timeT startTime) :
    EventContainer(),
    m_composition(0),
    m_startTime(startTime),
    m_endMarkerTime(0),
    m_endTime(startTime),
    m_trackId(0),
    m_type(segmentType),
    m_colourIndex(0),
    m_id(0),
    m_audioFileId(0),
    m_unstretchedFileId(0),
    m_stretchRatio(1.0),
    m_audioStartTime(0, 0),
    m_audioEndTime(0, 0),
    m_repeating(false),
    m_quantizer(new BasicQuantizer()),
    m_quantize(false),
    m_transpose(0),
    m_delay(0),
    m_realTimeDelay(0, 0),
    m_highestPlayable(127),
    m_lowestPlayable(0),
    m_percussionPitch(-1),
    m_clefKeyList(0),
    m_notifyResizeLocked(false),
    m_memoStart(0),
    m_memoEndMarkerTime(0),
    m_runtimeSegmentId(g_runtimeSegmentId++),
    m_snapGridSize(-1),
    m_viewFeatures(0),
    m_autoFade(false),
    m_fadeInTime(Rosegarden::RealTime::zeroTime),
    m_fadeOutTime(Rosegarden::RealTime::zeroTime),
    m_segmentLinker(0),
    m_isTmp(0),
    m_participation(normal),
    m_verseCount(-1),   // -1 => computation needed
    m_verse(0)
{
}

Segment::Segment(const Segment &segment):
    QObject(),
    EventContainer(),
    m_composition(0), // Composition should decide what's in it and what's not
    m_startTime(segment.getStartTime()),
    m_endMarkerTime(segment.m_endMarkerTime ?
                    new timeT(*segment.m_endMarkerTime) : nullptr),
    m_endTime(segment.getEndTime()),
    m_trackId(segment.getTrack()),
    m_type(segment.getType()),
    m_label(segment.getLabel()),
    m_colourIndex(segment.getColourIndex()),
    m_id(0),
    m_audioFileId(segment.getAudioFileId()),
    m_unstretchedFileId(segment.getUnstretchedFileId()),
    m_stretchRatio(segment.getStretchRatio()),
    m_audioStartTime(segment.getAudioStartTime()),
    m_audioEndTime(segment.getAudioEndTime()),
    m_repeating(segment.isRepeating()),
    m_quantizer(new BasicQuantizer(segment.m_quantizer->getUnit(),
                                   segment.m_quantizer->getDoDurations())),
    m_quantize(segment.hasQuantization()),
    m_transpose(segment.getTranspose()),
    m_delay(segment.getDelay()),
    m_realTimeDelay(segment.getRealTimeDelay()),
    m_highestPlayable(127),
    m_lowestPlayable(0),
    m_percussionPitch(-1),
    m_clefKeyList(0),
    m_notifyResizeLocked(false),  // To copy a segment while notifications
    m_memoStart(0),               // are locked doesn't sound as a good
    m_memoEndMarkerTime(0),       // idea.
    m_runtimeSegmentId(g_runtimeSegmentId++),
    m_snapGridSize(-1),
    m_viewFeatures(0),
    m_autoFade(segment.isAutoFading()),
    m_fadeInTime(segment.getFadeInTime()),
    m_fadeOutTime(segment.getFadeOutTime()),
    m_segmentLinker(0), //yes, this is intentional. clone() handles this
    m_isTmp(segment.isTmp()),
    m_participation(segment.m_participation),
    m_verseCount(-1),   // -1 => computation needed
    m_verse(0)   // Needs a global recomputation on the whole composition
{
    for (const_iterator it = segment.begin();
         it != segment.end(); ++it) {
        insert(new Event(**it));
    }
}

Segment*
Segment::cloneImpl() const
{
    Segment *s = new Segment(*this);

    if (isLinked()) {
        //if the segment is linked already, link the clone to our SegmentLinker
       getLinker()->addLinkedSegment(s);
    }

    return s;
}

Segment::~Segment()
{
    if (!m_observers.empty()) {
        cerr << "Warning: Segment::~Segment() with " << m_observers.size()
             << " observers still extant" << endl;
        cerr << "Observers are:";
        for (ObserverSet::const_iterator i = m_observers.begin();
             i != m_observers.end(); ++i) {
            cerr << " " << (void *)(*i);
            cerr << " [" << typeid(**i).name() << "]";
        }
        cerr << endl;
    }

    //unlink it
    SegmentLinker::unlinkSegment(this);

    notifySourceDeletion();

    if (m_composition) m_composition->detachSegment(this);

    if (m_clefKeyList) {
        // don't delete contents of m_clefKeyList: the pointers
        // are just aliases for events in the main segment
        delete m_clefKeyList;
    }

    // Clear EventRulers
    //
    EventRulerListIterator it;
    for (it = m_eventRulerList.begin(); it != m_eventRulerList.end(); ++it) delete *it;
    m_eventRulerList.clear();

    // delete content
    for (iterator it = begin(); it != end(); ++it) delete (*it);

    delete m_endMarkerTime;
}

bool
Segment::setAsReference() {
    if (!isLinked()) return false;
    getLinker()->setReference(this);
    return true;
}

Segment *
Segment::getRealSegment() {
    if (isLinked()) return getLinker()->getReference();
    return this;
}

const Segment *
Segment::getRealSegment() const {
    if (isLinked()) return getLinker()->getReference();
    return this;
}

void
Segment::setTmp() {
    m_isTmp = true;
    setGreyOut();
}

void
Segment::setGreyOut(void) {
    for (iterator it = begin(); it != end(); ++it) {
        (*it)->set<Bool>(BaseProperties::TMP, true, false);
    }
}

bool
Segment::isTrulyLinked() const {
    // If there is no SegmentLinker the segment is not linked
    if (!m_segmentLinker) return false;
    
    // If segment is a temporary one or is out of composition return false
    // That's arbitrary, but this method is designed to be used from
    // segments which are inside the composition.
    if (isTmp()) return false;
    if (!getComposition()) return false;

    // If the SegmentLinker is referencing only one segment the
    // segment is only linked to itself and is not truly linked.
    int numOfSegments = m_segmentLinker->getNumberOfLinkedSegments();
    if (numOfSegments < 2) return false;

    // The segment is truly linked if at least another segment which
    // is inside the composition and which is not a temporary one is
    // referenced by its linker.
    int numOfTmpSegments = m_segmentLinker->getNumberOfTmpSegments();
    int numOfOutsideSegments = m_segmentLinker->getNumberOfOutOfCompSegments();
    if ((numOfSegments - numOfTmpSegments - numOfOutsideSegments) > 1) return true;
    else return false;
}

bool
Segment::isPlainlyLinked() const {
    // A segment which is not a "true link" can't be a "plain link"
    if (!isTrulyLinked()) return false;

    // TODO: the real work...
    // Currently the features which make the difference between a link and a
    // plain link are at the embryonic stage and are not implemented
    // through the GUI still.
    // That's why I leave this method unfinished.
    return true;
}

bool
Segment::isLinkedTo(Segment * seg) const {
    if (!m_segmentLinker) return false;

    SegmentLinker * otherSegmentLinker = seg->getLinker();
    if (!otherSegmentLinker) return false;

    return m_segmentLinker == otherSegmentLinker;
}

bool
Segment::isPlainlyLinkedTo(Segment * seg) const {
    if (!isPlainlyLinked()) return false;
    if (!seg->isPlainlyLinked()) return false;
    return isLinkedTo(seg);
}

void
Segment::setTrack(TrackId id)
{
    if (m_participation != normal) {
        m_trackId = id;
        return;
    }
    
    Composition *c = m_composition;
    if (c) c->weakDetachSegment(this); // sets m_composition to 0
    TrackId oldTrackId = m_trackId;
    m_trackId = id;
    if (c) {
        c->weakAddSegment(this);
        c->updateRefreshStatuses();
        c->distributeVerses();
        c->notifySegmentTrackChanged(this, oldTrackId, id);
    }
}

timeT
Segment::getStartTime() const
{
    return m_startTime;
}

timeT
Segment::getClippedStartTime() const
{
    // If there is a composition, and our start time is before the beginning of
    // the composition, return the composition start time.  Otherwise (all
    // other cases) return our start time.
    if (m_composition) {
        timeT compStart = m_composition->getStartMarker();
         if (m_startTime < compStart) return compStart;
    }
    return m_startTime;
}

timeT
Segment::getEndMarkerTime(bool comp) const
{
    timeT endTime;

    if (m_type == Audio && m_composition) {

        RealTime startRT = m_composition->getElapsedRealTime(m_startTime);
        RealTime endRT = startRT - m_audioStartTime + m_audioEndTime;
        endTime = m_composition->getElapsedTimeForRealTime(endRT);

    } else {

        if (m_endMarkerTime) {
            endTime = *m_endMarkerTime;
        } else {
            endTime = getEndTime();
        }

        if (m_composition && comp) {
            endTime = std::min(endTime, m_composition->getEndMarker());
        }
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
    Profiler profiler("Segment::setStartTime()");
    typedef EventContainer base;
    int dt = t - m_startTime;
    if (dt == 0) return;
    timeT previousEndTime = m_endTime;

    // reset the time of all events.  can't just setAbsoluteTime on these,
    // partly 'cos we're not allowed, partly 'cos it might screw up the
    // quantizer (which is why we're not allowed)

    // still, this is rather unsatisfactory

    FastVector<Event *> events;

    /** This is effectively calling Segment::erase on each event after
        copyMoving it.  Segment::erase did the following:
        
        1. base::erase
	2. Delete the event
	3. Kept m_startTime up to date
	4. Kept m_endTime up to date
	5. updateRefreshStatuses
        6. Thru notifyRemove, notify observers eventRemoved
        7. Thru notifyRemove, remove clefs and keys from
	   m_clefKeyList

      1 is done explicitly here.  3, 4, 5, and 7 are done en masse
      below.  6 is accomplished via allEventsChanged.  2 is no longer
      wanted since we need to insert the same event.
     **/

    // We do the removing in two phases: A lightweight removal that
    // leaves events still in the multiset, and then we clear the
    // whole set.
    for (iterator i = begin(); i != end(); ++i) {
        Event *e = *i;
        // allEventsChanged is allowed to assume the address points to
        // the Event it knew about, so we need the same object to be
        // removed and added, so we can't use copyMoving.
        e->unsafeChangeTime(dt);
        events.push_back(e);
    }
    base::clear();

    if (m_clefKeyList) { m_clefKeyList->clear(); }
    
    m_endTime = previousEndTime + dt;
    if (m_endMarkerTime) *m_endMarkerTime += dt;

    if (m_composition) m_composition->setSegmentStartTime(this, t);
    else m_startTime = t;

    /** This is effectively calling Segment::insert on each event.
        Segment::insert did the following:

	1. base::insert
	2. Kept m_startTime up to date
	3. Kept m_endTime up to date
	4. Set the TMP property if applicable
	5. updateRefreshStatuses
        6. Thru notifyAdd, notified observers eventAdded
        7. Thru notifyAdd, added clefs and keys to m_clefKeyList

        1 and 7 are done explicitly here.  2, 3 & 5 are done en masse.
        6 is accomplished via allEventsChanged.  4 works because we
        keep the same event object.
     **/
    for (int i = 0; i < int(events.size()); ++i) {
        Event *e = events[i];
        base::insert(e);
        checkInsertAsClefKey(e);
    }

    // Handle updates and notifications just once.
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->allEventsChanged(this);
    }
    notifyEndMarkerChange(dt < 0);
    notifyStartChanged(m_startTime);
    updateRefreshStatuses(m_startTime, m_endTime);
}

void
Segment::setEndMarkerTime(timeT t)
{
    if (t < m_startTime) t = m_startTime;

    if (m_type == Audio) {
        if (m_endMarkerTime) *m_endMarkerTime = t;
        else m_endMarkerTime = new timeT(t);
        RealTime oldAudioEndTime = m_audioEndTime;
        if (m_composition) {
            m_audioEndTime = m_audioStartTime +
                m_composition->getRealTimeDifference(m_startTime, t);
            if (oldAudioEndTime != m_audioEndTime) {
                notifyEndMarkerChange(m_audioEndTime < oldAudioEndTime);
            }
        }
    } else {

        timeT endTime = getEndTime();
        timeT oldEndMarker = getEndMarkerTime();
        bool shorten = (t < oldEndMarker);

        if (t > endTime) {
            fillWithRests(endTime, t);
            if (oldEndMarker < endTime) {
                updateRefreshStatuses(oldEndMarker, t);
            }
        } else {
            // only need to do this if we aren't inserting or
            // deleting any actual events
            if (oldEndMarker < t) {
                updateRefreshStatuses(oldEndMarker, t);
            }
            updateRefreshStatuses(t, endTime);
        }

        if (m_endMarkerTime) *m_endMarkerTime = t;
        else m_endMarkerTime = new timeT(t);
        notifyEndMarkerChange(shorten);
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
            normalizeRests(endTime, t);
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
Segment::isBeforeEndMarker(const_iterator i) const
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
    m_endMarkerTime = nullptr;
    notifyEndMarkerChange(false);
}

const timeT *
Segment::getRawEndMarkerTime() const
{
    return m_endMarkerTime;
}


void
Segment::updateRefreshStatuses(timeT startTime, timeT endTime)
{
    Profiler profiler("Segment::updateRefreshStatuses()");

    // For each observer, indicate that a refresh is needed for this time
    // span.
    for(size_t i = 0; i < m_refreshStatusArray.size(); ++i)
        m_refreshStatusArray.getRefreshStatus(i).push(startTime, endTime);
}


Segment::iterator
Segment::insert(Event *e)
{
    Q_CHECK_PTR(e);

    // Event Start Time
    timeT t0 = e->getAbsoluteTime();
    // Event End Time
    timeT t1 = t0 + e->getGreaterDuration();

    // If this event starts before the segment start time
    if (t0 < m_startTime ||
        (begin() == end() && t0 > m_startTime)) {

        if (m_composition) m_composition->setSegmentStartTime(this, t0);
        else m_startTime = t0;
        notifyStartChanged(m_startTime);
    }

    // If this event ends after the segment end time
    if (t1 > m_endTime ||
        begin() == end()) {
        timeT oldTime = m_endTime;
        m_endTime = t1;
        notifyEndMarkerChange(m_endTime < oldTime);
    }

    // A segment having the TMP property should be displayed with a gray colour.
    // To do so each event in such a segment needs the TMP property.
    if (isTmp()) e->set<Bool>(BaseProperties::TMP, true, false);

    iterator i = EventContainer::insert(e);
    notifyAdd(e);

    // Fix #1548: Last syllable of lyrics is not copied between two
    // linked segments.
    // Ensure event is inside the refresh status range
    if (t1 == t0) t1 += 1;

    updateRefreshStatuses(t0, t1);
    return i;
}


void
Segment::updateEndTime()
{
    m_endTime = m_startTime;
    for (iterator i = begin(); i != end(); ++i) {
        timeT t = (*i)->getAbsoluteTime() + (*i)->getGreaterDuration();
        if (t > m_endTime) m_endTime = t;
    }
}


void
Segment::erase(iterator pos)
{
    Event *e = *pos;

    Q_CHECK_PTR(e);
    
    timeT t0 = e->getAbsoluteTime();
    timeT t1 = t0 + e->getGreaterDuration();

    EventContainer::erase(pos);
    notifyRemove(e);
    delete e;
    updateRefreshStatuses(t0, t1);

    if (t0 == m_startTime && begin() != end()) {
        timeT startTime = (*begin())->getAbsoluteTime();
        
        // Don't send any notification if startTime doesn't change.
        if (startTime != m_startTime) {
            if (m_composition) m_composition->setSegmentStartTime(this, startTime);
            else m_startTime = startTime;
            notifyStartChanged(m_startTime);
        }
    }
    if (t1 == m_endTime) {
        updateEndTime();
    }
}


void
Segment::erase(iterator from, iterator to)
{
    timeT startTime = 0, endTime = m_endTime;
    if (from != end()) startTime = (*from)->getAbsoluteTime();
    if (to != end()) endTime = (*to)->getAbsoluteTime() + (*to)->getGreaterDuration();

    // Not very efficient, but without an observer event for
    // multiple erase we can't do any better.

    for (Segment::iterator i = from; i != to; ) {

        Segment::iterator j(i);
        ++j;

        Event *e = *i;
        Q_CHECK_PTR(e);

        EventContainer::erase(i);
        notifyRemove(e);
        delete e;

        i = j;
    }

    if (startTime == m_startTime && begin() != end()) {
        timeT startTime = (*begin())->getAbsoluteTime();
        if (m_composition) m_composition->setSegmentStartTime(this, startTime);
        else m_startTime = startTime;
        notifyStartChanged(m_startTime);
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
Segment::findSingle(Event* e)
{
    iterator res = end();

    std::pair<iterator, iterator> interval = equal_range(e);

    for (iterator i = interval.first; i != interval.second; ++i) {
        if (*i == e) {
            res = i;
            break;
        }
    }
    return res;
}


Segment::iterator
Segment::findTime(timeT t)
{
    Event dummy("dummy", t, 0, MIN_SUBORDERING);
    return lower_bound(&dummy);
}


Segment::iterator
Segment::findNearestTime(timeT t)
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
        notifyStartChanged(m_startTime);
    }

    TimeSignature ts;
    timeT sigTime = 0;

    if (getComposition()) {
        sigTime = getComposition()->getTimeSignatureAt(startTime, ts);
    }

    timeT restDuration = endTime - startTime;
    if (restDuration <= 0) return;

#ifdef DEBUG_NORMALIZE_RESTS
    cerr << "fillWithRests (" << startTime << "->" << endTime << "), composition "
         << (getComposition() ? "exists" : "does not exist") << ", sigTime "
         << sigTime << ", timeSig duration " << ts.getBarDuration() << ", restDuration " << restDuration << endl;
#endif

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

#ifdef DEBUG_NORMALIZE_RESTS
    cerr << "normalizeRests (" << startTime << "->" << endTime << "), segment starts at " << m_startTime << endl;
#endif

    if (startTime < m_startTime) {
#ifdef DEBUG_NORMALIZE_RESTS
        cerr << "normalizeRests: pulling start time back from "
             << m_startTime << " to " << startTime << endl;
#endif
        if (m_composition) m_composition->setSegmentStartTime(this, startTime);
        else m_startTime = startTime;
        notifyStartChanged(m_startTime);
    }

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
#ifdef DEBUG_NORMALIZE_RESTS
                cerr << "normalizeRests: divide-and-conquer on timesig at " << nextSigTime << endl;
#endif
                normalizeRests(startTime, nextSigTime);
                normalizeRests(nextSigTime, endTime);
                return;
            }
        }
    }

    // First stage: erase all existing non-tupleted rests in this
    // range and establish the proper extents for the time range in
    // which rests may need to be re-filled.  Only note, text, and
    // existing rest events can form a boundary for the range.  (Other
    // events, such as controllers, may appear at any time including
    // times that are not reasonable to quantize rest positions or
    // durations to.)

    timeT segmentEndTime = m_endTime;

    // Begin iterator.
    iterator ia = findNearestTime(startTime);
    if (ia == end()) ia = begin();
    if (ia == end()) { // the segment is empty
#ifdef DEBUG_NORMALIZE_RESTS
        cerr << "normalizeRests: empty segment" << endl;
#endif
        fillWithRests(startTime, endTime);
        return;
    } else {
        while (!((*ia)->isa(Note::EventType) ||
                 (*ia)->isa(Text::EventType) ||
                 (*ia)->isa(Note::EventRestType))) {
            if (ia == begin()) break;
            --ia;
        }
        if (startTime > (*ia)->getNotationAbsoluteTime()) {
            startTime = (*ia)->getNotationAbsoluteTime();
        }
    }

    // End iterator.
    iterator ib = findTime(endTime);
    while (ib != end()) {
        if ((*ib)->isa(Note::EventType) ||
            (*ib)->isa(Text::EventType) ||
            (*ib)->isa(Note::EventRestType)) break;
        ++ib;
    }
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
    // between us and it, and if it doesn't have precisely the right
    // duration, then we need to normalize it too.  (This should be
    // fairly harmless, all it can do wrong is extend the region of
    // interest too far and make us work too hard)

    iterator scooter = ia;
    while (scooter-- != begin()) {
        if ((*scooter)->getDuration() > 0) {
            if ((*scooter)->getNotationAbsoluteTime() +
                (*scooter)->getNotationDuration() !=
                startTime) {
                startTime = (*scooter)->getNotationAbsoluteTime();
#ifdef DEBUG_NORMALIZE_RESTS
                cerr << "normalizeRests: scooting back to " << startTime << endl;
#endif
                ia = scooter;
            }
            break;
        }
    }

    // For each event within the adjusted range, erase each rest.
    for (iterator i = ia, j = i; i != ib && i != end(); i = j) {
        ++j;
        if ((*i)->isa(Note::EventRestType) &&
            !(*i)->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE) &&
            !(*i)->has(BaseProperties::INVISIBLE)) {
#ifdef DEBUG_NORMALIZE_RESTS
            cerr << "normalizeRests: erasing rest at " << (*i)->getAbsoluteTime() << endl;
#endif
            erase(i);
        }
    }

    // It's possible we've just removed all the events between here
    // and the end of the segment, if they were all rests.  Check.

    if (endTime < segmentEndTime && m_endTime < segmentEndTime) {
#ifdef DEBUG_NORMALIZE_RESTS
        cerr << "normalizeRests: new end time " << m_endTime << " is earlier than previous segment end time " << segmentEndTime << ", extending our working end time again" << endl;
#endif
        endTime = segmentEndTime;
    }

    // Second stage: find the gaps that need to be filled with
    // rests.  We don't mind about the case where two simultaneous
    // notes end at different times -- we're only interested in
    // the one ending sooner.  Each time an event ends, we start
    // a candidate gap.

    timeT lastNoteStarts = startTime;
    timeT lastNoteEnds = startTime;

    // Re-find this, as it might have been erased
    ia = findNearestTime(startTime);

    if (ia == end()) {
        // already have good lastNoteStarts, lastNoteEnds
        ia = begin();
    } else {
        while (!((*ia)->isa(Note::EventType) ||
                 (*ia)->isa(Text::EventType) ||
                 (*ia)->isa(Note::EventRestType))) {
            if (ia == begin()) break;
            --ia;
        }
        lastNoteStarts = (*ia)->getNotationAbsoluteTime();
        lastNoteEnds = lastNoteStarts;
    }

    std::vector<std::pair<timeT, timeT> > gaps;

    // For each event, add any gaps to the gaps vector.
    for (iterator i = ia; i != ib && i != end(); ++i) {

        // Boundary events for sets of rests may be notes (obviously),
        // text events (because they need to be "attached" to
        // something that has the correct timing), or rests (any
        // remaining rests in this area have tuplet data so should be
        // treated as "hard" rests);
        if (!((*i)->isa(Note::EventType) ||
              (*i)->isa(Text::EventType) ||
              (*i)->isa(Note::EventRestType))) {
            continue;
        }

        timeT thisNoteStarts = (*i)->getNotationAbsoluteTime();

#ifdef DEBUG_NORMALIZE_RESTS
        cerr << "normalizeRests: scanning: thisNoteStarts " << thisNoteStarts
             << ", lastNoteStarts " << lastNoteStarts
             << ", lastNoteEnds " << lastNoteEnds << endl;
#endif

        /* BR #988185: "Notation: Rest can be simultaneous with note but follow it"

           This conditional tested whether a note started before the
           preceding note ended, and if so inserted rests simultaneous
           with the preceding note to make up the gap.  Without the
           ability to lay out those rests partwise, this is never any
           better than plain confusing.  Revert the change.

        if (thisNoteStarts < lastNoteEnds &&
            thisNoteStarts > lastNoteStarts) {
            gaps.push_back(std::pair<timeT, timeT>
                           (lastNoteStarts,
                            thisNoteStarts - lastNoteStarts));
        }
        */

        if (thisNoteStarts > lastNoteEnds) {
#ifdef DEBUG_NORMALIZE_RESTS
            cerr << "normalizeRests: found gap between note/text/rest events from " << lastNoteEnds << " to " << thisNoteStarts << endl;
#endif
            gaps.push_back(std::pair<timeT, timeT>
                           (lastNoteEnds,
                            thisNoteStarts - lastNoteEnds));
        }

        lastNoteStarts = thisNoteStarts;
        lastNoteEnds = thisNoteStarts + (*i)->getNotationDuration();
    }

    if (endTime > lastNoteEnds) {
#ifdef DEBUG_NORMALIZE_RESTS
        cerr << "normalizeRests: need to fill up gap from last note/text/rest event end at " << lastNoteEnds << " to normalize end time at " << endTime << endl;
#endif
        gaps.push_back(std::pair<timeT, timeT>
                       (lastNoteEnds, endTime - lastNoteEnds));
    }

    timeT duration;

    // For each gap, fill it in with rests.
    for (size_t gi = 0; gi < gaps.size(); ++gi) {

#ifdef DEBUG_NORMALIZE_RESTS
        cerr << "normalizeRests: gap " << gi << ": " << gaps[gi].first << " -> " << (gaps[gi].first + gaps[gi].second) << endl;
#endif

        startTime = gaps[gi].first;
        duration = gaps[gi].second;

        if (duration >= Note(Note::Shortest).getDuration()) {
            fillWithRests(startTime, startTime + duration);
        }
    }
}



void Segment::getTimeSlice(timeT absoluteTime, iterator &start, iterator &end)
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

void Segment::getTimeSlice(timeT absoluteTime, const_iterator &start, const_iterator &end)
    const
{
    Event dummy("dummy", absoluteTime, 0, MIN_SUBORDERING);

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

const BasicQuantizer *
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
Segment::setDelay(timeT delay)
{
    m_delay = delay;
    if (m_composition) {
        // don't updateRefreshStatuses() - affects playback only
        m_composition->notifySegmentEventsTimingChanged(this, delay, RealTime::zeroTime);
    }
}

void
Segment::setRealTimeDelay(RealTime delay)
{
    m_realTimeDelay = delay;
    if (m_composition) {
        // don't updateRefreshStatuses() - affects playback only
        m_composition->notifySegmentEventsTimingChanged(this, 0, delay);
    }
}

void
Segment::setTranspose(int transpose)
{
    m_transpose = transpose;
    notifyTransposeChange();
    if (m_composition) {
        // don't updateRefreshStatuses() - affects playback only
        m_composition->notifySegmentTransposeChanged(this, transpose);
    }
}

void
Segment::setAudioFileId(unsigned int id)
{
    m_audioFileId = id;
    updateRefreshStatuses(getStartTime(), getEndTime());
}

void
Segment::setUnstretchedFileId(unsigned int id)
{
    m_unstretchedFileId = id;
}

void
Segment::setStretchRatio(float ratio)
{
    m_stretchRatio = ratio;
}

void
Segment::setAudioStartTime(const RealTime &time)
{
    m_audioStartTime = time;
    updateRefreshStatuses(getStartTime(), getEndTime());
}

void
Segment::setAudioEndTime(const RealTime &time)
{
    RealTime oldAudioEndTime = m_audioEndTime;
    m_audioEndTime = time;
    updateRefreshStatuses(getStartTime(), getEndTime());
    notifyEndMarkerChange(time < oldAudioEndTime);
}

void
Segment::setAutoFade(bool value)
{
    m_autoFade = value;
    updateRefreshStatuses(getStartTime(), getEndTime());
}

void
Segment::setFadeInTime(const RealTime &time)
{
    m_fadeInTime = time;
    updateRefreshStatuses(getStartTime(), getEndTime());
}

void
Segment::setFadeOutTime(const RealTime &time)
{
    m_fadeOutTime = time;
    updateRefreshStatuses(getStartTime(), getEndTime());
}

void
Segment::setLabel(const std::string &label)
{
    m_label = label;
    if (m_composition) m_composition->updateRefreshStatuses();
    notifyAppearanceChange();
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
    timeT ctime;
    return getClefAtTime(time, ctime);
}

Clef
Segment::getClefAtTime(timeT time, timeT &ctime) const
{
    if (!m_clefKeyList) return Clef();

    Event ec(Clef::EventType, time);
    ClefKeyList::iterator i = m_clefKeyList->lower_bound(&ec);

    while (i == m_clefKeyList->end() ||
           (*i)->getAbsoluteTime() > time ||
           (*i)->getType() != Clef::EventType) {

        if (i == m_clefKeyList->begin()) {
            ctime = getStartTime();
            return Clef();
        }
        --i;
    }

    try {
        ctime = (*i)->getAbsoluteTime();
        return Clef(**i);
    } catch (const Exception &e) {
        std::cerr << "Segment::getClefAtTime(" << time
                  << "): bogus clef in ClefKeyList: event dump follows:"
                  << std::endl;
        (*i)->dump(std::cerr);
        return Clef();
    }
}

bool
Segment::getNextClefTime(timeT time, timeT &nextTime) const
{
    if (!m_clefKeyList) return false;

    Event ec(Clef::EventType, time);
    ClefKeyList::iterator i = m_clefKeyList->lower_bound(&ec);

    while (i != m_clefKeyList->end() &&
           ((*i)->getAbsoluteTime() <= time ||
            (*i)->getType() != Clef::EventType)) {
        ++i;
    }

    if (i == m_clefKeyList->end()) return false;

    nextTime = (*i)->getAbsoluteTime();

    return true;
}

Key
Segment::getKeyAtTime(timeT time) const
{
    timeT ktime;
    return getKeyAtTime(time, ktime);
}

Key
Segment::getKeyAtTime(timeT time, timeT &ktime) const
{
    if (!m_clefKeyList) return Key();

    Event ek(Key::EventType, time);
    ClefKeyList::iterator i = m_clefKeyList->lower_bound(&ek);

    while (i == m_clefKeyList->end() ||
           (*i)->getAbsoluteTime() > time ||
           (*i)->getType() != Key::EventType) {

        if (i == m_clefKeyList->begin()) {
            ktime = getStartTime();
            return Key();
        }
        --i;
    }

    try {
        ktime = (*i)->getAbsoluteTime();
        Key k(**i);
//        std::cerr << "Segment::getKeyAtTime: Requested time " << time << ", found key " << k.getName() << " at time " << ktime << std::endl;
        return k;
    } catch (const Exception &e) {
        std::cerr << "Segment::getClefAtTime(" << time
                  << "): bogus key in ClefKeyList: event dump follows:"
                  << std::endl;
        (*i)->dump(std::cerr);
        return Key();
    }
}

bool
Segment::getNextKeyTime(timeT time, timeT &nextTime) const
{
    if (!m_clefKeyList) return false;

    Event ec(Key::EventType, time);
    ClefKeyList::iterator i = m_clefKeyList->lower_bound(&ec);

    while (i != m_clefKeyList->end() &&
           ((*i)->getAbsoluteTime() <= time ||
            (*i)->getType() != Key::EventType)) {
        ++i;
    }

    if (i == m_clefKeyList->end()) return false;

    nextTime = (*i)->getAbsoluteTime();

    return true;
}

void
Segment::getFirstClefAndKey(Clef &clef, Key &key)
{
    bool keyFound = false;
    bool clefFound = false;
    clef = Clef();          // Default clef
    key = Key();            // Default key signature

    iterator i = begin();
    while (i!=end()) {
        // Keep current clef and key as soon as a note or rest event is found
        if ((*i)->isa(Note::EventRestType) || (*i)->isa(Note::EventType)) return;

        // Remember the first clef event found
        if ((*i)->isa(Clef::EventType)) {
            clef = Clef(*(*i));
            // and return if a key has already been found
            if (keyFound) return;
            clefFound = true;
        }

        // Remember the first key event found
        if ((*i)->isa(Key::EventType)) {
            key = Key(*(*i));
            // and return if a clef has already been found
            if (clefFound) return;
            keyFound = true;
        }

        ++i;
    }
}


void
Segment::enforceBeginWithClefAndKey()
{
    bool keyFound = false;
    bool clefFound = false;

    iterator i = begin();
    while (i!=end()) {
        // Keep current clef and key as soon as a note or rest event is found
        if ((*i)->isa(Note::EventRestType) || (*i)->isa(Note::EventType)) break;

        // Remember if a clef event is found
        if ((*i)->isa(Clef::EventType)) {
            clefFound = true;
            // and stop looking for next events if a key has already been found
            if (keyFound) break;
        }

        // Remember if a key event is found
        if ((*i)->isa(Key::EventType)) {
            keyFound = true;
            // and stop looking for next events if a clef has already been found
            if (clefFound) break;
        }

        ++i;
    }

    // Insert default clef and key signature if needed
    if (!keyFound) insert(Key().getAsEvent(m_startTime));
    if (!clefFound) insert(Clef().getAsEvent(m_startTime));
}


timeT
Segment::getRepeatEndTime() const
{
    timeT endMarker = getEndMarkerTime();

    if (m_repeating && m_composition) {
        timeT endTime = m_composition->getEndMarker();
        
        for (Composition::iterator i(m_composition->begin());
             i != m_composition->end(); ++i) {
          
            if ((*i)->getTrack() != getTrack()) continue;
            
            timeT t1 = (*i)->getStartTime();
            timeT t2 = (*i)->getEndMarkerTime();

            if (t2 > endMarker) {
                if (t1 < endTime) {
                    if (t1 < endMarker) {
                        endTime = endMarker;
                        break;
                    } else {
                        endTime = t1;
                    }
                }
            }
        }

        return endTime;
    }

    return endMarker;
}

void
Segment::
checkInsertAsClefKey(Event *e) const
{
    if (e->isa(Clef::EventType) || e->isa(Key::EventType)) {
        if (!m_clefKeyList) m_clefKeyList = new ClefKeyList;
        m_clefKeyList->insert(e);
    }
}

void
Segment::notifyAdd(Event *e) const
{
    Profiler profiler("Segment::notifyAdd()");
    checkInsertAsClefKey(e);

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->eventAdded(this, e);
    }
}


void
Segment::notifyRemove(Event *e) const
{
    Profiler profiler("Segment::notifyRemove()");

    if (m_clefKeyList && (e->isa(Clef::EventType) || e->isa(Key::EventType))) {
        ClefKeyList::iterator i;
        for (i = m_clefKeyList->find(e); i != m_clefKeyList->end(); ++i) {
            // fix for bug#1485643 (crash erasing a duplicated key signature)
            if ((*i) == e) {
                m_clefKeyList->erase(i);
                break;
            }
        }
    }

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->eventRemoved(this, e);
    }
}


void
Segment::notifyAppearanceChange() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->appearanceChanged(this);
    }
}

void
Segment::notifyStartChanged(timeT newTime)
{
    Profiler profiler("Segment::notifyStartChanged()");
    if (m_notifyResizeLocked) return;

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->startChanged(this, newTime);
    }
    if (m_composition) {
        m_composition->distributeVerses();
        m_composition->notifySegmentStartChanged(this, newTime);
    }
}


void
Segment::notifyEndMarkerChange(bool shorten)
{
    Profiler profiler("Segment::notifyEndMarkerChange()");

    if (m_notifyResizeLocked) return;

    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->endMarkerTimeChanged(this, shorten);
    }
    if (m_composition) {
        m_composition->notifySegmentEndMarkerChange(this, shorten);
    }
}


void
Segment::notifyTransposeChange()
{
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end(); ++i) {
        (*i)->transposeChanged(this, m_transpose);
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
Segment::lockResizeNotifications()
{
    m_notifyResizeLocked = true;
    m_memoStart = m_startTime;
    m_memoEndMarkerTime = m_endMarkerTime ? new timeT(*m_endMarkerTime) : nullptr;
}

void
Segment::unlockResizeNotifications()
{
    m_notifyResizeLocked = false;
    if (m_startTime != m_memoStart) notifyStartChanged(m_startTime);
    if (!m_memoEndMarkerTime && !m_endMarkerTime) return;  // ???
    bool shorten = false;
    if (m_memoEndMarkerTime && m_endMarkerTime) {
        if (*m_memoEndMarkerTime > *m_endMarkerTime) shorten = true;
        else if (*m_memoEndMarkerTime == *m_endMarkerTime) return;
    }
    
    // What if m_memoEndMarkerTime=0 and m_endMarkerTime!=0 (or the
    // opposite) ?   Is such a case possible ?
    
    if (m_memoEndMarkerTime) delete m_memoEndMarkerTime;
    m_memoEndMarkerTime = nullptr;
    notifyEndMarkerChange(shorten);
}

void
Segment::setColourIndex(const unsigned int input)
{
    m_colourIndex = input;
    updateRefreshStatuses(getStartTime(), getEndTime());
    if (m_composition) m_composition->updateRefreshStatuses();
    notifyAppearanceChange();
}

QColor
Segment::getPreviewColour() const
{
    // Select a preview colour for best visibility against the segment's
    // colour.

    // StaffHeader::updateHeader() does something similar.  Might want
    // to offer a Colour::getVisibleForeground() to handle this concept
    // in a more central location.  Like the unused
    // Colour::getContrastingColour().

    if (!m_composition) {
        return Qt::black;
    }

    QColor segmentColour = GUIPalette::convertColour(
            m_composition->getSegmentColourMap().getColourByIndex(m_colourIndex));

    int intensity = qGray(segmentColour.rgb());

    // Go with black for bright backgrounds
    if (intensity > 127) {
        return Qt::black;
    }

    // And white for dark backgrounds
    return Qt::white;
}

void
Segment::addEventRuler(const std::string &type, int controllerValue, bool active)
{
    EventRulerListConstIterator it;

    for (it = m_eventRulerList.begin(); it != m_eventRulerList.end(); ++it)
        if ((*it)->m_type == type && (*it)->m_controllerValue == controllerValue)
            return;

    m_eventRulerList.push_back(new EventRuler(type, controllerValue, active));
}

bool
Segment::deleteEventRuler(const std::string &type, int controllerValue)
{
    EventRulerListIterator it;

    for (it = m_eventRulerList.begin(); it != m_eventRulerList.end(); ++it)
    {
        if ((*it)->m_type == type && (*it)->m_controllerValue == controllerValue)
        {
            delete *it;
            m_eventRulerList.erase(it);
            return true;
        }
    }

    return false;
}


Segment::EventRuler*
Segment::getEventRuler(const std::string &type, int controllerValue)
{
    EventRulerListConstIterator it;
    for (it = m_eventRulerList.begin(); it != m_eventRulerList.end(); ++it)
        if ((*it)->m_type == type && (*it)->m_controllerValue == controllerValue)
            return *it;

    return nullptr;
}


int
Segment::getVerseCount()
{
    if (m_verseCount == -1) countVerses();
    return m_verseCount;
}

int
Segment::getVerseWrapped()
{
    int count = getVerseCount();
    return count ? getVerse() % count : 0;
}

// Following code moved from LyricEditDialog.cpp
void
Segment::countVerses()
{
    m_verseCount = 0;

    for (iterator i = begin(); isBeforeEndMarker(i); ++i) {

        if ((*i)->isa(Text::EventType)) {

            std::string textType;
            if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
                textType == Text::Lyric) {

                long verse = 0;
                (*i)->get<Int>(Text::LyricVersePropertyName, verse);

                if (verse >= m_verseCount) m_verseCount = verse + 1;
            }
        }
    }
}

SegmentMultiSet&
Segment::
getCompositionSegments(void)
{
    Composition* composition = DocumentGet::getComposition();
    return composition->getSegments();
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

/// YG: Only for debug
void
Segment::dumpObservers()
{
    std::cerr << "Observers of segment " << this << " are:\n";
     for (ObserverSet::const_iterator i = m_observers.begin();
          i != m_observers.end(); ++i) {
        std::cerr << " " << (*i);
    }
    std::cerr << "\n";
    for (ObserverSet::const_iterator i = m_observers.begin();
          i != m_observers.end(); ++i) {
              Segment * seg = dynamic_cast<Segment *>(*i);
              StaffHeader * sh = dynamic_cast<StaffHeader *>(*i);
        std::cerr << "        " << (*i);
        if (seg) std::cerr << " ==> Segment " << seg;
        if (sh) std::cerr << " ==> StaffHeader " << sh;
        std::cerr << "\n";
    }
}

void
SegmentObserver::
allEventsChanged(const Segment *s)
{
    Profiler profiler("SegmentObserver::allEventsChanged");
    for (Segment::iterator i = s->begin(); i != s->end(); ++i) {
        Event *e = *i;
        eventRemoved(s, e);
        eventAdded(s, e);
    }
}

// Find the next Event of "type".
EventContainer::iterator
EventContainer::findEventOfType(EventContainer::iterator i,
                                const std::string &type)
{
    for (; i != end(); ++i) {
        Event *e = *i;
        if (e->isa(type)) { return i; }
    }
    return i;
}

}

