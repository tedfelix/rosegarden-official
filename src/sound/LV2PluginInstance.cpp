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
#include "misc/Debug.h"

namespace
{
    std::map<std::string, int> uridMap;
    std::map<int, std::string> uridUnmap;
    int nextId = 1;

    LV2_URID LV2UridMap(LV2_URID_Map_Handle,
                        const char *uri)
    {
        auto it = uridMap.find(uri);
        if (it == uridMap.end()) {
            int id = nextId;
            nextId++;
            uridMap[uri] = id;
            uridUnmap[id] = uri;
        }
        int ret = uridMap[uri];
        return ret;
    }
    static LV2_URID_Map map = {nullptr, &LV2UridMap};
    static LV2_Feature uridMapFeature = {LV2_URID__map, &map};

    const char* LV2UridUnmap(LV2_URID_Unmap_Handle,
                             LV2_URID urid)
    {
        auto it = uridUnmap.find(urid);
        if (it == uridUnmap.end()) {
            return "";
        }
        return (*it).second.c_str();
    }
    static LV2_URID_Unmap unmap = {nullptr, &LV2UridUnmap};
    static LV2_Feature uridUnmapFeature = {LV2_URID__unmap, &unmap};

}

namespace Rosegarden
{


LV2PluginInstance::LV2PluginInstance(PluginFactory *factory,
        InstrumentId instrument,
        QString identifier,
        int position,
        unsigned long sampleRate,
        size_t blockSize,
        int idealChannelCount,
        LilvWorld* world,
        const QString& uri,
        const LV2PluginData& pluginData) :
        RunnablePluginInstance(factory, identifier),
        m_instrument(instrument),
        m_position(position),
        m_instanceCount(0),
        m_world(world),
        m_uri(uri),
        m_pluginData(pluginData),
        m_blockSize(blockSize),
        m_sampleRate(sampleRate),
        m_latencyPort(nullptr),
        m_run(false),
        m_bypassed(false)
{
    RG_DEBUG << "create plugin" << uri;
    init(idealChannelCount);

    m_inputBuffers = new sample_t * [m_instanceCount * m_audioPortsIn.size()];
    m_outputBuffers = new sample_t * [m_instanceCount * m_audioPortsOut.size()];

    for (size_t i = 0; i < m_instanceCount * m_audioPortsIn.size(); ++i) {
        m_inputBuffers[i] = new sample_t[blockSize];
    }
    for (size_t i = 0; i < m_instanceCount * m_audioPortsOut.size(); ++i) {
        m_outputBuffers[i] = new sample_t[blockSize];
    }

    QByteArray ba = m_uri.toLocal8Bit();
    const char *uris = ba.data();
    LilvNode* pluginUri = lilv_new_uri(m_world, uris);
    const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
    m_plugin = lilv_plugins_get_by_uri(plugins, pluginUri);

    instantiate(sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
}

void
LV2PluginInstance::init(int idealChannelCount)
{
    RG_DEBUG << "LV2PluginInstance::init(" << idealChannelCount <<
        "): plugin has" << m_pluginData.ports.size() << "ports";
    // Discover ports numbers and identities
    //
    for (unsigned long i = 0; i < m_pluginData.ports.size(); ++i) {
        const LV2PortData& portData = m_pluginData.ports[i];
        if (! portData.isControl) {
            if (portData.isInput) {
                RG_DEBUG << "LV2PluginInstance::init: port " << i << " is audio in";
                m_audioPortsIn.push_back(i);
            } else {
                RG_DEBUG << "LV2PluginInstance::init: port " << i << " is audio out";
                m_audioPortsOut.push_back(i);
            }
        } else {
            if (portData.isInput) {
                RG_DEBUG << "LV2PluginInstance::init: port " << i << " is control in";
                m_controlPortsIn.
                    push_back(std::pair<unsigned long, float>(i, 0.0));
            } else {
                RG_DEBUG << "LV2PluginInstance::init: port " << i << " is control out";
                m_controlPortsOut.
                    push_back(std::pair<unsigned long, float>(i, 0.0));
                if ((portData.name == "latency") ||
                    (portData.name == "_latency")) {
                    RG_DEBUG << "Wooo! We have a latency port!";
                    float& value = m_controlPortsOut.back().second;
                    m_latencyPort = &(value);
                }
            }
        }
    }

    m_instanceCount = 1;

    if (idealChannelCount > 0) {
        if (m_audioPortsIn.size() == 1) {
            // mono plugin: duplicate it if need be
            m_instanceCount = idealChannelCount;
        }
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
            run(RealTime::zeroTime);
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
LV2PluginInstance::setIdealChannelCount(size_t channels)
{
    if (m_audioPortsIn.size() != 1 || channels == m_instanceCount) {
        silence();
        return ;
    }

    if (isOK()) {
        deactivate();
    }

    //!!! don't we need to reallocate inputBuffers and outputBuffers?

    cleanup();
    m_instanceCount = channels;
    instantiate(m_sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
}

LV2PluginInstance::~LV2PluginInstance()
{
    RG_DEBUG << "LV2PluginInstance::~LV2PluginInstance";

    if (m_instances.size() != 0) {
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

    LV2_Feature* features[3];
    features[0] = &uridMapFeature;
    features[1] = &uridUnmapFeature;
    features[2] = nullptr;

    for (size_t i = 0; i < m_instanceCount; ++i) {
        LilvInstance* instance =
            lilv_plugin_instantiate(m_plugin, sampleRate, features);
        if (!instance) {
            RG_WARNING << "Failed to instantiate plugin" << m_uri;
        } else {
            m_instances.push_back(instance);
        }
    }
    lilv_nodes_free(feats);
}

void
LV2PluginInstance::activate()
{
    for (auto i = m_instances.begin();
         i != m_instances.end(); ++i) {
        lilv_instance_activate(*i);
    }
}

void
LV2PluginInstance::connectPorts()
{
    size_t inbuf = 0, outbuf = 0;

    for (auto it = m_instances.begin();
         it != m_instances.end(); ++it) {

        for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
            lilv_instance_connect_port((*it), m_audioPortsIn[i],
                                       m_inputBuffers[inbuf]);
            ++inbuf;
        }

        for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
            lilv_instance_connect_port((*it), m_audioPortsOut[i],
                                       m_outputBuffers[outbuf]);
            ++outbuf;
        }

        // If there is more than one instance, they all share the same
        // control port ins (and outs, for the moment, because we
        // don't actually do anything with the outs anyway -- but they
        // do have to be connected as the plugin can't know if they're
        // not and will write to them anyway).

        for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
            lilv_instance_connect_port((*it), m_controlPortsIn[i].first,
                                       &(m_controlPortsIn[i].second));
        }

        for (size_t i = 0; i < m_controlPortsOut.size(); ++i) {
            lilv_instance_connect_port(*it, m_controlPortsOut[i].first,
                                       &(m_controlPortsOut[i].second));
        }
    }
}

void
LV2PluginInstance::setPortValue(unsigned int portNumber, float value)
{
    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        if (m_controlPortsIn[i].first == portNumber) {
            if (value < m_pluginData.ports[portNumber].min)
                value = m_pluginData.ports[portNumber].min;
            if (value > m_pluginData.ports[portNumber].max)
                value = m_pluginData.ports[portNumber].max;
            (m_controlPortsIn[i].second) = value;
        }
    }
}

float
LV2PluginInstance::getPortValue(unsigned int portNumber)
{
    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        if (m_controlPortsIn[i].first == portNumber) {
            return (m_controlPortsIn[i].second);
        }
    }

    return 0.0;
}

void
LV2PluginInstance::run(const RealTime &rt)
{
    RG_DEBUG << "run" << rt;
    for (auto i = m_instances.begin();
         i != m_instances.end(); ++i) {
        lilv_instance_run(*i, m_blockSize);
    }

    m_run = true;
}

void
LV2PluginInstance::deactivate()
{
    for (auto i = m_instances.begin();
         i != m_instances.end(); ++i) {
        lilv_instance_deactivate(*i);
    }
}

void
LV2PluginInstance::cleanup()
{
    for (auto i = m_instances.begin();
         i != m_instances.end(); ++i) {
        lilv_instance_free(*i);
    }

    m_instances.clear();
}

}
