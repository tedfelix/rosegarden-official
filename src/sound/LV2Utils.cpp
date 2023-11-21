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
#include "sound/LV2PluginInstance.h"
#include "gui/studio/AudioPluginLV2GUI.h"
#include "gui/studio/LV2Gtk.h"

#include <lv2/midi/midi.h>
#include <lv2/ui/ui.h>

// LV2Utils is used in different threads
#define LOCKED QMutexLocker rg_utils_locker(&m_mutex)

namespace
{
    LV2_URID LV2UridMap(LV2_URID_Map_Handle handle,
                        const char *uri)
    {
        Rosegarden::LV2Utils* lv2i =
            static_cast<Rosegarden::LV2Utils*>(handle);
        return lv2i->uridMap(uri);
    }

    const char* LV2UridUnmap(LV2_URID_Unmap_Handle handle,
                             LV2_URID urid)
    {
        Rosegarden::LV2Utils* lv2i =
            static_cast<Rosegarden::LV2Utils*>(handle);
        return lv2i->uridUnmap(urid);
    }
}

namespace Rosegarden
{

LV2Utils *
LV2Utils::getInstance()
{
    static LV2Utils instance;
    return &instance;
}

LV2_URID
LV2Utils::uridMap(const char *uri)
{
    LOCKED;
    auto it = m_uridMap.find(uri);
    if (it == m_uridMap.end()) {
        int id = m_nextId;
        m_nextId++;
        m_uridMap[uri] = id;
        m_uridUnmap[id] = uri;
    }
    int ret = m_uridMap[uri];
    return ret;
}

const char*
LV2Utils::uridUnmap(LV2_URID urid)
{
    LOCKED;
    auto it = m_uridUnmap.find(urid);
    if (it == m_uridUnmap.end()) {
        return "";
    }
    return (*it).second.c_str();
}

LV2Utils::LV2Utils()
{
    LOCKED;

    m_lv2gtk = new LV2Gtk;
    m_worker = nullptr;

    m_nextId = 1;
    m_map = {this, &LV2UridMap};
    m_unmap = {this, &LV2UridUnmap};

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
            LilvNode *atn = lilv_new_uri(m_world, LILV_URI_ATOM_PORT);
            LilvNode *ipn = lilv_new_uri(m_world, LILV_URI_INPUT_PORT);
            bool cntrl = lilv_port_is_a(plugin, port, cpn);
            bool atom = lilv_port_is_a(plugin, port, atn);
            bool inp = lilv_port_is_a(plugin, port, ipn);
            lilv_node_free(cpn);
            lilv_node_free(atn);
            lilv_node_free(ipn);
            LilvNode* portNameNode = lilv_port_get_name(plugin, port);
            QString portName;
            if (portNameNode) {
                portName = lilv_node_as_string(portNameNode);
                lilv_free(portNameNode);
            }

            RG_DEBUG << "Port" << p << "Name:" << portName <<
                "Control:" << cntrl << "Input:" << inp << "Atom:" << atom;
            portData.name = portName;
            portData.portType = LV2AUDIO;
            if (cntrl || atom) portData.portType = LV2CONTROL;
            if (atom && inp) {
                LilvNode* midiNode = lilv_new_uri(m_world, LV2_MIDI__MidiEvent);
                bool isMidi = lilv_port_supports_event(plugin, port, midiNode);
                lilv_node_free(midiNode);
                if (isMidi) {
                    portData.portType = LV2MIDI;
                }
            }
            portData.isInput = inp;
            portData.portProtocol = LV2FLOAT;
            if (atom) portData.portProtocol = LV2ATOM;

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
    m_plugins = lilv_world_get_all_plugins(m_world);
}

LV2Utils::~LV2Utils()
{
    LOCKED;
    lilv_world_free(m_world);
    delete m_lv2gtk;
}

const std::map<QString, LV2Utils::LV2PluginData>& LV2Utils::getAllPluginData() const
{
    return m_pluginData;
}

const LilvPlugin* LV2Utils::getPluginByUri(const QString& uri) const
{
    LilvNode* pluginUri = makeURINode(uri);
    const LilvPlugin* plugin = lilv_plugins_get_by_uri(m_plugins, pluginUri);
    lilv_node_free(pluginUri);
    return plugin;
}

LV2Utils::LV2PluginData LV2Utils::getPluginData(const QString& uri) const
{
    LV2PluginData emptyData;
    emptyData.isInstrument = false;
    auto it = m_pluginData.find(uri);
    if (it == m_pluginData.end()) return emptyData;
    return (*it).second;
}

const LilvUIs* LV2Utils::getPluginUIs(const QString& uri) const
{
    const LilvPlugin* plugin = getPluginByUri(uri);
    LilvUIs *uis = lilv_plugin_get_uis(plugin);
    return uis;
}

LilvNode* LV2Utils::makeURINode(const QString& uri) const
{
    LilvNode* node = lilv_new_uri(m_world, qPrintable(uri));
    return node;
}

void LV2Utils::lock()
{
    m_mutex.lock();
}

void LV2Utils::unlock()
{
    m_mutex.unlock();
}

void LV2Utils::registerPlugin(InstrumentId instrument,
                              int position,
                              LV2PluginInstance* pluginInstance)
{
    LOCKED;
    RG_DEBUG << "register plugin" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    m_pluginGuis[pp].pluginInstance = pluginInstance;
}

void LV2Utils::registerGUI(InstrumentId instrument,
                           int position,
                           AudioPluginLV2GUI* gui)
{
    LOCKED;
    RG_DEBUG << "register gui" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    m_pluginGuis[pp].gui = gui;
}

void LV2Utils::registerWorker(Worker* worker)
{
    m_worker = worker;
}

void LV2Utils::unRegisterPlugin(InstrumentId instrument,
                                int position)
{
    LOCKED;
    RG_DEBUG << "unregister plugin" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    auto pit = m_pluginGuis.find(pp);
    if (pit == m_pluginGuis.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return;
    }
    LV2UPlugin& pgdata = (*pit).second;
    pgdata.pluginInstance = nullptr;
    if (pgdata.gui == nullptr) {
        // both 0 - delete entry
        m_pluginGuis.erase(pit);
    }
}

void LV2Utils::unRegisterGUI(InstrumentId instrument,
                             int position)
{
    LOCKED;
    RG_DEBUG << "unregister gui" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    auto pit = m_pluginGuis.find(pp);
    if (pit == m_pluginGuis.end()) {
        RG_DEBUG << "gui not found" << instrument << position;
        return;
    }
    LV2UPlugin& pgdata = (*pit).second;
    pgdata.gui = nullptr;
    if (pgdata.pluginInstance == nullptr) {
        // both 0 - delete entry
        m_pluginGuis.erase(pit);
    }
}

void LV2Utils::unRegisterWorker()
{
    m_worker = nullptr;
}

void LV2Utils::setPortValue(InstrumentId instrument,
                            int position,
                            int index,
                            unsigned int protocol,
                            const QByteArray& data)
{
    RG_DEBUG << "setPortValue" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    LOCKED;
    auto pit = m_pluginGuis.find(pp);
    if (pit == m_pluginGuis.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return;
    }
    LV2UPlugin& pgdata = (*pit).second;
    LV2PluginInstance* instance = pgdata.pluginInstance;
    instance->setPortByteArray(index, protocol, data);
}

void LV2Utils::updatePortValue(InstrumentId instrument,
                               int position,
                               int index,
                               const LV2_Atom* atom)
{
    // no lock here as this is called from the run method which is
    // already locked
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    auto pit = m_pluginGuis.find(pp);
    if (pit == m_pluginGuis.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return;
    }
    const LV2UPlugin& pgdata = (*pit).second;
    if (pgdata.gui == nullptr) {
        RG_DEBUG << "no gui at" << instrument << position;
        return;
    }

    pgdata.gui->updatePortValue(index, atom);
}

int LV2Utils::numInstances(InstrumentId instrument,
                           int position) const
{
    RG_DEBUG << "numInstances" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    auto pit = m_pluginGuis.find(pp);
    if (pit == m_pluginGuis.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return 1;
    }
    const LV2UPlugin& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "numInstances no pluginInstance";
        return 1;
    }
    return pgdata.pluginInstance->numInstances();
}

LV2Utils::Worker* LV2Utils::getWorker() const
{
    return m_worker;
}

void LV2Utils::runWork(const PluginPosition& pp,
                       uint32_t size,
                       const void* data,
                       LV2_Worker_Respond_Function resp)
{
    auto pit = m_pluginGuis.find(pp);
    if (pit == m_pluginGuis.end()) {
        RG_DEBUG << "runWork: plugin not found" <<
            pp.instrument << pp.position;
        return;
    }
    const LV2UPlugin& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "runWork no pluginInstance" <<
            pp.instrument << pp.position;
        return;
    }
    pgdata.pluginInstance->runWork(size, data, resp);
}

void LV2Utils::getControlOutValues(InstrumentId instrument,
                                   int position,
                                   std::map<int, float>& controlValues)
{
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    auto pit = m_pluginGuis.find(pp);
    if (pit == m_pluginGuis.end()) {
        RG_DEBUG << "getControlOutValues plugin not found" <<
            instrument << position;
        return;
    }
    const LV2UPlugin& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "getControlOutValues no pluginInstance";
        return;
    }
    LOCKED;
    pgdata.pluginInstance->getControlOutValues(controlValues);
}

const LV2PluginInstance* LV2Utils::getPluginInstance(InstrumentId instrument,
                                                     int position)
{
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    auto pit = m_pluginGuis.find(pp);
    if (pit == m_pluginGuis.end()) {
        RG_DEBUG << "getPluginInstance plugin not found" <<
            instrument << position;
        return nullptr;
    }
    const LV2UPlugin& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "getPluginInstance no pluginInstance";
        return nullptr;
    }
    return pgdata.pluginInstance;
}

LV2Gtk* LV2Utils::getLV2Gtk() const
{
  return m_lv2gtk;
}

}
