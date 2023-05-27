/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2022 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LV2Utils]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2Utils.h"

#include "misc/Debug.h"
#include "base/AudioPluginInstance.h"

// LV2Utils is used in different threads
#define LOCKED QMutexLocker rg_utils_locker(&m_mutex)

namespace Rosegarden
{

LV2Utils *
LV2Utils::getInstance()
{
    static LV2Utils instance;
    return &instance;
}

LV2Utils::LV2Utils()
{
    // the LilvWorld knows all plugins
    m_world = lilv_world_new();
    lilv_world_load_all(m_world);
    const LilvPlugins* plugins = lilv_world_get_all_plugins(m_world);

    LILV_FOREACH (plugins, i, plugins) {
        const LilvPlugin* plugin = lilv_plugins_get(plugins, i);
        QString uri = lilv_node_as_uri(lilv_plugin_get_uri(plugin));
        RG_DEBUG << "got plugin" << uri;

        LV2PluginData pluginData;
        LilvNode* nameNode = lilv_plugin_get_name(plugin);
        RG_DEBUG << "Name:" << lilv_node_as_string(nameNode);
        pluginData.name = lilv_node_as_string(nameNode);
        lilv_free(nameNode);

        QString label = pluginData.name;
        if (label.length() > 20) {
            label.truncate(18);
            label += "..";
        }
        pluginData.label = label;

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
        pluginData.isInstrument = ((curis == LV2_CORE__InstrumentPlugin) ||
                                   (curis == LV2_CORE__OscillatorPlugin));

        unsigned int nports = lilv_plugin_get_num_ports(plugin);
        RG_DEBUG << "Plugin ports:" << nports;

        for (unsigned long p = 0; p < nports; ++p) {
            const LilvPort* port = lilv_plugin_get_port_by_index(plugin, p);

            LV2PortData portData;
            LilvNode *cpn = lilv_new_uri(m_world, LILV_URI_CONTROL_PORT);
            LilvNode *mpn = lilv_new_uri(m_world, LILV_URI_ATOM_PORT);
            LilvNode *ipn = lilv_new_uri(m_world, LILV_URI_INPUT_PORT);
            bool cntrl = lilv_port_is_a(plugin, port, cpn);
            bool midi = lilv_port_is_a(plugin, port, mpn);
            bool inp = lilv_port_is_a(plugin, port, ipn);
            lilv_node_free(cpn);
            lilv_node_free(mpn);
            lilv_node_free(ipn);
            LilvNode* portNameNode = lilv_port_get_name(plugin, port);
            QString portName;
            if (portNameNode) {
                portName = lilv_node_as_string(portNameNode);
                lilv_free(portNameNode);
            }

            RG_DEBUG << "Port" << p << "Name:" << portName <<
                "Control:" << cntrl << "Input:" << inp << "Midi:" << midi;
            portData.name = portName;
            portData.portType = LV2AUDIO;
            if (cntrl) portData.portType = LV2CONTROL;
            if (midi && inp) portData.portType = LV2MIDI;
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

LV2Utils::~LV2Utils()
{
    lilv_world_free(m_world);
}

const std::map<QString, LV2Utils::LV2PluginData>& LV2Utils::getAllPluginData()
{
    return m_pluginData;
}

const LilvPlugin* LV2Utils::getPluginByUri(const QString& uri) const
{
    const LilvPlugins* plugins = lilv_world_get_all_plugins(m_world);
    QByteArray ba = uri.toLocal8Bit();
    const char *uris = ba.data();
    LilvNode* pluginUri = lilv_new_uri(m_world, uris);
    const LilvPlugin* plugin = lilv_plugins_get_by_uri(plugins, pluginUri);
    lilv_node_free(pluginUri);
    return plugin;
}

LV2Utils::LV2PluginData LV2Utils::getPluginData(const QString& uri) const
{
    LV2PluginData emptyData;
    auto it = m_pluginData.find(uri);
    if (it == m_pluginData.end()) return emptyData;
    return (*it).second;
}

}
