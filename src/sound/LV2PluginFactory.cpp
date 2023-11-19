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

#define RG_MODULE_STRING "[LV2PluginFactory]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2PluginFactory.h"

#include <cstdlib>
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <dlfcn.h>
#include <QDir>
#include <cmath>

#include "sound/LV2PluginInstance.h"
#include "sound/LV2Utils.h"
#include "MappedStudio.h"
#include "PluginIdentifier.h"

namespace Rosegarden
{

LV2PluginFactory::LV2PluginFactory()
{
    RG_DEBUG << "constructor";
}

LV2PluginFactory::~LV2PluginFactory()
{
    RG_DEBUG << "destructor";
    for (std::set
                <RunnablePluginInstance *>::iterator i = m_instances.begin();
                i != m_instances.end(); ++i) {
            (*i)->setFactory(nullptr);
            delete *i;
        }
    m_instances.clear();
}

const std::vector<QString> &
LV2PluginFactory::getPluginIdentifiers() const
{
    return m_identifiers;
}

void
LV2PluginFactory::enumeratePlugins(MappedObjectPropertyList &list)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    auto allPluginData = lv2utils->getAllPluginData();
    for(auto pair : allPluginData) {
        const QString& uri = pair.first;
        list.push_back(uri);
        const LV2Utils::LV2PluginData& pluginData = pair.second;
        list.push_back(pluginData.name);
        // uri is the id
        list.push_back(uri);
        list.push_back(pluginData.label);
        list.push_back(pluginData.author);
        // no copywrite in lv2
        list.push_back("");

        if (pluginData.isInstrument) {
            list.push_back("true"); // is synth
        } else {
            list.push_back("false"); // is synth
        }
        list.push_back("false"); // is grouped

        if (m_taxonomy.find(uri) != m_taxonomy.end() &&
                m_taxonomy[uri] != "") {
            list.push_back(m_taxonomy[uri]);

        } else {
            list.push_back("");
        }

        unsigned int nports = pluginData.ports.size();
        list.push_back(QString("%1").arg(nports));

        for (unsigned long p = 0; p < nports; ++p) {
            const LV2Utils::LV2PortData portData = pluginData.ports[p];
            int type = 0;

            if (portData.portType == LV2Utils::LV2CONTROL ||
                portData.portType == LV2Utils::LV2MIDI) {
                type |= PluginPort::Control;
            } else {
                type |= PluginPort::Audio;
            }
            if (portData.isInput) {
                type |= PluginPort::Input;
            } else {
                type |= PluginPort::Output;
            }

            list.push_back(QString("%1").arg(p));
            list.push_back(portData.name);
            list.push_back(QString("%1").arg(type));
            list.push_back(QString("%1").arg(portData.displayHint));

            list.push_back(QString("%1").arg(portData.min));
            list.push_back(QString("%1").arg(portData.max));
            list.push_back(QString("%1").arg(portData.def));
        }
    }
}


void
LV2PluginFactory::populatePluginSlot(QString identifier, MappedPluginSlot &slot)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    LV2Utils::LV2PluginData pluginData = lv2utils->getPluginData(identifier);

    slot.setStringProperty(MappedPluginSlot::Label, pluginData.label);
    slot.setStringProperty(MappedPluginSlot::PluginName, pluginData.name);
    slot.setStringProperty(MappedPluginSlot::Author, pluginData.author);
    slot.setStringProperty(MappedPluginSlot::Copyright, "");
    slot.setProperty(MappedPluginSlot::PortCount, pluginData.ports.size());

    if (m_taxonomy.find(identifier) != m_taxonomy.end() &&
                m_taxonomy[identifier] != "") {
        slot.setStringProperty(MappedPluginSlot::Category,
                               m_taxonomy[identifier]);

    } else {
        slot.setStringProperty(MappedPluginSlot::Category, "");
    }

    slot.destroyChildren();

    for (unsigned long i = 0; i < pluginData.ports.size(); i++) {
        const LV2Utils::LV2PortData& portData = pluginData.ports[i];
        if ((portData.portType == LV2Utils::LV2CONTROL ||
             portData.portType == LV2Utils::LV2MIDI) &&
            portData.isInput) {
            MappedStudio *studio =
                dynamic_cast<MappedStudio *>(slot.getParent());
            if (!studio) {
                RG_WARNING << "WARNING: LV2PluginFactory::populatePluginSlot: can't find studio";
                return ;
            }

            MappedPluginPort *port =
                dynamic_cast<MappedPluginPort *>
                (studio->createObject(MappedObject::PluginPort));

            slot.addChild(port);
            port->setParent(&slot);

            port->setProperty(MappedPluginPort::PortNumber, i);
            port->setStringProperty(MappedPluginPort::Name, portData.name);
            port->setProperty(MappedPluginPort::Maximum, portData.max);
            port->setProperty(MappedPluginPort::Minimum, portData.min);
            port->setProperty(MappedPluginPort::Default, portData.def);
            port->setProperty(MappedPluginPort::DisplayHint, portData.displayHint);
        }
    }
}

RunnablePluginInstance *
LV2PluginFactory::instantiatePlugin(QString identifier,
                                    int instrumentId,
                                    int position,
                                    unsigned int sampleRate,
                                    unsigned int blockSize,
                                    unsigned int channels)
{
    RG_DEBUG << "instantiate plugin" << identifier;

    const QString& uri = identifier;
    LV2PluginInstance *instance =
        new LV2PluginInstance
        (this, instrumentId, identifier,
         position, sampleRate, blockSize, channels, uri);

    m_instances.insert(instance);

    return instance;
}

void
LV2PluginFactory::releasePlugin(RunnablePluginInstance *instance,
                                QString)
{
    if (m_instances.find(instance) == m_instances.end()) {
        RG_WARNING << "WARNING: LV2luginFactory::releasePlugin: Not one of mine!";
        return ;
    }

    m_instances.erase(m_instances.find(instance));
}

void
LV2PluginFactory::discoverPlugins()
{
    RG_DEBUG << "discoverPlugins()";

    generateTaxonomy();
}

void
LV2PluginFactory::generateTaxonomy()
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    auto allPluginData = lv2utils->getAllPluginData();
    for(auto pair : allPluginData) {
        const QString& uri = pair.first;
        const LV2Utils::LV2PluginData& pluginData = pair.second;

        m_taxonomy[uri] = pluginData.pluginClass;
        m_identifiers.push_back(uri);
    }
}

}
