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


#define RG_MODULE_STRING "[LADSPAPluginInstance]"
#define RG_NO_DEBUG_PRINT

#include "LADSPAPluginInstance.h"

#include "LADSPAPluginFactory.h"
#include "misc/Debug.h"

#include <QtGlobal>

//#define DEBUG_LADSPA 1


namespace Rosegarden
{


// Allocate enough for two instances of a mono to stereo plugin.  That
// would have 2 ins and 4 outs.
// See setIdealChannelCount().
// ??? Is there a situation where we would need more than 4?
static constexpr size_t maxBuffers = 4ul;

LADSPAPluginInstance::LADSPAPluginInstance(
        PluginFactory *factory,
        InstrumentId instrument,
        QString identifier,
        int position,
        unsigned long sampleRate,
        size_t blockSize,
        int idealChannelCount,
        const LADSPA_Descriptor* descriptor) :
    RunnablePluginInstance(factory, identifier),
    m_instrument(instrument),
    m_position(position),
    m_descriptor(descriptor),
    m_blockSize(blockSize),
    m_ownBuffers(true),
    m_sampleRate(sampleRate)
{
    init(idealChannelCount);

    // We're only expecting 1 or 2.  If we actually see more, we'll need to
    // allocate more buffers.  E.g. a compressor with a side-chain input.
    // We probably don't handle those well anyway.  We don't have a way
    // to patch the side-chain input someplace else.
    if (m_audioPortsIn.size() > 2) {
        RG_WARNING << "ctor: Encountered plugin with more than 2 inputs.  Ignoring inputs beyond 2.";
        m_audioPortsIn.resize(2);
    }
    if (m_audioPortsOut.size() > 2) {
        RG_WARNING << "ctor: Encountered plugin with more than 2 outputs.  Ignoring outputs beyond 2.";
        m_audioPortsOut.resize(2);
    }

    m_inputBuffers = new sample_t *[maxBuffers];
    m_outputBuffers = new sample_t *[maxBuffers];

    for (size_t i = 0; i < maxBuffers; ++i) {
        m_inputBuffers[i] = new sample_t[blockSize];
    }
    for (size_t i = 0; i < maxBuffers; ++i) {
        m_outputBuffers[i] = new sample_t[blockSize];
    }

    instantiate(sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
}

LADSPAPluginInstance::LADSPAPluginInstance(
        PluginFactory *factory,
        InstrumentId instrument,
        QString identifier,
        int position,
        unsigned long sampleRate,
        size_t blockSize,
        sample_t **inputBuffers,
        sample_t **outputBuffers,
        const LADSPA_Descriptor* descriptor) :
    RunnablePluginInstance(factory, identifier),
    m_instrument(instrument),
    m_position(position),
    m_descriptor(descriptor),
    m_blockSize(blockSize),
    m_inputBuffers(inputBuffers),
    m_outputBuffers(outputBuffers),
    m_ownBuffers(false),
    m_sampleRate(sampleRate)
{
    init();

    instantiate(sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
}


void
LADSPAPluginInstance::init(int idealChannelCount)
{
#ifdef DEBUG_LADSPA
    RG_DEBUG << "LADSPAPluginInstance::init(" << idealChannelCount << "): plugin has"
    << m_descriptor->PortCount << "ports";
#endif

    // Discover ports numbers and identities
    //
    for (unsigned long i = 0; i < m_descriptor->PortCount; ++i) {
        if (LADSPA_IS_PORT_AUDIO(m_descriptor->PortDescriptors[i])) {
            if (LADSPA_IS_PORT_INPUT(m_descriptor->PortDescriptors[i])) {
#ifdef DEBUG_LADSPA
                RC_DEBUG << "LADSPAPluginInstance::init: port " << i << " is audio in";
#endif

                m_audioPortsIn.push_back(i);
            } else {  // output
#ifdef DEBUG_LADSPA
                RG_DEBUG << "LADSPAPluginInstance::init: port " << i << " is audio out";
#endif

                m_audioPortsOut.push_back(i);
            }
        } else
            if (LADSPA_IS_PORT_CONTROL(m_descriptor->PortDescriptors[i])) {
                if (LADSPA_IS_PORT_INPUT(m_descriptor->PortDescriptors[i])) {
#ifdef DEBUG_LADSPA
                    RG_DEBUG << "LADSPAPluginInstance::init: port " << i << " is control in";
#endif

                    LADSPA_Data *data = new LADSPA_Data(0.0);
                    m_controlPortsIn.push_back(
                        std::pair<unsigned long, LADSPA_Data*>(i, data));
                } else {
#ifdef DEBUG_LADSPA
                    RG_DEBUG << "LADSPAPluginInstance::init: port " << i << " is control out";
#endif

                    LADSPA_Data *data = new LADSPA_Data(0.0);
                    m_controlPortsOut.push_back(
                        std::pair<unsigned long, LADSPA_Data*>(i, data));
                    if (!strcmp(m_descriptor->PortNames[i], "latency") ||
                            !strcmp(m_descriptor->PortNames[i], "_latency")) {
#ifdef DEBUG_LADSPA
                        RG_DEBUG << "Wooo! We have a latency port!";
#endif

                        m_latencyPort = data;
                    }
                }
            }
#ifdef DEBUG_LADSPA
            else
                RG_DEBUG << "LADSPAPluginInstance::init - "
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
LADSPAPluginInstance::getLatency()
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
LADSPAPluginInstance::silence()
{
    if (isOK()) {
        deactivate();
        activate();
    }
}

void
LADSPAPluginInstance::setIdealChannelCount(size_t channels)
{
    if (m_audioPortsIn.size() != 1 || channels == m_instanceCount) {
        silence();
        return ;
    }

    if (isOK()) {
        deactivate();
    }

    cleanup();

    m_instanceCount = channels;

    instantiate(m_sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
    }
}


LADSPAPluginInstance::~LADSPAPluginInstance()
{
#ifdef DEBUG_LADSPA
    RG_DEBUG << "LADSPAPluginInstance::~LADSPAPluginInstance";
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
        for (size_t i = 0; i < maxBuffers; ++i) {
            delete[] m_inputBuffers[i];
        }
        for (size_t i = 0; i < maxBuffers; ++i) {
            delete[] m_outputBuffers[i];
        }

        delete[] m_inputBuffers;
        delete[] m_outputBuffers;
    }

    m_audioPortsIn.clear();
    m_audioPortsOut.clear();
}


void
LADSPAPluginInstance::instantiate(unsigned long sampleRate)
{
    if (!m_descriptor)
        return;

#ifdef DEBUG_LADSPA
    RG_DEBUG << "LADSPAPluginInstance::instantiate - plugin unique id = "
    << m_descriptor->UniqueID;
#endif

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
LADSPAPluginInstance::activate()
{
    if (!m_descriptor || !m_descriptor->activate)
        return ;

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->activate(*hi);
    }
}

void
LADSPAPluginInstance::connectPorts()
{
    if (!m_descriptor || !m_descriptor->connect_port)
        return ;

    static_assert(sizeof(LADSPA_Data) == sizeof(float));
    static_assert(sizeof(sample_t) == sizeof(float));

    size_t inbuf = 0, outbuf = 0;

    // For each instance...
    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
         hi != m_instanceHandles.end();
         ++hi) {

        // For each audio in...
        for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
            if (inbuf > maxBuffers - 1) {
                RG_WARNING << "connectPorts(): Not enough in buffers." << m_instrument << m_position;
                break;
            }
            m_descriptor->connect_port(*hi,
                                       m_audioPortsIn[i],
                                       m_inputBuffers[inbuf]);
            ++inbuf;
        }

        // For each audio out...
        for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
            if (outbuf > maxBuffers - 1) {
                RG_WARNING << "connectPorts(): Not enough out buffers." << m_instrument << m_position;
                break;
            }
            m_descriptor->connect_port(*hi,
                                       m_audioPortsOut[i],
                                       m_outputBuffers[outbuf]);
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
LADSPAPluginInstance::setPortValue(unsigned int portNumber, float value)
{
    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        if (m_controlPortsIn[i].first == portNumber) {
            LADSPAPluginFactory *f = dynamic_cast<LADSPAPluginFactory *>(m_factory);
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
LADSPAPluginInstance::getPortValue(unsigned int portNumber)
{
    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        if (m_controlPortsIn[i].first == portNumber) {
            return (*m_controlPortsIn[i].second);
        }
    }

    return 0.0;
}

void
LADSPAPluginInstance::run(const RealTime &)
{
    if (!m_descriptor || !m_descriptor->run)
        return ;

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->run(*hi, m_blockSize);
    }

    m_run = true;
}

void
LADSPAPluginInstance::deactivate()
{
    if (!m_descriptor || !m_descriptor->deactivate)
        return ;

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->deactivate(*hi);
    }
}

void
LADSPAPluginInstance::cleanup()
{
    if (!m_descriptor)
        return ;

    if (!m_descriptor->cleanup) {
        RG_WARNING << "Bad plugin: plugin id " << m_descriptor->UniqueID
        << ":" << m_descriptor->Label
        << " has no cleanup method!";
        return ;
    }

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
            hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->cleanup(*hi);
    }

    m_instanceHandles.clear();
}



}
