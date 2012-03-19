/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ControllerContext.h"

#include "base/BaseProperties.h"
#include "base/Controllable.h"
#include "base/MidiTypes.h"
#include "base/Profiler.h"
#include "base/Segment.h"
#include "gui/rulers/ControllerEventAdapter.h"
#include "gui/seqmanager/InternalSegmentMapper.h"
#include "misc/Debug.h"
#include <assert.h>
#include <limits>

// #define DEBUG_CONTROLLER_CONTEXT 1

namespace Rosegarden
{

    /*** ControllerSearch ***/
ControllerSearch::
ControllerSearch(const std::string eventType,
                 int controllerId,
                 InternalSegmentMapper *mapper) :
    m_eventType(eventType),
    m_controllerId(controllerId),
    m_instrument(mapper->getInstrument())
{}

// Return the last value of controller before noLaterThan in segment s.
// @author Tom Breton (Tehom)
ControllerSearch::Maybe
ControllerSearch::
searchSegment(const Segment *s, timeT noEarlierThan, timeT noLaterThan) const
{
    Profiler profiler("ControllerSearch::searchSegment", false);
    if (!s)
        { return Maybe(false, ControllerSearchValue(0,0)); }

    // Get the latest relevant event before or at noEarlierThan.
    Segment::reverse_iterator latest(s->findTime(noLaterThan));

    // Search backwards for a match.
    for (Segment::reverse_iterator j = latest; j != s->rend(); ++j) {
        timeT t = (*j)->getAbsoluteTime();

        // Event is too early.  No controller we can find will satisfy
        // the search.
        if ((t <= noEarlierThan))
            { break; }

        // Treat a controller event
        if (matches(*j)) {
            long value = 0;
            ControllerEventAdapter(*j).getValue(value);
            return Maybe(true, ControllerSearchValue(value,t));
        }
    }

    return Maybe(false, ControllerSearchValue(0,0));
}

ControllerSearch::Maybe
ControllerSearch::
search(InternalSegmentMapper *mapper, timeT noLaterThan) const
{
    Profiler profiler("ControllerSearch::search", false);
    ControllerSearch::Maybe runningResult =
        searchSegment(mapper->m_segment, std::numeric_limits<int>::min(),
               noLaterThan);
    {
        timeT noEarlierThan = runningResult.first ?
            runningResult.second.m_when :
            std::numeric_limits<int>::min();
        ControllerSearch::Maybe result2 =
            searchSegment(mapper->m_triggeredEvents, noEarlierThan,
                          noLaterThan);
        if (result2.first)
            { runningResult = result2; }
    }

    return runningResult;
}

// Return true just if event e is what we're processing in the current search.
// @author Tom Breton (Tehom)
bool
ControllerSearch::
matches(Event *e) const
{
    return 
        e->isa(m_eventType) &&
        ((m_eventType != Controller::EventType) ||
         (e->has(Controller::NUMBER) &&
          e->get <Int>(Controller::NUMBER) == m_controllerId));
}

// Get the static value for the controller and mapper we are
// searching about.
// @author Tom Breton (Tehom)
int
ControllerContextMap::
getStaticValue(InternalSegmentMapper *mapper,
               const std::string eventType,
               int controllerId) 
{
    if (eventType == Controller::EventType)
        { return mapper->getInstrument()->getControllerValue(controllerId); }
    else
        // Neutral pitch-bend value
        { return 8192; }
}

// Get the value for controller controllerId at time t for mapper.
// @author Tom Breton (Tehom)
int
ControllerContextMap::
getControllerValue(InternalSegmentMapper *mapper,
                   timeT searchTime, const std::string eventType,
                   int controllerId)
{
    Profiler profiler("ControllerContextMap::getControllerValue", false);

    // Should only get these two types.
    assert((eventType == Controller::EventType) ||
           (eventType == PitchBend::EventType));
    
    // We have cached the latest value of all controllers we've
    // inserted.  Find the relevant cache.

    ControllerSearchValue * lastValue;
    bool valueExists;
    if (eventType == Controller::EventType) {
        Cache::iterator found = m_latestValues.find(controllerId);
        valueExists = (found != m_latestValues.end());
        lastValue = &(found->second);
    } else {
        valueExists = m_PitchBendLatestValue.first;
        lastValue   = &m_PitchBendLatestValue.second;
    }

    // If cache don'es have that controller, then no such controller
    // is in this segment, so use the static value.
    if (!valueExists) {
        return getStaticValue(mapper, eventType, controllerId);
    }

    // If we are servicing a repeating segment, the segment may repeat
    // many times but we only have to search it once.  Figure out what
    // time we really should look at; mutate searchTime accordingly.
    bool firstRepeat;
    if (mapper->getSegmentRepeatCount() > 0) {
        Segment *s = mapper->m_segment;
        timeT segmentStartTime = s->getStartTime();
        timeT segmentEndTime   = s->getEndMarkerTime();
        timeT segmentDuration = segmentEndTime - segmentStartTime;

        if (searchTime >= segmentEndTime) {
            timeT timeRelativeToStart = (searchTime - segmentStartTime);
            firstRepeat = false;
            searchTime =
                (timeRelativeToStart % segmentDuration) + segmentStartTime;
        } else {
            firstRepeat = true;
        }
    } else {
        firstRepeat = true;
    }

    // If search time is later than the last time we inserted a
    // controller, it's the last value inserted, which we know from
    // the cache.  This lets us essentially skip segments that have no
    // controller of that type and gives us relative controllers for
    // triggers immediately too.
    if (lastValue->time() < searchTime)
        { return lastValue->value(); }

    // Some non-static values exist for this controller but the last
    // value isn't it, so search.
    const ControllerSearch params(eventType, controllerId, mapper);
    Maybe foundInEvents = params.search(mapper, searchTime);

    // Found it, done.
    if (foundInEvents.first)
        { return foundInEvents.second.value(); } 

    // If this is a repeat, we've wrapped around, so the value is the
    // last value from a previous repeat, which is the same as cached
    // value.
    if (!firstRepeat)
        { return lastValue->value(); }

    // Search time is before all controller events, so the static
    // value is it.
    return getStaticValue(mapper, eventType, controllerId);
}

// Get the respective control parameter.
// @author Tom Breton (Tehom)
const ControlParameter *
ControllerContextMap::
getControlParameter(InternalSegmentMapper *mapper,
                    const std::string eventType,
                    const int controllerId)
{
    Device * device = mapper->getInstrument()->getDevice();
    const Controllable *c = device->getControllable();
    assert(c);
    return c->getControlParameter(eventType, controllerId);
}

// Clip a controller value to appropriate limits
// @author Tom Breton (Tehom)
int
ControllerContextMap::
makeAbsolute(const ControlParameter * controlParameter, int value) const
{
    int max = controlParameter->getMax();
    int min = controlParameter->getMin();
    value -= controlParameter->getDefault(); 

    if (value > max) { value = max; }
    if (value < min) { value = min; }
    return value;
}

// Transform e's controller value from relative to absolute.
// @param at the time at which to look.  This is typically not e's time.
// @author Tom Breton (Tehom)
void
ControllerContextMap::
makeControlValueAbsolute(InternalSegmentMapper *mapper, Event *e, timeT at)
{
    Profiler profiler("ControllerContextMap::makeControlValueAbsolute", false);
    const std::string eventType = e->getType();
    const int controllerId =
        e->has(Controller::NUMBER) ?
        e->get <Int>(Controller::NUMBER) : 0;
    const ControllerSearch params(eventType, controllerId, mapper);
    ControllerSearch::Maybe result = params.search(mapper, at);
    int baseline;
    if (result.first)
        { baseline = result.second.value(); }
    else
        { baseline = getStaticValue(mapper, eventType, controllerId); }
    ControllerEventAdapter adapter(e);
    long oldValue;
    adapter.getValue(oldValue);
    const ControlParameter * m_controlParameter =
        getControlParameter(mapper, eventType, controllerId);
    long newValue =
        makeAbsolute(m_controlParameter, oldValue + baseline);
    adapter.setValue(newValue);
#ifdef DEBUG_CONTROLLER_CONTEXT
    SEQMAN_DEBUG << "ControllerContextMap::makeControlValueAbsolute"
                 << "oldValue - " << oldValue
                 << "newValue - " << newValue
                 << "baseline - " << result.second.value()
                 << endl
        ;
#endif
}

// Store e as the latest controller event.  e must really be a
// controller event.
// @author Tom Breton (Tehom)
void
ControllerContextMap::
storeLatestValue(Event *e)
{
    Profiler profiler("ControllerContextMap::storeLatestValue", false);
    timeT at = e->getAbsoluteTime();
    const std::string eventType = e->getType();
    const int controllerId =
        e->has(Controller::NUMBER) ?
        e->get <Int>(Controller::NUMBER) : 0;
    long value;
    ControllerEventAdapter eAsController(e);
    eAsController.getValue(value);

    // Both branches store a search-value as if from a search.
    ControllerSearchValue toCache(value, at);
    if (eventType == Controller::EventType) {
        // Create or replace it.
        m_latestValues[controllerId] = toCache;
    } else {
        // We are only expecting these two types.
        assert(eventType == PitchBend::EventType);
        // Set it.
        m_PitchBendLatestValue = Maybe(true, toCache);
    } 
}

// Clear the cache.
// @author Tom Breton (Tehom)
void
ControllerContextMap::
clear(void)
{
    m_latestValues.clear();
    m_PitchBendLatestValue = Maybe(false,ControllerSearchValue());
}


} // End namespace Rosegarden

