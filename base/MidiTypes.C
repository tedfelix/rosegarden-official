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

const PropertyName PitchBend::MSBPropertyName = "msb";
const PropertyName PitchBend::LSBPropertyName = "lsb";

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
    e->set<Int>(MSBPropertyName, (int)m_msb);
    e->set<Int>(LSBPropertyName, (int)m_lsb);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Controller
//////////////////////////////////////////////////////////////////////
const std::string Controller::EventType = "controller";
const int Controller::EventSubOrdering = -70;

const PropertyName Controller::Data1PropertyName = "data1";
const PropertyName Controller::Data2PropertyName = "data2";

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
    e->set<Int>(Data1PropertyName, (int)m_data1);
    e->set<Int>(Data2PropertyName, (int)m_data2);
    return e;
}

}

