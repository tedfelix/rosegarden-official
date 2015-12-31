/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PlaceControllersCommand.h"
#include "base/ControlParameter.h"
#include "base/MidiTypes.h"
#include "base/Segment.h"
#include "gui/rulers/ControllerEventAdapter.h"

#include <QtGlobal>

namespace Rosegarden
{

PlaceControllersCommand::PlaceControllersCommand
(EventSelection &selection,
 const Instrument *instrument,
 const ControlParameter *cp) :
  BasicSelectionCommand(globalName, selection, true),
    m_selection(&selection),
    m_eventType(cp->getType()),
    m_controllerId(cp->getControllerValue()),
    m_controllerValue(getDefaultValue(instrument, cp))
{ }
                                    
const QString &PlaceControllersCommand::globalName = "Place controllers";

int
PlaceControllersCommand::getDefaultValue(const Instrument *instrument,
                                         const ControlParameter *cp)
{
    Q_CHECK_PTR(cp);
    if (cp->getType() == Controller::EventType)
        // For controllers, use the static value if there is one.
        {
            try {
                return
                    instrument->getControllerValue(cp->getControllerValue()); }
            catch (...) { return cp->getDefault(); }
        }
    else
        // For pitchBend, we use zero.
        { return 8192; }
}

void
PlaceControllersCommand::
modifySegment(void)
{
    typedef EventSelection::eventcontainer container;
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

