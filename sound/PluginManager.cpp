// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

#include "PluginManager.h"


#include <dlfcn.h>

#include <qdir.h>

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

Plugin::Plugin(PluginType type, PluginId id, const std::string &libraryName):
    m_type(type),
    m_id(id),
    m_libraryName(libraryName)
{
}

Plugin::~Plugin()
{
}

#ifdef HAVE_LADSPA
LADSPAPlugin::LADSPAPlugin(const LADSPA_Descriptor *descriptor,
                           PluginId id,
                           const std::string &libraryName):
    Plugin(LADSPA, id, libraryName),
    m_descriptor(descriptor)
{
    if (m_descriptor)
    {
        m_name = descriptor->Name;
    }
}

LADSPAPlugin::~LADSPAPlugin()
{
    if (m_descriptor) delete m_descriptor;
} 

#endif

PluginManager::PluginManager():
    m_path(""),
    m_runningId(0)
{
}

PluginManager::~PluginManager()
{
    clearPlugins();
}

#ifdef HAVE_LADSPA
void
PluginManager::getenvLADSPAPath()
{
    m_path = std::string(getenv("LADSPA_PATH"));

    // try a default value
    if (m_path == "")
        m_path = "/usr/lib/ladspa";

}

void
PluginManager::setLADSPAPath(const std::string &path)
{
    m_path = path;
}

void
PluginManager::addLADSPAPath(const std::string &path)
{
    m_path += path;
}
#endif 

void
PluginManager::clearPlugins()
{
    PluginIterator it;
    for (it = m_plugins.begin(); it != m_plugins.end(); it++)
        delete *it;

    m_plugins.erase(m_plugins.begin(), m_plugins.end());
}

void
PluginManager::discoverPlugins()
{
    QDir dir(QString(m_path.c_str()), "*.so");

    clearPlugins();
    m_runningId = 0;

    for (unsigned int i = 0; i < dir.count(); i++ )
        loadPlugin(m_path + std::string("/") + std::string(dir[i].data()));
}

void
PluginManager::loadPlugin(const std::string &path)
{
#ifdef HAVE_LADSPA
    LADSPA_Descriptor_Function descrFn = 0;
    void *pluginHandle = 0;

    pluginHandle = dlopen(path.c_str(), RTLD_LAZY);

    descrFn = (LADSPA_Descriptor_Function)dlsym(pluginHandle,
                                                "ladspa_descriptor");

    if (descrFn)
    {
        const LADSPA_Descriptor *data;

        int index = 0;

        do
        {
            data = descrFn(index);

            if (data)
            {
                m_plugins.push_back(
                        new LADSPAPlugin(data, m_runningId++, path));
                index++;
            }
        }
        while(data);
    }
    dlclose(pluginHandle);
#endif // HAVE_LADSPA

}

};



