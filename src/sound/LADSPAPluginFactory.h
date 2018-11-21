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

#ifndef RG_LADSPA_PLUGIN_FACTORY_H
#define RG_LADSPA_PLUGIN_FACTORY_H

#include "PluginFactory.h"
#include <ladspa.h>

#include <vector>
#include <map>
#include <set>
#include <QString>

namespace Rosegarden
{

class LADSPAPluginInstance;

class LADSPAPluginFactory : public PluginFactory
{
public:
    ~LADSPAPluginFactory() override;

    void discoverPlugins() override;

    const std::vector<QString> &getPluginIdentifiers() const override;

    void enumeratePlugins(MappedObjectPropertyList &list) override;

    void populatePluginSlot(QString identifier, MappedPluginSlot &slot) override;

    RunnablePluginInstance *instantiatePlugin(QString identifier,
                                                      int instrumentId,
                                                      int position,
                                                      unsigned int sampleRate,
                                                      unsigned int blockSize,
                                                      unsigned int channels) override;

    MappedObjectValue getPortMinimum(const LADSPA_Descriptor *, int port);
    MappedObjectValue getPortMaximum(const LADSPA_Descriptor *, int port);
    MappedObjectValue getPortDefault(const LADSPA_Descriptor *, int port);
    int getPortDisplayHint(const LADSPA_Descriptor *, int port);

protected:
    LADSPAPluginFactory();
    friend class PluginFactory;

    virtual std::vector<QString> getPluginPath();

    virtual std::vector<QString> getLRDFPath(QString &baseUri);

    virtual void discoverPlugin(const QString &soName);
    virtual void generateTaxonomy(QString uri, QString base);
    virtual void generateFallbackCategories();

    void releasePlugin(RunnablePluginInstance *, QString) override;

    virtual const LADSPA_Descriptor *getLADSPADescriptor(QString identifier);

    void loadLibrary(QString soName);
    void unloadLibrary(QString soName);
    void unloadUnusedLibraries();

    std::vector<QString> m_identifiers;

    std::map<unsigned long, QString> m_taxonomy;
    std::map<QString, QString> m_fallbackCategories;
    std::map<unsigned long, std::map<int, float> > m_portDefaults;

    std::set<RunnablePluginInstance *> m_instances;

    typedef std::map<QString, void *> LibraryHandleMap;
    LibraryHandleMap m_libraryHandles;
};

}

#endif

