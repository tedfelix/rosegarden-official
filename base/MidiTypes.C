// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MidiTypes.h"

namespace Rosegarden
{

//////////////////////////////////////////////////////////////////////
// PitchBend
//////////////////////////////////////////////////////////////////////
const std::string PitchBend::EventType = "pitchbend";
const int PitchBend::EventSubOrdering = -70;

const PropertyName PitchBend::MSB = "msb";
const PropertyName PitchBend::LSB = "lsb";

PitchBend::PitchBend(Rosegarden::MidiByte msb,
                     Rosegarden::MidiByte lsb):
                     m_msb(msb),
                     m_lsb(lsb)
{
}

PitchBend::~PitchBend()
{
}

Event*
PitchBend::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(MSB, (int)m_msb);
    e->set<Int>(LSB, (int)m_lsb);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Controller
//////////////////////////////////////////////////////////////////////
const std::string Controller::EventType = "controller";
const int Controller::EventSubOrdering = -70;

const PropertyName Controller::DATA1 = "data1";
const PropertyName Controller::DATA2 = "data2";

const std::string Controller::UnspecifiedType = "unspecified";
const std::string Controller::Modulation = "modulation";
const std::string Controller::Pan = "pan";


Controller::Controller(const std::string &type,
                       Rosegarden::MidiByte data1,
                       Rosegarden::MidiByte data2):
  m_type(type),
  m_data1(data1),
  m_data2(data2)
{
}

Controller::~Controller()
{
}

Event*
Controller::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(DATA1, (int)m_data1);
    e->set<Int>(DATA2, (int)m_data2);
    return e;
}

//////////////////////////////////////////////////////////////////////
// Key Pressure
//////////////////////////////////////////////////////////////////////
const std::string KeyPressure::EventType = "keypressue";
const int KeyPressure::EventSubOrdering = -70;

const PropertyName KeyPressure::PITCH = "pitch";
const PropertyName KeyPressure::PRESSURE = "pressure";

KeyPressure::KeyPressure(Rosegarden::MidiByte pitch,
                         Rosegarden::MidiByte pressure):
  m_pitch(pitch),
  m_pressure(pressure)
{
}

KeyPressure::~KeyPressure()
{
}

Event*
KeyPressure::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PITCH, (int)m_pitch);
    e->set<Int>(PRESSURE, (int)m_pressure);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Channel Pressure
//////////////////////////////////////////////////////////////////////
const std::string ChannelPressure::EventType = "channelpressue";
const int ChannelPressure::EventSubOrdering = -70;

const PropertyName ChannelPressure::PRESSURE = "pressure";

ChannelPressure::ChannelPressure(Rosegarden::MidiByte pressure):
  m_pressure(pressure)
{
}

ChannelPressure::~ChannelPressure()
{
}

Event*
ChannelPressure::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PRESSURE, (int)m_pressure);
    return e;
}

//////////////////////////////////////////////////////////////////////
// ProgramChange
//////////////////////////////////////////////////////////////////////
const std::string ProgramChange::EventType = "programchange";
const int ProgramChange::EventSubOrdering = -70;

const PropertyName ProgramChange::PROGRAM = "program";

ProgramChange::ProgramChange(Rosegarden::MidiByte program):
  m_program(program)
{
}

ProgramChange::~ProgramChange()
{
}

Event*
ProgramChange::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PROGRAM, (int)m_program);
    return e;
}


//////////////////////////////////////////////////////////////////////
// SystemExclusive
//////////////////////////////////////////////////////////////////////
const std::string SystemExclusive::EventType = "systemexclusive";
const int SystemExclusive::EventSubOrdering = -70;

SystemExclusive::SystemExclusive()
{
}

SystemExclusive::~SystemExclusive()
{
}

Event*
SystemExclusive::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    return e;
}


}

