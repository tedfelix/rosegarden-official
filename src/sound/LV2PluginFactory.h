/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
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
//class LV2Worker;


/// Creates LV2PluginInstance objects.
class LV2PluginFactory : public PluginFactory
{
public:
    ~LV2PluginFactory() override;

    void discoverPlugins() override;

    //const std::vector<QString> &getPluginIdentifiers() const override;

    void enumeratePlugins(std::vector<QString> &list) override;

    void populatePluginSlot(QString identifier, MappedPluginSlot &slot) override;

    RunnablePluginInstance *instantiatePlugin
        (QString identifier,
         int instrumentId,
         int position,
         unsigned int sampleRate,
         unsigned int blockSize,
         unsigned int channels,
         AudioInstrumentMixer* amixer) override;

protected:

    void releasePlugin(RunnablePluginInstance *, QString) override;

private:

    // Created only by PluginFactory.  See PluginFactory::instance().
    LV2PluginFactory();
    friend class PluginFactory;

    void generateTaxonomy();

    std::set<RunnablePluginInstance *> m_instances;

    //std::vector<QString> m_identifiers;
    // Plugin Class Map
    std::map<QString /*uri*/, QString /*class*/> m_taxonomy;

    //LV2Worker* m_worker;
};


}

#endif
