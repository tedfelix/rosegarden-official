/*
    Rosegarden-4
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

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

namespace Rosegarden
{

AudioPluginInstance::AudioPluginInstance(unsigned int position):
    m_mappedId(-1),
    m_id(0),
    m_position(position),
    m_assigned(false),
    m_bypass(false)
{
}

AudioPluginInstance::AudioPluginInstance(unsigned long id,
                                         unsigned int position):
                m_mappedId(-1),
                m_id(id),
                m_position(position),
                m_assigned(true)
{
}

std::string 
AudioPluginInstance::toXmlString()
{

    std::stringstream plugin;

    if (m_assigned == false)
    {
#if (__GNUC__ < 3)
        plugin << std::ends;
#endif
        return plugin.str();
    }
    

    plugin << "            <plugin position=\""
           << m_position
           << "\" id=\""
           << m_id
           << "\" bypassed=\"";

    if (m_bypass)
        plugin << "true";
    else
        plugin << "false";

    plugin << "\">" << std::endl;

    for (unsigned int i = 0; i < m_ports.size(); i++)
    {
        plugin << "                <port id=\""
               << m_ports[i]->id
               << "\" value=\""
               << m_ports[i]->value
               << "\"/>" << std::endl;
    }

    plugin << "            </plugin>"

#if (__GNUC__ < 3)
                   << std::endl << std::ends;
#else
                   << std::endl;
#endif

    return plugin.str();
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

    for (; it != m_ports.end(); ++it)
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

    for (; it != m_ports.end(); ++it)
    {
        if ((*it)->id == id)
            return *it;
    }

    return 0;
}

void
AudioPluginInstance::clearPorts()
{
    PortInstanceIterator it = m_ports.begin();
    for (; it != m_ports.end(); ++it)
        delete (*it);
    m_ports.erase(m_ports.begin(), m_ports.end());

}

}

