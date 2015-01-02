/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TriggerSegment.h"

#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/ControllerContext.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentLinker.h"
#include "base/SegmentNotationHelper.h"
#include "base/SegmentPerformanceHelper.h"
#include <limits>
#include <queue>

namespace Rosegarden
{

/*** Helper classes ***/

// @class LinearTimeScale
// A linear transform of time to and from the internal
// times of a triggered segment.
// @author Tom Breton (Tehom)
class LinearTimeScale
{
 public:
    LinearTimeScale(const TriggerSegmentRec *rec,
                    Segment::iterator        trigger,
                    const Segment           *oversegment,
                    const LinearTimeScale    timeScale);

    // Convert the triggered segment's intrinsic times to outside
    // times.
    timeT  toPerformance(timeT t) const
    { return timeT((t + m_offset) * m_ratio); }
    timeT  toPerformanceDuration(timeT t) const
    { return timeT(t * m_ratio); }

    // Convert outside times to the triggered segment's intrinsic
    // times (Unused)
    timeT  toInternal(timeT t) const
    { return timeT((t/m_ratio) - m_offset); }
    timeT  toInternalDuration(timeT t) const
    { return timeT(t/m_ratio); }

    // Whether the performance duration is different than the
    // segment's intrinsic duration.
    bool isSquished(void) const
    { return m_ratio != 1.0; }
    
    bool isPerformable(void) const
    { return m_ratio != 0.0; }
    static const LinearTimeScale m_identity;
    static const LinearTimeScale m_unperformable;
 private:
    LinearTimeScale(double ratio, timeT  offset) :
        m_ratio(ratio), m_offset(offset) {}
    
    // The ratio of performance time to event time.
    double m_ratio;
    // How much performance time is advanced relative to event time,
    // after scaled for performance.
    timeT  m_offset;
};
    
// @class TriggerExpansionContext
// All the data neccessary to expand a trigger segment correctly.
// This is constant across one expansion; it contains no state data.
// @author Tom Breton (Tehom)
class TriggerExpansionContext
{
    typedef std::pair<timeT,timeT> TimeInterval;
    typedef std::vector<TimeInterval> TimeIntervalVector;
    
public:    
    typedef std::queue<TriggerExpansionContext> Queue;
    typedef std::vector<Segment::iterator> iteratorcontainer;

    TriggerExpansionContext(int maxDepth,
                            const TriggerSegmentRec *rec,
                            Segment::iterator        iTrigger,
                            Segment                 *containing,
                            ControllerContextParams *controllerContextParams,
                            const LinearTimeScale timeScale) :
        m_maxDepth(maxDepth),
        m_rec(rec),
        m_timeScale(LinearTimeScale(rec, iTrigger,
                                     containing, timeScale)),
        m_pitchDiff(rec->getTranspose(*iTrigger)),
        m_velocityDiff(rec->getVelocityDiff(*iTrigger)),
        m_controllerContextParams(controllerContextParams),
        m_intervals(getSoundingIntervals(iTrigger, containing, timeScale))
        { m_retune = (m_pitchDiff != 0); }

private:
    // Nearly direct assignment
    TriggerExpansionContext(const TimeIntervalVector &intervals,
                            int                      maxDepth,
                            const TriggerSegmentRec *rec,
                            int                      pitchDiff,
                            int                      velocityDiff,
                            ControllerContextParams *controllerContextParams,
                            const LinearTimeScale timeScale) :
        m_maxDepth(maxDepth),
        m_rec(rec),
        m_timeScale(timeScale),
        m_pitchDiff(pitchDiff),
        m_velocityDiff(velocityDiff),
        m_controllerContextParams(controllerContextParams),
        m_intervals(intervals)
        { m_retune = (m_pitchDiff != 0); }
public:
    
    bool isPerformable(void) const {
        return
            !m_intervals.empty() &&
            m_timeScale.isPerformable();
    }

    bool Expand(Segment *target, Queue& queue) const;

private:
    static TimeIntervalVector
    getSoundingIntervals(Segment::iterator iTrigger,
                         const Segment *containing,
                         const LinearTimeScale timeScale);

    static TimeIntervalVector
    mergeTimeIntervalVectors(const TimeIntervalVector &a,
                             const TimeIntervalVector &b);

    TriggerExpansionContext
    makeNestedContext(Segment::iterator iTrigger,
                      const Segment *containing) const;
        
    // Data members
    int                       m_maxDepth;
    const TriggerSegmentRec  *m_rec;
    LinearTimeScale           m_timeScale;
    int                       m_pitchDiff;
    bool                      m_retune;
    int                       m_velocityDiff;
    // May be NULL
    ControllerContextParams  *m_controllerContextParams;
    TimeIntervalVector        m_intervals;   
};

/*** TriggerSegmentRec definitions ***/

TriggerSegmentRec::~TriggerSegmentRec()
{
    // nothing -- we don't delete the segment here
}

TriggerSegmentRec::TriggerSegmentRec(TriggerSegmentId id,
				     Segment *segment,
				     int basePitch,
				     int baseVelocity,
				     std::string timeAdjust, 
				     bool retune) :
    m_id(id),
    m_segment(segment),
    m_basePitch(basePitch),
    m_baseVelocity(baseVelocity),
    m_defaultTimeAdjust(timeAdjust),
    m_defaultRetune(retune)
{
    if (m_defaultTimeAdjust == "") {
	m_defaultTimeAdjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH;
    }

    calculateBases();
    updateReferences();
}

TriggerSegmentRec::TriggerSegmentRec(const TriggerSegmentRec &rec) :
    m_id(rec.m_id),
    m_segment(rec.m_segment),
    m_basePitch(rec.m_basePitch),
    m_baseVelocity(rec.m_baseVelocity),
    m_defaultTimeAdjust(rec.m_defaultTimeAdjust),
    m_defaultRetune(rec.m_defaultRetune),
    m_references(rec.m_references)
{
    // nothing else
}

TriggerSegmentRec &
TriggerSegmentRec::operator=(const TriggerSegmentRec &rec)
{
    if (&rec == this) return *this;
    m_id = rec.m_id;
    m_segment = rec.m_segment;
    m_basePitch = rec.m_basePitch;
    m_baseVelocity = rec.m_baseVelocity;
    m_references = rec.m_references;
    return *this;
}

void
TriggerSegmentRec::updateReferences()
{
    if (!m_segment) return;

    Composition *c = m_segment->getComposition();
    if (!c) return;

    m_references.clear();

    for (Composition::iterator i = c->begin(); i != c->end(); ++i) {
	for (Segment::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {
	    if ((*j)->has(BaseProperties::TRIGGER_SEGMENT_ID) &&
		(*j)->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID) == long(m_id)) {
		m_references.insert((*i)->getRuntimeId());
		break; // from inner loop only: go on to next segment
	    }
	}
    }
}

void
TriggerSegmentRec::calculateBases()
{
    if (!m_segment) return;

    for (Segment::iterator i = m_segment->begin();
	 m_segment->isBeforeEndMarker(i); ++i) {

	if (m_basePitch >= 0 && m_baseVelocity >= 0) return;

	if (m_basePitch < 0) {
	    if ((*i)->has(BaseProperties::PITCH)) {
		m_basePitch = (*i)->get<Int>(BaseProperties::PITCH);
	    }
	}

	if (m_baseVelocity < 0) {
	    if ((*i)->has(BaseProperties::VELOCITY)) {
		m_baseVelocity = (*i)->get<Int>(BaseProperties::VELOCITY);
	    }
	}
    }

    if (m_basePitch < 0) m_basePitch = 60;
    if (m_baseVelocity < 0) m_baseVelocity = 100;
}

// @return
// The amount by which to adjust pitch when this trigger performs this
// ornament.
// @author Tom Breton (Tehom)
int
TriggerSegmentRec::
getTranspose(const Event *trigger) const
{
    if(!trigger->has(BaseProperties::PITCH))
        { return 0; }

    bool retune = getDefaultRetune(); 
    trigger->get<Bool>(BaseProperties::TRIGGER_SEGMENT_RETUNE,
                       retune);
    if (!retune)
        { return 0; }

    return
        trigger->get<Int>(BaseProperties::PITCH) - getBasePitch();
}

// @return
// The amount by which to adjust velocity when this trigger performs
// this ornament.
// @author Tom Breton (Tehom)
int
TriggerSegmentRec::
getVelocityDiff(const Event *trigger) const
{
    long evVelocity = getBaseVelocity();
    (void)trigger->get<Int>(BaseProperties::VELOCITY, evVelocity);
    return evVelocity - getBaseVelocity();
}

// @return
// A segment linked to the trigger segment, adjusted in pitch and time
// to match the ornament as performed by trigger.  Caller owns this.
// This can be NULL if no meaningful segment could be made.
// @param trigger
// The triggering event.
// @param containing
// The segment containing the triggering event
// @author Tom Breton (Tehom)
Segment *
TriggerSegmentRec::makeLinkedSegment
(Event *trigger, Segment *containing) 
{
    LinearTimeScale
        timeScale(this, containing->findSingle(trigger), containing,
                   LinearTimeScale::m_identity);

    // If we squash or stretch, we can't make a proper linked segment
    // so return NULL;
    if (timeScale.isSquished())
        { return 0; }
    
    Segment *link =
        SegmentLinker::createLinkedSegment(getSegment());

    // getTranspose is relative to the note, so add the containing
    // segment's transpose to it to get the true relation from event
    // pitch to performance pitch.
    const int transpose =
        getTranspose(trigger) + containing->getTranspose();
    // !!! Punt for now.
    const int steps = transpose * 12 / 7;

    link->setLinkTransposeParams
        (Segment::LinkTransposeParams(true, steps, transpose, false));

    link->getLinker()->refreshSegment(link);
    // Adjust the start.  We have no way to truncate the start, as
    // TRIGGER_SEGMENT_ADJUST_SYNC_END wants, but starting it early
    // does no harm.
    timeT adjustedStart =
        timeScale.toPerformance(getSegment()->getStartTime());
    timeT adjustedEnd =
        timeScale.toPerformance(getSegment()->getEndMarkerTime());
    link->setStartTime(adjustedStart);
    link->setEndMarkerTime(adjustedEnd);
    getSegment()->setAsReference();
    return link;
}

//
// @return
// A fresh segment containing the ornament fully expanded.  Caller
// owns this.  This can be NULL if no meaningful segment could be
// made.
// @param trigger
// The triggering event.
// @param containing
// The segment containing the triggering event
// @param instrument
// The instrument this is playing on.  Currently unused.
// @remarks
// We don't handle setting controllers to correct absolute values.  To
// do so would probably require that segments store their current
// expansions and a controller cache.
// @author Tom Breton (Tehom)
Segment*
TriggerSegmentRec::makeExpansion(Event *trigger, 
                                 Segment *containing,
                                 Instrument */*instrument*/) const
{
    Segment *s = new Segment();
    ExpandInto(s, containing->findSingle(trigger), containing, 0);

    if (s->empty()) {
        delete s;
        return 0;
    }

    timeT dummy;
    bool gotClef = s->getNextClefTime(s->getStartTime() - 1, dummy);
    if (!gotClef) {
        Clef clef =
            SegmentNotationHelper(*s).guessClef(s->begin(), s->end());
        s->insert(clef.getAsEvent(s->getStartTime()));
    }
    return s;
}

// Expand the ornament into target.  
// @return
// True if anything was inserted
// @param target
// The segment to put events into.  It need not be empty.
// @param iTrigger
// An iterator to the triggering event.
// @param containing
// The segment containing the triggering event
// @param controllerContext
// A ControllerContextMap on `containing'.
// @author Tom Breton (Tehom)
bool
TriggerSegmentRec::
ExpandInto(Segment *target,
           Segment::iterator iTrigger,
           Segment *containing,
           ControllerContextParams *controllerContextParams) const
{
    if (!this || !getSegment() || getSegment()->empty())
        { return false; }
    const int maxDepth = 10;

    bool insertedSomething = false;
    TriggerExpansionContext::Queue queue;
    // Put the initial expansion context into the queue.
    queue.push(TriggerExpansionContext(maxDepth, this, iTrigger,
                                       containing,
                                       controllerContextParams,
                                       LinearTimeScale::m_identity));

    // Expand entries in the queue, possibly acquiring more entries as
    // we go along.  We won't loop forever because maxDepth limits
    // recursion.
    for (; !queue.empty(); queue.pop()) {
        if (!queue.front().isPerformable()) { continue; }
        // Queue might acquire more entries here.
        bool insertedThisTime =
            queue.front().Expand(target, queue);
        insertedSomething |= insertedThisTime;
    }

    return insertedSomething;
}

/*** LinearTimeScale definitions ***/

const LinearTimeScale
LinearTimeScale::m_identity(1.0, 0);
const LinearTimeScale
LinearTimeScale::m_unperformable(0.0, 0);

// @ctor for LinearTimeScale.
// Computes it relative to the given trigger segment and event.
// @param rec is the triggered segment's rec.
// @param trigger is an iterator corresponding to the triggering
// event.
// @param oversegment is the segment that the trigger is in.
// @author Tom Breton (Tehom)
// Adapted from SegmentMapper.cpp
LinearTimeScale::
LinearTimeScale(const TriggerSegmentRec *rec,
                Segment::iterator iTrigger,
                const Segment *oversegment,
                const LinearTimeScale timeScale)
{
    Event * ev = *iTrigger;

    // Get local performance times, which aren't scaled to top level.
    const timeT unscaledPerformanceStart =
        ev->getAbsoluteTime();
    // We can cast away the const because getSoundingDuration doesn't
    // change the segment, though it can change some properties of
    // events in it.
    const timeT unscaledPerformanceDuration =
        SegmentPerformanceHelper(*const_cast<Segment *>(oversegment)).
        getSoundingDuration(iTrigger);
    if (unscaledPerformanceDuration <= 0) {
        *this = m_unperformable;
        return;
    }

    // Scale to top-level performance times.
    const timeT performanceStart =
        timeScale.toPerformance(unscaledPerformanceStart);
    const timeT performanceDuration =
        timeScale.toPerformanceDuration(unscaledPerformanceDuration);

    
    const timeT performanceEnd = performanceStart + performanceDuration;
    const timeT trStart = rec->getSegment()->getStartTime();
    const timeT trEnd = rec->getSegment()->getEndMarkerTime();
    const timeT trDuration = trEnd - trStart;

    std::string adjustmentMode = BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;
    ev->get<String>(BaseProperties::TRIGGER_SEGMENT_ADJUST_TIMES, adjustmentMode);
    
    // To avoid dividing by zero in the squish case, if duration is
    // zero, we use the default case instead.
    if ((adjustmentMode == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) &&
        (trDuration != 0)) {
        m_ratio = (double(performanceDuration) / double(trDuration));
        // m_offset is after the ratio.
        m_offset = performanceStart - timeT(double(trStart) / m_ratio);
    } else if (adjustmentMode == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
        m_ratio = 1.0;
        m_offset = performanceEnd - trEnd;
    } else {
        // TRIGGER_SEGMENT_ADJUST_NONE,
        // TRIGGER_SEGMENT_ADJUST_SYNC_START, or zero duration
        // TRIGGER_SEGMENT_ADJUST_SQUISH.  We handle clipped to the
        // trigger end elsewhere.
        m_ratio = 1.0;
        m_offset = performanceStart - trStart;
    }
    
}

/*** TriggerExpansionContext definitions ***/

// @param oversegment is the segment that the trigger is in.
// @param iTrigger
// iterator to the trigger note.  iTrigger must indicate the trigger
// note itself, not just a tied note in the same group.
// @author Tom Breton (Tehom)
TriggerExpansionContext::TimeIntervalVector
TriggerExpansionContext::
getSoundingIntervals(Segment::iterator iTrigger,
                     const Segment *oversegment,
                     const LinearTimeScale timeScale)
{
    std::string adjustmentMode = BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;
    (*iTrigger)->get<String>(BaseProperties::TRIGGER_SEGMENT_ADJUST_TIMES, adjustmentMode);

    // Get the tied notes that constitute trigger.  This gives us the
    // complete clipping frame, even if the triggered segment is
    // shorter.  We can cast away the const because getTiedNotes
    // doesn't change segment, though it can change some properties of
    // events in it.
    iteratorcontainer tiedNotes =
        SegmentPerformanceHelper(*const_cast<Segment *>(oversegment)).
        getTiedNotes(iTrigger);

    
    iteratorcontainer::iterator ci = tiedNotes.begin();
    if (ci == tiedNotes.end()) { return TimeIntervalVector(); }

    /** Define state variables across iterations **/

    // Whether the previous iteration was masked.
    bool wasMasked = true;
    // The time that the next forthcoming interval starts at.  Not
    // valid when wasMasked is true.
    timeT startInterval;
    // Container for the intervals we collect.
    TimeIntervalVector  intervals;

    /** Do the loop itself **/
    
    while (true) {
        const Event *e = **ci;
        const timeT unscaledT = e->getAbsoluteTime();
        const timeT t = timeScale.toPerformance(unscaledT);
        
        const bool nowMasked = e->maskedInTrigger();

        if (nowMasked != wasMasked) {

            // If we have changed mask state, either make an interval
            // (if we just finished sounding) or remember when the
            // next interval will starts (if we just started
            // sounding).
            if (!wasMasked)
                { intervals.push_back(TimeInterval(startInterval, t)); }
            else
                { startInterval = t; }
        }

        // Step the loop
        wasMasked = nowMasked;
        ++ci;

        // Check whether we just hit the end.
        if (ci == tiedNotes.end()) {
            // If the end terminates a playing section, make a final
            // time interval.  This is only done when the trigger's
            // last tied note is unmasked; if user masked the end, we
            // assume he doesn't want the ornament to start playing
            // when the trigger ends, not even for ...ADJUST_NONE.
            if (!wasMasked) {
                timeT endT;
                // For ...ADJUST_NONE + unmasked, we play the full
                // ornament regardless the length of the trigger.  We
                // only clip to containing segment's end time.
                if (adjustmentMode == BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE) {
                        endT = oversegment->getEndMarkerTime();
                    } else {
                    timeT unscaledDuration =
                        e->getDuration();
                    timeT d = timeScale.toPerformance(unscaledDuration);
                    endT = t + d;
                }

                if (endT <= t) { break; }

                // Make the final interval.
                intervals.push_back(TimeInterval(startInterval, endT));
            }
            break;
        }
    }
    
    return intervals;
}

// @return
// Returns the timewise intersection of two time interval vectors.
// That is, it contains only the points in time that are contained in
// both of the inputs.
// @author Tom Breton (Tehom)
TriggerExpansionContext::TimeIntervalVector
TriggerExpansionContext::
mergeTimeIntervalVectors
(const TimeIntervalVector &a, const TimeIntervalVector &b)
{
    TimeIntervalVector results;
    TimeIntervalVector::const_iterator iA = a.begin();
    TimeIntervalVector::const_iterator iB = b.begin();
    // Traverse both in parallel, trying to find overlaps.  We are
    // always considering the earliest intervals that we haven't fully
    // checked yet, one each from A and B.
    while ((iA != a.end()) && (iB != b.end())) {
        // If the time intervals don't even overlap, step the earliest
        // one and try again.
        if (iA->second <= iB->first)
            { ++iA; continue; }
        if (iB->second <= iA->first)
            { ++iB; continue; }

        /* We've got an overlap. */

        // Find the bounds of the overlap.
        timeT startT = std::max(iA->first, iB->first);
        timeT endT   = std::min(iA->second, iB->second);

        // Store it.
        results.push_back(TimeInterval(startT,endT));
        
        // We considered up to the end of at least one and maybe both,
        // so step each if we are at its end.
        if (iA->second <= endT) { ++iA; }
        if (iB->second <= endT) { ++iB; }
    }
    return results;
}

// @return
// Return a nested expansion context.
// @remarks
// We make no attempt to handle controller values nicely, so
// relativizing controllers will only work for the first ply of ornaments.
// @author Tom Breton (Tehom)
TriggerExpansionContext
TriggerExpansionContext::
makeNestedContext(Segment::iterator     iTrigger,
                  const Segment        *containing) const
{
    // Can it happen that we have no composition?  ISTM no, both top
    // container and triggered segments are assumed elsewhere to have
    // a composition.
    Composition *comp = containing->getComposition();
    long triggerId = -1;
    (*iTrigger)->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID, triggerId);
    TriggerSegmentRec *rec =
        comp->getTriggerSegmentRec(triggerId);
    
    const TimeIntervalVector triggerIntervals =
        getSoundingIntervals(iTrigger, containing, m_timeScale);
    const TimeIntervalVector mergedIntervals =
        mergeTimeIntervalVectors(triggerIntervals, m_intervals);

    const int pitchDiff =
        m_pitchDiff + rec->getTranspose(*iTrigger);
    const int velocityDiff =
        m_velocityDiff + rec->getVelocityDiff(*iTrigger);
    const LinearTimeScale timeScale(rec, iTrigger, containing, m_timeScale);

    // For m_controllerContextParams, we punt on chaining contexts and
    // just pass the top-level params everywhere.
    return
        TriggerExpansionContext(mergedIntervals, m_maxDepth - 1, rec,
                                pitchDiff, velocityDiff,
                                m_controllerContextParams, timeScale);
}

// Expand the ornament into target.  The TriggerExpansionContext
// object gives the full context.
// @param target
// The segment that the events go into.
// @param queue
// A queue of TriggerExpansionContexts for this function to push
// nested expansions into.
// @return
// True if anything was inserted
// @author Tom Breton (Tehom)
bool
TriggerExpansionContext::
Expand(Segment *target, Queue& queue) const
{
    const Segment *source = m_rec->getSegment();
    const timeT baseTime = source->getStartTime();

    bool insertedSomething = false;

    /** Initial values **/
    TimeIntervalVector::const_iterator interval = m_intervals.begin();
    timeT startT = interval->first;
    timeT endT = interval->second;
    
    for (Segment::iterator i = source->begin();
         i != source->getEndMarker();
         ++i) {

        long triggerId = -1;
        (*i)->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID, triggerId);

        // Handle triggers whether we are inserting or not.  We will
        // clip them to masked time, so this won't result in ornaments
        // playing during masked times.
        if (triggerId >= 0) {
            // Ignore triggers beyond a certain depth, to avoid
            // infinite recursion.  maxDepth is decremented each
            // level, so comparing it to 0 is correct.
            if (m_maxDepth <= 0) { continue; }

            // Add a new one that the loop will explore.
            queue.push(makeNestedContext(i, source));
                
            // Go on to the next event.  Skipping this one doesn't
            // mess up any state.
            continue;
        }

        // Since we may move everything pitchwise, Clef may no longer
        // be reasonable.  makeExpansion will add its own clef and
        // InternalSegmentMapper doesn't want clefs anyways.
        if ((*i)->isa(Clef::EventType)) { continue; }

        // !!! Could try to transpose keys, but we probably won't need
        // !!! it.
        if ((*i)->isa(Key::EventType)) { continue; }

        // Find performance time
        timeT t =
            m_timeScale.toPerformance((*i)->getAbsoluteTime() - baseTime);

        // Update the time interval and the clip times.
        while (t >= endT) {
            ++interval;

            // We can end right now.  Everything past this event would
            // never have been inserted.  Even triggers would never
            // cause any insertions.
            if (interval == m_intervals.end()) 
                { return insertedSomething; }
            startT = interval->first;
            endT = interval->second;
        }

        /** Now the time interval either contains the start of the
           event or is the next time interval after it. **/

        // Find performance duration.
        timeT d =
            m_timeScale.toPerformanceDuration((*i)->getDuration());

        /** Clip to current time interval.  We may delay the start of
            notes or end them early, but we don't try to make long
            notes sound in multiple time intervals.  **/

        // Clip to the start.
        if (t < startT) {
            if (t + d <= startT)
                { continue; }
            else {
                d -= (startT - t);
                t = startT;
            }
        }

        // Clip to the end.
        if (t + d > endT) {
            if (t >= endT)
                { continue; }
            else {
                d = endT - t;
            }
        }


        // Make the event but don't insert it until we modify it.
        Event *newEvent = new Event(**i, t, d);

        if (m_retune && newEvent->has(BaseProperties::PITCH)) {
            int pitch =
                newEvent->get<Int>(BaseProperties::PITCH) + m_pitchDiff;
            if (pitch > 127)
                pitch = 127;
            if (pitch < 0)
                pitch = 0;
            newEvent->set<Int>(BaseProperties::PITCH, pitch);
        }

        if (newEvent->has(BaseProperties::VELOCITY)) {
            int velocity =
                newEvent->get<Int>(BaseProperties::VELOCITY) + m_velocityDiff;
            if (velocity > 127)
                velocity = 127;
            if (velocity < 0)
                velocity = 0;
            newEvent->set<Int>(BaseProperties::VELOCITY, velocity);
        }

        if (newEvent->isa(Controller::EventType) ||
            newEvent->isa(PitchBend::EventType)) {
            if (m_controllerContextParams) {
                m_controllerContextParams->makeControlValueAbsolute(newEvent);
            }
        }

        /** Finished all modifications to newEvent **/

        target->insert(newEvent);
        insertedSomething = true;
    }

    return insertedSomething;
}


}

