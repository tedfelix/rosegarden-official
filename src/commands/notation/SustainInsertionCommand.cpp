/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SustainInsertionCommand.h"

#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/Segment.h"
#include "document/BasicCommand.h"
#include "base/Selection.h"
#include <QString>


namespace Rosegarden
{

SustainInsertionCommand::SustainInsertionCommand(Segment &segment, timeT time,
        bool down,
        int controllerNumber) :
        BasicCommand(getGlobalName(down), segment, time, time),
        m_down(down),
        m_controllerNumber(controllerNumber),
        m_lastInsertedEvent(nullptr)
{
    // nothing
}

SustainInsertionCommand::~SustainInsertionCommand()
{
    // nothing
}

EventSelection *
SustainInsertionCommand::getSubsequentSelection()
{
    EventSelection *selection = new EventSelection(getSegment());
    selection->addEvent(getLastInsertedEvent());
    return selection;
}

void
SustainInsertionCommand::modifySegment()
{
    Event *e = new Event(Controller::EventType, getStartTime(), 0,
                         Controller::EventSubOrdering);
    e->set
    <Int>(Controller::NUMBER, m_controllerNumber);
    e->set
    <Int>(Controller::VALUE, m_down ? 127 : 0);
    m_lastInsertedEvent = *getSegment().insert(e);
}

}
