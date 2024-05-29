/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
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
#include "misc/Preferences.h"

#include "LADSPAPluginFactory.h"
#include "DSSIPluginFactory.h"
#ifdef HAVE_LILV
#include "LV2PluginFactory.h"
#endif

#include <locale.h>

namespace Rosegarden
{

int PluginFactory::m_sampleRate = 48000;

static LADSPAPluginFactory *ladspaInstance = nullptr;
static LADSPAPluginFactory *dssiInstance = nullptr;
#ifdef HAVE_LILV
static LV2PluginFactory *lv2Instance = nullptr;
#endif

PluginFactory *
PluginFactory::instance(QString pluginType)
{
    if (pluginType == "ladspa") {
        if (!ladspaInstance) {
            //RG_DEBUG << "instance(" << pluginType << "): creating new LADSPAPluginFactory";
            ladspaInstance = new LADSPAPluginFactory();
            ladspaInstance->discoverPlugins();
        }
        return ladspaInstance;
    } else if (pluginType == "dssi") {
        if (!dssiInstance) {
            //RG_DEBUG << "instance(" << pluginType << "): creating new DSSIPluginFactory";
            dssiInstance = new DSSIPluginFactory();
            dssiInstance->discoverPlugins();
        }
        return dssiInstance;
#ifdef HAVE_LILV
    } else if (pluginType == "lv2") {
        if (!lv2Instance) {
            //RG_DEBUG << "instance(" << pluginType << "): creating new LV2PluginFactory";
            lv2Instance = new LV2PluginFactory();
            lv2Instance->discoverPlugins();
        }
        return lv2Instance;
#endif
    } else {
        return nullptr;
    }
}

PluginFactory *
PluginFactory::instanceFor(QString identifier)
{
    QString type, soName, label, arch;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label, arch);
    return instance(arch);
}

void
PluginFactory::enumerateAllPlugins(std::vector<QString> &list)
{
    RG_INFO << "enumerateAllPlugins() begin...  Enumerating and loading all plugins...";

    PluginFactory *factory;

    // Plugins can change the locale, store it for reverting afterwards
    std::string loc = setlocale(LC_ALL, nullptr);

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

#ifdef HAVE_LILV
    if (Preferences::getLV2())
    {
        factory = instance("lv2");
        if (factory)
            factory->enumeratePlugins(list);
    }
#endif

    setlocale(LC_ALL, loc.c_str());

    RG_INFO << "enumerateAllPlugins() end.";
}

PluginFactory::~PluginFactory()
{
}

}
