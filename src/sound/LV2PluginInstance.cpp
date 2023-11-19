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
//#define RG_NO_DEBUG_PRINT 1

#include "LV2PluginInstance.h"
#include "LV2PluginFactory.h"

#include <QtGlobal>
#include "misc/Debug.h"
#include "sound/LV2Utils.h"

#include <lv2/midi/midi.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>

namespace Rosegarden
{

#define EVENT_BUFFER_SIZE 1023

LV2PluginInstance::LV2PluginInstance(PluginFactory *factory,
        InstrumentId instrument,
        QString identifier,
        int position,
        unsigned long sampleRate,
        size_t blockSize,
        int idealChannelCount,
        const QString& uri) :
        RunnablePluginInstance(factory, identifier),
        m_instrument(instrument),
        m_position(position),
        m_instance(nullptr),
        m_uri(uri),
        m_plugin(nullptr),
        m_channelCount(0),
        m_midiPort(-1),
        m_midiParser(nullptr),
        m_blockSize(blockSize),
        m_sampleRate(sampleRate),
        m_latencyPort(nullptr),
        m_run(false),
        m_bypassed(false),
        m_distributeChannels(false)
{
    RG_DEBUG << "create plugin" << uri;

    LV2Utils* lv2utils = LV2Utils::getInstance();

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

    // create the atom port
    // use double to get 64 bit alignment
    double* dbuf = new double[EVENT_BUFFER_SIZE];
    m_midiIn = reinterpret_cast<LV2_Atom_Sequence*>(dbuf);
    lv2_atom_sequence_clear(m_midiIn);
    LV2_URID type = lv2utils->uridMap(LV2_ATOM__Sequence);
    m_midiIn->atom.type = type;

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
            if (portData.isInput) {
                RG_DEBUG << "LV2PluginInstance::init: port" << i << "is control in";
                controlPortsIn.
                    push_back(std::pair<unsigned long, float>(i, 0.0));
            } else {
                RG_DEBUG << "LV2PluginInstance::init: port" << i << "is control out";
                controlPortsOut.
                    push_back(std::pair<unsigned long, float>(i, 0.0));
                if ((portData.name == "latency") ||
                    (portData.name == "_latency")) {
                    RG_DEBUG << "Wooo! We have a latency port!";
                    float& value =
                        controlPortsOut.back().second;
                    m_latencyPort = &(value);
                }
            }
            break;
        case LV2Utils::LV2MIDI:
            RG_DEBUG << "LV2PluginInstance::init: port" << i << "is midi";
            m_midiPort = i;
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
    m_eventBuffer.clear();
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

LV2PluginInstance::~LV2PluginInstance()
{
    RG_DEBUG << "LV2PluginInstance::~LV2PluginInstance";
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->unRegisterPlugin(m_instrument, m_position);

    if (m_instance != nullptr) {
        deactivate();
    }

    cleanup();

    controlPortsIn.clear();
    controlPortsOut.clear();

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        delete[] m_inputBuffers[i];
    }
    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
        delete[] m_outputBuffers[i];
    }

    delete[](m_midiIn);

    delete[] m_inputBuffers;
    delete[] m_outputBuffers;

    m_audioPortsIn.clear();
    m_audioPortsOut.clear();

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
    }
    lilv_nodes_free(feats);
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

    for (size_t i = 0;
         i < controlPortsIn.size();
         ++i) {
        RG_DEBUG << "connect control in:" <<
            controlPortsIn[i].first;
        lilv_instance_connect_port
            (m_instance,
             controlPortsIn[i].first,
             &(controlPortsIn[i].second));
    }

    for (size_t i = 0; i < controlPortsOut.size(); ++i) {
        RG_DEBUG << "connect control out:" <<
            controlPortsOut[i].first;
        lilv_instance_connect_port
            (m_instance,
             controlPortsOut[i].first,
             &(controlPortsOut[i].second));
    }

    if (m_midiPort != -1) {
        RG_DEBUG << "connect midi port" << m_midiPort;
        lilv_instance_connect_port(m_instance, m_midiPort, m_midiIn);
    }
}

void
LV2PluginInstance::setPortValue
(unsigned int portNumber, float value)
{
    RG_DEBUG << "setPortValue" << portNumber << value;
    for (size_t i = 0; i < controlPortsIn.size(); ++i) {
        if (controlPortsIn[i].first == portNumber) {
            if (value < m_pluginData.ports[portNumber].min)
                value = m_pluginData.ports[portNumber].min;
            if (value > m_pluginData.ports[portNumber].max)
                value = m_pluginData.ports[portNumber].max;
            controlPortsIn[i].second = value;
        }
    }
}

void
LV2PluginInstance::setPortByteArray(unsigned int port, const QByteArray& ba)
{
    RG_DEBUG << "setPortByteArray" << port;
    // only supported for midi port
    if (static_cast<int>(port) != m_midiPort) {
        RG_DEBUG << "setPortByteArray called for non midi port" << port;
        return;
    }
    const LV2_Atom_Event* event =
        reinterpret_cast<const LV2_Atom_Event*>(ba.data());
    RG_DEBUG << "setPortByteArray send bytes" << ba.size();
    lv2_atom_sequence_append_event(m_midiIn,
                                   ba.size(),
                                   event);
}

float
LV2PluginInstance::getPortValue(unsigned int portNumber)
{
    for (size_t i = 0; i < controlPortsIn.size(); ++i) {
        if (controlPortsIn[i].first == portNumber) {
            return (controlPortsIn[i].second);
        }
    }

    return 0.0;
}

void
LV2PluginInstance::sendEvent(const RealTime& eventTime,
                             const void* event)
{
    snd_seq_event_t *seqEvent = (snd_seq_event_t *)event;
    m_eventBuffer[eventTime] = *seqEvent;
}

void
LV2PluginInstance::run(const RealTime &rt)
{
    RG_DEBUG << "run" << rt;

    RealTime bufferStart = rt;
    auto it = m_eventBuffer.begin();
    while(it != m_eventBuffer.end()) {
        RealTime evTime = (*it).first;
        snd_seq_event_t& qEvent = (*it).second;
        if (evTime < bufferStart) evTime = bufferStart;
        size_t frameOffset =
            (size_t)RealTime::realTime2Frame(evTime - bufferStart,
                                             m_sampleRate);
        if (frameOffset > m_blockSize) break;
        // the event is in this block

        unsigned char buf[100];
        int bytes = snd_midi_event_decode(m_midiParser, buf, 100, &qEvent);
        if (bytes <= 0) {
            RG_DEBUG << "error decoding midi event";
            return;
        }

#ifndef NDEBUG
        QString rawMidi;
        for(int irm=0; irm<bytes; irm++) {
            QString byte = QString("%1").arg((int)buf[irm]);
            rawMidi += byte;
            if (irm < bytes-1) rawMidi += "/";
        }
        RG_DEBUG << "send event to plugin" << evTime << frameOffset << rawMidi;
#endif
        auto iterToDelete = it;
        it++;
        m_eventBuffer.erase(iterToDelete);

        char midiBuf[1000];
        LV2_Atom_Event* event = (LV2_Atom_Event*)midiBuf;
        event->time.frames = frameOffset;
        event->body.size = bytes;
        event->body.type = m_midiEventUrid;

        int ebs = sizeof(LV2_Atom_Event);
        for(int ib=0; ib<bytes; ib++) {
            midiBuf[ebs + ib] = buf[ib];
        }
        LV2_Atom_Event* atom =
            lv2_atom_sequence_append_event(m_midiIn,
                                           8*EVENT_BUFFER_SIZE,
                                           event);
        if (atom == nullptr) {
            RG_DEBUG << "midi event overflow";
            return;
        }
    }

    if (m_distributeChannels) {
        // the input buffers contain stereo channels - combine for a
        // mono plugin. If this is the case there are exactly 2 input
        // and output buffers. The first ones are connected to the
        // plugin
        RG_DEBUG << "distribute stereo -> mono";
        for (size_t i = 0; i < m_blockSize; ++i) {
                    m_inputBuffers[0][i] =
                        (m_inputBuffers[0][i] + m_inputBuffers[1][i]) / 2.0;
        }
    }

    // get any worker responses
    LV2Utils* lv2utils = LV2Utils::getInstance();
    LV2Utils::Worker* worker = lv2utils->getWorker();

    LV2Utils::PluginPosition pp;
    pp.instrument = m_instrument;
    pp.position = m_position;
    if (m_workerInterface) {
        while(LV2Utils::WorkerJob* job = worker->getResponse(pp)) {
            m_workerInterface->work_response(m_instance, job->size, job->data);
            delete job;
        }

        if (m_workerInterface->end_run) m_workerInterface->end_run(m_instance);
    }

  lilv_instance_run(m_instance, m_blockSize);

    if (m_distributeChannels) {
        // after running distribute the output to the two channels
        for (size_t i = 0; i < m_blockSize; ++i) {
            m_outputBuffers[1][i] = m_outputBuffers[0][i];
        }
    }

    m_run = true;

    lv2_atom_sequence_clear(m_midiIn);
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

}
