// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

#include "ControlParameter.h"
#include "MidiTypes.h"

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
        m_ipbPosition(-1)  // doesn't appear on IPB by default
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


std::string
ControlParameter::toXmlString()
{ 
    std::stringstream control;

    control << "    <control name=\"" << encode(m_name)
            << "\" type=\"" << encode(m_type)
            << "\" description=\"" << encode(m_description)
            << "\" min=\"" << m_min
            << "\" max=\"" << m_max
            << "\" default=\"" << m_default
            << "\" controllervalue=\"" << int(m_controllerValue)
            << "\" colourindex=\"" << m_colourIndex
            << "\" ipbposition=\"" << m_ipbPosition;

#if (__GNUC__ < 3)
    control << "\"/>" << endl << std::ends;
#else
    control << "\"/>" << std::endl;
#endif

    return control.str();
}

}
