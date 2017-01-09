/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ScriptAPI.h"

#include "Composition.h"
#include "base/Segment.h"
#include "Event.h"
#include "Sets.h"

#include <map>

namespace Rosegarden
{

class ScriptRep
{
public:

    //!!! Needs to be a SegmentObserver _and_ CompositionObserver.
    // If an event is removed from a segment, we have to drop it too.
    // If a segment is removed from a composition likewise

    Event *getEvent(ScriptInterface::EventId id);
    

protected:
    Composition *m_composition;
    CompositionTimeSliceAdapter *m_adapter;
    GlobalChord *m_chord;
    std::map<ScriptInterface::EventId, Event *> m_events;
};

Event *
ScriptRep::getEvent(ScriptInterface::EventId id)
{
    return m_events[id];
}
    
class ScriptInterface::ScriptContainer :
    public std::map<ScriptId, ScriptRep *> { };

ScriptInterface::ScriptInterface(Rosegarden::Composition *composition) :
    m_composition(composition),
    m_scripts(new ScriptContainer())
{
}

ScriptInterface::~ScriptInterface()
{
}

std::string
ScriptInterface::getEventType(ScriptId id, EventId eventId)
{
    ScriptRep *rep = (*m_scripts)[id];
    if (!rep) return "";

    Event *event = rep->getEvent(eventId);
    if (!event) return "";

    return event->getType();
}


}

