// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
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

#include "LADSPAPluginInstance.h"

#ifdef HAVE_LADSPA

namespace Rosegarden
{


LADSPAPluginInstance::LADSPAPluginInstance(Rosegarden::InstrumentId instrument,
                                           unsigned long ladspaId,
                                           int position,
                                           const LADSPA_Descriptor* descriptor):
        m_instrument(instrument),
        m_ladspaId(ladspaId),
        m_position(position),
        m_instanceHandle(0),
        m_descriptor(descriptor),
        m_processed(false)
{
    // Discover ports numbers and identities
    //
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
            //cout << "ADDING CONTROL PORT" << endl;
            LADSPA_Data *data = new LADSPA_Data(0.0);
            m_controlPorts.push_back(
                    std::pair<unsigned long, LADSPA_Data*>(i, data));
        }
        else
            std::cerr << "LADSPAPluginInstance::LADSPAPluginInstance - "
                      << "unrecognised port type" << std::endl;
    }

    /*
    std::cout << m_audioPortsIn.size() << " AUDIO PORTS IN" << std::endl;
    std::cout << m_audioPortsOut.size() << " AUDIO PORTS OUT" << std::endl;
    std::cout << m_controlPorts.size() << " CONTROL PORTS" << std::endl;
    */

}

LADSPAPluginInstance::~LADSPAPluginInstance()
{
    for (unsigned int i = 0; i < m_controlPorts.size(); ++i)
        delete m_controlPorts[i].second;

    m_controlPorts.erase(m_controlPorts.begin(), m_controlPorts.end());
    m_audioPortsIn.clear();
    m_audioPortsOut.clear();
}




void
LADSPAPluginInstance::instantiate(unsigned long sampleRate)
{
    std::cout << "LADSPAPluginInstance::instantiate - plugin unique id = "
              << m_descriptor->UniqueID << std::endl;

    m_instanceHandle = m_descriptor->instantiate(m_descriptor, sampleRate);
}

void
LADSPAPluginInstance::activate()
{
    if (m_instanceHandle && m_descriptor->activate)
        m_descriptor->activate(m_instanceHandle);
    else
        std::cerr << "LADSPAPluginInstance::activate - no ACTIVATE method"
                  << std::endl;
}

void
LADSPAPluginInstance::connectPorts(LADSPA_Data *dataIn1,
                                   LADSPA_Data *dataIn2,
                                   LADSPA_Data *dataOut1,
                                   LADSPA_Data *dataOut2)
{
    if (m_descriptor == 0) return;

    // Ensure we connect _all_ audio ports to our two
    // choices.
    //
    LADSPA_Data *dataIn = dataIn1;
    for (unsigned int i = 0; i < m_audioPortsIn.size(); ++i)
    {
        m_descriptor->connect_port(m_instanceHandle,
                                   m_audioPortsIn[i],
                                   dataIn);
        if (dataIn == dataIn1) dataIn = dataIn2;
        else dataIn = dataIn1;
    }

    LADSPA_Data *dataOut = dataOut1;
    for (unsigned int i = 0; i < m_audioPortsOut.size(); ++i)
    {
        m_descriptor->connect_port(m_instanceHandle,
                                   m_audioPortsOut[i],
                                   dataOut);

        if (dataOut == dataOut1) dataOut = dataOut2;
        else dataOut = dataOut1;
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
LADSPAPluginInstance::setPortValue(unsigned long portNumber, LADSPA_Data value)
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
LADSPAPluginInstance::run(unsigned long sampleCount)
{
    if (m_descriptor && m_descriptor->run)
    {
        /*
        std::cout << "LADSPAPluginInstance::run - running plugin "
                  << "for " << sampleCount << " frames" << std::endl;
                  */
        m_descriptor->run(m_instanceHandle, sampleCount);
        m_processed = true;
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
    else
        std::cout << "LADSPAPluginInstance::deactivate - " 
                  << "no DEACTIVATE method" << std::endl;
}

void
LADSPAPluginInstance::cleanup()
{
    m_descriptor->cleanup(m_instanceHandle);
    m_instanceHandle = 0;
}



}

#endif // HAVE_LADSPA


