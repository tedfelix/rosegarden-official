/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <sstream>
#include <cstdio>

#include "ControlParameter.h"
#include "base/MidiTypes.h"
#include "gui/rulers/ControllerEventAdapter.h"

namespace Rosegarden
{

ControlParameter::ControlParameter():
    m_name("<unnamed>"),
    m_type(Rosegarden::Controller::EventType),
    m_description("<none>"),
    m_min(0),
    m_max(127),
    m_default(0),
    m_controllerValue(0),
    m_colourIndex(0),
    m_ipbPosition(-1)  // doesn't appear on IPB by default
{
}


ControlParameter::ControlParameter(const std::string &name,
                                   const std::string &type,
                                   const std::string &description,
                                   int min,
                                   int max,
                                   int def,
                                   MidiByte controllerValue,
                                   unsigned int colour,
                                   int ipbPosition):
        m_name(name),
        m_type(type),
        m_description(description),
        m_min(min),
        m_max(max),
        m_default(def),
        m_controllerValue(controllerValue),
        m_colourIndex(colour),
        m_ipbPosition(ipbPosition)
{
}


ControlParameter::ControlParameter(const ControlParameter &control):
        XmlExportable(),
        m_name(control.getName()),
        m_type(control.getType()),
        m_description(control.getDescription()),
        m_min(control.getMin()),
        m_max(control.getMax()),
        m_default(control.getDefault()),
        m_controllerValue(control.getControllerValue()),
        m_colourIndex(control.getColourIndex()),
        m_ipbPosition(control.getIPBPosition())
{
}

ControlParameter& 
ControlParameter::operator=(const ControlParameter &control)
{
    m_name = control.getName();
    m_type = control.getType();
    m_description = control.getDescription();
    m_min = control.getMin();
    m_max = control.getMax();
    m_default = control.getDefault();
    m_controllerValue = control.getControllerValue();
    m_colourIndex = control.getColourIndex();
    m_ipbPosition = control.getIPBPosition();

    return *this;
}

bool ControlParameter::operator==(const ControlParameter &control)
{
    return m_type == control.getType() &&
        m_controllerValue == control.getControllerValue() &&
        m_min == control.getMin() &&
        m_max == control.getMax();
}

bool operator<(const ControlParameter &a, const ControlParameter &b)
{
    if (a.m_type != b.m_type)
        return a.m_type < b.m_type;
    else if (a.m_controllerValue != b.m_controllerValue)
        return a.m_controllerValue < b.m_controllerValue;
    else
	return false;
}


std::string
ControlParameter::toXmlString() const
{ 
    std::stringstream control;

    control << "            <control name=\"" << encode(m_name)
            << "\" type=\"" << encode(m_type)
            << "\" description=\"" << encode(m_description)
            << "\" min=\"" << m_min
            << "\" max=\"" << m_max
            << "\" default=\"" << m_default
            << "\" controllervalue=\"" << int(m_controllerValue)
            << "\" colourindex=\"" << m_colourIndex
            << "\" ipbposition=\"" << m_ipbPosition;

    control << "\"/>" << std::endl;

    return control.str();
}

// Return a new event setting our controller to VALUE at TIME
// @author Tom Breton (Tehom)
Event *
ControlParameter::
newEvent(timeT time, int value) const
{
    Event *event = new Event (getType(), time);
    ControllerEventAdapter(event).setValue(value);
    
    if (getType() == Controller::EventType) {
        event->set<Int>(Controller::NUMBER, m_controllerValue);
    }
    return event;
}

// Return whether "e" is this type of controller / pitchbend.
// @author Tom Breton (Tehom)
bool
ControlParameter::
matches(Event *e) const
{
    return 
        e->isa(m_type) &&
        ((m_type != Controller::EventType) ||
         (e->has(Controller::NUMBER) &&
          e->get <Int>(Controller::NUMBER) == m_controllerValue));    
}

    
// These exists to support calling PitchBendSequenceDialog because
// some calls to PitchBendSequenceDialog always pitchbend or
// expression rather than getting a ControlParameter from a ruler or
// device.  This can't be just a static member of ControlParameter, in
// order to prevent the "static initialization order fiasco".
const ControlParameter&
ControlParameter::
getPitchBend(void)
{
    static const ControlParameter
        pitchBend(
                  "PitchBend", Rosegarden::PitchBend::EventType, "<none>", 
                  0, 16383, 8192, MidiByte(1), 4, -1);
    return pitchBend;
}

const ControlParameter&
ControlParameter::
getExpression(void)
{
    static const ControlParameter
        expression(
                  "Expression", Rosegarden::Controller::EventType,
                  "<none>", 0, 127, 100, MidiByte(11), 2, -1);
    return expression;
}
    
}
