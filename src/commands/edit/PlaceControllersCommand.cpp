/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PlaceControllersCommand.h"

#include "gui/rulers/ControllerEventAdapter.h"
#include "base/ControlParameter.h"
#include "base/MidiTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"


namespace Rosegarden
{


namespace
{
    int getDefaultValue(const Instrument *instrument,
                        const ControlParameter *cp)
    {
        if (!instrument)
            return 0;
        if (!cp)
            return 0;

        // For controllers, use the static value if there is one.
        if (cp->getType() == Controller::EventType) {
            try {
                return instrument->getControllerValue(cp->getControllerNumber());
            } catch (...) {
                return cp->getDefault();
            }
        } else {  // Pitchbend (probably)
            return 8192;
        }
    }
}


PlaceControllersCommand::PlaceControllersCommand(
        EventSelection &selection,
        const Instrument *instrument,
        const ControlParameter *cp) :
    BasicCommand(tr("Place Controllers"),
                 selection,
                 true),  // bruteForceRedo
    m_selection(&selection),
    m_eventType(cp->getType()),
    m_controllerId(cp->getControllerNumber()),
    m_controllerValue(getDefaultValue(instrument, cp))
{
}

void
PlaceControllersCommand::modifySegment()
{
    typedef EventContainer container;
    typedef container::iterator iterator;

    timeT prevEventTime = -1;
    const container &events = m_selection->getSegmentEvents();
    Segment *s = &m_selection->getSegment();
  
    // For each note in the selection
    for (iterator i = events.begin(); i != events.end(); ++i) {

        // Only do this for notes and rests.  Rests are included
        // because it's sometimes convenient to put a controller just
        // at the end of a phrase.
        if (!(*i)->isa(Note::EventType) &&
            (*i)->isa(Note::EventRestType)) { continue; }

        timeT t = (*i)->getAbsoluteTime();

        // If it's at the same time as a note we've already done this
        // for, skip it.
        if (t == prevEventTime) { continue; }
        prevEventTime = t;
      
        Event *e = new Event(m_eventType,t);
        ControllerEventAdapter(e).setValue(long(m_controllerValue));
        e->set<Int>(Controller::NUMBER, m_controllerId);
        s->insert(e);
    }
}
  
}

