// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2004
  Guillaume Laurent   <glaurent@telegraph-road.org>,
  Chris Cannam        <cannam@all-day-breakfast.com>,
  Richard Bown        <bownie@bownie.com>

  The moral right of the authors to claim authorship of this work
  has been asserted.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include <iostream>
#include <cassert>

#include "LADSPAPluginInstance.h"

#ifdef HAVE_LADSPA

#define DEBUG_LADSPA 1

namespace Rosegarden
{


LADSPAPluginInstance::LADSPAPluginInstance(PluginFactory *factory,
					   Rosegarden::InstrumentId instrument,
					   QString identifier,
                                           int position,
					   unsigned long sampleRate,
					   size_t blockSize,
					   int idealChannelCount,
                                           const LADSPA_Descriptor* descriptor) :
    RunnablePluginInstance(factory, identifier),
    m_instrument(instrument),
    m_position(position),
    m_instanceCount(0),
    m_descriptor(descriptor),
    m_blockSize(blockSize),
    m_sampleRate(sampleRate),
    m_bypassed(false)
{
    init(idealChannelCount);

    m_inputBuffers  = new sample_t*[m_instanceCount * m_audioPortsIn.size()];
    m_outputBuffers = new sample_t*[m_instanceCount * m_audioPortsOut.size()];

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

LADSPAPluginInstance::LADSPAPluginInstance(PluginFactory *factory,
					   Rosegarden::InstrumentId instrument,
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
    m_instanceCount(0),
    m_descriptor(descriptor),
    m_blockSize(blockSize),
    m_inputBuffers(inputBuffers),
    m_outputBuffers(outputBuffers),
    m_ownBuffers(false),
    m_sampleRate(sampleRate),
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
LADSPAPluginInstance::init(int idealChannelCount)
{
    // Discover ports numbers and identities
    //
    for (unsigned long i = 0; i < m_descriptor->PortCount; ++i)
    {
        if (LADSPA_IS_PORT_AUDIO(m_descriptor->PortDescriptors[i]))
        {
            if (LADSPA_IS_PORT_INPUT(m_descriptor->PortDescriptors[i]))
                m_audioPortsIn.push_back(i);
            else
                m_audioPortsOut.push_back(i);
        }
        else
        if (LADSPA_IS_PORT_CONTROL(m_descriptor->PortDescriptors[i]))
        {
	    if (LADSPA_IS_PORT_INPUT(m_descriptor->PortDescriptors[i])) {
		LADSPA_Data *data = new LADSPA_Data(0.0);
		m_controlPortsIn.push_back(
                    std::pair<unsigned long, LADSPA_Data*>(i, data));
	    } else {
		LADSPA_Data *data = new LADSPA_Data(0.0);
		m_controlPortsOut.push_back(
                    std::pair<unsigned long, LADSPA_Data*>(i, data));
	    }
        }
#ifdef DEBUG_LADSPA
        else
            std::cerr << "LADSPAPluginInstance::LADSPAPluginInstance - "
                      << "unrecognised port type" << std::endl;
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
	return;
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


LADSPAPluginInstance::~LADSPAPluginInstance()
{
    std::cerr << "LADSPAPluginInstance::~LADSPAPluginInstance" << std::endl;

    if (m_instanceHandles.size() != 0) { // "isOK()"
	deactivate();
    }

    cleanup();

    for (unsigned int i = 0; i < m_controlPortsIn.size(); ++i)
        delete m_controlPortsIn[i].second;

    for (unsigned int i = 0; i < m_controlPortsOut.size(); ++i)
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
LADSPAPluginInstance::instantiate(unsigned long sampleRate)
{
#ifdef DEBUG_LADSPA
    std::cout << "LADSPAPluginInstance::instantiate - plugin unique id = "
              << m_descriptor->UniqueID << std::endl;
#endif
    if (!m_descriptor) return;

    if (!m_descriptor->instantiate) {
	std::cerr << "Bad plugin: plugin id " << m_descriptor->UniqueID
		  << ":" << m_descriptor->Label
		  << " has no instantiate method!" << std::endl;
	return;
    }

    for (int i = 0; i < m_instanceCount; ++i) {
	m_instanceHandles.push_back
	    (m_descriptor->instantiate(m_descriptor, sampleRate));
    }
}

void
LADSPAPluginInstance::activate()
{
    if (!m_descriptor || !m_descriptor->activate) return;

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
	 hi != m_instanceHandles.end(); ++hi) {
	m_descriptor->activate(*hi);
    }
}

void
LADSPAPluginInstance::connectPorts()
{
    if (!m_descriptor || !m_descriptor->connect_port) return;

    assert(sizeof(LADSPA_Data) == sizeof(float));
    assert(sizeof(sample_t) == sizeof(float));

    int inbuf = 0, outbuf = 0;

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
	 hi != m_instanceHandles.end(); ++hi) {

	for (unsigned int i = 0; i < m_audioPortsIn.size(); ++i) {
	    m_descriptor->connect_port(*hi,
				       m_audioPortsIn[i],
				       (LADSPA_Data *)m_inputBuffers[inbuf]);
	    ++inbuf;
	}

	for (unsigned int i = 0; i < m_audioPortsOut.size(); ++i) {
	    m_descriptor->connect_port(*hi,
				       m_audioPortsOut[i],
				       (LADSPA_Data *)m_outputBuffers[outbuf]);
	    ++outbuf;
	}

	// If there is more than one instance, they all share the same
	// control port ins (and outs, for the moment, because we
	// don't actually do anything with the outs anyway -- but they
	// do have to be connected as the plugin can't know if they're
	// not and will write to them anyway).

	for (unsigned int i = 0; i < m_controlPortsIn.size(); ++i) {
	    m_descriptor->connect_port(*hi,
				       m_controlPortsIn[i].first,
				       m_controlPortsIn[i].second);
	}

	for (unsigned int i = 0; i < m_controlPortsOut.size(); ++i) {
	    m_descriptor->connect_port(*hi,
				       m_controlPortsOut[i].first,
				       m_controlPortsOut[i].second);
	}
    }
}

void
LADSPAPluginInstance::setPortValue(unsigned int portNumber, float value)
{
    for (unsigned int i = 0; i < m_controlPortsIn.size(); ++i)
    {
        if (m_controlPortsIn[i].first == portNumber)
        {
            (*m_controlPortsIn[i].second) = value;
        }
    }
}

void
LADSPAPluginInstance::run(const RealTime &)
{
    if (!m_descriptor || !m_descriptor->run) return;

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
	 hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->run(*hi, m_blockSize);
    }
}

void
LADSPAPluginInstance::deactivate()
{
    if (!m_descriptor || !m_descriptor->deactivate) return;

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
	 hi != m_instanceHandles.end(); ++hi) {
        m_descriptor->deactivate(*hi);
    }
}

void
LADSPAPluginInstance::cleanup()
{
    if (!m_descriptor) return;

    if (!m_descriptor->cleanup) {
	std::cerr << "Bad plugin: plugin id " << m_descriptor->UniqueID
		  << ":" << m_descriptor->Label
		  << " has no cleanup method!" << std::endl;
	return;
    }

    for (std::vector<LADSPA_Handle>::iterator hi = m_instanceHandles.begin();
	 hi != m_instanceHandles.end(); ++hi) {
	m_descriptor->cleanup(*hi);
    }

    m_instanceHandles.clear();
}



}

#endif // HAVE_LADSPA


