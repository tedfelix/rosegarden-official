// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef _LADSPA_PLUGIN_FACTORY_H_
#define _LADSPA_PLUGIN_FACTORY_H_

#include "config.h"

#ifdef HAVE_LADSPA

#include "PluginFactory.h"
#include <ladspa.h>

#include <vector>
#include <map>
#include <set>
#include <qstring.h>

namespace Rosegarden
{

class LADSPAPluginInstance;

class LADSPAPluginFactory : public PluginFactory
{
public:
    virtual ~LADSPAPluginFactory();

    virtual void discoverPlugins();

    virtual const std::vector<QString> &getPluginIdentifiers() const;

    virtual void enumeratePlugins(MappedObjectPropertyList &list);

    virtual void populatePluginSlot(QString identifier, MappedPluginSlot &slot);

    virtual RunnablePluginInstance *instantiatePlugin(QString identifier,
						      int instrumentId,
						      int position,
						      unsigned int sampleRate,
						      unsigned int blockSize,
						      unsigned int channels);

protected:
    LADSPAPluginFactory();
    friend class PluginFactory;

    virtual std::vector<QString> getPluginPath();
    virtual std::vector<QString> getLRDFPath(QString &baseUri);

    virtual void discoverPlugins(QString soName);
    virtual void generateTaxonomy(QString uri, QString base);

    virtual void releasePlugin(RunnablePluginInstance *, QString);

    MappedObjectValue getPortMinimum(const LADSPA_Descriptor *, int port);
    MappedObjectValue getPortMaximum(const LADSPA_Descriptor *, int port);
    MappedObjectValue getPortDefault(const LADSPA_Descriptor *, int port);
    int getPortDisplayHint(const LADSPA_Descriptor *, int port);

    virtual const LADSPA_Descriptor *getLADSPADescriptor(QString identifier);

    void loadLibrary(QString soName);
    void unloadLibrary(QString soName);
    void unloadUnusedLibraries();

    std::vector<QString> m_identifiers;

    std::map<unsigned long, QString> m_taxonomy;
    std::map<unsigned long, std::map<int, float> > m_portDefaults;

    std::set<RunnablePluginInstance *> m_instances;

    typedef std::map<QString, void *> LibraryHandleMap;
    LibraryHandleMap m_libraryHandles;
};

}

#endif

#endif

