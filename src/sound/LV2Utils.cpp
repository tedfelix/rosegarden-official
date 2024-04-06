/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2024 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LV2Utils]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2Utils.h"

#include "LV2World.h"
#include "LV2URIDMapper.h"
#include "LV2PluginInstance.h"

#include "misc/Debug.h"
#include "base/AudioPluginInstance.h"  // For PluginPort
#include "sound/AudioInstrumentMixer.h"
#include "gui/studio/AudioPluginLV2GUI.h"

#include <lv2/midi/midi.h>
#include <lv2/presets/presets.h>
#include <lv2/patch/patch.h>

#include <QFileInfo>
#include <QDir>


namespace Rosegarden
{


LV2Utils *
LV2Utils::getInstance()
{
    // Guaranteed in C++11 to be lazy initialized and thread-safe.
    // See ISO/IEC 14882:2011 6.7(4).
    static LV2Utils instance;
    return &instance;
}

LV2Utils::LV2Utils()
{
}

LV2Utils::~LV2Utils()
{
}

const LilvPlugin* LV2Utils::getPluginByUri(const QString& uri) const
{
    LilvNode* pluginUri = makeURINode(uri);
    const LilvPlugin* plugin = lilv_plugins_get_by_uri(
            lilv_world_get_all_plugins(LV2World::get()), pluginUri);
    lilv_node_free(pluginUri);
    return plugin;
}

LilvState* LV2Utils::getStateByUri(const QString& uri)
{
    LilvNode* uriNode = makeURINode(uri);
    LilvState *state = lilv_state_new_from_world
        (LV2World::get(),
         LV2URIDMapper::getURIDMapFeature(),
         uriNode);
    lilv_node_free(uriNode);
    return state;
}

LilvState* LV2Utils::getStateFromInstance
(const LilvPlugin* plugin,
 LilvInstance* instance,
 LilvGetPortValueFunc getPortValueFunc,
 LV2PluginInstance* lv2Instance,
 const LV2_Feature*const* features)
{
    // this is called from the gui thread

    constexpr uint32_t flags = 0;

    return lilv_state_new_from_instance(
            plugin,
            instance,
            LV2URIDMapper::getURIDMapFeature(),
            nullptr,  // scratch_dir
            nullptr,  // copy_dir
            nullptr,  // link_dir
            "./savedir",  // save_dir
            getPortValueFunc,  // get_value
            lv2Instance,  // user_data
            flags,
            features);
}

QString LV2Utils::getStateStringFromInstance
(const LilvPlugin* plugin,
 const QString& uri,
 LilvInstance* instance,
 LilvGetPortValueFunc getPortValueFunc,
 LV2PluginInstance* lv2Instance,
 const LV2_Feature*const* features)
{
    // this is called from the gui thread
    LilvState* state = getStateFromInstance(plugin,
                                            instance,
                                            getPortValueFunc,
                                            lv2Instance,
                                            features);
    std::string uris = uri.toStdString();
    char* s = lilv_state_to_string(LV2World::get(),
                                   LV2URIDMapper::getURIDMapFeature(),
                                   LV2URIDMapper::getURIDUnmapFeature(),
                                   state,
                                   uris.c_str(),
                                   nullptr);
    lilv_state_free(state);
    return QString(s);
}

void LV2Utils::setInstanceStateFromString
    (const QString& stateString,
     LilvInstance* instance,
     LilvSetPortValueFunc setPortValueFunc,
     LV2PluginInstance* lv2Instance,
     const LV2_Feature*const* features)
{
    RG_DEBUG << "setInstanceStateFromString" << stateString;
    std::string str = stateString.toStdString();
    LilvState* state = lilv_state_new_from_string
        (LV2World::get(),
         LV2URIDMapper::getURIDMapFeature(),
         str.c_str());
    uint32_t flags = 0;
    lilv_state_restore(state,
                       instance,
                       setPortValueFunc,
                       lv2Instance,
                       flags,
                       features);
    lilv_state_free(state);
}

LilvState* LV2Utils::getStateFromFile(const LilvNode* uriNode,
                                      const QString& filename)
{
    LilvState* state = lilv_state_new_from_file(LV2World::get(),
                                                LV2URIDMapper::getURIDMapFeature(),
                                                uriNode,
                                                qPrintable(filename));
    return state;
}

void LV2Utils::saveStateToFile(const LilvState* state, const QString& filename)
{
    QFileInfo info(filename);
    QDir dir = info.dir();
    QString basename = info.fileName();
    lilv_state_save(LV2World::get(),
                    LV2URIDMapper::getURIDMapFeature(),
                    LV2URIDMapper::getURIDUnmapFeature(),
                    state,
                    nullptr,
                    qPrintable(dir.absolutePath()),
                    qPrintable(basename));
}

int LV2Utils::getPortIndexFromSymbol(const QString& portSymbol,
                                     const LilvPlugin* plugin)
{
    std::string portSymbolStr = portSymbol.toStdString();

    LilvNode* symNode = lilv_new_string(LV2World::get(), portSymbolStr.c_str());
    const LilvPort* port = lilv_plugin_get_port_by_symbol(plugin,
                                                          symNode);

    lilv_free(symNode);
    int index =  lilv_port_get_index(plugin, port);
    return index;
}

LilvNode* LV2Utils::makeURINode(const QString& uri) const
{
    LilvNode* node = lilv_new_uri(LV2World::get(), qPrintable(uri));
    return node;
}

LilvNode* LV2Utils::makeStringNode(const QString& string) const
{
    LilvNode* node = lilv_new_string(LV2World::get(), qPrintable(string));
    return node;
}

void LV2Utils::setPortValue(InstrumentId instrument,
                            int position,
                            int index,
                            unsigned int protocol,
                            const QByteArray& data)
{
    RG_DEBUG << "setPortValue()" << instrument << position;

    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    // Use lv2_atom_sequence_append_event() to send the update.
    lpi->setPortByteArray(index, protocol, data);
}

void LV2Utils::runWork(const PluginPosition& pp,
                       uint32_t size,
                       const void* data,
                       LV2_Worker_Respond_Function resp)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(
            pp.instrument, pp.position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->runWork(size, data, resp);
}

void LV2Utils::getControlInValues(InstrumentId instrument,
                                  int position,
                                  std::map<int, float>& controlValues)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->getControlInValues(controlValues);
}

void LV2Utils::getControlOutValues(InstrumentId instrument,
                                   int position,
                                   std::map<int, float>& controlValues)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->getControlOutValues(controlValues);
}

void LV2Utils::getConnections(InstrumentId instrument,
                              int position,
                              PluginPort::ConnectionList& clist)
{
    clist.clear();

    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->getConnections(clist);
}

void LV2Utils::setConnections
(InstrumentId instrument,
 int position,
 const PluginPort::ConnectionList& clist)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->setConnections(clist);
}

void LV2Utils::setupPluginParameters
(const QString& uri, LV2PluginParameter::Parameters& params) const
{
    RG_DEBUG << "setupPluginParameters";
    params.clear();
    LilvNode* pluginUri = makeURINode(uri);
    LilvNode* pwn = lilv_new_uri(LV2World::get(), LV2_PATCH__writable);
    LilvNodes* properties =
        lilv_world_find_nodes(LV2World::get(), pluginUri, pwn, nullptr);
    lilv_node_free(pwn);
    fillParametersFromProperties(params, properties, true);
    lilv_nodes_free(properties);
    LilvNode* pwr = lilv_new_uri(LV2World::get(), LV2_PATCH__readable);
    properties =
        lilv_world_find_nodes(LV2World::get(), pluginUri, pwr, nullptr);
    lilv_node_free(pwr);
    lilv_node_free(pluginUri);
    fillParametersFromProperties(params, properties, false);
    lilv_nodes_free(properties);
}

bool LV2Utils::hasParameters(InstrumentId instrument,
                             int position)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return false;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return false;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return false;

    return lpi->hasParameters();
}

void LV2Utils::getParameters(InstrumentId instrument,
                             int position,
                             AudioPluginInstance::PluginParameters& params)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->getParameters(params);
}

void LV2Utils::updatePluginParameter
(InstrumentId instrument,
 int position,
 const QString& paramId,
 const AudioPluginInstance::PluginParameter& param)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

   lpi->updatePluginParameter(paramId, param);
}

void LV2Utils::setupPluginPresets
(const QString& uri,
 AudioPluginInstance::PluginPresetList& presets) const
{
    RG_DEBUG << "setupPluginPresets" << uri;
    presets.clear();
    LilvNode* presetUri = makeURINode(LV2_PRESETS__Preset);
    LilvNode* labelUri =
        makeURINode("http://www.w3.org/2000/01/rdf-schema#label");
    const LilvPlugin* plugin = getPluginByUri(uri);
    LilvNodes* presetNodes = lilv_plugin_get_related(plugin, presetUri);
    LILV_FOREACH(nodes, it, presetNodes)
        {
            const LilvNode* n = lilv_nodes_get(presetNodes, it);
            lilv_world_load_resource(LV2World::get(), n);
            QString presetLabel;
            LilvNodes *presetLabels =
                lilv_world_find_nodes(LV2World::get(), n, labelUri, nullptr);
            if (presetLabels) {
                const LilvNode *labelNode = lilv_nodes_get_first(presetLabels);
                presetLabel = lilv_node_as_string(labelNode);
            }
            lilv_nodes_free(presetLabels);

            const char* pstr = lilv_node_as_string(n);
            RG_DEBUG << "found preset: " << pstr;
            AudioPluginInstance::PluginPreset pp;
            pp.uri = pstr;
            pp.label = presetLabel;
            presets.push_back(pp);
        }
    lilv_node_free(presetUri);
    lilv_node_free(labelUri);
    RG_DEBUG << "setupPluginPresets found" << presets.size();
}

void LV2Utils::getPresets(InstrumentId instrument,
                          int position,
                          AudioPluginInstance::PluginPresetList& presets)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->getPresets(presets);
}

void LV2Utils::setPreset(InstrumentId instrument,
                         int position,
                         const QString& uri)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->setPreset(uri);
}

void LV2Utils::loadPreset(InstrumentId instrument,
                          int position,
                          const QString& file)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->loadPreset(file);
}

void LV2Utils::savePreset(InstrumentId instrument,
                          int position,
                          const QString& file)
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return;

    RunnablePluginInstance *rpi = aim->getPluginInstance(instrument, position);
    if (!rpi)
        return;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
    if (!lpi)
        return;

    lpi->savePreset(file);
}

void LV2Utils::fillParametersFromProperties
(LV2PluginParameter::Parameters& params,
 const LilvNodes* properties,
 bool write)
{
    LILV_FOREACH (nodes, p, properties) {
        const LilvNode* property =
            lilv_nodes_get(properties, p);
        const char* propertyUri =
            lilv_node_as_uri(property);
        auto iter = params.find(propertyUri);
        if (iter != params.end()) {
            // the parameter is already there. Just set the flag
            LV2PluginParameter& pd = (*iter).second;
            if (write) {
                pd.setWritable(true);
            } else {
                pd.setReadable(true);
            }
            continue;
        }
        LilvNode* ln = lilv_new_uri(LV2World::get(), LILV_NS_RDFS "label");
        LilvNode* labelNode =
            lilv_world_get(LV2World::get(), property, ln, nullptr);
        LilvNode* rn = lilv_new_uri(LV2World::get(), LILV_NS_RDFS "range");
        LilvNode* rangeNode =
            lilv_world_get(LV2World::get(), property, rn, nullptr);
        lilv_node_free(ln);
        lilv_node_free(rn);
        QString labelStr = "";
        if (labelNode)
            {
                labelStr = lilv_node_as_string(labelNode);
            }
        lilv_node_free(labelNode);
        AudioPluginInstance::ParameterType pType =
            AudioPluginInstance::ParameterType::UNKNOWN;
        if (rangeNode)
            {
                QString rangeStr = lilv_node_as_string(rangeNode);
                //RG_DEBUG << "rangeStr:" << rangeStr;
                if (rangeStr == LV2_ATOM__Bool)
                    pType = AudioPluginInstance::ParameterType::BOOL;
                if (rangeStr == LV2_ATOM__Double)
                    pType = AudioPluginInstance::ParameterType::DOUBLE;
                if (rangeStr == LV2_ATOM__Float)
                    pType = AudioPluginInstance::ParameterType::FLOAT;
                if (rangeStr == LV2_ATOM__Int)
                    pType = AudioPluginInstance::ParameterType::INT;
                if (rangeStr == LV2_ATOM__Long)
                    pType = AudioPluginInstance::ParameterType::LONG;
                if (rangeStr == LV2_ATOM__Path)
                    pType = AudioPluginInstance::ParameterType::PATH;
                if (rangeStr == LV2_ATOM__String)
                    pType = AudioPluginInstance::ParameterType::STRING;
            }
        lilv_node_free(rangeNode);
        LV2PluginParameter pd(propertyUri, pType);
        pd.setWritable(write);
        pd.setReadable(! write);
        pd.setLabel(labelStr);
        params[propertyUri] = pd;
    }
}


}
