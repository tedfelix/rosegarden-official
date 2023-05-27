/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LV2_PLUGIN_FACTORY_H
#define RG_LV2_PLUGIN_FACTORY_H

#include "PluginFactory.h"
#include "LV2PluginInstance.h"

#include <vector>
#include <map>
#include <set>
#include <QString>

namespace Rosegarden
{

class LV2PluginInstance;

class LV2PluginFactory : public PluginFactory
{
 public:
    ~LV2PluginFactory() override;

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

 protected:
    LV2PluginFactory();
    friend class PluginFactory;

 private:
    void releasePlugin(RunnablePluginInstance *, QString) override;
    virtual void generateTaxonomy();

    std::set<RunnablePluginInstance *> m_instances;

    std::vector<QString> m_identifiers;
    std::map<QString, QString> m_taxonomy;

};

}

#endif
