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

#include "LV2PluginInstance.h"
#include "LV2PluginFactory.h"

#include <QtGlobal>
#include "misc/Debug.h"


//#define DEBUG_LV2 1

namespace Rosegarden
{


LV2PluginInstance::LV2PluginInstance(PluginFactory *factory,
        InstrumentId instrument,
        QString identifier,
        int position,
        unsigned long sampleRate,
        size_t blockSize,
        int idealChannelCount,
        const LV2_Descriptor* descriptor) :
        RunnablePluginInstance(factory, identifier),
        m_instrument(instrument),
        m_position(position),
        m_instanceCount(0),
        m_descriptor(descriptor),
        m_blockSize(blockSize),
        m_sampleRate(sampleRate),
        m_latencyPort(nullptr),
        m_run(false),
        m_bypassed(false)
{
    init(idealChannelCount);

    m_inputBuffers = new sample_t * [m_instanceCount * m_audioPortsIn.size()];
    m_outputBuffers = new sample_t * [m_instanceCount * m_audioPortsOut.size()];

    for (size_t i = 0; i < m_instanceCount * m_audioPortsIn.size(); ++i) {
        m_inputBuffers[i] = new sample_t[blockSize];
    }
    for (size_t i = 0; i < m_instanceCount * m_audioPortsOut.size(); ++i) {
        m_outputBuffers[i] = new sample_t[blockSize];
    }

    m_ownBuffers = true;

    instantiate(sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
}

LV2PluginInstance::LV2PluginInstance(PluginFactory *factory,
        InstrumentId instrument,
        QString identifier,
        int position,
        unsigned long sampleRate,
        size_t blockSize,
        sample_t **inputBuffers,
        sample_t **outputBuffers,
        const LV2_Descriptor* descriptor) :
        RunnablePluginInstance(factory, identifier),
        m_instrument(instrument),
        m_position(position),
        m_instanceCount(0),
        m_descriptor(descriptor),
        m_blockSize(blockSize),
        m_inputBuffers(inputBuffers),
        m_outputBuffers(outputBuffers),
        m_ownBuffers(false),
        m_sampleRate(sampleRate),
        m_latencyPort(nullptr),
        m_run(false),
        m_bypassed(false)
{
    init();

    instantiate(sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
}


void
LV2PluginInstance::init(int idealChannelCount)
{
#ifdef DEBUG_LV2
    RG_DEBUG << "LV2PluginInstance::init(" << idealChannelCount << "): plugin has"
    << m_descriptor->PortCount << "ports";
#endif

    // Discover ports numbers and identities
    //
    for (unsigned long i = 0; i < m_descriptor->PortCount; ++i) {
        if (LV2_IS_PORT_AUDIO(m_descriptor->PortDescriptors[i])) {
            if (LV2_IS_PORT_INPUT(m_descriptor->PortDescriptors[i])) {
#ifdef DEBUG_LV2
                RC_DEBUG << "LV2PluginInstance::init: port " << i << " is audio in";
#endif

                m_audioPortsIn.push_back(i);
            } else {
#ifdef DEBUG_LV2
                RG_DEBUG << "LV2PluginInstance::init: port " << i << " is audio out";
#endif

                m_audioPortsOut.push_back(i);
            }
        } else
            if (LV2_IS_PORT_CONTROL(m_descriptor->PortDescriptors[i])) {
                if (LV2_IS_PORT_INPUT(m_descriptor->PortDescriptors[i])) {
#ifdef DEBUG_LV2
                    RG_DEBUG << "LV2PluginInstance::init: port " << i << " is control in";
#endif

                    LV2_Data *data = new LV2_Data(0.0);
                    m_controlPortsIn.push_back(
                        std::pair<unsigned long, LV2_Data*>(i, data));
                } else {
#ifdef DEBUG_LV2
                    RG_DEBUG << "LV2PluginInstance::init: port " << i << " is control out";
#endif

                    LV2_Data *data = new LV2_Data(0.0);
                    m_controlPortsOut.push_back(
                        std::pair<unsigned long, LV2_Data*>(i, data));
                    if (!strcmp(m_descriptor->PortNames[i], "latency") ||
                            !strcmp(m_descriptor->PortNames[i], "_latency")) {
#ifdef DEBUG_LV2
                        RG_DEBUG << "Wooo! We have a latency port!";
#endif

                        m_latencyPort = data;
                    }
                }
            }
#ifdef DEBUG_LV2
            else
                RG_DEBUG << "LV2PluginInstance::init - "
                << "unrecognised port type";
#endif

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
#ifdef DEBUG_LV2
    RG_DEBUG << "LV2PluginInstance::~LV2PluginInstance";
#endif

    if (m_instanceHandles.size() != 0) { // "isOK()"
        deactivate();
    }

    cleanup();

    for (size_t i = 0; i < m_controlPortsIn.size(); ++i)
        delete m_controlPortsIn[i].second;

    for (size_t i = 0; i < m_controlPortsOut.size(); ++i)
        delete m_controlPortsOut[i].second;

    m_controlPortsIn.clear();
    m_controlPortsOut.clear();

    if (m_ownBuffers) {
        for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
            delete[] m_inputBuffers[i];
        }
        for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
            delete[] m_outputBuffers[i];
        }

        delete[] m_inputBuffers;
        delete[] m_outputBuffers;
    }

    m_audioPortsIn.clear();
    m_audioPortsOut.clear();
}


void
LV2PluginInstance::instantiate(unsigned long sampleRate)
{
#ifdef DEBUG_LV2
    RG_DEBUG << "LV2PluginInstance::instantiate - plugin unique id = "
    << m_descriptor->UniqueID;
#endif

    if (!m_descriptor)
        return ;

    if (!m_descriptor->instantiate) {
        RG_WARNING << "Bad plugin: plugin id " << m_descriptor->UniqueID
        << ":" << m_descriptor->Label
        << " has no instantiate method!";
        return ;
    }

    for (size_t i = 0; i < m_instanceCount; ++i) {
        m_instanceHandles.push_back
        (m_descriptor->instantiate(m_descriptor, sampleRate));
    }
}

void
LV2PluginInstance::activate()
{
    if (!m_descriptor || !m_descriptor->activate)
        return ;

    for (std::vector<LV2_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->activate(*hi);
    }
}

void
LV2PluginInstance::connectPorts()
{
    if (!m_descriptor || !m_descriptor->connect_port)
        return ;

    Q_ASSERT(sizeof(LV2_Data) == sizeof(float));
    Q_ASSERT(sizeof(sample_t) == sizeof(float));

    size_t inbuf = 0, outbuf = 0;

    for (std::vector<LV2_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {

        for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
            m_descriptor->connect_port(*hi,
                                       m_audioPortsIn[i],
                                       (LV2_Data *)m_inputBuffers[inbuf]);
            ++inbuf;
        }

        for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
            m_descriptor->connect_port(*hi,
                                       m_audioPortsOut[i],
                                       (LV2_Data *)m_outputBuffers[outbuf]);
            ++outbuf;
        }

        // If there is more than one instance, they all share the same
        // control port ins (and outs, for the moment, because we
        // don't actually do anything with the outs anyway -- but they
        // do have to be connected as the plugin can't know if they're
        // not and will write to them anyway).

        for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
            m_descriptor->connect_port(*hi,
                                       m_controlPortsIn[i].first,
                                       m_controlPortsIn[i].second);
        }

        for (size_t i = 0; i < m_controlPortsOut.size(); ++i) {
            m_descriptor->connect_port(*hi,
                                       m_controlPortsOut[i].first,
                                       m_controlPortsOut[i].second);
        }
    }
}

void
LV2PluginInstance::setPortValue(unsigned int portNumber, float value)
{
    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        if (m_controlPortsIn[i].first == portNumber) {
            LV2PluginFactory *f = dynamic_cast<LV2PluginFactory *>(m_factory);
            if (f) {
                if (value < f->getPortMinimum(m_descriptor, portNumber)) {
                    value = f->getPortMinimum(m_descriptor, portNumber);
                }
                if (value > f->getPortMaximum(m_descriptor, portNumber)) {
                    value = f->getPortMaximum(m_descriptor, portNumber);
                }
            }
            (*m_controlPortsIn[i].second) = value;
        }
    }
}

float
LV2PluginInstance::getPortValue(unsigned int portNumber)
{
    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        if (m_controlPortsIn[i].first == portNumber) {
            return (*m_controlPortsIn[i].second);
        }
    }

    return 0.0;
}

void
LV2PluginInstance::run(const RealTime &)
{
    if (!m_descriptor || !m_descriptor->run)
        return ;

    for (std::vector<LV2_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->run(*hi, m_blockSize);
    }

    m_run = true;
}

void
LV2PluginInstance::deactivate()
{
    if (!m_descriptor || !m_descriptor->deactivate)
        return ;

    for (std::vector<LV2_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->deactivate(*hi);
    }
}

void
LV2PluginInstance::cleanup()
{
    if (!m_descriptor)
        return ;

    if (!m_descriptor->cleanup) {
        RG_WARNING << "Bad plugin: plugin id " << m_descriptor->UniqueID
        << ":" << m_descriptor->Label
        << " has no cleanup method!";
        return ;
    }

    for (std::vector<LV2_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->cleanup(*hi);
    }

    m_instanceHandles.clear();
}



}
