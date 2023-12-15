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

#define RG_MODULE_STRING "[LV2PluginInstance]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2PluginInstance.h"
#include "LV2PluginFactory.h"

#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonArray>

#include "misc/Debug.h"
#include "sound/LV2Utils.h"
#include "sound/Midi.h"
#include "sound/AudioProcess.h"
#include "gui/application/RosegardenMainWindow.h"

#include <lv2/midi/midi.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>

namespace
{
    const void* getPortValueFunc(const char *port_symbol,
                                 void *user_data,
                                 uint32_t *size,
                                 uint32_t *type)
    {
        Rosegarden::LV2PluginInstance* pi =
            (Rosegarden::LV2PluginInstance*)user_data;
        return pi->getPortValue(port_symbol, size, type);
    }

    void setPortValueFunc(const char *port_symbol,
                          void *user_data,
                          const void *value,
                          uint32_t size,
                          uint32_t type)
    {
        Rosegarden::LV2PluginInstance* pi =
            (Rosegarden::LV2PluginInstance*)user_data;
        pi->setPortValue(port_symbol, value, size, type);
    }
}

namespace Rosegarden
{

#define EVENT_BUFFER_SIZE 1023
#define ABUFSIZED 100000

LV2PluginInstance::LV2PluginInstance
(PluginFactory *factory,
 InstrumentId instrument,
 const QString& identifier,
 int position,
 unsigned long sampleRate,
 size_t blockSize,
 int idealChannelCount,
 const QString& uri,
 AudioInstrumentMixer* amixer) :
        RunnablePluginInstance(factory, identifier),
        m_instrument(instrument),
        m_position(position),
        m_instance(nullptr),
        m_uri(uri),
        m_plugin(nullptr),
        m_channelCount(0),
        m_midiParser(nullptr),
        m_blockSize(blockSize),
        m_sampleRate(sampleRate),
        m_latencyPort(nullptr),
        m_run(false),
        m_bypassed(false),
        m_distributeChannels(false),
        m_pluginHasRun(false),
        m_amixer(amixer)
{
    RG_DEBUG << "create plugin" << uri << m_instrument << m_position;

    LV2Utils* lv2utils = LV2Utils::getInstance();
    m_atomTransferUrid = lv2utils->uridMap(LV2_ATOM__eventTransfer);

    m_workerHandle.instrument = instrument;
    m_workerHandle.position = position;

    m_pluginData = lv2utils->getPluginData(uri);

    init(idealChannelCount);

    m_inputBuffers = new sample_t * [m_audioPortsIn.size()];
    m_outputBuffers = new sample_t * [m_audioPortsOut.size()];

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        m_inputBuffers[i] = new sample_t[blockSize];
        memset(m_inputBuffers[i], 0, blockSize * sizeof(sample_t));
    }
    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
        m_outputBuffers[i] = new sample_t[blockSize];
        memset(m_outputBuffers[i], 0, blockSize * sizeof(sample_t));
    }

    m_plugin = lv2utils->getPluginByUri(m_uri);

    snd_midi_event_new(100, &m_midiParser);
    m_midiEventUrid = lv2utils->uridMap(LV2_MIDI__MidiEvent);

    instantiate(sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
    m_workerInterface = (LV2_Worker_Interface*)
        lilv_instance_get_extension_data(m_instance, LV2_WORKER__interface);
    RG_DEBUG << "worker interface" << m_workerInterface;
    if (m_workerInterface) RG_DEBUG << (void*)m_workerInterface->work <<
                               (void*)m_workerInterface->work_response <<
                               (void*)m_workerInterface->end_run;

    RG_DEBUG << "register plugin";
    lv2utils->registerPlugin(m_instrument, m_position, this);
}

void
LV2PluginInstance::init(int idealChannelCount)
{
    RG_DEBUG << "LV2PluginInstance::init(" << idealChannelCount <<
        "): plugin has" << m_pluginData.ports.size() << "ports";

    m_channelCount = idealChannelCount;

    // Discover ports numbers and identities
    //
    LV2Utils* lv2utils = LV2Utils::getInstance();
    for (unsigned long i = 0; i < m_pluginData.ports.size(); ++i) {
        const LV2Utils::LV2PortData& portData = m_pluginData.ports[i];
        switch(portData.portType) {
        case LV2Utils::LV2AUDIO:
            if (portData.isInput) {
                RG_DEBUG << "LV2PluginInstance::init: port" << i << "is audio in";
                m_audioPortsIn.push_back(i);
            } else {
                RG_DEBUG << "LV2PluginInstance::init: port" << i << "is audio out";
                m_audioPortsOut.push_back(i);
            }
            break;
        case LV2Utils::LV2CONTROL:
        case LV2Utils::LV2MIDI:
            if (portData.portProtocol == LV2Utils::LV2ATOM) {
                if (portData.isInput) {
                    RG_DEBUG << "Atom in port" << i << portData.name;
                    AtomPort ap;
                    ap.index = i;
                    ap.isMidi = (portData.portType == LV2Utils::LV2MIDI);
                    // create the atom port
                    // use double to get 64 bit alignment
                    double* dbuf = new double[ABUFSIZED];
                    bzero(dbuf, 8 * ABUFSIZED);
                    LV2_Atom_Sequence* atomIn =
                        reinterpret_cast<LV2_Atom_Sequence*>(dbuf);
                    lv2_atom_sequence_clear(atomIn);
                    LV2_URID type = lv2utils->uridMap(LV2_ATOM__Sequence);
                    atomIn->atom.type = type;
                    atomIn->atom.size = 8 * ABUFSIZED - 8;
                    ap.atomSeq = atomIn;
                    RG_DEBUG << "created atom sequence" << atomIn;
                    m_atomInputPorts.push_back(ap);
                } else {
                    RG_DEBUG << "Atom out port" << i << portData.name;
                    AtomPort ap;
                    ap.index = i;
                    ap.isMidi = (portData.portType == LV2Utils::LV2MIDI);
                    // create the atom port
                    // use double to get 64 bit alignment
                    double* dbuf = new double[ABUFSIZED];
                    bzero(dbuf, 8 * ABUFSIZED);
                    LV2_Atom_Sequence* atomOut =
                        reinterpret_cast<LV2_Atom_Sequence*>(dbuf);
                    lv2_atom_sequence_clear(atomOut);
                    LV2_URID type = lv2utils->uridMap(LV2_ATOM__Sequence);
                    atomOut->atom.type = type;
                    atomOut->atom.size = 8 * ABUFSIZED - 8;
                    ap.atomSeq = atomOut;
                    RG_DEBUG << "created atom sequence" << atomOut;
                    m_atomOutputPorts.push_back(ap);
                }
            } else {
                if (portData.isInput) {
                    RG_DEBUG << "LV2PluginInstance::init: port" <<
                        i << "is control in";
                    m_controlPortsIn[i] = 0.0;
                } else {
                    RG_DEBUG << "LV2PluginInstance::init: port" <<
                        i << "is control out";
                    m_controlPortsOut[i] = 0.0;
                    if ((portData.name == "latency") ||
                        (portData.name == "_latency")) {
                        RG_DEBUG << "Wooo! We have a latency port!";
                        float& value = m_controlPortsOut[i];
                        m_latencyPort = &(value);
                    }
                }
            }
            break;
        }
    }
    // if we have a stereo channel and a mono plugin we combine the
    // stereo input to the mono plugin and distribute the plugin
    // output to the stereo channels !
    m_distributeChannels = false;
    if (m_audioPortsIn.size() == 1 &&
        m_audioPortsOut.size() == 1 &&
        m_channelCount == 2)
        {
            m_audioPortsIn.push_back(-1);
            m_audioPortsOut.push_back(-1);
            m_distributeChannels = true;
        }

    // set up the default connections
    m_connections.clear();
    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        if (m_audioPortsIn[i] == -1) continue; // for distributeChannels
        PluginPortConnection::Connection c;
        c.isOutput = false;
        c.isAudio = true;
        c.pluginPort = lv2utils->getPortName(m_uri, m_audioPortsIn[i]);
        c.instrumentId = 0;
        c.channel = 0;
        if (i == 0) {
            c.instrumentId = m_instrument;
            c.channel = 0;
        }
        if (i == 1 && m_channelCount == 2) {
            c.instrumentId = m_instrument;
            c.channel = 1;
        }
        m_connections.push_back(c);
    }
}

size_t
LV2PluginInstance::getLatency()
{
    if (m_latencyPort) {
        if (!m_run) {
            for (size_t i = 0; i < getAudioInputCount(); ++i) {
                for (size_t j = 0; j < m_blockSize; ++j) {
                    m_inputBuffers[i][j] = 0.f;
                }
            }
            run(RealTime::zero());
	}
        return *m_latencyPort;
    }
    return 0;
}

void
LV2PluginInstance::silence()
{
    if (isOK()) {
        deactivate();
        activate();
    }
}

void
LV2PluginInstance::discardEvents()
{
    RG_DEBUG << "discardEvents";
    m_eventBuffer.clear();
    // it is not always enough just to clear the buffer. If notes are
    // playing they should be stopped with all notes off
    unsigned char status = 0xb0;
    unsigned char data1 = MIDI_CONTROLLER_ALL_NOTES_OFF;
    unsigned char data2 = 0;
    // all channels
    for(int i=0; i<16; i++) {
        QByteArray rawMidi;
        rawMidi.append(status + i);
        rawMidi.append(data1);
        rawMidi.append(data2);
        sendMidiData(rawMidi, 0);
    }
}

void
LV2PluginInstance::setIdealChannelCount(size_t channels)
{
    RG_DEBUG << "setIdealChannelCount" << channels;

    if (channels == m_channelCount) return;

    if (isOK()) {
        deactivate();
    }

    cleanup();

    instantiate(m_sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
}

int LV2PluginInstance::numInstances() const
{
    RG_DEBUG << "numInstances";
    if (m_instance == nullptr) return 0;
    // always 1
    return 1;
}

void LV2PluginInstance::runWork(uint32_t size,
                                const void* data,
                                LV2_Worker_Respond_Function resp)
{
    if (! m_workerInterface) return;
    LV2_Handle handle = lilv_instance_get_handle(m_instance);
    LV2Utils::PluginPosition pp;
    pp.instrument = m_instrument;
    pp.position = m_position;
    LV2_Worker_Status status =
        m_workerInterface->work(handle,
                                resp,
                                &pp,
                                size,
                                data);
    RG_DEBUG << "work return:" << status;
}

void LV2PluginInstance::getControlInValues
(std::map<int, float>& controlValues)
{
    controlValues.clear();
    for (auto& pair : m_controlPortsIn) {
        int portIndex = pair.first;
        float value = pair.second;
        controlValues[portIndex] = value;
    }
}

void LV2PluginInstance::getControlOutValues
(std::map<int, float>& controlValues)
{
    controlValues.clear();
    for (auto& pair : m_controlPortsOut) {
        int portIndex = pair.first;
        float value = pair.second;
        controlValues[portIndex] = value;
    }
}

const LV2_Descriptor* LV2PluginInstance::getLV2Descriptor() const
{
    const LV2_Descriptor* desc = lilv_instance_get_descriptor(m_instance);
    return desc;
}

LV2_Handle LV2PluginInstance::getHandle() const
{
    LV2_Handle handle = lilv_instance_get_handle(m_instance);
    return handle;
}

int LV2PluginInstance::getSampleRate() const
{
    return m_sampleRate;
}

void LV2PluginInstance::audioProcessingDone()
{
    // the plugin should always be run to handle ui - plugin
    // communication. So if rosegarden is not playing or recording we
    // must run the plugin here.
    if (! m_pluginHasRun) {
        run(RealTime::fromSeconds(-100.0));
    }
    // reset the status
    m_pluginHasRun = false;
}

void LV2PluginInstance::getConnections
(PluginPortConnection::ConnectionList& clist) const
{
    // called from the gui thread
    RG_DEBUG << "getConnections";
    clist = m_connections;
}

void LV2PluginInstance::setConnections
(const PluginPortConnection::ConnectionList& clist)
{
    RG_DEBUG << "setConnections";
    for(auto& c : clist) {
        RG_DEBUG << c.isOutput << c.isAudio << c.pluginPort <<
            c.instrumentId << c.channel;
    }
    m_connections = clist;
}

LV2PluginInstance::~LV2PluginInstance()
{
    RG_DEBUG << "LV2PluginInstance::~LV2PluginInstance";
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->unRegisterPlugin(m_instrument, m_position);

    if (m_instance != nullptr) {
        deactivate();
    }

    cleanup();

    m_controlPortsIn.clear();
    m_controlPortsOut.clear();

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        delete[] m_inputBuffers[i];
    }
    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
        delete[] m_outputBuffers[i];
    }

    delete[] m_inputBuffers;
    delete[] m_outputBuffers;

    m_audioPortsIn.clear();
    m_audioPortsOut.clear();

    for (auto& aip : m_atomInputPorts) {
        delete[] aip.atomSeq;
    }
    m_atomInputPorts.clear();

    for (auto& aop : m_atomOutputPorts) {
        delete[] aop.atomSeq;
    }
    m_atomOutputPorts.clear();

    snd_midi_event_free(m_midiParser);
}

void
LV2PluginInstance::instantiate(unsigned long sampleRate)
{
    RG_DEBUG << "LV2PluginInstance::instantiate - plugin uri = "
             << m_uri;

    LilvNodes* feats = lilv_plugin_get_required_features(m_plugin);
    RG_DEBUG << "instantiate num features" <<  lilv_nodes_size(feats);
    LILV_FOREACH (nodes, i, feats) {
        const LilvNode* fnode = lilv_nodes_get(feats, i);
        RG_DEBUG << "feature:" << lilv_node_as_string(fnode);
    }
    lilv_nodes_free(feats);

    LV2Utils* lv2utils = LV2Utils::getInstance();

    m_uridMapFeature = {LV2_URID__map, &(lv2utils->m_map)};
    m_uridUnmapFeature = {LV2_URID__unmap, &(lv2utils->m_unmap)};

    m_workerSchedule.handle = &m_workerHandle;
    m_workerSchedule.schedule_work = lv2utils->getWorker()->getScheduler();
    RG_DEBUG << "schedule_work" << (void*)m_workerSchedule.schedule_work;
    m_workerFeature = {LV2_WORKER__schedule, &m_workerSchedule};

    LV2_URID nbl_urid = lv2utils->uridMap(LV2_BUF_SIZE__nominalBlockLength);
    LV2_URID mbl_urid = lv2utils->uridMap(LV2_BUF_SIZE__maxBlockLength);
    LV2_URID ai_urid = lv2utils->uridMap(LV2_ATOM__Int);
    LV2_Options_Option opt;
    opt.context = LV2_OPTIONS_INSTANCE;
    opt.subject = 0;
    opt.key = nbl_urid;
    opt.size = 4;
    opt.type = ai_urid;
    opt.value = &m_blockSize;
    m_options.push_back(opt);
    opt.key = mbl_urid;
    opt.value = &m_blockSize;
    m_options.push_back(opt);
    opt.key = 0;
    opt.type = 0;
    opt.value = 0;
    m_options.push_back(opt);
    m_optionsFeature = {LV2_OPTIONS__options, m_options.data()};

    m_features.push_back(&m_uridMapFeature);
    m_features.push_back(&m_uridUnmapFeature);
    m_features.push_back(&m_workerFeature);
    m_features.push_back(&m_optionsFeature);
    m_features.push_back(nullptr);

    m_instance =
        lilv_plugin_instantiate(m_plugin, sampleRate, m_features.data());
    if (!m_instance) {
        RG_WARNING << "Failed to instantiate plugin" << m_uri;
        return;
    }
}

void
LV2PluginInstance::activate()
{
    RG_DEBUG << "activate";
    lilv_instance_activate(m_instance);
}

void
LV2PluginInstance::connectPorts()
{
    RG_DEBUG << "connectPorts";
    size_t inbuf = 0, outbuf = 0;

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        if (m_audioPortsIn[i] == -1) continue;
        RG_DEBUG << "connect audio in:" << m_audioPortsIn[i];
        lilv_instance_connect_port(m_instance, m_audioPortsIn[i],
                                   m_inputBuffers[inbuf]);
        ++inbuf;
    }

    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
        if (m_audioPortsOut[i] == -1) continue;
        RG_DEBUG << "connect audio out:" << m_audioPortsOut[i];
        lilv_instance_connect_port(m_instance, m_audioPortsOut[i],
                                   m_outputBuffers[outbuf]);
        ++outbuf;
    }

    for (auto& pair : m_controlPortsIn) {
        RG_DEBUG << "connect control in:" <<
            pair.first;
        lilv_instance_connect_port
            (m_instance,
             pair.first,
             &pair.second);
    }

    for (auto& pair : m_controlPortsOut) {
        RG_DEBUG << "connect control out:" <<
            pair.first;
        lilv_instance_connect_port
            (m_instance,
             pair.first,
             &pair.second);
    }

    for (auto& aip : m_atomInputPorts) {
        RG_DEBUG << "connect atom in port" << aip.index;
        lilv_instance_connect_port(m_instance, aip.index, aip.atomSeq);
    }

    for (auto& aop : m_atomOutputPorts) {
        RG_DEBUG << "connect atom out port" << aop.index;
        lilv_instance_connect_port(m_instance, aop.index, aop.atomSeq);
    }

    // initialze the plugin state
    RG_DEBUG << "setting default state";
    LV2Utils* lv2utils = LV2Utils::getInstance();
    LilvState* defaultState = lv2utils->getDefaultStateByUri(m_uri);
    lilv_state_restore(defaultState,
                       m_instance,
                       setPortValueFunc,
                       this,
                       0,
                       m_features.data());
    lilv_state_free(defaultState);
    RG_DEBUG << "setting default state done";
}

void
LV2PluginInstance::setPortValue
(unsigned int portNumber, float value)
{
    RG_DEBUG << "setPortValue" << portNumber << value;
    auto it = m_controlPortsIn.find(portNumber);
   if (it == m_controlPortsIn.end()) {
        RG_DEBUG << "port not found";
        return;
    }
    if (value < m_pluginData.ports[portNumber].min)
        value = m_pluginData.ports[portNumber].min;
    if (value > m_pluginData.ports[portNumber].max)
        value = m_pluginData.ports[portNumber].max;
    m_controlPortsIn[portNumber] = value;
}

void LV2PluginInstance::setPortValue(const char *port_symbol,
                                     const void *value,
                                     uint32_t size,
                                     uint32_t type)
{
    RG_DEBUG << "setPortValue" << port_symbol;

    LV2Utils* lv2utils = LV2Utils::getInstance();
    LilvNode* symNode = lv2utils->makeStringNode(port_symbol);
    const LilvPort* port = lilv_plugin_get_port_by_symbol(m_plugin,
                                                          symNode);

    lilv_free(symNode);
    int index =  lilv_port_get_index(m_plugin, port);

    uint32_t floatType = lv2utils->uridMap(LV2_ATOM__Float);
    uint32_t intType = lv2utils->uridMap(LV2_ATOM__Int);
    if (size != 4) {
        RG_DEBUG << "bad size" << size;
        return;
    }
    if (type == floatType) {
        float fval = *((float*)value);
        RG_DEBUG << "setting float value" << fval;
        setPortValue(index, fval);
    } else if (type == intType) {
        int ival = *((int*)value);
        RG_DEBUG << "setting int value" << ival;
        setPortValue(index, (float)ival);
    } else {
        RG_DEBUG << "unknown type" << type <<lv2utils->uridUnmap(type);
    }
}

void
LV2PluginInstance::setPortByteArray(unsigned int port,
                                    unsigned int protocol,
                                    const QByteArray& ba)
{
    RG_DEBUG << "setPortByteArray" << port << protocol;
    if (protocol == m_atomTransferUrid) {
        for(auto& input : m_atomInputPorts) {
            AtomPort& ap = input;
            if (ap.index == port) {
                RG_DEBUG << "setPortByteArray send bytes" << ba.size() <<
                    ba.toHex() << protocol << ap.atomSeq;

                int ebs = sizeof(double);
                int bufsize = ba.size() + ebs;
                char buf[bufsize];
                LV2_Atom_Event* event = (LV2_Atom_Event*)buf;
                event->time.frames = 0;
                for(int ib=0; ib<ba.size(); ib++) {
                    buf[ebs + ib] = ba[ib];
                }

                lv2_atom_sequence_append_event(ap.atomSeq,
                                               8 * ABUFSIZED,
                                               event);
                return;
            }
        }
    } else {
        RG_DEBUG << "setPortValue unknown protocol" << protocol;
    }
}

float
LV2PluginInstance::getPortValue(unsigned int portNumber)
{
    auto it = m_controlPortsIn.find(portNumber);
    if (it == m_controlPortsIn.end()) {
        RG_DEBUG << "getPortValue port not found";
        return 0.0;
    }
    return (m_controlPortsIn[portNumber]);
}

void* LV2PluginInstance::getPortValue(const char *port_symbol,
                                      uint32_t *size,
                                      uint32_t *type)
{
    RG_DEBUG << "getPortValue from symbol:" << port_symbol;
    LV2Utils* lv2utils = LV2Utils::getInstance();
    int portIndex = lv2utils->getPortIndexFromSymbol(port_symbol, m_plugin);

    auto it = m_controlPortsIn.find(portIndex);
    if (it == m_controlPortsIn.end()) {
        RG_DEBUG << "control in not found" << portIndex;
        return nullptr;
    }
    static uint32_t portValueSize = 4;
    static uint32_t portValueType = lv2utils->uridMap(LV2_ATOM__Float);
    *size = portValueSize;
    *type = portValueType;
    return &((*it).second);
}

QString LV2PluginInstance::configure(const QString& key, const QString& value)
{
    RG_DEBUG << "configure" << key << value;
    if (key == "LV2State") {
        // set the state
        QString stateString = QByteArray::fromBase64(value.toUtf8());
        RG_DEBUG << "state string" << stateString;
        LV2Utils* lv2utils = LV2Utils::getInstance();
        lv2utils->setInstanceStateFromString(stateString,
                                             m_instance,
                                             setPortValueFunc,
                                             this,
                                             m_features.data());
    }
    if (key == "LV2Connections") {
        RG_DEBUG << "set connections" << value;
        QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());
        QJsonArray arr = doc.array();
        m_connections.clear();
        for (auto jct : arr)
            {
                QJsonArray arr1 = jct.toArray();
                QJsonValue oval = arr1[0];
                QJsonValue aval = arr1[1];
                QJsonValue pval = arr1[2];
                QJsonValue ival = arr1[3];
                QJsonValue cval = arr1[4];
                PluginPortConnection::Connection c;
                c.isOutput = oval.toBool();
                c.isAudio = aval.toBool();
                c.pluginPort = pval.toString();
                c.instrumentId = ival.toInt();
                c.channel = cval.toInt();
                m_connections.push_back(c);
            }
    }
    return "";
}

void LV2PluginInstance::savePluginState()
{
    // called in the gui thread
    RG_DEBUG << "savePluginState";
    LV2Utils* lv2utils = LV2Utils::getInstance();
    QString stateString = lv2utils->getStateFromInstance
        (m_plugin,
         m_uri,
         m_instance,
         getPortValueFunc,
         this,
         m_features.data());
    RG_DEBUG << "state string" << stateString;
    QString stateString64 = stateString.toUtf8().toBase64();
    RG_DEBUG << "state string base64" << stateString64;
    RosegardenMainWindow* mw = RosegardenMainWindow::self();
    mw->slotChangePluginConfiguration(m_instrument,
                                      m_position,
                                      false,
                                      "LV2State",
                                      stateString64);

    // also save the connections to a json string
    QJsonArray conns;
    for(auto& c : m_connections) {
        int ii = c.instrumentId;
        QJsonValue val1(c.isOutput);
        QJsonValue val2(c.isAudio);
        QJsonValue val3(c.pluginPort);
        QJsonValue val4(ii);
        QJsonValue val5(c.channel);
        QJsonArray arr;
        arr.append(val1);
        arr.append(val2);
        arr.append(val3);
        arr.append(val4);
        arr.append(val5);
        conns.append(arr);
    }
    QJsonDocument doc(conns);
    QString strJson(doc.toJson(QJsonDocument::Compact));
    RG_DEBUG << "connections:" << strJson;
    mw->slotChangePluginConfiguration(m_instrument,
                                      m_position,
                                      false,
                                      "LV2Connections",
                                      strJson);
}

void
LV2PluginInstance::sendEvent(const RealTime& eventTime,
                             const void* event)
{
    snd_seq_event_t *seqEvent = (snd_seq_event_t *)event;
    unsigned char buf[100];
    int bytes = snd_midi_event_decode(m_midiParser, buf, 100, seqEvent);
    if (bytes <= 0) {
        RG_DEBUG << "error decoding midi event";
    }
    QByteArray rawMidi;
    for(int irm=0; irm<bytes; irm++) {
        rawMidi.append(buf[irm]);
    }
    MidiEvent me;
    me.time = eventTime;
    me.data = rawMidi;
    m_eventBuffer.push_back(me);
    RG_DEBUG << "sendEvent" <<
        eventTime << rawMidi.toHex() << m_eventBuffer.size();
}

void
LV2PluginInstance::run(const RealTime &rt)
{
    m_pluginHasRun = true;
    LV2Utils* lv2utils = LV2Utils::getInstance();
    // get connected buffers
    int bufIndex = 0;
    for(auto& c : m_connections) {
        if (c.instrumentId != 0 && c.instrumentId != m_instrument) {
            auto ib = m_amixer->getAudioBuffer(c.instrumentId, c.channel);
            if (ib) {
                //RG_DEBUG << "copy" << c.instrumentId << c.channel <<
                //    "to port" <<
                //    lv2utils->getPortName(m_uri, m_audioPortsIn[bufIndex]);

                memcpy(m_inputBuffers[bufIndex],
                       ib,
                       m_blockSize * sizeof(sample_t));
            }
        }
        bufIndex++;
    }

    lv2utils->lock();

    RealTime bufferStart = rt;
    auto it = m_eventBuffer.begin();
    while(it != m_eventBuffer.end()) {
        RealTime evTime = (*it).time;
        QByteArray rawMidi = (*it).data;
        if (evTime < bufferStart) evTime = bufferStart;
        size_t frameOffset =
            (size_t)RealTime::realTime2Frame(evTime - bufferStart,
                                             m_sampleRate);
        if (frameOffset > m_blockSize) {
            it++;
            continue;
        }
        // the event is in this block
        RG_DEBUG << "send event to plugin" << evTime;
        auto iterToDelete = it;
        it++;
        m_eventBuffer.erase(iterToDelete);

        sendMidiData(rawMidi, frameOffset);
    }

    if (m_distributeChannels) {
        // the input buffers contain stereo channels - combine for a
        // mono plugin. If this is the case there are exactly 2 input
        // and output buffers. The first ones are connected to the
        // plugin
        //RG_DEBUG << "distribute stereo -> mono";
        for (size_t i = 0; i < m_blockSize; ++i) {
                    m_inputBuffers[0][i] =
                        (m_inputBuffers[0][i] + m_inputBuffers[1][i]) / 2.0;
        }
    }

    // init atom out
    for(auto& ap : m_atomOutputPorts) {
        LV2_Atom_Sequence* aseq = ap.atomSeq;
        aseq->atom.size = 8 * ABUFSIZED - 8;
    }

    lilv_instance_run(m_instance, m_blockSize);

    // get any worker responses
    LV2Utils::Worker* worker = lv2utils->getWorker();

    LV2Utils::PluginPosition pp;
    pp.instrument = m_instrument;
    pp.position = m_position;
    if (m_workerInterface && worker) {
        while(LV2Utils::WorkerJob* job = worker->getResponse(pp)) {
            m_workerInterface->work_response(m_instance, job->size, job->data);
            delete job;
        }

        if (m_workerInterface->end_run) m_workerInterface->end_run(m_instance);
    }

    // clear atom in buffers
    for(auto& input : m_atomInputPorts) {
        AtomPort& ap = input;
        lv2_atom_sequence_clear(ap.atomSeq);
    }

    // get atom out data
    for(auto& ap : m_atomOutputPorts) {
        //RG_DEBUG << "check atom out" << ap.index;
        LV2_Atom_Sequence* aseq = ap.atomSeq;
        LV2_ATOM_SEQUENCE_FOREACH(aseq, ev) {
           if (ev->body.type == m_midiEventUrid) {
                // midi out not used
            } else {
                //RG_DEBUG << "updatePortValue";
                lv2utils->updatePortValue(m_instrument,
                                          m_position,
                                          ap.index,
                                          &(ev->body));
            }
        }
        // and clear the buffer
        lv2_atom_sequence_clear(aseq);
    }

    if (m_distributeChannels) {
        // after running distribute the output to the two channels
        for (size_t i = 0; i < m_blockSize; ++i) {
            m_outputBuffers[1][i] = m_outputBuffers[0][i];
        }
    }

    m_run = true;
    lv2utils->unlock();
}

void
LV2PluginInstance::deactivate()
{
    lilv_instance_deactivate(m_instance);
}

void
LV2PluginInstance::cleanup()
{
    lilv_instance_free(m_instance);
}

void LV2PluginInstance::sendMidiData(const QByteArray& rawMidi,
                                     size_t frameOffset)
{
    RG_DEBUG << "sendMidiData" << frameOffset << rawMidi.toHex();
    char midiBuf[1000];
    LV2_Atom_Event* event = (LV2_Atom_Event*)midiBuf;
    event->time.frames = frameOffset;
    event->body.size = rawMidi.size();
    event->body.type = m_midiEventUrid;

    int ebs = sizeof(LV2_Atom_Event);
    for(int ib=0; ib<rawMidi.size(); ib++) {
        midiBuf[ebs + ib] = rawMidi[ib];
    }
    for (auto& aip : m_atomInputPorts) {
        if (aip.isMidi) {
            LV2_Atom_Event* atom =
                lv2_atom_sequence_append_event(aip.atomSeq,
                                               8 * ABUFSIZED,
                                               event);
            if (atom == nullptr) {
                RG_DEBUG << "midi event overflow";
            }
        }
    }
}

}
