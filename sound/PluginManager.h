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

#ifndef _PLUGINMANAGER_H_
#define _PLUGINMANAGER_H_

#include "config.h"

#ifdef HAVE_LADSPA
#include <ladspa.h>
#endif

#include <string>
#include <vector>

namespace Rosegarden
{

// Load and manage audio plugins - for the moment we only deal
// with LADSPA types.
//

typedef unsigned int PluginId;

typedef enum
{
    LADSPA
} PluginType;

class Plugin
{
public:
    Plugin(PluginType type, PluginId id, const std::string &libraryName);
    ~Plugin();

    std::string getName() { return m_name; }
    PluginType getType() { return m_type; }

    PluginId getId() { return m_id; }
    void setId(PluginId id) { m_id = id; };

    std::string getLibraryName() { return m_libraryName; }
    void setLibraryName(const std::string &libraryName)
        { m_libraryName = libraryName; }

protected:

    PluginType   m_type;
    std::string  m_name;
    PluginId     m_id;
    std::string  m_libraryName;

};

#ifdef HAVE_LADSPA
class LADSPAPlugin : public Plugin
{
public:
    LADSPAPlugin(const LADSPA_Descriptor *descriptor,
                 PluginId id,
                 const std::string &libraryName);
    ~LADSPAPlugin();

protected:

    const LADSPA_Descriptor *m_descriptor;

};
#endif

typedef std::vector<Plugin*>::const_iterator PluginIterator;

class PluginManager
{
public:
    PluginManager();
    ~PluginManager();

#ifdef HAVE_LADSPA
    // Modify path
    void getenvLADSPAPath();
    std::string getLADSPAPath() { return m_path; }
    void setLADSPAPath(const std::string &path);
    void addLADSPAPath(const std::string &path);
#endif

    // Search path for all plugins
    void discoverPlugins();

    // Load a given plugin
    void loadPlugin(const std::string &path);

    // Return the plugin to load for a given id
    //
    Plugin* getPluginForId(PluginId id);

    // Clear list down
    //
    void clearPlugins();

    // Iterate over plugins
    //
    PluginIterator begin() { return m_plugins.begin(); }
    PluginIterator end() { return m_plugins.end(); }
    
protected:

    std::string m_path;

    std::vector<Plugin*> m_plugins;
    PluginId             m_runningId;

};


};

#endif // _PLUGINMANAGER_H_

