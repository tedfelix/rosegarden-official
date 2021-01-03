/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ControlRulerEventInsertCommand.h"
#include "base/MidiTypes.h"

namespace Rosegarden
{

ControlRulerEventInsertCommand::ControlRulerEventInsertCommand(const std::string &type,
                                                               timeT insertTime,
                                                               long number, long initialValue,
                                                               Segment &segment,timeT duration)
    : BasicCommand(tr("Insert Controller Event"),
                   segment,
                   insertTime, 
                   (duration!=0)?(insertTime+duration):(insertTime + Rosegarden::Note(Rosegarden::Note::Quaver).getDuration())), // must have a duration other undo doesn't work
      m_type(type),
      m_number(number),
      m_initialValue(initialValue)
{
}

void ControlRulerEventInsertCommand::modifySegment()
{
    Event* controllerEvent = new Event(m_type, getStartTime());

    if (m_type == Rosegarden::Controller::EventType)
    {
        controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::VALUE, m_initialValue);
        controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::NUMBER, m_number);
    }
    else if (m_type == Rosegarden::PitchBend::EventType)
    {
        // Convert to PitchBend MSB/LSB
        int lsb = m_initialValue & 0x7f;
        int msb = (m_initialValue >> 7) & 0x7f;
        controllerEvent->set<Rosegarden::Int>(Rosegarden::PitchBend::MSB, msb);
        controllerEvent->set<Rosegarden::Int>(Rosegarden::PitchBend::LSB, lsb);
    }
    
    getSegment().insert(controllerEvent);
}

}
