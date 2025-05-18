/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LV2PluginDatabase]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2PluginDatabase.h"

#include "LV2World.h"

#include "misc/Debug.h"
#include "base/AudioPluginInstance.h"  // For PluginPort
#include "gui/application/RosegardenMainWindow.h"
#include "gui/seqmanager/SequenceManager.h"

#include <lv2/midi/midi.h>
#include <lv2/patch/patch.h>
#include <lilv/lilv.h>

#include <mutex>
#include <unistd.h>

namespace
{

static int sampleRate = 0;

static void getSampleRate()
{
    // this function should be called late enough in the enumeration
    // thread so that the creation of the SequenceManager and setting
    // of the sampleRate has already occured. Note the LV2 plugins are
    // initialised after the LADSPA and DSSI plugins. If this is not
    // the case a plugin may get 0 sample rate. If this happens this
    // code could be put in a loop until the sampleRate is non
    // zero. Note the loop should not be infinite to take care of the
    // case that jack is not running.
    Rosegarden::SequenceManager *seqManager =
            Rosegarden::RosegardenMainWindow::self()->getSequenceManager();
    if (seqManager)
        sampleRate = seqManager->getSampleRate();
    RG_DEBUG << "getSampleRate sampleRate:" << sampleRate;
}

/**
 * Thread Safe.
 *
 * Since we are using std::call_once(), initPluginData(), the only writer,
 * will be called exactly once across all threads.  The rest of the routines
 * are readers, so there will be no data races.
 *
 * The Enumerator thread and the UI thread use this.
 */
Rosegarden::LV2PluginDatabase::PluginDatabase pluginDatabase;

std::once_flag initPluginDataOnceFlag;

/// Assemble plugin data for each plugin and add to m_pluginData.
void
initPluginData()
{
    RG_DEBUG << "initPluginData";

    LilvWorld * const world = Rosegarden::LV2World::get();

    const LilvPlugins *allPlugins = lilv_world_get_all_plugins(world);

    LilvNode *cpn = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
    LilvNode *atn = lilv_new_uri(world, LILV_URI_ATOM_PORT);
    LilvNode *ipn = lilv_new_uri(world, LILV_URI_INPUT_PORT);
    LilvNode* midiNode = lilv_new_uri(world, LV2_MIDI__MidiEvent);
    LilvNode* patchNode = lilv_new_uri(world, LV2_PATCH__Message);
    LilvNode* portSampleRateNode = lilv_new_uri(world, LV2_CORE__sampleRate);
    LilvNode* toggledNode =
        lilv_new_uri(world, "http://lv2plug.in/ns/lv2core#toggled");
    LilvNode* integerNode =
        lilv_new_uri(world, "http://lv2plug.in/ns/lv2core#integer");
    LilvNode* logarithmicNode =
        lilv_new_uri(world, "http://lv2plug.in/ns/ext/port-props#logarithmic");

    LILV_FOREACH (plugins, i, allPlugins) {
        const LilvPlugin* plugin = lilv_plugins_get(allPlugins, i);
        QString uri = lilv_node_as_uri(lilv_plugin_get_uri(plugin));
        RG_DEBUG << "got plugin" << uri;

        Rosegarden::LV2PluginDatabase::LV2PluginData pluginData;
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

            Rosegarden::LV2PluginDatabase::LV2PortData portData;
            bool cntrl = lilv_port_is_a(plugin, port, cpn);
            bool atom = lilv_port_is_a(plugin, port, atn);
            bool inp = lilv_port_is_a(plugin, port, ipn);
            LilvNode* portNameNode = lilv_port_get_name(plugin, port);
            QString portName;
            if (portNameNode) {
                portName = lilv_node_as_string(portNameNode);
                lilv_free(portNameNode);
            }

            RG_DEBUG << "Port" << p << "Name:" << portName <<
                "Control:" << cntrl << "Input:" << inp << "Atom:" << atom;
            portData.name = portName;
            portData.portType = Rosegarden::LV2PluginDatabase::LV2AUDIO;
            if (cntrl || atom) portData.portType = Rosegarden::LV2PluginDatabase::LV2CONTROL;
            if (atom && inp) {
                bool isMidi =
                    lilv_port_supports_event(plugin, port, midiNode);
                bool isPatch =
                    lilv_port_supports_event(plugin, port, patchNode);
                if (isMidi) {
                    portData.portType = Rosegarden::LV2PluginDatabase::LV2MIDI;
                }
                if (isPatch) {
                    portData.isPatch = true;
                }
            }
            portData.isInput = inp;
            portData.portProtocol = Rosegarden::LV2PluginDatabase::LV2FLOAT;
            if (atom) portData.portProtocol = Rosegarden::LV2PluginDatabase::LV2ATOM;

            LilvNode* def;
            LilvNode* min;
            LilvNode* max;
            lilv_port_get_range(plugin, port, &def, &min, &max);

            float defVal = lilv_node_as_float(def);
            float minVal = lilv_node_as_float(min);
            float maxVal = lilv_node_as_float(max);

            bool portSampleRate =
                lilv_port_has_property(plugin, port, portSampleRateNode);
            if (portSampleRate) {
                if (! sampleRate) getSampleRate();
                // some plugins scale the defVal others don't !
                if (defVal >= minVal && defVal <= maxVal) {
                    // defVal is in range - scale it
                    defVal *= sampleRate;
                }
                minVal *= sampleRate;
                maxVal *= sampleRate;
            }
            portData.def = defVal;
            portData.min = minVal;
            portData.max = maxVal;

            RG_DEBUG << "range" << minVal << "-" << maxVal <<
                "default:" << defVal;

            lilv_node_free(def);
            lilv_node_free(min);
            lilv_node_free(max);

            // display hint
            bool isToggled =
                lilv_port_has_property(plugin, port, toggledNode);
            bool isInteger =
                lilv_port_has_property(plugin, port, integerNode);
            bool isLogarithmic =
                lilv_port_has_property(plugin, port, logarithmicNode);

            RG_DEBUG << "displayHint:" <<
                isToggled << isInteger << isLogarithmic;

            int hint = Rosegarden::PluginPort::NoHint;
            if (isToggled)
                hint |= Rosegarden::PluginPort::Toggled;
            if (isInteger)
                hint |= Rosegarden::PluginPort::Integer;
            if (isLogarithmic)
                hint |= Rosegarden::PluginPort::Logarithmic;

            portData.displayHint = hint;

            pluginData.ports.push_back(portData);
        }
        pluginDatabase[uri] = pluginData;
    }
    lilv_node_free(cpn);
    lilv_node_free(atn);
    lilv_node_free(ipn);
    lilv_node_free(midiNode);
    lilv_node_free(patchNode);
    lilv_node_free(portSampleRateNode);
    lilv_node_free(toggledNode);
    lilv_node_free(integerNode);
    lilv_node_free(logarithmicNode);

    RG_DEBUG << "initPluginData done";
}

}


namespace Rosegarden
{


const LV2PluginDatabase::PluginDatabase &
LV2PluginDatabase::getAllPluginData()
{
    std::call_once(initPluginDataOnceFlag, initPluginData);

    return pluginDatabase;
}

LV2PluginDatabase::LV2PluginData
LV2PluginDatabase::getPluginData(const QString &uri)
{
    std::call_once(initPluginDataOnceFlag, initPluginData);

    auto it = pluginDatabase.find(uri);
    if (it == pluginDatabase.end()) return LV2PluginData();

    // COPY.  Thread safe.  Can we return a const reference for speed?
    return it->second;
}

QString
LV2PluginDatabase::getPortName(const QString &uri, int portIndex)
{
    std::call_once(initPluginDataOnceFlag, initPluginData);

    auto it = pluginDatabase.find(uri);
    if (it == pluginDatabase.end()) return "";
    const LV2PluginData& pdat = it->second;

    // COPY.  Thread safe.  Can we return a const reference for speed?
    return pdat.ports[portIndex].name;
}


}
