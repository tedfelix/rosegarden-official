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


#include <dlfcn.h>

#include <qdir.h>

#include "PluginManager.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

Plugin::Plugin(PluginType type):m_type(type)
{
}

Plugin::~Plugin()
{
}

LADSPAPlugin::LADSPAPlugin():Plugin(LADSPA)
{
}

LADSPAPlugin::~LADSPAPlugin()
{
} 

PluginManager::PluginManager()
{
}

PluginManager::~PluginManager()
{
}

void
PluginManager::getenvLADSPAPath()
{
    m_path = std::string(getenv("LADSPA_PATH"));
}

void
PluginManager::setLADSPAPath(const std::string &path)
{
}

void
PluginManager::addLADSPAPath(const std::string &path)
{
}

std::string
PluginManager::getLADSPAPath()
{
}

void
PluginManager::discoverPlugins()
{
    QDir dir(QString(m_path.c_str()), "*.so");

    for ( int i = 0; i < dir.count(); i++ )
        loadPlugin(m_path + std::string("/") + std::string(dir[i].data()));
}

void
PluginManager::loadPlugin(const std::string &path)
{
    cout << "LOADING \"" << path << "\"" << endl;

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
                cout << "NAME = " << std::string(data->Name) << endl;
                index++;
            }
        }
        while(data);

    }
    dlclose(pluginHandle);

}

};


