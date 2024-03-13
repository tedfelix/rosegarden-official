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
#include "gui/studio/AudioPluginLV2GUI.h"

#include <lv2/midi/midi.h>
#include <lv2/presets/presets.h>
#include <lv2/patch/patch.h>

#include <QFileInfo>
#include <QDir>

// LV2Utils is used in different threads
#define LOCKED QMutexLocker rg_utils_locker(&m_mutex)


namespace Rosegarden
{


LV2Utils *
LV2Utils::getInstance()
{
    RG_DEBUG << "create instance";

    // Guaranteed in C++11 to be lazy initialized and thread-safe.
    // See ISO/IEC 14882:2011 6.7(4).
    static LV2Utils instance;
    return &instance;
}

LV2Utils::LV2Utils()
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    // Qt5 offers QMutexRecursive, but only for 5.14 and later.
    // This works for all Qt5.x.
    : m_mutex(QMutex::Recursive)
#endif
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
    uint32_t flags = 0;

    //lock();
    // ??? Was this just for the URID map feature?  Remove if so.
    LOCKED;
    LilvState* state = lilv_state_new_from_instance
        (plugin,
         instance,
         LV2URIDMapper::getURIDMapFeature(),
         nullptr,
         nullptr,
         nullptr,
         "./savedir",
         getPortValueFunc,
         lv2Instance,
         flags,
         features);
    //unlock();
    return state;
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

void LV2Utils::lock()
{
#ifdef THREAD_DEBUG
    // Very noisy, but interesting.  We definitely see both the UI
    // thread and the JACK process thread hitting this.
    //RG_WARNING << "lock(): gettid(): " << gettid();
#endif

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
    RG_DEBUG << "register plugin" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;

    LOCKED;
    m_pluginInstanceData[pp].pluginInstance = pluginInstance;
}

void LV2Utils::registerGUI(InstrumentId instrument,
                           int position,
                           AudioPluginLV2GUI* gui)
{
    RG_DEBUG << "register gui" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;

    LOCKED;
    m_pluginInstanceData[pp].gui = gui;
}

void LV2Utils::unRegisterPlugin(InstrumentId instrument,
                                int position,
                                LV2PluginInstance* pluginInstance)
{
    RG_DEBUG << "unregister plugin" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;

    LOCKED;
    PluginInstanceDataMap::iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return;
    }
    PluginInstanceData &pgdata = pit->second;
    if (pgdata.pluginInstance != pluginInstance) {
        // this can happen if a plugin is replaced - the old plugin is
        // deleted later (scavenged) after the new plugin is registered
        RG_DEBUG << "unRegisterPlugin plugin already replaced";
        return;
    }
    pgdata.pluginInstance = nullptr;
    if (pgdata.gui == nullptr) {
        // both 0 - delete entry
        m_pluginInstanceData.erase(pit);
    }
}

void LV2Utils::unRegisterGUI(InstrumentId instrument,
                             int position)
{
    RG_DEBUG << "unregister gui" << instrument << position;
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;

    LOCKED;
    PluginInstanceDataMap::iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "gui not found" << instrument << position;
        return;
    }
    PluginInstanceData &pgdata = pit->second;
    pgdata.gui = nullptr;
    if (pgdata.pluginInstance == nullptr) {
        // both 0 - delete entry
        m_pluginInstanceData.erase(pit);
    }
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

    // We are only locking m_pluginInstanceData here which might change
    // if a plugin is added or removed (which likely can't happen while
    // a port value change is happening anyway).
    // I assume lv2_atom_sequence_append_event() which does the actual
    // work of getting the port value update to the plugin's audio thread
    // processing is thread-safe.
    LOCKED;

    PluginInstanceDataMap::iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return;
    }
    PluginInstanceData &pgdata = pit->second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "setPortValue no pluginInstance";
        return;
    }
    // Use lv2_atom_sequence_append_event() to send the update.
    pgdata.pluginInstance->setPortByteArray(index, protocol, data);
}

void LV2Utils::updatePortValue(InstrumentId instrument,
                               int position,
                               int index,
                               const LV2_Atom* atom)
{
    // !!! No lock here as this is called from LV2PluginInstance::run() which
    //     calls lock().
    // This needs to be locked since it is called by the audio thread and
    // it uses m_pluginInstanceData and modifies the portValueQueue which
    // is also modified by triggerPortUpdates() on the UI thread.
    //LOCKED;

    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;

    PluginInstanceDataMap::iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return;
    }
    PluginInstanceData &pgdata = pit->second;
    if (pgdata.gui == nullptr) {
        RG_DEBUG << "no gui at" << instrument << position;
        while (! pgdata.portValueQueue.empty()) {
            PortValueItem* item = pgdata.portValueQueue.front();
            delete item;
            pgdata.portValueQueue.pop();
        }
        return;
    }

    // We want to call the ui updatePortValue from the gui thread so
    // queue the events
    int asize = sizeof(LV2_Atom) + atom->size;
    PortValueItem* item = new PortValueItem;
    item->portIndex= index;
    char* buf = new char[asize];
    memcpy(buf, atom, asize);
    item->valueAtom = reinterpret_cast<const LV2_Atom*>(buf);
    pgdata.portValueQueue.push(item);
}

void LV2Utils::triggerPortUpdates(InstrumentId instrument,
                                  int position)
{
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;

    // This needs to be locked since it is called by the UI thread
    // (AudioPluginLV2GUIWindow's timer) and it uses m_pluginInstanceData
    // and modifies the portValueQueue which is also modified by
    // updatePortValue() on the audio thread.
    LOCKED;

    PluginInstanceDataMap::iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return;
    }
    PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.gui == nullptr) {
        RG_DEBUG << "no gui at" << instrument << position;
        // No GUI, clear the portValueQueue.
        while (! pgdata.portValueQueue.empty()) {
            PortValueItem* item = pgdata.portValueQueue.front();
            delete item;
            pgdata.portValueQueue.pop();
        }
        return;
    }

#ifndef NDEBUG
    if (!pgdata.portValueQueue.empty())
        RG_DEBUG << "triggerPortUpdates()" << pgdata.portValueQueue.size();
#endif

    while (! pgdata.portValueQueue.empty()) {
        PortValueItem* item = pgdata.portValueQueue.front();
        pgdata.gui->updatePortValue(item->portIndex, item->valueAtom);
        delete item;
        pgdata.portValueQueue.pop();
    }
}

void LV2Utils::runWork(const PluginPosition& pp,
                       uint32_t size,
                       const void* data,
                       LV2_Worker_Respond_Function resp)
{
    // Locking is not necessary here.  We are called by LV2Worker in the
    // worker thread which right now is the UI thread.  This routine
    // only reads m_pluginInstanceData.  The only functionality
    // that can modify m_pluginInstanceData and cause a data race would be
    // adding or removing a plugin.  But that is only done in the UI thread.
    //LOCKED;

    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "runWork: plugin not found" <<
            pp.instrument << pp.position;
        return;
    }
    const PluginInstanceData& pgdata = pit->second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "runWork no pluginInstance" <<
            pp.instrument << pp.position;
        return;
    }
    pgdata.pluginInstance->runWork(size, data, resp);
}

void LV2Utils::getControlInValues(InstrumentId instrument,
                                  int position,
                                  std::map<int, float>& controlValues)
{
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;

    LOCKED;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "getControlInValues plugin not found" <<
            instrument << position;
        return;
    }
    const PluginInstanceData &pgdata = pit->second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "getControlInValues no pluginInstance";
        return;
    }
    pgdata.pluginInstance->getControlInValues(controlValues);
}

void LV2Utils::getControlOutValues(InstrumentId instrument,
                                   int position,
                                   std::map<int, float>& controlValues)
{
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;

    LOCKED;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "getControlOutValues plugin not found" <<
            instrument << position;
        return;
    }
    const PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "getControlOutValues no pluginInstance";
        return;
    }
    pgdata.pluginInstance->getControlOutValues(controlValues);
}

LV2PluginInstance* LV2Utils::getPluginInstance(InstrumentId instrument,
                                               int position) const
{
    // ??? LOCK?

    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "getPluginInstance plugin not found" <<
            instrument << position;
        return nullptr;
    }
    const PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "getPluginInstance no pluginInstance";
        return nullptr;
    }
    return pgdata.pluginInstance;
}

void LV2Utils::getConnections(InstrumentId instrument,
                              int position,
                              PluginPort::ConnectionList& clist) const
{
    clist.clear();
    const LV2PluginInstance* lv2inst = getPluginInstance(instrument, position);
    if (!lv2inst) return;
    lv2inst->getConnections(clist);
}

void LV2Utils::setConnections
(InstrumentId instrument,
 int position,
 const PluginPort::ConnectionList& clist) const
{
    LV2PluginInstance* lv2inst = getPluginInstance(instrument, position);
    if (!lv2inst) return;
    lv2inst->setConnections(clist);
}

void LV2Utils::setupPluginParameters
(const QString& uri, LV2PluginParameter::Parameters& params)
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
                             int position) const
{
    LV2PluginInstance* lv2inst = getPluginInstance(instrument, position);
    if (!lv2inst) return false;
    return lv2inst->hasParameters();
}

void LV2Utils::getParameters(InstrumentId instrument,
                             int position,
                             AudioPluginInstance::PluginParameters& params)
{
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "getParameters plugin not found" <<
            instrument << position;
        return;
    }
    const PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "getPrameters no pluginInstance";
        return;
    }
    pgdata.pluginInstance->getParameters(params);
}

void LV2Utils::updatePluginParameter
(InstrumentId instrument,
 int position,
 const QString& paramId,
 const AudioPluginInstance::PluginParameter& param)
{
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "updatePluginParameter plugin not found" <<
            instrument << position;
        return;
    }
    const PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "updatePluginParameter no pluginInstance";
        return;
    }
    pgdata.pluginInstance->updatePluginParameter(paramId, param);
}

void LV2Utils::setupPluginPresets
(const QString& uri,
 AudioPluginInstance::PluginPresetList& presets)
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
                          AudioPluginInstance::PluginPresetList& presets) const
{
    // ??? LOCK?

    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "getPresets plugin not found" <<
            instrument << position;
        return;
    }
    const PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "getPresets no pluginInstance";
        return;
    }
    pgdata.pluginInstance->getPresets(presets);
}

void LV2Utils::setPreset(InstrumentId instrument,
                         int position,
                         const QString& uri)
{
    // ??? LOCK?  There is locking inside of LV2PluginInstance::setPreset().

    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "setPreset plugin not found" <<
            instrument << position;
        return;
    }
    const PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "setPreset no pluginInstance";
        return;
    }
    pgdata.pluginInstance->setPreset(uri);
}

void LV2Utils::loadPreset(InstrumentId instrument,
                          int position,
                          const QString& file)
{
    // ??? LOCK?

    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "loadPreset plugin not found" <<
            instrument << position;
        return;
    }
    const PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "loadPreset no pluginInstance";
        return;
    }
    pgdata.pluginInstance->loadPreset(file);
}

void LV2Utils::savePreset(InstrumentId instrument,
                          int position,
                          const QString& file)
{
    // ??? LOCK?

    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    PluginInstanceDataMap::const_iterator pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "savePreset plugin not found" <<
            instrument << position;
        return;
    }
    const PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.pluginInstance == nullptr) {
        RG_DEBUG << "savePreset no pluginInstance";
        return;
    }
    pgdata.pluginInstance->savePreset(file);
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

LV2Utils::PortValueItem::~PortValueItem()
{
    // This was originally allocated as an array of char.
    // Cast back to that and use delete[] as is customary.
    delete[] reinterpret_cast<const char *>(valueAtom);
}

}
