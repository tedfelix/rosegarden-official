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

#include <iostream>

#include "rgapplication.h"
#include "audiopluginmanager.h"
#include "rosegardendcop.h"

namespace Rosegarden
{

// ---------------- AudioPluginManager -------------------
//
//
AudioPluginManager::AudioPluginManager():m_sampleRate(0)
{
    // fetch from sequencer
    fetchSampleRate();
}

AudioPlugin*
AudioPluginManager::addPlugin(MappedObjectId id,
                              const QString &name,
                              unsigned long uniqueId,
                              const QString &label,
                              const QString &author,
                              const QString &copyright)
{
    AudioPlugin *newPlugin = new AudioPlugin(id, 
                                             name, 
                                             uniqueId,
                                             label,
                                             author,
                                             copyright);
    m_plugins.push_back(newPlugin);

    return newPlugin;
}

bool
AudioPluginManager::removePlugin(MappedObjectId id)
{
    std::vector<AudioPlugin*>::iterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it)
    {
        if ((*it)->getId() == id)
        {
            delete *it;
            m_plugins.erase(it);
            return true;
        }
    }

    return false;
}

std::vector<QString>
AudioPluginManager::getPluginNames()
{
    std::vector<QString> names;

    PluginIterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it)
        names.push_back((*it)->getName());

    return names;
}

AudioPlugin* 
AudioPluginManager::getPlugin(int number)
{
    if (number < 0 || number > (int(m_plugins.size()) - 1))
        return 0;

    return m_plugins[number];
}

int
AudioPluginManager::getPositionByUniqueId(unsigned long uniqueId)
{
    int pos = 0;
    PluginIterator it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it)
    {
        if ((*it)->getUniqueId() == uniqueId)
            return pos;

        pos++;
    }

    return -1;
}

AudioPlugin*
AudioPluginManager::getPluginByUniqueId(unsigned long uniqueId)
{
    PluginIterator it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it)
    {
        if ((*it)->getUniqueId() == uniqueId)
            return (*it);
    }

    return 0;
}


void
AudioPluginManager::fetchSampleRate()
{
    QCString replyType;
    QByteArray replyData;

    if (rgapp->sequencerCall("getSampleRate()", replyType, replyData)) {

        QDataStream streamIn(replyData, IO_ReadOnly);
        unsigned int result;
        streamIn >> result;
        m_sampleRate = result;
    }
}




// ----------------- AudioPlugin ---------------------
//
//
AudioPlugin::AudioPlugin(MappedObjectId id,
                         const QString &name,
                         unsigned long uniqueId,
                         const QString &label,
                         const QString &author,
                         const QString &copyright):
    m_id(id),
    m_name(name),
    m_uniqueId(uniqueId),
    m_label(label),
    m_author(author),
    m_copyright(copyright)
{
}

void
AudioPlugin::addPort(MappedObjectId id,
                     const QString &name,
                     PluginPort::PortType type,
                     PluginPort::PortRange range,
                     PortData lowerBound,
                     PortData upperBound,
		     PortData defaultValue)
{
    PluginPort *port = new PluginPort(id,
                                      name,
                                      type,
                                      range,
                                      lowerBound,
                                      upperBound,
				      defaultValue);
    m_ports.push_back(port);

}

// ------------------ PluginPort ---------------------
//

PluginPort::PluginPort(MappedObjectId id,
                       const QString &name,
                       PluginPort::PortType type,
                       PluginPort::PortRange range,
                       PortData lowerBound,
                       PortData upperBound,
		       PortData defaultValue):
    m_id(id),
    m_name(name),
    m_type(type),
    m_range(range),
    m_lowerBound(lowerBound),
    m_upperBound(upperBound),
    m_default(defaultValue)
{
}


}


