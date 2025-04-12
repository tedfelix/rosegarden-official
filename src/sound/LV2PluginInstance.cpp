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

#define RG_MODULE_STRING "[LV2PluginInstance]"
#define RG_NO_DEBUG_PRINT 1

//#define LV2RUN_PROFILE 1

#include "LV2PluginInstance.h"

#include "LV2Worker.h"
#include "LV2Utils.h"
#include "LV2URIDMapper.h"
#include "PluginAudioSource.h"
#include "gui/studio/AudioPluginLV2GUI.h"

#include "base/Profiler.h"
#include "misc/Debug.h"
#include "sound/Midi.h"
#include "sound/AudioInstrumentMixer.h"  // For AudioInstrumentMixer
#include "gui/application/RosegardenMainWindow.h"

#include <lv2/midi/midi.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/atom/forge.h>
#include <lv2/urid/urid.h>
#include <lv2/patch/patch.h>

//#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>

#include <unistd.h>  // gettid()


namespace
{
    // ??? Why does this expect a size and type when it's always float?
    const void* getPortValueFunc(const char *port_symbol,
                                 void *user_data,
                                 uint32_t *size,
                                 uint32_t *type)
    {
        Rosegarden::LV2PluginInstance* pi =
            (Rosegarden::LV2PluginInstance*)user_data;
        return pi->getPortValue(port_symbol, size, type);
    }

    // ??? Why does this expect a size and type when it's always float?
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


LV2PluginInstance::LV2PluginInstance(
        PluginFactory *factory,
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
    m_amixer(amixer),
    m_profilerName("LV2: " + m_uri.toStdString()),
    m_eventsDiscarded(false),
    m_pluginState(nullptr)
{
#ifdef THREAD_DEBUG
    RG_WARNING << "LV2PluginInstance: gettid(): " << gettid();
#endif

    RG_DEBUG << "create plugin" << uri << m_instrument << m_position;

    m_atomTransferUrid = LV2URIDMapper::uridMap(LV2_ATOM__eventTransfer);

    m_workerHandle.instrument = instrument;
    m_workerHandle.position = position;

    m_pluginData = LV2PluginDatabase::getPluginData(uri);

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

    LV2Utils* lv2utils = LV2Utils::getInstance();
    m_plugin = lv2utils->getPluginByUri(m_uri);

    snd_midi_event_new(100, &m_midiParser);
    snd_midi_event_no_status(m_midiParser, 1); // disable merging

    m_midiEventUrid = LV2URIDMapper::uridMap(LV2_MIDI__MidiEvent);

    setupFeatures();

    instantiate(sampleRate);
    if (! isOK()) return;

    connectPorts();
    activate();

    m_workerInterface = (LV2_Worker_Interface*)
        lilv_instance_get_extension_data(m_instance, LV2_WORKER__interface);
    RG_DEBUG << "worker interface" << m_workerInterface;
    if (m_workerInterface) RG_DEBUG << (void*)m_workerInterface->work <<
                               (void*)m_workerInterface->work_response <<
                               (void*)m_workerInterface->end_run;

    // parameters
    lv2utils->setupPluginParameters(m_uri, m_params);
    for(auto& param : m_params) {
        const LV2PluginParameter& pd = param.second;
        RG_DEBUG << "param" << param.first << pd.isReadable() <<
            pd.isWritable() << pd.getLabel() << (int)pd.getType();
    }

    // presets
    lv2utils->setupPluginPresets(m_uri, m_presets);
}

void
LV2PluginInstance::init(int idealChannelCount)
{
    RG_DEBUG << "LV2PluginInstance::init(" << idealChannelCount <<
        "): plugin has" << m_pluginData.ports.size() << "ports";

    m_channelCount = idealChannelCount;

    // Discover ports numbers and identities
    //
    for (unsigned long i = 0; i < m_pluginData.ports.size(); ++i) {
        const LV2PluginDatabase::LV2PortData& portData = m_pluginData.ports[i];
        switch(portData.portType) {
        case LV2PluginDatabase::LV2AUDIO:
            if (portData.isInput) {
                RG_DEBUG << "LV2PluginInstance::init: port" << i << "is audio in";
                m_audioPortsIn.push_back(i);
            } else {
                RG_DEBUG << "LV2PluginInstance::init: port" << i << "is audio out";
                m_audioPortsOut.push_back(i);
            }
            break;
        case LV2PluginDatabase::LV2CONTROL:
        case LV2PluginDatabase::LV2MIDI:
            if (portData.portProtocol == LV2PluginDatabase::LV2ATOM) {
                if (portData.isInput) {
                    RG_DEBUG << "Atom in port" << i << portData.name;
                    AtomPort ap;
                    ap.index = i;
                    ap.isMidi =
                        (portData.portType == LV2PluginDatabase::LV2MIDI);
                    ap.isPatch = portData.isPatch;
                    // create the atom port
                    // use double to get 64 bit alignment
                    double* dbuf = new double[ABUFSIZED];
                    bzero(dbuf, 8 * ABUFSIZED);
                    LV2_Atom_Sequence* atomIn =
                        reinterpret_cast<LV2_Atom_Sequence*>(dbuf);
                    lv2_atom_sequence_clear(atomIn);
                    LV2_URID type = LV2URIDMapper::uridMap(LV2_ATOM__Sequence);
                    atomIn->atom.type = type;
                    ap.atomSeq = atomIn;
                    RG_DEBUG << "created atom sequence" << atomIn;
                    m_atomInputPorts.push_back(ap);
                } else {
                    RG_DEBUG << "Atom out port" << i << portData.name;
                    AtomPort ap;
                    ap.index = i;
                    ap.isMidi =
                        (portData.portType == LV2PluginDatabase::LV2MIDI);
                    ap.isPatch = portData.isPatch;
                    // create the atom port
                    // use double to get 64 bit alignment
                    double* dbuf = new double[ABUFSIZED];
                    bzero(dbuf, 8 * ABUFSIZED);
                    LV2_Atom_Sequence* atomOut =
                        reinterpret_cast<LV2_Atom_Sequence*>(dbuf);
                    lv2_atom_sequence_clear(atomOut);
                    // to tell the plugin the capacity
                    LV2_URID type = LV2URIDMapper::uridMap(LV2_ATOM__Sequence);
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
                    m_controlPortsIn[i] = portData.def;
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
    // if we have a stereo channel and a mono plugin or a mono
    // instrument we combine the stereo input to the mono plugin and
    // distribute the plugin output to the stereo channels !
    m_distributeChannels = false;
    if (m_audioPortsIn.size() <= 1 &&
        m_audioPortsOut.size() == 1 &&
        m_channelCount == 2)
        {
            if (m_audioPortsIn.size() == 1) m_audioPortsIn.push_back(-1);
            m_audioPortsOut.push_back(-1);
            m_distributeChannels = true;
        }

    // set up the default connections
    m_connections.baseInstrument = m_instrument;
    m_connections.numChannels = m_channelCount;
    m_connections.connections.clear();
    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        if (m_audioPortsIn[i] == -1) continue; // for distributeChannels
        PluginPort::Connection c;
        c.isOutput = false;
        c.isAudio = true;
        c.portIndex = m_audioPortsIn[i];
        c.pluginPort =
            LV2PluginDatabase::getPortName(m_uri, m_audioPortsIn[i]);
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
        m_connections.connections.push_back(c);
    }
    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
        if (m_audioPortsOut[i] == -1) continue; // for distributeChannels
        PluginPort::Connection c;
        c.isOutput = true;
        c.isAudio = true;
        c.portIndex = m_audioPortsOut[i];
        c.pluginPort =
            LV2PluginDatabase::getPortName(m_uri, m_audioPortsOut[i]);
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
        m_connections.connections.push_back(c);
    }
    RG_DEBUG << "connections:" << m_connections.connections.size();
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
    RG_DEBUG << "discardEvents()";

    {
        QMutexLocker lock(&m_eventBufferMutex);
        m_eventBuffer.clear();
    }

    // it is not always enough just to clear the buffer. If notes are
    // playing they should be stopped with all notes off
    unsigned char status = 0xb0;
    unsigned char data1 = MIDI_CONTROLLER_ALL_NOTES_OFF;
    unsigned char data2 = 0;
    // channel 0
    QByteArray rawMidi;
    // channel 0
    rawMidi.append(status + 0);
    rawMidi.append(data1);
    rawMidi.append(data2);
    sendMidiData(rawMidi, 0);
    m_eventsDiscarded = true;
}

void
LV2PluginInstance::setIdealChannelCount(size_t channels)
{
    RG_DEBUG << "setIdealChannelCount" << channels << m_channelCount;

    if (channels == m_channelCount) return;
    m_channelCount = channels;
    m_connections.numChannels = m_channelCount;

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
(std::map<int, float>& controlValues) const
{
    controlValues.clear();
    for (const auto& pair : m_controlPortsIn) {
        int portIndex = pair.first;
        float value = pair.second;
        controlValues[portIndex] = value;
    }
}

void LV2PluginInstance::getControlOutValues
(std::map<int, float>& controlValues) const
{
    controlValues.clear();
    for (const auto& pair : m_controlPortsOut) {
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
    // must run the plugin here. A soft synth always runs anyway

    bool isSoftSynth = (m_instrument >= SoftSynthInstrumentBase);

    //RG_DEBUG << "audioProcessingDone" << isSoftSynth << m_pluginHasRun;
    if (! m_pluginHasRun && ! isSoftSynth) {
        run(RealTime::fromSeconds(-100.0));
    }
    // reset the status
    m_pluginHasRun = false;
}

void LV2PluginInstance::getConnections
(PluginPort::ConnectionList& clist) const
{
    // called from the gui thread
    RG_DEBUG << "getConnections";
    clist = m_connections;
}

void LV2PluginInstance::setConnections
(const PluginPort::ConnectionList& clist)
{
#ifndef NDEBUG
    RG_DEBUG << "setConnections";
    for(const auto& c : clist.connections) {
        RG_DEBUG << c.isOutput << c.isAudio << c.pluginPort <<
            c.instrumentId << c.channel;
    }
#endif

    m_connections = clist;
}

bool LV2PluginInstance::hasParameters() const
{
    return (m_params.size() > 0);
}

void LV2PluginInstance::getParameters
(AudioPluginInstance::PluginParameters& params) const
{
    params.clear();
    for (const auto& pair : m_params) {
        const QString& paramUri = pair.first;
        const LV2PluginParameter& lparam = pair.second;
        AudioPluginInstance::PluginParameter param;
        // fill the params struct
        param.type = lparam.getType();
        param.readable = lparam.isReadable();
        param.writable = lparam.isWritable();
        param.label = lparam.getLabel();
        if (lparam.isValueSet()) {
            switch(param.type) {
            case AudioPluginInstance::ParameterType::BOOL:
                param.value = lparam.getBool();
                break;
            case AudioPluginInstance::ParameterType::INT:
                param.value = lparam.getInt();
                break;
            case AudioPluginInstance::ParameterType::LONG:
                param.value = (int)lparam.getLong();
                break;
            case AudioPluginInstance::ParameterType::FLOAT:
                param.value = lparam.getFloat();
                break;
            case AudioPluginInstance::ParameterType::DOUBLE:
                param.value = lparam.getDouble();
                break;
            case AudioPluginInstance::ParameterType::STRING:
                param.value = lparam.getString();
                break;
            case AudioPluginInstance::ParameterType::PATH:
                param.value = lparam.getPath();
                break;
            case AudioPluginInstance::ParameterType::UNKNOWN:
            default:
                break;
            }
        }
        params[paramUri] = param;
    }
}

void LV2PluginInstance::updatePluginParameter
(const QString& paramId,
 const AudioPluginInstance::PluginParameter& param)
{
    RG_DEBUG << "updatePluginParameter" << paramId << param.label;
    // save the parameter
    LV2PluginParameter& lparam = m_params[paramId];
    // only the value can change
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (param.value.typeId() != QVariant::Invalid) {
#else
    if (param.value.type() != QVariant::Invalid) {
#endif
        switch(param.type) {
        case AudioPluginInstance::ParameterType::BOOL:
            lparam.setBool(param.value.toBool());
            break;
        case AudioPluginInstance::ParameterType::INT:
            lparam.setInt(param.value.toInt());
            break;
        case AudioPluginInstance::ParameterType::LONG:
            lparam.setLong(param.value.toInt());
            break;
        case AudioPluginInstance::ParameterType::FLOAT:
            lparam.setFloat(param.value.toFloat());
            break;
        case AudioPluginInstance::ParameterType::DOUBLE:
            lparam.setDouble(param.value.toDouble());
            break;
        case AudioPluginInstance::ParameterType::STRING:
            lparam.setString(param.value.toString());
            break;
        case AudioPluginInstance::ParameterType::PATH:
            lparam.setPath(param.value.toString());
            break;
        case AudioPluginInstance::ParameterType::UNKNOWN:
        default:
            break;
        }
    }
    // send to plugin
    sendPluginParameter(lparam);
}

void LV2PluginInstance::sendPluginParameter(const LV2PluginParameter& lparam)
{
    // send to the plugin
    LV2_Atom_Forge forge;
    lv2_atom_forge_init(&forge, LV2URIDMapper::getURIDMapFeature());
    uint8_t buffer[2000];
    lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));
    LV2_Atom_Forge_Frame frame;

    LV2_URID setUrid = LV2URIDMapper::uridMap(LV2_PATCH__Set);
    LV2_URID patchPropertyUrid = LV2URIDMapper::uridMap(LV2_PATCH__property);
    LV2_URID patchValueUrid = LV2URIDMapper::uridMap(LV2_PATCH__value);
    LV2_URID paramUrid = lparam.getParameterUrid();

    lv2_atom_forge_object(&forge, &frame, 0, setUrid);
    lv2_atom_forge_key(&forge, patchPropertyUrid);
    lv2_atom_forge_urid(&forge, paramUrid);
    lv2_atom_forge_key(&forge, patchValueUrid);
    // the value atom
    const LV2_Atom* valueAtom = lparam.getValue();
    int valueSize = lv2_atom_total_size(valueAtom);
    lv2_atom_forge_raw(&forge, valueAtom, valueSize);

    const LV2_Atom *atom = lv2_atom_forge_deref(&forge, frame.ref);

    lv2_atom_forge_pop(&forge, &frame);
    QByteArray ba((char*)atom, lv2_atom_total_size(atom));
    RG_DEBUG << "updatePluginParameter" << ba.toHex();

    for(auto& input : m_atomInputPorts) {
        AtomPort& ap = input;
        if (ap.isPatch) {
            RG_DEBUG << "sending set patch to port" << ap.index;
            setPortByteArray(ap.index, m_atomTransferUrid, ba);
        }
    }
}

void LV2PluginInstance::getPresets
(AudioPluginInstance::PluginPresetList& presets) const
{
    presets = m_presets;
}

void LV2PluginInstance::setPreset(const QString& uri)
{
    RG_DEBUG << "setPreset" << uri;
    LV2Utils* lv2utils = LV2Utils::getInstance();
    LilvNode* presetUri = lv2utils->makeURINode(uri);
    char* filename = lilv_file_uri_parse(qPrintable(uri), nullptr);
    LilvState* presetState;
    if (filename) {
        // check if it is really a file
        const QFileInfo fileInfo(filename);
        bool isFile = (fileInfo.exists() && fileInfo.isFile());
        if (! isFile) filename = nullptr;
    }

    if (filename) {
        RG_DEBUG << "got filename" << filename;
        presetState = lv2utils->getStateFromFile(presetUri, filename);
        lilv_free(filename);
    } else {
        presetState = lv2utils->getStateByUri(uri);
    }

    lilv_state_restore(presetState,
                       m_instance,
                       setPortValueFunc,
                       this,
                       0,
                       m_features.data());

    lilv_state_free(presetState);
    lilv_free(presetUri);
}

void LV2PluginInstance::loadPreset(const QString& file)
{
    RG_DEBUG << "loadPreset" << file;
    LV2Utils* lv2utils = LV2Utils::getInstance();
    LilvState* state = lv2utils->getStateFromFile(nullptr, file);
    if (state == nullptr) {
        RG_DEBUG << "load failed";
        return;
    }

    lilv_state_restore(state,
                       m_instance,
                       setPortValueFunc,
                       this,
                       0,
                       m_features.data());

    lilv_state_free(state);
}

void LV2PluginInstance::savePreset(const QString& file)
{
    RG_DEBUG << "savePreset" << file;
    LV2Utils* lv2utils = LV2Utils::getInstance();
    LilvState* state = lv2utils->getStateFromInstance
        (m_plugin,
         m_instance,
         getPortValueFunc,
         this,
         m_features.data());
    lv2utils->saveStateToFile(state, file);
    lilv_state_free(state);
}

LV2PluginInstance::~LV2PluginInstance()
{
    RG_DEBUG << "LV2PluginInstance::~LV2PluginInstance" << m_uri;

    if (m_instance != nullptr) {
        deactivate();
    }

    cleanup();
    if (m_pluginState) lilv_state_free(m_pluginState);

    for (auto& pair : m_audioSources) {
        pair.second->pluginFinished();
    }
    m_audioSources.clear();

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

    for (const auto& aip : m_atomInputPorts) {
        delete[] aip.atomSeq;
    }
    m_atomInputPorts.clear();

    for (const auto& aop : m_atomOutputPorts) {
        delete[] aop.atomSeq;
    }
    m_atomOutputPorts.clear();

    snd_midi_event_free(m_midiParser);
#ifdef LV2RUN_PROFILE
    Profiles::getInstance()->dump();
#endif
}

void
LV2PluginInstance::instantiate(unsigned long sampleRate)
{
    RG_DEBUG << "LV2PluginInstance::instantiate - plugin uri = "
             << m_uri;

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

    // For each audio in port...
    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        if (m_audioPortsIn[i] == -1) continue;
        RG_DEBUG << "connect audio in:" << m_audioPortsIn[i];
        lilv_instance_connect_port(m_instance, m_audioPortsIn[i],
                                   m_inputBuffers[inbuf]);
        ++inbuf;
    }

    // For each audio out port...
    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
        if (m_audioPortsOut[i] == -1) continue;
        RG_DEBUG << "connect audio out:" << m_audioPortsOut[i];
        lilv_instance_connect_port(m_instance, m_audioPortsOut[i],
                                   m_outputBuffers[outbuf]);
        ++outbuf;
    }

    // For each control in port...
    for (auto& pair : m_controlPortsIn) {
        RG_DEBUG << "connect control in:" <<
            pair.first;
        lilv_instance_connect_port
            (m_instance,
             pair.first,
             &pair.second);
    }

    // For each control out port...
    for (auto& pair : m_controlPortsOut) {
        RG_DEBUG << "connect control out:" <<
            pair.first;
        lilv_instance_connect_port
            (m_instance,
             pair.first,
             &pair.second);
    }

    // For each atom in port...
    for (const auto& aip : m_atomInputPorts) {
        RG_DEBUG << "connect atom in port" << aip.index;
        lilv_instance_connect_port(m_instance, aip.index, aip.atomSeq);
    }

    // For each atom out port...
    for (const auto& aop : m_atomOutputPorts) {
        RG_DEBUG << "connect atom out port" << aop.index;
        lilv_instance_connect_port(m_instance, aop.index, aop.atomSeq);
    }

}

void
LV2PluginInstance::setPortValue
(unsigned int portNumber, float value)
{
    RG_DEBUG << "setPortValue" << portNumber << value;
    const auto it = m_controlPortsIn.find(portNumber);
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
    if (!port) return;

    int index =  lilv_port_get_index(m_plugin, port);

    uint32_t floatType = LV2URIDMapper::uridMap(LV2_ATOM__Float);
    uint32_t intType = LV2URIDMapper::uridMap(LV2_ATOM__Int);
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
        RG_DEBUG << "unknown type" << type << LV2URIDMapper::uridUnmap(type);
    }
}

void
LV2PluginInstance::setPortByteArray(unsigned int port,
                                    unsigned int protocol,
                                    const QByteArray& ba) const
{
    RG_DEBUG << "setPortByteArray" << port << protocol;
    if (protocol == m_atomTransferUrid) {
        QMutexLocker lock(&m_atomInputMutex);
        for (const AtomPort& ap : m_atomInputPorts) {
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
    const auto it = m_controlPortsIn.find(portNumber);
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

    const auto it = m_controlPortsIn.find(portIndex);
    if (it == m_controlPortsIn.end()) {
        RG_DEBUG << "control in not found" << portIndex;
        return nullptr;
    }
    static uint32_t portValueSize = 4;
    static uint32_t portValueType = LV2URIDMapper::uridMap(LV2_ATOM__Float);
    *size = portValueSize;
    *type = portValueType;
    return &(it->second);
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
        m_connections.connections.clear();
        for (const QJsonValue &jct : arr) {
            QJsonArray arr1 = jct.toArray();
            QJsonValue val1 = arr1[0];
            QJsonValue val2 = arr1[1];
            QJsonValue val3 = arr1[2];
            QJsonValue val4 = arr1[3];
            QJsonValue val5 = arr1[4];
            QJsonValue val6 = arr1[5];
            PluginPort::Connection c;
            c.isOutput = val1.toBool();
            c.isAudio = val2.toBool();
            c.portIndex = val3.toInt();
            c.pluginPort = val4.toString();
            c.instrumentId = val5.toInt();
            c.channel = val6.toInt();
            m_connections.connections.push_back(c);
        }
    }

    if (key == "LV2Parameters") {
        RG_DEBUG << "set parameters" << value;
        QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());
        QJsonArray arr = doc.array();
        for (const QJsonValue &jct : arr) {
            QJsonArray arr1 = jct.toArray();
            QJsonValue uriVal = arr1[0];
            QJsonValue valVal = arr1[1];
            QString paramUri = uriVal.toString();
            QString valueString = valVal.toString();
            LV2PluginParameter& param = m_params[paramUri];
            param.setValueFromString(valueString);
            sendPluginParameter(param);
        }
    }

    return "";
}

void LV2PluginInstance::savePluginState()
{
    // called in the gui thread
    RG_DEBUG << "savePluginState";
    LV2Utils* lv2utils = LV2Utils::getInstance();
    QString stateString = lv2utils->getStateStringFromInstance
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
    for (const PluginPort::Connection &c : m_connections.connections) {
        int ii = c.instrumentId;
        QJsonValue val1(c.isOutput);
        QJsonValue val2(c.isAudio);
        QJsonValue val3(c.portIndex);
        QJsonValue val4(c.pluginPort);
        QJsonValue val5(ii);
        QJsonValue val6(c.channel);
        QJsonArray arr;
        arr.append(val1);
        arr.append(val2);
        arr.append(val3);
        arr.append(val4);
        arr.append(val5);
        arr.append(val6);
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

    // now save the parameters
    QJsonArray params;
    for (const auto& pair : m_params) {
        const QString& paramUri = pair.first;
        const LV2PluginParameter& param = pair.second;

        if (param.isValueSet()) {
            QString valStr = param.getValueAsString();

            QJsonValue val1(paramUri);
            QJsonValue val2(valStr);
            QJsonArray arr;
            arr.append(val1);
            arr.append(val2);
            params.append(arr);
        }
    }
    QJsonDocument docp(params);
    QString strJsonp(docp.toJson(QJsonDocument::Compact));
    RG_DEBUG << "params:" << strJsonp;
    mw->slotChangePluginConfiguration(m_instrument,
                                      m_position,
                                      false,
                                      "LV2Parameters",
                                      strJsonp);
}

void LV2PluginInstance::getPluginPlayableAudio
(std::vector<PlayableData*>& playable)
{
    RG_DEBUG << "getPluginPlayableAudio";
    for (const PluginPort::Connection &c : m_connections.connections) {
        RG_DEBUG << "getPluginPlayableAudio" << c.isOutput << c.isAudio <<
            c.portIndex << c.pluginPort << c.instrumentId <<
            c.channel;
        if (c.instrumentId != 0 &&
            c.instrumentId != m_instrument &&
            c.isOutput == true) {
            PluginAudioSource *pas = new PluginAudioSource(this,
                                                           c.instrumentId,
                                                           c.portIndex,
                                                           c.channel,
                                                           m_blockSize);
            playable.push_back(pas);
            m_audioSources[c.portIndex] = pas;
        }
    }
    RG_DEBUG << "getPluginPlayableAudio sources:" << m_audioSources.size();
}

void LV2PluginInstance::removeAudioSource(int portIndex)
{
    RG_DEBUG << "removeAudioSource" << portIndex;
    // the PluginAudioSource is deleted elsewhere
    m_audioSources.erase(portIndex);
}

void
LV2PluginInstance::sendEvent(const RealTime& eventTime,
                             const void* event)
{
    snd_seq_event_t *seqEvent = (snd_seq_event_t *)event;
    snd_seq_event_t ev(*seqEvent);
    unsigned char buf[100];
    int bytes = snd_midi_event_decode(m_midiParser, buf, 100, &ev);
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
    {
        QMutexLocker lock(&m_eventBufferMutex);
        m_eventBuffer.push_back(me);

#ifndef NDEBUG
        RG_DEBUG << "sendEvent" <<
            ev.type << eventTime << rawMidi.toHex() << m_eventBuffer.size();
#endif
    }

}

void
LV2PluginInstance::run(const RealTime &rt)
{
    // Audio Thread.

#ifdef LV2RUN_PROFILE
    Profiler profiler(m_profilerName.c_str(), true);
#endif

    if (!m_instance)
        return;
    // ??? @lman
    // ??? Seeing some crashes in the lilv_instance_run() call below.
    //     This usually catches it, but not always.
    if (!m_instance->lv2_descriptor)
        return;

    //RG_DEBUG << "run" << rt << m_eventsDiscarded;
    m_pluginHasRun = true;

    // Get connected buffers.
    int bufIndex = 0;
    for (const PluginPort::Connection &c : m_connections.connections) {
        if (c.instrumentId != 0 &&
            c.instrumentId != m_instrument &&
            c.isOutput == false) {
            auto ib = m_amixer->getAudioBuffer(c.instrumentId, c.channel);
            if (ib) {
                //RG_DEBUG << "copy" << c.instrumentId << c.channel;

                memcpy(m_inputBuffers[bufIndex],
                       ib,
                       m_blockSize * sizeof(sample_t));
            }
        }
        bufIndex++;
    }

    RealTime bufferStart = rt;

    {
        QMutexLocker lock(&m_eventBufferMutex);

        auto it = m_eventBuffer.begin();
        // Send each event.
        while(it != m_eventBuffer.end()) {
            RealTime evTime = (*it).time;
            QByteArray rawMidi = (*it).data;
            if (evTime < bufferStart) evTime = bufferStart;
            size_t frameOffset =
                (size_t)RealTime::realTime2Frame(evTime - bufferStart,
                                                 m_sampleRate);
            if (frameOffset >= m_blockSize) {
                //RG_DEBUG << "event not in frame" << frameOffset;
                it++;
                continue;
            }
            // the event is in this block
            RG_DEBUG << "send event to plugin" << evTime;
            auto iterToDelete = it;
            ++it;
            m_eventBuffer.erase(iterToDelete);

            // if we have just been reset with discardEvents make sure we
            // send this data after the "stop all notes"
            if (m_eventsDiscarded && frameOffset == 0) {
                RG_DEBUG << "adjusting frameOffset to be after all notes off";
                frameOffset = 1;
            }
            sendMidiData(rawMidi, frameOffset);
        }
    }

    if (m_distributeChannels && m_audioPortsIn.size() > 1) {
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
    for (const AtomPort &ap : m_atomOutputPorts) {
        LV2_Atom_Sequence* aseq = ap.atomSeq;
        aseq->atom.size = 8 * ABUFSIZED - 8;
    }

    // ??? @lman
    //     I'm seeing sigsegv's here.  m_instance->lv2_descriptor appears to
    //     be the culprit.  Usually the address is zero, but sometimes it is
    //     very low like 0x20.  I'm using the sidechain on the ACE compressor
    //     and this seems to happen 100% on first load of an .rg file.  Or
    //     when loading after File > New.  Reloading over top of itself seems
    //     to fix the problem.  This appears to only happen with connections.
    //     My normal test case with several effects and synths works fine.
    //
    //     See the "if" above for m_instance->lv2_descriptor.  Remove it
    //     to see the crashes.
    {
        QMutexLocker lock(&m_atomInputMutex);
        lilv_instance_run(m_instance, m_blockSize);
        // clear atom in buffers
        for (const AtomPort &ap : m_atomInputPorts) {
            lv2_atom_sequence_clear(ap.atomSeq);
        }
    }

    for (const AtomPort &ap : m_atomOutputPorts) {
        if (ap.atomSeq->atom.size == 8 * ABUFSIZED - 8) {
            // if the plugin has not touched the output buffer it
            // should be reset
            RG_DEBUG << "run resetting unused buffer" << ap.index;
            lv2_atom_sequence_clear(ap.atomSeq);
        }
    }

    /*
    // debug
    if (m_audioPortsOut.size() > 0) {
        double vmin = 1.0e10;
        double vmax = -1.0e10;
        double vms = 0.0;
        for (size_t i = 0; i < m_blockSize; ++i) {
            float val = m_outputBuffers[0][i];
            if (val < vmin) vmin = val;
            if (val > vmax) vmax = val;
            vms += val * val;
        }
        RG_DEBUG << "run - output:" << vmin << vmax << sqrt(vms);
    }
    */

    // If the plugin provides a worker interface, process any responses.
    if (m_workerInterface) {
        LV2Utils::PluginPosition pp;
        pp.instrument = m_instrument;
        pp.position = m_position;

        const LV2_Handle handle = lilv_instance_get_handle(m_instance);

        // For each available response...
        while (const LV2Worker::WorkerData *response =
                       LV2Worker::getInstance()->getResponse(pp)) {
            // Pass it to the plugin via the worker interface.
            m_workerInterface->work_response(
                    handle, response->size, response->data);

            delete[] static_cast<const char *>(response->data);
            delete response;
        }

        if (m_workerInterface->end_run) m_workerInterface->end_run(m_instance);
    }

    // get atom out data
    for (const AtomPort &ap : m_atomOutputPorts) {
        //RG_DEBUG << "check atom out" << ap.index;
        LV2_Atom_Sequence* aseq = ap.atomSeq;
        LV2_ATOM_SEQUENCE_FOREACH(aseq, ev) {
            if (ev->body.type == m_midiEventUrid) {
                // midi out not used
            } else {
                //RG_DEBUG << "updatePortValue";
                if (ev->body.type != 0) {
                    updatePortValue(ap.index, &(ev->body));
                }
            }
        }
        // and clear the buffer
        lv2_atom_sequence_clear(aseq);
    }

    // check for mono plugin on stereo channel or mono synth instrument
    if (m_distributeChannels) {
        // after running distribute the output to the two channels
        for (size_t i = 0; i < m_blockSize; ++i) {
            m_outputBuffers[1][i] = m_outputBuffers[0][i];
        }
    }

    // pass any required audio output to the virtual audio sources
    // For each audio out port...
    int outbuf = 0;
    int portIndex0 = 0;
    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
        if (m_audioPortsOut[i] == -1) continue;
        int portIndex = m_audioPortsOut[i];
        if (i == 0) portIndex0 = portIndex;
        auto iter = m_audioSources.find(portIndex);
        if (iter != m_audioSources.end()) {
            //double ms = 0.0;
            //for (unsigned int si=0; si<m_blockSize; si++)
            //  ms += m_outputBuffers[outbuf][si] * m_outputBuffers[outbuf][si];
            //RG_DEBUG << "send data to audio source for port" << portIndex <<
            //    outbuf << ms;
            PluginAudioSource* pas = (*iter).second;
            if (pas) pas->setAudioData(m_outputBuffers[outbuf]);
            if (i == 1) {
                // The right channel is sent to another instrument so
                // it should not be played here. Check the connection
                // to decide what to do with the left channel
                for (const PluginPort::Connection &c :
                         m_connections.connections) {
                    if (c.portIndex != portIndex0) continue;
                    if (c.channel == 0) {
                        // only play the left channel - mute right
                        bzero(m_outputBuffers[1],
                              m_blockSize * sizeof(sample_t));
                    }
                    if (c.channel == 1) {
                        // play the left channel on the right !
                        memcpy(m_outputBuffers[1],
                               m_outputBuffers[0],
                               m_blockSize * sizeof(sample_t));
                        // mute left
                        bzero(m_outputBuffers[0],
                              m_blockSize * sizeof(sample_t));
                    }
                    if (c.channel == -1) {
                        // play the left channel on both
                        memcpy(m_outputBuffers[1],
                               m_outputBuffers[0],
                               m_blockSize * sizeof(sample_t));
                    }
                }
            }
        } else {
            // there is no audio source - that may be because no
            // instrument is set - then we do nothing here or the
            // instrument is set to the base instrument of the plugin
            // - then we must add in the data
            for (const PluginPort::Connection &c :
                     m_connections.connections) {
                // find the connection for this port index
                if (c.portIndex != portIndex) continue;
                if (c.instrumentId != m_instrument) continue;
                // so the plugin output goes to this instrument
                if (c.channel == 0 || c.channel == -1) {
                    for (size_t is = 0; is < m_blockSize; ++is) {
                        m_outputBuffers[0][is] += m_outputBuffers[outbuf][is];
                    }
                }
                if (m_channelCount == 2 &&
                    (c.channel == 1 || c.channel == -1)) {
                    for (size_t is = 0; is < m_blockSize; ++is) {
                        m_outputBuffers[1][is] += m_outputBuffers[outbuf][is];
                    }
                }

            }
        }
        ++outbuf;
    }

    m_run = true;
    m_eventsDiscarded = false;

    //RG_DEBUG << "run done";
}

void
LV2PluginInstance::deactivate()
{
    lilv_instance_deactivate(m_instance);
}

void
LV2PluginInstance::cleanup()
{
    if (!m_instance)
        return;

    lilv_instance_free(m_instance);
    m_instance = nullptr;
}

void LV2PluginInstance::sendMidiData(const QByteArray& rawMidi,
                                     size_t frameOffset) const
{
    RG_DEBUG << "sendMidiData" << frameOffset << rawMidi.toHex();
    char midiBuf[1000];
    LV2_Atom_Event* event = (LV2_Atom_Event*)midiBuf;
    event->time.frames = frameOffset;
    event->body.size = rawMidi.size();
    event->body.type = m_midiEventUrid;
    int ebs = sizeof(LV2_Atom_Event);
    // memcpy() rawMidi to event via midiBuf.
    for(int ib=0; ib<rawMidi.size(); ib++) {
        midiBuf[ebs + ib] = rawMidi[ib];
    }

    // Guard writing to m_atomInputPorts.
    QMutexLocker lock(&m_atomInputMutex);

    for (const AtomPort &aip : m_atomInputPorts) {
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

void LV2PluginInstance::setupFeatures()
{
    LilvNodes* feats = lilv_plugin_get_required_features(m_plugin);
    RG_DEBUG << "instantiate num required features" <<  lilv_nodes_size(feats);
    LILV_FOREACH (nodes, i, feats) {
        const LilvNode* fnode = lilv_nodes_get(feats, i);
        RG_DEBUG << "required feature:" << lilv_node_as_string(fnode);
    }
    lilv_nodes_free(feats);

    m_uridMapFeature = {LV2_URID__map, LV2URIDMapper::getURIDMapFeature()};
    m_uridUnmapFeature = {LV2_URID__unmap, LV2URIDMapper::getURIDUnmapFeature()};

    m_workerSchedule.handle = &m_workerHandle;
    m_workerSchedule.schedule_work = LV2Worker::getInstance()->getScheduler();
    RG_DEBUG << "schedule_work" << (void*)m_workerSchedule.schedule_work;
    m_workerFeature = {LV2_WORKER__schedule, &m_workerSchedule};

    LV2_URID nbl_urid = LV2URIDMapper::uridMap(LV2_BUF_SIZE__nominalBlockLength);
    LV2_URID minbl_urid = LV2URIDMapper::uridMap(LV2_BUF_SIZE__minBlockLength);
    LV2_URID maxbl_urid = LV2URIDMapper::uridMap(LV2_BUF_SIZE__maxBlockLength);
    LV2_URID ai_urid = LV2URIDMapper::uridMap(LV2_ATOM__Int);
    LV2_Options_Option opt;
    opt.context = LV2_OPTIONS_INSTANCE;
    opt.subject = 0;
    opt.key = nbl_urid;
    opt.size = 4;
    opt.type = ai_urid;
    opt.value = &m_blockSize;
    m_options.push_back(opt);
    opt.key = minbl_urid;
    opt.value = &m_blockSize;
    m_options.push_back(opt);
    opt.key = maxbl_urid;
    opt.value = &m_blockSize;
    m_options.push_back(opt);
    opt.key = 0;
    opt.type = 0;
    opt.value = 0;
    m_options.push_back(opt);
    m_optionsFeature = {LV2_OPTIONS__options, m_options.data()};

    m_boundedBlockLengthFeature = {LV2_BUF_SIZE__boundedBlockLength, nullptr };

    m_features.push_back(&m_uridMapFeature);
    m_features.push_back(&m_uridUnmapFeature);
    m_features.push_back(&m_workerFeature);
    m_features.push_back(&m_optionsFeature);
    m_features.push_back(&m_boundedBlockLengthFeature);
    m_features.push_back(nullptr);

}

LV2PluginInstance::PortValueItem::~PortValueItem()
{
    // This was originally allocated as an array of char.
    // Cast back to that and use delete[] as is customary.
    delete[] reinterpret_cast<const char *>(valueAtom);
}

void LV2PluginInstance::updatePortValue(int index, const LV2_Atom *value)
{
    // If we have no GUI, bail.
    if (!m_gui)
    {
        // Note: The GUI thread will clear the queue if the GUI is missing.
        //       See triggerPortUpdates().  We do not want to clear the
        //       queue here as this is called by the audio thread.
        return;
    }

    // We want to call the ui updatePortValue from the gui thread so
    // queue the events
    int asize = sizeof(LV2_Atom) + value->size;
    PortValueItem* item = new PortValueItem;
    item->portIndex= index;
    char* buf = new char[asize];
    memcpy(buf, value, asize);
    item->valueAtom = reinterpret_cast<const LV2_Atom*>(buf);

    // This needs to be locked since it is called by the audio thread and
    // it modifies the m_portValueQueue which is also modified by
    // triggerPortUpdates() on the UI thread.
    QMutexLocker lock(&m_portValueQueueMutex);
    m_portValueQueue.push(item);
}

void LV2PluginInstance::triggerPortUpdates()
{
    // No GUI?  Clear and bail.
    if (!m_gui)
    {
        // Clear the m_portValueQueue.
        while (!m_portValueQueue.empty()) {
            PortValueItem* item = m_portValueQueue.front();
            delete item;
            m_portValueQueue.pop();
        }
        return;
    }

#ifndef NDEBUG
    if (!m_portValueQueue.empty())
        RG_DEBUG << "triggerPortUpdates()" << m_portValueQueue.size();
#endif

    // This needs to be locked since it is called by the UI thread
    // (AudioPluginLV2GUIWindow's timer) and it modifies the portValueQueue
    // which is also modified by updatePortValue() on the audio thread.
    QMutexLocker lock(&m_portValueQueueMutex);

    // For each port/value pair, send it to the plugin.
    while (!m_portValueQueue.empty()) {
        PortValueItem* item = m_portValueQueue.front();
        // ??? We can reduce the locking time by copying the updates
        //     to a temporary local queue, unlocking, then sending them off
        //     to m_gui.  Or use a lock-free queue.
        m_gui->updatePortValue(item->portIndex, item->valueAtom);
        delete item;
        m_portValueQueue.pop();
    }
}


}
