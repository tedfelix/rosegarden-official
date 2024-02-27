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

#include "LV2URIDMapper.h"
#include "LV2Worker.h"

#include "misc/Debug.h"
#include "base/AudioPluginInstance.h"  // For PluginPort
#include "sound/LV2PluginInstance.h"
#include "gui/studio/AudioPluginLV2GUI.h"

#include <lv2/midi/midi.h>
#include <lv2/presets/presets.h>

#include <QThread>
#include <QFileInfo>
#include <QDir>

// LV2Utils is used in different threads
#define LOCKED QMutexLocker rg_utils_locker(&m_mutex)


namespace Rosegarden
{


LV2Utils *
LV2Utils::getInstance()
{
    // C++11 and up guarantee that static construction is thread-safe.
    static LV2Utils instance;
    return &instance;
}

LV2Utils::LV2Utils() :
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    // Qt5 offers QMutexRecursive, but only for 5.14 and later.
    // This works for all Qt5.x.
    m_mutex(QMutex::Recursive), // recursive
#endif
    m_world(lilv_world_new())
{
    // It is not necessary to lock here.  C++11 and up guarantee that static
    // construction is thread-safe.  This object is static constructed in
    // its getInstance() function.

    lilv_world_load_all(m_world);

    m_plugins = lilv_world_get_all_plugins(m_world);
}

LV2Utils::~LV2Utils()
{
    // No need to lock here.  We are going down at static destruction
    // time.  See getInstance().  Everyone who ever talked to us should
    // be gone by now.  If there is thread contention at this point in
    // time, we've got Static Destruction Order Fiasco (some other dtor
    // is trying to talk to us at static destruction time) and that won't
    // be solved with a lock.

    lilv_world_free(m_world);
}

void LV2Utils::initPluginData()
{
    LILV_FOREACH (plugins, i, m_plugins) {
        const LilvPlugin* plugin = lilv_plugins_get(m_plugins, i);
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
}

const std::map<QString /* URI */, LV2Utils::LV2PluginData> &
LV2Utils::getAllPluginData()
{
    if (m_pluginData.size() == 0) {
        // plugin data has not yet been set (or maybe there are no plugins)
        initPluginData();
    }
    return m_pluginData;
}

const LilvPlugin* LV2Utils::getPluginByUri(const QString& uri) const
{
    LilvNode* pluginUri = makeURINode(uri);
    const LilvPlugin* plugin = lilv_plugins_get_by_uri(m_plugins, pluginUri);
    lilv_node_free(pluginUri);
    return plugin;
}

LilvState* LV2Utils::getStateByUri(const QString& uri)
{
    LilvNode* uriNode = makeURINode(uri);
    LilvState *state = lilv_state_new_from_world
        (m_world,
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
    char* s = lilv_state_to_string(m_world,
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
        (m_world,
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
    LilvState* state = lilv_state_new_from_file(m_world,
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
    lilv_state_save(m_world,
                    LV2URIDMapper::getURIDMapFeature(),
                    LV2URIDMapper::getURIDUnmapFeature(),
                    state,
                    nullptr,
                    qPrintable(dir.absolutePath()),
                    qPrintable(basename));
}

LV2Utils::LV2PluginData LV2Utils::getPluginData(const QString& uri) const
{
    LV2PluginData emptyData;
    emptyData.isInstrument = false;
    auto it = m_pluginData.find(uri);
    if (it == m_pluginData.end()) return emptyData;

    // COPY.  Can we do a const & for speed?
    return it->second;
}

int LV2Utils::getPortIndexFromSymbol(const QString& portSymbol,
                                     const LilvPlugin* plugin)
{
    std::string portSymbolStr = portSymbol.toStdString();

    LilvNode* symNode = lilv_new_string(m_world, portSymbolStr.c_str());
    const LilvPort* port = lilv_plugin_get_port_by_symbol(plugin,
                                                          symNode);

    lilv_free(symNode);
    int index =  lilv_port_get_index(plugin, port);
    return index;
}

LilvNode* LV2Utils::makeURINode(const QString& uri) const
{
    LilvNode* node = lilv_new_uri(m_world, qPrintable(uri));
    return node;
}

LilvNode* LV2Utils::makeStringNode(const QString& string) const
{
    LilvNode* node = lilv_new_string(m_world, qPrintable(string));
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

void LV2Utils::registerWorker(LV2Worker* worker)
{
    m_worker = worker;
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
    pgdata.pluginInstance->setPortByteArray(index, protocol, data);
}

void LV2Utils::updatePortValue(InstrumentId instrument,
                               int position,
                               int index,
                               const LV2_Atom* atom)
{

    // !!! No lock here as this is called from LV2PluginInstance::run() which
    //     calls lock().
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
        while (! pgdata.atomQueue.empty()) {
            AtomQueueItem* item = pgdata.atomQueue.front();
            delete item;
            pgdata.atomQueue.pop();
        }
        return;
    }

    // We want to call the ui updatePortValue from the gui thread so
    // queue the events
    int asize = sizeof(LV2_Atom) + atom->size;
    AtomQueueItem* item = new AtomQueueItem;
    item->portIndex= index;
    char* buf = new char[asize];
    memcpy(buf, atom, asize);
    item->atomBuffer = reinterpret_cast<const LV2_Atom*>(buf);
    pgdata.atomQueue.push(item);
}

void LV2Utils::triggerPortUpdates(InstrumentId instrument,
                                  int position)
{
    PluginPosition pp;
    pp.instrument = instrument;
    pp.position = position;
    auto pit = m_pluginInstanceData.find(pp);
    if (pit == m_pluginInstanceData.end()) {
        RG_DEBUG << "plugin not found" << instrument << position;
        return;
    }
    PluginInstanceData& pgdata = (*pit).second;
    if (pgdata.gui == nullptr) {
        RG_DEBUG << "no gui at" << instrument << position;
        while (! pgdata.atomQueue.empty()) {
            AtomQueueItem* item = pgdata.atomQueue.front();
            delete item;
            pgdata.atomQueue.pop();
        }
        return;
    }
    RG_DEBUG << "triggerPortUpdates" << pgdata.atomQueue.size();
    while (! pgdata.atomQueue.empty()) {
        AtomQueueItem* item = pgdata.atomQueue.front();
        pgdata.gui->updatePortValue(item->portIndex, item->atomBuffer);
        delete item;
        pgdata.atomQueue.pop();
    }
}

LV2Worker* LV2Utils::getWorker() const
{
    return m_worker;
}

void LV2Utils::runWork(const PluginPosition& pp,
                       uint32_t size,
                       const void* data,
                       LV2_Worker_Respond_Function resp)
{
    // ??? LOCK?

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

QString LV2Utils::getPortName(const QString& uri, int portIndex) const
{
    auto it = m_pluginData.find(uri);
    if (it == m_pluginData.end()) return "";
    const LV2PluginData& pdat = it->second;
    return pdat.ports[portIndex].name;
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
            lilv_world_load_resource(m_world, n);
            QString presetLabel;
            LilvNodes *presetLabels =
                lilv_world_find_nodes(m_world, n, labelUri, nullptr);
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

LV2Utils::AtomQueueItem::AtomQueueItem() :
    portIndex(0),
    atomBuffer(nullptr)
{
}

LV2Utils::AtomQueueItem::~AtomQueueItem()
{
    if (atomBuffer) delete[] atomBuffer;
}

}
