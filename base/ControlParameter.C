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

#include "ControlParameter.h"

namespace Rosegarden
{

ControlParameter::ControlParameter(const std::string &name,
                                   const std::string &type,
                                   const std::string &description,
                                   int min,
                                   int max,
                                   int def,
                                   MidiByte controllerValue):
        m_name(name),
        m_type(type),
        m_description(description),
        m_min(min),
        m_max(max),
        m_default(def),
        m_controllerValue(controllerValue)
{
}

std::string
ControlParameter::toXmlString()
{ 
    return std::string("");
}

}
