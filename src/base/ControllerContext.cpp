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

#include "base/Controllable.h"
#include "base/MidiTypes.h"
#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "gui/rulers/ControllerEventAdapter.h"

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

// Get a controller context for controllerId at time t for the
// instrument playing segment s.
// Always returns a usable ControllerContext even if no controller is found.
// @author Tom Breton (Tehom)
const ControllerContext
ControllerContext::
getControllerContext(RosegardenDocument *doc, Segment *s, timeT t,
                     const std::string eventType, int controllerId)
{
    Composition &comp = doc->getComposition();
    Composition::segmentcontainer segments =
        comp.getInstrumentSegments(s, t);

    Instrument *instrument = doc->getInstrument(s);
    if (!instrument) { return ControllerContext::dummyContext127; }

    Device * device = instrument->getDevice();
            
    const Controllable *c = device->getControllable();
    if (!c) { return ControllerContext::dummyContext127; }

    const ControlParameter *controlParameter =
        c->getControlParameter(eventType, controllerId);
    if (!controlParameter) { return ControllerContext::dummyContext127; }
    
    bool checkControllerId = (eventType == Controller::EventType);

    int defaultValue = controlParameter->getDefault();

    long prevValue = 0;
    const timeT beforeAll = comp.getStartMarker();
    timeT foundAt = beforeAll;
    bool foundP = false;

    // Consider each segment, but no earlier than foundAt
    for (Composition::segmentcontainer::iterator i = segments.begin();
        i != segments.end();
        ++i) {

        // If one has already been found, we need search no earlier
        // than the timeT it was found at.  Otherwise search to the
        // beginning.
        Segment::iterator rEnd =
            foundP ?
            (*i)->findTime(foundAt) :
            --((*i)->begin());
        for (Segment::iterator j  = (*i)->findNearestTime(t);
             j != rEnd;
             --j) {
            if ((*j)->isa(eventType) &&
                (!checkControllerId ||
                 ((*j)->has(Controller::NUMBER) &&
                  (*j)->get <Int>(Controller::NUMBER) == controllerId))) {
                
                foundP = true;
                foundAt = (*j)->getAbsoluteTime();
                ControllerEventAdapter(*j).getValue(prevValue);

                // End the inner loop.  The outer loop is still going.
                break;
            }
        }
    }
    if (!foundP) {
        prevValue = (eventType == Controller::EventType) ?
            instrument->getControllerValue(controllerId) : 0;
    }

    return ControllerContext(prevValue - defaultValue,
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

