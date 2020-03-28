/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PluginContainer]"

#include "PluginContainer.h"

#include "base/AudioPluginInstance.h"

namespace Rosegarden
{


PluginContainer::PluginContainer(bool havePlugins)
{
    if (havePlugins) {
        // Add a number of plugin place holders (unassigned)
        for (unsigned int i = 0; i < PLUGIN_COUNT; i++)
            addPlugin(new AudioPluginInstance(i));
    }
}

PluginContainer::~PluginContainer()
{
    clearPlugins();
}

void
PluginContainer::addPlugin(AudioPluginInstance *instance)
{
    m_audioPlugins.push_back(instance);
}

bool
PluginContainer::removePlugin(unsigned int position)
{
    AudioPluginVector::iterator it = m_audioPlugins.begin();

    for (; it != m_audioPlugins.end(); ++it)
    {
        if ((*it)->getPosition() == position)
        {
            delete (*it);
            m_audioPlugins.erase(it);
            return true;
        }

    }

    return false;
}

void
PluginContainer::clearPlugins()
{
    AudioPluginVector::iterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); ++it)
        delete (*it);

    m_audioPlugins.erase(m_audioPlugins.begin(), m_audioPlugins.end());
}

void 
PluginContainer::emptyPlugins()
{
    AudioPluginVector::iterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); ++it)
    {
        (*it)->setAssigned(false);
        (*it)->setBypass(false);
        (*it)->clearPorts();
    }
}


// Get an instance for an index
//
AudioPluginInstance *
PluginContainer::getPlugin(unsigned int position) const
{
    // For each plugin
    for (AudioPluginVector::const_iterator it = m_audioPlugins.begin();
         it != m_audioPlugins.end();
         ++it) {
        // Found?  Return it.
        if ((*it)->getPosition() == position)
            return *it;
    }

    return nullptr;
}


}
