// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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


LADSPAPluginInstance::LADSPAPluginInstance(Rosegarden::InstrumentId instrument,
                                           unsigned long ladspaId,
                                           int position,
					   unsigned long sampleRate,
					   size_t bufferSize,
                                           const LADSPA_Descriptor* descriptor):
    m_instrument(instrument),
    m_ladspaId(ladspaId),
    m_position(position),
    m_instanceHandle(0),
    m_descriptor(descriptor),
    m_bufferSize(bufferSize),
    m_bypassed(false)
{
    init();

    m_inputBuffers = new sample_t*[m_audioPortsIn.size()];
    m_outputBuffers = new sample_t*[m_audioPortsOut.size()];

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
	m_inputBuffers[i] = new sample_t[bufferSize];
    }
    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
	m_outputBuffers[i] = new sample_t[bufferSize];
    }

    m_ownBuffers = true;

    instantiate(sampleRate);
    if (isOK()) connectPorts();

    /*
    std::cout << m_audioPortsIn.size() << " AUDIO PORTS IN" << std::endl;
    std::cout << m_audioPortsOut.size() << " AUDIO PORTS OUT" << std::endl;
    std::cout << m_controlPorts.size() << " CONTROL PORTS" << std::endl;
    */
}

LADSPAPluginInstance::LADSPAPluginInstance(Rosegarden::InstrumentId instrument,
                                           unsigned long ladspaId,
                                           int position,
					   unsigned long sampleRate,
					   size_t bufferSize,
					   sample_t **inputBuffers,
					   sample_t **outputBuffers,
                                           const LADSPA_Descriptor* descriptor):
    m_instrument(instrument),
    m_ladspaId(ladspaId),
    m_position(position),
    m_instanceHandle(0),
    m_descriptor(descriptor),
    m_bufferSize(bufferSize),
    m_inputBuffers(inputBuffers),
    m_outputBuffers(outputBuffers),
    m_ownBuffers(false),
    m_bypassed(false)
{
    init();

    /*
    std::cout << m_audioPortsIn.size() << " AUDIO PORTS IN" << std::endl;
    std::cout << m_audioPortsOut.size() << " AUDIO PORTS OUT" << std::endl;
    std::cout << m_controlPorts.size() << " CONTROL PORTS" << std::endl;
    */

    instantiate(sampleRate);
    if (isOK()) connectPorts();
}


void
LADSPAPluginInstance::init()
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
		//cout << "ADDING CONTROL PORT" << endl;
		LADSPA_Data *data = new LADSPA_Data(0.0);
		m_controlPorts.push_back(
                    std::pair<unsigned long, LADSPA_Data*>(i, data));
	    } else {
		// We don't do anything at all with control output ports
	    }
        }
#ifdef DEBUG_LADSPA
        else
            std::cerr << "LADSPAPluginInstance::LADSPAPluginInstance - "
                      << "unrecognised port type" << std::endl;
#endif
    }
}

LADSPAPluginInstance::~LADSPAPluginInstance()
{
    for (unsigned int i = 0; i < m_controlPorts.size(); ++i)
        delete m_controlPorts[i].second;

    m_controlPorts.erase(m_controlPorts.begin(), m_controlPorts.end());

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

    m_instanceHandle = m_descriptor->instantiate(m_descriptor, sampleRate);
}

void
LADSPAPluginInstance::activate()
{
    if (m_instanceHandle && m_descriptor->activate)
        m_descriptor->activate(m_instanceHandle);
    /*
    else
        std::cerr << "LADSPAPluginInstance::activate - no ACTIVATE method"
                  << std::endl;
                  */
}

void
LADSPAPluginInstance::connectPorts()
{
    if (m_descriptor == 0) return;

    assert(sizeof(LADSPA_Data) == sizeof(float));
    assert(sizeof(sample_t) == sizeof(float));

    // Ensure we connect _all_ audio ports to our two
    // choices.
    //
    for (unsigned int i = 0; i < m_audioPortsIn.size(); ++i)
    {
        m_descriptor->connect_port(m_instanceHandle,
                                   m_audioPortsIn[i],
                                   (LADSPA_Data *)m_inputBuffers[i]);
    }

    for (unsigned int i = 0; i < m_audioPortsOut.size(); ++i)
    {
        m_descriptor->connect_port(m_instanceHandle,
                                   m_audioPortsOut[i],
                                   (LADSPA_Data *)m_outputBuffers[i]);
    }

    // Connect all control ports
    //
    for (unsigned int i = 0; i < m_controlPorts.size(); ++i)
    {
        m_descriptor->connect_port(m_instanceHandle,
                                   m_controlPorts[i].first,
                                   m_controlPorts[i].second);
    }

}

void
LADSPAPluginInstance::setPortValue(unsigned int portNumber, float value)
{
    for (unsigned int i = 0; i < m_controlPorts.size(); ++i)
    {
        if (m_controlPorts[i].first == portNumber)
        {
            /*
            std::cout << "LADSPAPluginInstance::setPortValue - "
                      << "setting value = " << value << std::endl;
                      */

            (*m_controlPorts[i].second) = value;
        }
    }
}

void
LADSPAPluginInstance::run()
{
    if (m_descriptor && m_descriptor->run)
    {
        /*
        std::cout << "LADSPAPluginInstance::run - running plugin "
                  << "for " << sampleCount << " frames" << std::endl;
                  */
        m_descriptor->run(m_instanceHandle, m_bufferSize);
    }
}

void
LADSPAPluginInstance::deactivate()
{
    /*
    std::cout << "LADSPAPluginInstance::deactivate - " 
              << "descriptor = " << m_descriptor << std::endl;

    std::cout << "LADSPAPluginInstance::deactivate - "
              << "instance handle = " << m_instanceHandle << std::endl;
              */

    if (m_descriptor && m_descriptor->deactivate)
    {
        m_descriptor->deactivate(m_instanceHandle);
    }
    /*
    else
        std::cout << "LADSPAPluginInstance::deactivate - " 
                  << "no DEACTIVATE method" << std::endl;
                  */
}

void
LADSPAPluginInstance::cleanup()
{
    m_descriptor->cleanup(m_instanceHandle);
    m_instanceHandle = 0;
}



}

#endif // HAVE_LADSPA


