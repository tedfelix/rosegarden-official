// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

#include "Plugins.h"


#include <dlfcn.h>

#include <qdir.h>

// using std::cout;
// using std::cerr;
// using std::endl;

namespace Rosegarden
{

Plugin::Plugin(PluginType type, const std::string &libraryName):
    m_type(type),
    m_libraryName(libraryName)
{
}

Plugin::~Plugin()
{
}

#ifdef HAVE_LADSPA

LADSPAPlugin::LADSPAPlugin():Plugin(LADSPA, ""), m_descriptor(0)
{
}

LADSPAPlugin::LADSPAPlugin(const LADSPA_Descriptor *descriptor,
                           const std::string &libraryName):
    Plugin(LADSPA, libraryName),
    m_descriptor(descriptor)
{
}

LADSPAPlugin::~LADSPAPlugin()
{
} 

std::string
LADSPAPlugin::getPluginName()
{
    std::string name;
   
    if (m_descriptor)
        name = std::string(m_descriptor->Name);
    else
        return m_pluginName;

    return name;
}


#endif

};



