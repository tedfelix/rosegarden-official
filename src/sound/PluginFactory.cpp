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

#define RG_MODULE_STRING "[PluginFactory]"

#include "PluginFactory.h"
#include "PluginIdentifier.h"
#include "misc/Strings.h"
#include "misc/Debug.h"

#include "LADSPAPluginFactory.h"
#include "DSSIPluginFactory.h"

#include <locale.h>

namespace Rosegarden
{

int PluginFactory::m_sampleRate = 48000;

static LADSPAPluginFactory *ladspaInstance = 0;
static LADSPAPluginFactory *dssiInstance = 0;

PluginFactory *
PluginFactory::instance(QString pluginType)
{
    if (pluginType == "ladspa") {
        if (!ladspaInstance) {
            RG_DEBUG << "PluginFactory::instance(" << pluginType << "): creating new LADSPAPluginFactory";
            ladspaInstance = new LADSPAPluginFactory();
            ladspaInstance->discoverPlugins();
        }
        return ladspaInstance;
    } else if (pluginType == "dssi") {
        if (!dssiInstance) {
            RG_DEBUG << "PluginFactory::instance(" << pluginType << "): creating new DSSIPluginFactory";
            dssiInstance = new DSSIPluginFactory();
            dssiInstance->discoverPlugins();
        }
        return dssiInstance;
    } else {
        return 0;
    }
}

PluginFactory *
PluginFactory::instanceFor(QString identifier)
{
    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);
    return instance(type);
}

void
PluginFactory::enumerateAllPlugins(MappedObjectPropertyList &list)
{
    PluginFactory *factory;

    // Plugins can change the locale, store it for reverting afterwards
    char *loc = setlocale(LC_ALL, 0);

    // Query DSSI plugins before LADSPA ones.
    // This is to provide for the interesting possibility of plugins
    // providing either DSSI or LADSPA versions of themselves,
    // returning both versions if the LADSPA identifiers are queried
    // first but only the DSSI version if the DSSI identifiers are
    // queried first.

    factory = instance("dssi");
    if (factory)
        factory->enumeratePlugins(list);

    factory = instance("ladspa");
    if (factory)
        factory->enumeratePlugins(list);

    setlocale(LC_ALL, loc);
}

PluginFactory::~PluginFactory()
{
}


}

