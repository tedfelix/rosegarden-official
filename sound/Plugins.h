// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

#ifndef _PLUGINS_H_
#define _PLUGINS_H_

#include "config.h"

#ifdef HAVE_LADSPA
#include <ladspa.h>
#endif

#include <string>
#include <vector>

#include <qstring.h>

namespace Rosegarden
{

// Load and manage audio plugins - for the moment we only deal
// with LADSPA types.
//

typedef unsigned int PluginId;
typedef unsigned int PluginPortId;

typedef enum
{
    LADSPA
} PluginType;

class Plugin
{
public:
    Plugin(PluginType type, const std::string &libraryName);
    virtual ~Plugin();

    std::string getLibraryName() { return m_libraryName; }
    void setLibraryName(const std::string &libraryName)
        { m_libraryName = libraryName; }

protected:

    PluginType   m_type;
    std::string  m_libraryName;

};

#ifdef HAVE_LADSPA
class LADSPAPlugin : public Plugin
{
public:
    LADSPAPlugin();
    LADSPAPlugin(const LADSPA_Descriptor *descriptor,
                 const std::string &libraryName);
    virtual ~LADSPAPlugin();

    const LADSPA_Descriptor* getDescriptor() { return m_descriptor; }
    void setDescriptor(const LADSPA_Descriptor *descriptor)
        { m_descriptor = descriptor; }

    // Plugin name
    //
    std::string getPluginName();
    void setPluginName(const std::string &name) { m_pluginName = name; }

protected:

    const LADSPA_Descriptor *m_descriptor;

    std::string m_pluginName; // store in the object as we don't always
                              // have the descriptor available

};
#endif // HAVE_LADSPA

};

#endif // _PLUGINS_H_

