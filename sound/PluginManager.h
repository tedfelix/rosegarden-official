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

#include <ladspa.h>
#include <string>
#include <vector>

#ifndef _PLUGINMANAGER_H_
#define _PLUGINMANAGER_H_

namespace Rosegarden
{

typedef enum
{
    LADSPA
} PluginType;

class Plugin
{
public:
    Plugin(PluginType type);
    ~Plugin();

    std::string getName() { return m_name; }
    PluginType getType() { return m_type; }

protected:

    PluginType  m_type;
    std::string m_name;
};

class LADSPAPlugin : public Plugin
{
public:
    LADSPAPlugin();
    ~LADSPAPlugin();

protected:

    LADSPA_Descriptor *m_descriptor;

};

typedef vector<Plugin*>::const_iterator PluginIterator;

class PluginManager
{
public:
    PluginManager();
    ~PluginManager();

    // Modify path
    void getenvLADSPAPath();
    void setLADSPAPath(const std::string &path);
    void addLADSPAPath(const std::string &path);
    std::string getLADSPAPath();

    // Search path for all plugins
    void discoverPlugins();

    // Load a given plugin
    void loadPlugin(const std::string &path);

    // Iterate over plugins
    //
    PluginIterator begin() { return m_plugins.begin(); }
    PluginIterator end() { return m_plugins.end(); }
    

protected:
    std::string m_path;

    vector<Plugin*> m_plugins;

};


};


#endif // _PLUGINMANAGER_H_
