/*
    Rosegarden-4 v0.2
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

#include "AudioPluginInstance.h"

namespace Rosegarden
{

AudioPluginInstance::AudioPluginInstance(unsigned int position):
    m_id(0),
    m_position(position),
    m_assigned(false)
{
}

AudioPluginInstance::AudioPluginInstance(unsigned long id,
                                         unsigned int position):
                m_id(id),
                m_position(position),
                m_assigned(true)
{
}

std::string 
AudioPluginInstance::toXmlString()
{
    return std::string();
}


void
AudioPluginInstance::addPort(unsigned int id, PortData value)
{
    m_ports.push_back(new PluginPortInstance(id, value));
}


bool
AudioPluginInstance::removePort(unsigned int id)
{
    PortInstanceIterator it = m_ports.begin();

    for (; it != m_ports.end(); it++)
    {
        if ((*it)->id == id)
        {
            delete (*it);
            m_ports.erase(it);
            return true;
        }
    }

    return false;
}


PluginPortInstance* 
AudioPluginInstance::getPort(unsigned int id)
{
    PortInstanceIterator it = m_ports.begin();

    for (; it != m_ports.end(); it++)
    {
        if ((*it)->id == id)
            return *it;
    }

    return 0;
}


}

