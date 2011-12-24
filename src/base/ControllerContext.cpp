/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

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
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenDocument.h"
#include "gui/rulers/ControllerEventAdapter.h"
#include <limits>
#include <stack>

namespace Rosegarden
{

// Dummy controller context.  Returned when no controller is found.
// @author Tom Breton (Tehom)
const ControllerContext
ControllerContext::dummyContext127 = ControllerContext(0,0,127);

// Given the relative value of a controller, return its absolute value
// @author Tom Breton (Tehom)
unsigned int
ControllerContext::
getAbsoluteValue(unsigned int relativeValue) const
{
    int value =
        relativeValue + m_diffValue;
    if (value > m_max) { value = m_max; }
    if (value < m_min) { value = m_min; }
    return value;
}

// Adjust a controller event.  Before the call, its value is relative to its
// context.  After the call, the value is absolute.
// Don't call this twice on the same event.
// @author Tom Breton (Tehom)
void
ControllerContext::
adjustControllerValue(Event *e) const
{
    long oldValue;
    ControllerEventAdapter eAsController(e);
    eAsController.getValue(oldValue);
    eAsController.setValue(getAbsoluteValue(oldValue));
}

// Look for a value in segment s and update this ControllerSearchValue
// accordingly.  It may be a relative value; caller has the
// responsibility to handle that.
// @author Tom Breton (Tehom)
ControllerSearch::Maybe
ControllerSearch::
search(Segment *s, timeT noEarlierThan, timeT noLaterThan,
       bool forceAbsolute) const
{
    // Stack will hold a possible cascade of relative events from
    // triggered segments, each getting its baseline from the next.
    std::stack<ControllerSearchValue>  stack;
    // A baseline, when we have one.
    ControllerSearchValue baseline;
    bool gotBaseline     = false;
    bool getControllers = true;

    // The latest a relevant event could be.
    Segment::iterator latest  = s->findNearestTime(noLaterThan);
    // The terminator at the beginning of the segment, one step before
    // the first event.
    Segment::iterator rEnd    = --s->begin();

    for (Segment::iterator j  = latest; j != rEnd; --j) {
        timeT t = (*j)->getAbsoluteTime();

        // If we have no relative controls that need to be baselined
        // and this event is before the time range we're looking at,
        // no controller we can find will satisfy the search
        // (triggered segments still may).
        if ((t <= noEarlierThan) && stack.size() == 0)
            { getControllers = false; }

        // Treat a controller event
        if (getControllers && matches(*j)) {
            getControllers  = false;
            gotBaseline     = true;
            // Presumptively assign it to the state.
            ControllerEventAdapter(*j).getValue(baseline.m_value);
            baseline.m_when = t;
        }

        // Keep looking for trigger segments even if we've found a
        // baseline controller event.  No matter how early one is
        // triggered, it might still be playing at the time we're
        // looking for controllers at.

        // We block nested explorations of triggered segments because
        // they might cause endless loops.
        else if (forceAbsolute) {
            long triggerId = -1;
            (*j)->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID, triggerId);
            if (triggerId >= 0) {
                TriggerSegmentRec *rec =
                    m_comp.getTriggerSegmentRec(triggerId);

                if (rec && rec->getSegment()) {
                    TriggerSegmentTimeAdjust timeAdjust(rec, j, s);

                    // If we have found a baseline already, controller
                    // events before it don't matter, so then we're
                    // only looking for controller events that linger
                    // into the current time.
                    timeT noEarlier =
                        gotBaseline ? baseline.m_when : -1;

                    Maybe result =
                        search(rec->getSegment(),
                               timeAdjust.toUnsquished(noEarlier),
                               timeAdjust.toUnsquished(noLaterThan),
                               false);
                    
                    if(result.first) {
                        // If we found something, it's relative so we
                        // still need to find its baseline.
                        ControllerSearchValue &found = result.second;

                        // Convert its time to our time.
                        found.m_when =
                            timeAdjust.toSquished(found.m_when);
                        
                        // The new relative value can serve in turn as
                        // baseline for relative values that come
                        // after it timewise.  But those that play
                        // earlier than it are obscured by it, so
                        // remove them.  Since the stack is in time
                        // order, any such elements will be at the
                        // top.
                        while (!stack.empty() &&
                               // We break ties this way just because
                               // we prefer the stack smaller.
                               (stack.top().m_when <= found.m_when))
                            { (void)stack.pop(); }

                        // Record this relative value.
                        stack.push(found);
                        // We will look for a baseline for it.
                        gotBaseline = false;
                        getControllers = true;
                    }
                }
            }
        }
    }

    // If we still don't have a baseline, get something suitable.
    if (!gotBaseline) {
        if (forceAbsolute) {
            baseline.m_value = getStaticValue();
        } else {
            // Relative values just use 0 as baseline.
            baseline.m_value = 0;
        }
        baseline.m_when = std::numeric_limits<int>::min();
    }

    // Add the relative values in the stack to what we found.  The
    // earliest one has the relevant time, which we find by
    // successively overwriting the time field.
    while (!stack.empty()) {
        ControllerSearchValue &top = stack.top();
        baseline.m_value += top.m_value;
        baseline.m_when  = top.m_when;
        stack.pop();
    }

    // The value we return is relative to the default.
    int defaultValue = m_controlParameter->getDefault();
    baseline.m_value -= defaultValue;

    // True if we found a relevant controller of any kind
    bool found = gotBaseline || forceAbsolute || !stack.empty();
    return Maybe(found, baseline);
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

// Get the static value for the controller and instrument we are
// searching about.
// @author Tom Breton (Tehom)
int
ControllerSearch::getStaticValue(void) const
{
    if (m_eventType == Controller::EventType) {
        return m_instrument->getControllerValue(m_controllerId);
    }
    else { return 0; }
}

// Get a controller context for controllerId at time t for the
// instrument playing segment s.
// Always returns a usable ControllerContext even if no controller is found.
// @author Tom Breton (Tehom)
const ControllerContext
ControllerContext::
getControllerContext(RosegardenDocument *doc, Segment *s, timeT noLaterThan,
                     const std::string eventType, int controllerId)
{
    Composition &comp = doc->getComposition();
    Composition::segmentcontainer segments =
        comp.getInstrumentSegments(s, noLaterThan);

    Instrument *instrument = doc->getInstrument(s);
    if (!instrument) { return ControllerContext::dummyContext127; }

    Device * device = instrument->getDevice();
            
    const Controllable *c = device->getControllable();
    if (!c) { return ControllerContext::dummyContext127; }

    const ControlParameter *controlParameter =
        c->getControlParameter(eventType, controllerId);
    if (!controlParameter) { return ControllerContext::dummyContext127; }
    
    const ControllerSearch params(eventType, controllerId, comp,
                                        instrument, controlParameter);
    ControllerSearchValue state;

    // Since we take all segments playing segment s's instrument
    // including s, there must be at least one segment.
    for (Composition::segmentcontainer::iterator i = segments.begin();
        i != segments.end();
        ++i) {
        ControllerSearch::Maybe result =
            params.search(*i, comp.getStartMarker() - 1,
                          noLaterThan, true);
        // Since we are forcing absolute values, this is guaranteed to
        // be true for the first segment.
        if (result.first)
            { state = result.second; }
    }

    return ControllerContext(state.value(),
                             controlParameter->getMin(),
                             controlParameter->getMax());
}


// Return a pointer to a controller context for controllerId at time t
// for the instrument playing segment s.  Uses a cache.
// @author Tom Breton (Tehom)
const ControllerContext * 
ControllerContextMap::
findControllerContext(RosegardenDocument *doc, Segment *s, timeT t,
                      const std::string eventType, int controllerId)
{
    if (eventType == Controller::EventType) {
        ControllerContextMap::iterator c = find(controllerId);
        if (c == end()) {
            insert(value_type(controllerId,
                              ControllerContext::getControllerContext
                                  (doc, s, t, eventType, controllerId)));
            // Find it again.  Now we can't fail.
            c = find(controllerId);
        }
        return &c->second;
    } else if (eventType == PitchBend::EventType) {
        // Since there's only one type of pitchbend controller, we
        // just store one ControllerContext for that and a flag
        // whether it's been initialized.
        if (!m_havePitchBendContext)
            {
                m_pitchBendContext =
                    ControllerContext::getControllerContext
                        (doc, s, t, eventType, controllerId);
                m_havePitchBendContext = true;
            }
        return &m_pitchBendContext;
    }
    // We should never get here, but let's be safe if we do.
    else
        { return &ControllerContext::dummyContext127; }
}
  
} // End namespace Rosegarden

