// -*- c-basic-offset: 4 -*-

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

#include "DSSIPluginInstance.h"

#ifdef HAVE_DSSI

#define DEBUG_DSSI 1

namespace Rosegarden
{

DSSIPluginInstance::DSSIPluginInstance(PluginFactory *factory,
				       Rosegarden::InstrumentId instrument,
				       QString identifier,
				       int position,
				       unsigned long sampleRate,
				       size_t bufferSize,
				       int idealChannelCount,
				       const DSSI_Descriptor* descriptor) :
    RunnablePluginInstance(factory, identifier),
    m_instrument(instrument),
    m_position(position),
    m_descriptor(descriptor),
    m_bufferSize(bufferSize),
    m_idealChannelCount(idealChannelCount),
    m_bypassed(false)
{
    init();

    m_inputBuffers  = new sample_t*[m_audioPortsIn.size()];
    m_outputBuffers = new sample_t*[m_outputBufferCount];

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
	m_inputBuffers[i] = new sample_t[bufferSize];
    }
    for (size_t i = 0; i < m_outputBufferCount; ++i) {
	m_outputBuffers[i] = new sample_t[bufferSize];
    }

    m_ownBuffers = true;

    instantiate(sampleRate);
    if (isOK()) connectPorts();
}

DSSIPluginInstance::DSSIPluginInstance(PluginFactory *factory,
				       Rosegarden::InstrumentId instrument,
				       QString identifier,
				       int position,
				       unsigned long sampleRate,
				       size_t bufferSize,
				       sample_t **inputBuffers,
				       sample_t **outputBuffers,
				       const DSSI_Descriptor* descriptor) :
    RunnablePluginInstance(factory, identifier),
    m_instrument(instrument),
    m_position(position),
    m_descriptor(descriptor),
    m_bufferSize(bufferSize),
    m_inputBuffers(inputBuffers),
    m_outputBuffers(outputBuffers),
    m_ownBuffers(false),
    m_idealChannelCount(0),
    m_bypassed(false)
{
    init();

    instantiate(sampleRate);
    if (isOK()) connectPorts();
}


void
DSSIPluginInstance::init()
{
    // Discover ports numbers and identities
    //
    const LADSPA_Descriptor *descriptor = m_descriptor->LADSPA_Plugin;

    for (unsigned long i = 0; i < descriptor->PortCount; ++i)
    {
        if (LADSPA_IS_PORT_AUDIO(descriptor->PortDescriptors[i]))
        {
            if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[i]))
                m_audioPortsIn.push_back(i);
            else
                m_audioPortsOut.push_back(i);
        }
        else
        if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[i]))
        {
	    if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[i])) {
		LADSPA_Data *data = new LADSPA_Data(0.0);
		m_controlPortsIn.push_back(
                    std::pair<unsigned long, LADSPA_Data*>(i, data));
	    } else {
		LADSPA_Data *data = new LADSPA_Data(0.0);
		m_controlPortsOut.push_back(
                    std::pair<unsigned long, LADSPA_Data*>(i, data));
	    }
        }
#ifdef DEBUG_DSSI
        else
            std::cerr << "DSSIPluginInstance::DSSIPluginInstance - "
                      << "unrecognised port type" << std::endl;
#endif
    }

    m_outputBufferCount = std::max(m_idealChannelCount, int(m_audioPortsOut.size()));
}

void
DSSIPluginInstance::setIdealChannelCount(unsigned long sampleRate, int channels)
{
    if (channels == m_idealChannelCount) return;
    m_idealChannelCount = channels;

    if (channels > m_outputBufferCount) {

	for (size_t i = 0; i < m_outputBufferCount; ++i) {
	    delete[] m_outputBuffers[i];
	}

	m_outputBufferCount = channels;

	m_outputBuffers = new sample_t*[m_outputBufferCount];

	for (size_t i = 0; i < m_outputBufferCount; ++i) {
	    m_outputBuffers[i] = new sample_t[m_bufferSize];
	}
    }
}

DSSIPluginInstance::~DSSIPluginInstance()
{
    std::cerr << "DSSIPluginInstance::~DSSIPluginInstance" << std::endl;

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
	for (size_t i = 0; i < m_outputBufferCount; ++i) {
	    delete[] m_outputBuffers[i];
	}

	delete[] m_inputBuffers;
	delete[] m_outputBuffers;
    }

    m_audioPortsIn.clear();
    m_audioPortsOut.clear();
}


void
DSSIPluginInstance::instantiate(unsigned long sampleRate)
{
#ifdef DEBUG_DSSI
    std::cout << "DSSIPluginInstance::instantiate - plugin unique id = "
              << m_descriptor->LADSPA_Plugin->UniqueID << std::endl;
#endif
    if (!m_descriptor) return;

    const LADSPA_Descriptor *descriptor = m_descriptor->LADSPA_Plugin;

    if (!descriptor->instantiate) {
	std::cerr << "Bad plugin: plugin id " << descriptor->UniqueID
		  << ":" << descriptor->Label
		  << " has no instantiate method!" << std::endl;
	return;
    }

    m_instanceHandle = descriptor->instantiate(descriptor, sampleRate);
}

void
DSSIPluginInstance::activate()
{
    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->activate) return;
    m_descriptor->LADSPA_Plugin->activate(m_instanceHandle);
}

void
DSSIPluginInstance::connectPorts()
{
    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->connect_port) return;

    assert(sizeof(LADSPA_Data) == sizeof(float));
    assert(sizeof(sample_t) == sizeof(float));

    int inbuf = 0, outbuf = 0;

    for (unsigned int i = 0; i < m_audioPortsIn.size(); ++i) {
	m_descriptor->LADSPA_Plugin->connect_port
	    (m_instanceHandle,
	     m_audioPortsIn[i],
	     (LADSPA_Data *)m_inputBuffers[inbuf]);
	++inbuf;
    }

    for (unsigned int i = 0; i < m_audioPortsOut.size(); ++i) {
	m_descriptor->LADSPA_Plugin->connect_port
	    (m_instanceHandle,
	     m_audioPortsOut[i],
	     (LADSPA_Data *)m_outputBuffers[outbuf]);
	++outbuf;
    }

    for (unsigned int i = 0; i < m_controlPortsIn.size(); ++i) {
	m_descriptor->LADSPA_Plugin->connect_port
	    (m_instanceHandle,
	     m_controlPortsIn[i].first,
	     m_controlPortsIn[i].second);
    }

    for (unsigned int i = 0; i < m_controlPortsOut.size(); ++i) {
	m_descriptor->LADSPA_Plugin->connect_port
	    (m_instanceHandle,
	     m_controlPortsOut[i].first,
	     m_controlPortsOut[i].second);
    }
}

void
DSSIPluginInstance::setPortValue(unsigned int portNumber, float value)
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
DSSIPluginInstance::run()
{
    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->run) return;
    m_descriptor->LADSPA_Plugin->run(m_instanceHandle, m_bufferSize);

    if (m_idealChannelCount < m_audioPortsOut.size()) {
	if (m_idealChannelCount == 1) {
	    // mix down to mono
	    for (size_t ch = 1; ch < m_audioPortsOut.size(); ++ch) {
		for (size_t i = 0; i < m_bufferSize; ++i) {
		    m_outputBuffers[0][i] += m_outputBuffers[ch][i];
		}
	    }
	}
    } else if (m_idealChannelCount > m_audioPortsOut.size()) {
	// duplicate
	for (size_t ch = m_audioPortsOut.size(); ch < m_idealChannelCount; ++ch) {
	    size_t sch = (ch - m_audioPortsOut.size()) % m_audioPortsOut.size();
	    for (size_t i = 0; i < m_bufferSize; ++i) {
		m_outputBuffers[ch][i] = m_outputBuffers[sch][i];
	    }
	}
    }	
}

void
DSSIPluginInstance::deactivate()
{
    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->deactivate) return;
    m_descriptor->LADSPA_Plugin->deactivate(m_instanceHandle);
}

void
DSSIPluginInstance::cleanup()
{
    if (!m_descriptor) return;

    if (!m_descriptor->LADSPA_Plugin->cleanup) {
	std::cerr << "Bad plugin: plugin id "
		  << m_descriptor->LADSPA_Plugin->UniqueID
		  << ":" << m_descriptor->LADSPA_Plugin->Label
		  << " has no cleanup method!" << std::endl;
	return;
    }

    m_descriptor->LADSPA_Plugin->cleanup(m_instanceHandle);
    m_instanceHandle = 0;
}



}

#endif // HAVE_DSSI


