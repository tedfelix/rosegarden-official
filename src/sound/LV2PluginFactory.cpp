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

#include "base/AudioPluginInstance.h"
#include "LV2PluginInstance.h"
#include "MappedStudio.h"
#include "PluginIdentifier.h"

namespace Rosegarden
{

LV2PluginFactory::LV2PluginFactory()
{
    // the LilvWorld knows all plugins
    m_world = lilv_world_new();
    lilv_world_load_all(m_world);
    const LilvPlugins* plugins = lilv_world_get_all_plugins(m_world);

    LILV_FOREACH (plugins, i, plugins) {
        const LilvPlugin* plugin = lilv_plugins_get(plugins, i);
        QString uri = lilv_node_as_uri(lilv_plugin_get_uri(plugin));
        RG_DEBUG << "got plugin" << uri;

        LV2PluginInstance::LV2PluginData pluginData;
        LilvNode* nameNode = lilv_plugin_get_name(plugin);
        RG_DEBUG << "Name:" << lilv_node_as_string(nameNode);
        pluginData.name = lilv_node_as_string(nameNode);
        lilv_free(nameNode);
        const LilvPluginClass* pclass = lilv_plugin_get_class(plugin);
        const LilvNode* classLabelNode =
            lilv_plugin_class_get_label(pclass);
        QString cstr;
        if (classLabelNode) {
            cstr = lilv_node_as_string(classLabelNode);
        }
        RG_DEBUG << "Class:" << lilv_node_as_string(classLabelNode);
        pluginData.pluginClass = cstr;
        QString aname;
        LilvNode* anameNode = lilv_plugin_get_author_name(plugin);
        if (anameNode) {
            aname = lilv_node_as_string(anameNode);
            lilv_free(anameNode);
        }
        RG_DEBUG << "Author:" << aname;
        pluginData.author = aname;

        const LilvNode* classUri = lilv_plugin_class_get_uri(pclass);
        QString curis = lilv_node_as_uri(classUri);
        RG_DEBUG << "class uri is" << curis;
        pluginData.isInstrument = (curis == LV2_CORE__InstrumentPlugin);

        unsigned int nports = lilv_plugin_get_num_ports(plugin);
        RG_DEBUG << "Plugin ports:" << nports;

        for (unsigned long p = 0; p < nports; ++p) {
            const LilvPort* port = lilv_plugin_get_port_by_index(plugin, p);

            LV2PluginInstance::LV2PortData portData;
            LilvNode *cpn = lilv_new_uri(m_world, LILV_URI_CONTROL_PORT);
            LilvNode *ipn = lilv_new_uri(m_world, LILV_URI_INPUT_PORT);
            bool cntrl = lilv_port_is_a(plugin, port, cpn);
            bool inp = lilv_port_is_a(plugin, port, ipn);
            lilv_node_free(cpn);
            lilv_node_free(ipn);
            LilvNode* portNameNode = lilv_port_get_name(plugin, port);
            QString portName;
            if (portNameNode) {
                portName = lilv_node_as_string(portNameNode);
                lilv_free(portNameNode);
            }

            RG_DEBUG << "Port" << p << "Name:" << portName <<
                "Control:" << cntrl << " Input:" << inp;
            portData.name = portName;
            portData.isControl = cntrl;
            portData.isInput = inp;

            LilvNode* def;
            LilvNode* min;
            LilvNode* max;
            lilv_port_get_range(plugin, port, &def, &min, &max);

            float defVal = lilv_node_as_float(def);
            float minVal = lilv_node_as_float(min);
            float maxVal = lilv_node_as_float(max);

            portData.def = defVal;
            portData.min = minVal;
            portData.max = maxVal;

            RG_DEBUG << "range" << minVal << "-" << maxVal <<
                "default:" << defVal;

            lilv_node_free(def);
            lilv_node_free(min);
            lilv_node_free(max);

            // display hint
            LilvNode* toggledNode =
                lilv_new_uri(m_world,
                             "http://lv2plug.in/ns/lv2core#toggled");
            bool isToggled =
                lilv_port_has_property(plugin, port, toggledNode);
            LilvNode* integerNode =
                lilv_new_uri(m_world,
                             "http://lv2plug.in/ns/lv2core#integer");
            bool isInteger =
                lilv_port_has_property(plugin, port, integerNode);
            LilvNode* logarithmicNode =
                lilv_new_uri(m_world,
                             "http://lv2plug.in/ns/ext/port-props#logarithmic");
            bool isLogarithmic =
                lilv_port_has_property(plugin, port, logarithmicNode);

            lilv_node_free(toggledNode);
            lilv_node_free(integerNode);
            lilv_node_free(logarithmicNode);

            RG_DEBUG << "displayHint:" <<
                isToggled << isInteger << isLogarithmic;

            int hint = PluginPort::NoHint;
            if (isToggled)
                hint |= PluginPort::Toggled;
            if (isInteger)
                hint |= PluginPort::Integer;
            if (isLogarithmic)
                hint |= PluginPort::Logarithmic;

            portData.displayHint = hint;

            pluginData.ports.push_back(portData);
        }
        m_pluginData[uri] = pluginData;
    }
}

LV2PluginFactory::~LV2PluginFactory()
{
    for (std::set
                <RunnablePluginInstance *>::iterator i = m_instances.begin();
                i != m_instances.end(); ++i) {
            (*i)->setFactory(nullptr);
            delete *i;
        }
    m_instances.clear();
    lilv_world_free(m_world);
}

const std::vector<QString> &
LV2PluginFactory::getPluginIdentifiers() const
{
    return m_identifiers;
}

void
LV2PluginFactory::enumeratePlugins(MappedObjectPropertyList &list)
{
    for(auto pair : m_pluginData) {
        const QString& uri = pair.first;
        list.push_back(uri);
        const LV2PluginInstance::LV2PluginData& pluginData = pair.second;
        list.push_back(pluginData.name);
        // uri is the id
        list.push_back(uri);
        list.push_back(pluginData.pluginClass);
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
            const LV2PluginInstance::LV2PortData portData = pluginData.ports[p];
            int type = 0;

            if (portData.isControl) {
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
    auto it = m_pluginData.find(identifier);
    if (it == m_pluginData.end()) {
        RG_DEBUG << "plugin" << identifier << "not found";
        return;
    }

    const LV2PluginInstance::LV2PluginData pluginData = (*it).second;
    slot.setStringProperty(MappedPluginSlot::Label, pluginData.pluginClass);
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
        const LV2PluginInstance::LV2PortData& portData = pluginData.ports[i];
        if (portData.isControl && portData.isInput) {
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
                                    int instrument,
                                    int position,
                                    unsigned int sampleRate,
                                    unsigned int blockSize,
                                    unsigned int channels)
{
    RG_DEBUG << "instantiate plugin" << identifier;
    auto it = m_pluginData.find(identifier);
    if (it == m_pluginData.end()) {
        RG_DEBUG << "plugin" << identifier << "not found";
        return nullptr;
    }

    const QString& uri = (*it).first;
    const LV2PluginInstance::LV2PluginData& pdata = (*it).second;
    LV2PluginInstance *instance =
        new LV2PluginInstance
        (this, instrument, identifier,
         position, sampleRate, blockSize, channels,
         m_world, uri, pdata);

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
    for(auto pair : m_pluginData) {
        const QString& uri = pair.first;
        const LV2PluginInstance::LV2PluginData& pluginData = pair.second;

        m_taxonomy[uri] = pluginData.pluginClass;
        m_identifiers.push_back(uri);
    }
}

}
