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
//#define DEBUG_DSSI_PROCESS 1

namespace Rosegarden
{

#define EVENT_BUFFER_SIZE 1023

DSSIPluginInstance::DSSIPluginInstance(PluginFactory *factory,
				       Rosegarden::InstrumentId instrument,
				       QString identifier,
				       int position,
				       unsigned long sampleRate,
				       size_t blockSize,
				       int idealChannelCount,
				       const DSSI_Descriptor* descriptor) :
    RunnablePluginInstance(factory, identifier),
    m_instrument(instrument),
    m_position(position),
    m_descriptor(descriptor),
    m_eventBuffer(EVENT_BUFFER_SIZE),
    m_blockSize(blockSize),
    m_idealChannelCount(idealChannelCount),
    m_sampleRate(sampleRate),
    m_bypassed(false)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::DSSIPluginInstance(" << identifier << ")"
	      << std::endl;
#endif

    init();

    m_inputBuffers  = new sample_t*[m_audioPortsIn.size()];
    m_outputBuffers = new sample_t*[m_outputBufferCount];

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
	m_inputBuffers[i] = new sample_t[blockSize];
    }
    for (size_t i = 0; i < m_outputBufferCount; ++i) {
	m_outputBuffers[i] = new sample_t[blockSize];
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
				       size_t blockSize,
				       sample_t **inputBuffers,
				       sample_t **outputBuffers,
				       const DSSI_Descriptor* descriptor) :
    RunnablePluginInstance(factory, identifier),
    m_instrument(instrument),
    m_position(position),
    m_descriptor(descriptor),
    m_eventBuffer(EVENT_BUFFER_SIZE),
    m_blockSize(blockSize),
    m_inputBuffers(inputBuffers),
    m_outputBuffers(outputBuffers),
    m_ownBuffers(false),
    m_idealChannelCount(0),
    m_sampleRate(sampleRate),
    m_bypassed(false)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::DSSIPluginInstance[buffers supplied](" << identifier << ")"
	      << std::endl;
#endif

    init();

    instantiate(sampleRate);
    if (isOK()) connectPorts();
}


void
DSSIPluginInstance::init()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::init" << std::endl;
#endif

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

    m_outputBufferCount = std::max(m_idealChannelCount, m_audioPortsOut.size());
}

void
DSSIPluginInstance::setIdealChannelCount(size_t channels)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::setIdealChannelCount: channel count "
	      << channels << " (was " << m_idealChannelCount << ")" << std::endl;
#endif

    if (channels == m_idealChannelCount) return;
    m_idealChannelCount = channels;

    if (channels > m_outputBufferCount) {

	for (size_t i = 0; i < m_outputBufferCount; ++i) {
	    delete[] m_outputBuffers[i];
	}

	delete[] m_outputBuffers;

	m_outputBufferCount = channels;

	m_outputBuffers = new sample_t*[m_outputBufferCount];

	for (size_t i = 0; i < m_outputBufferCount; ++i) {
	    m_outputBuffers[i] = new sample_t[m_blockSize];
	}

	connectPorts();
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

QStringList
DSSIPluginInstance::getPrograms()
{
    QStringList programs;

    if (!m_descriptor || !m_descriptor->get_program) return programs;

    unsigned long index = 0;
    DSSI_Program_Descriptor programDescriptor;

    while (m_descriptor->get_program(m_instanceHandle, index, &programDescriptor)) {
	++index;
	programs.append(QString("%1. %2").arg(index).arg(programDescriptor.Name));
	free(programDescriptor.Name);
    }
    
    return programs;
}

void
DSSIPluginInstance::selectProgram(QString program)
{
    // better if this were more efficient!

    if (!m_descriptor || !m_descriptor->get_program || !m_descriptor->select_program) return;

    unsigned long index = 0;
    DSSI_Program_Descriptor programDescriptor;

    bool found = false;
    unsigned long bankNo = 0, programNo = 0;

    while (m_descriptor->get_program(m_instanceHandle, index, &programDescriptor)) {
	++index;
	QString name = QString("%1. %2").arg(index).arg(programDescriptor.Name);
	free(programDescriptor.Name);
	if (name == program) {
	    bankNo = programDescriptor.Bank;
	    programNo = programDescriptor.Program;
	    found = true;
	    break;
	}
    }

    if (found) {
	m_descriptor->select_program(m_instanceHandle, bankNo, programNo);
    }
}

void
DSSIPluginInstance::activate()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::activate" << std::endl;
#endif

    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->activate) return;
    m_eventBuffer.reset();
    m_descriptor->LADSPA_Plugin->activate(m_instanceHandle);
}

void
DSSIPluginInstance::connectPorts()
{
    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->connect_port) return;
    std::cerr << "DSSIPluginInstance::connectPorts: " << m_audioPortsIn.size() 
	      << " audio ports in, " << m_audioPortsOut.size() << " out, "
	      << m_outputBufferCount << " output buffers" << std::endl;

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
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::setPortValue(" << portNumber << ") to " << value << std::endl;
#endif
    for (unsigned int i = 0; i < m_controlPortsIn.size(); ++i)
    {
        if (m_controlPortsIn[i].first == portNumber)
        {
            (*m_controlPortsIn[i].second) = value;
        }
    }
}

void
DSSIPluginInstance::sendEvent(const RealTime &eventTime,
			      const snd_seq_event_t *event)
{
    //!!! how to ensure events are ordered in the target buffer?

#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::sendEvent at " << eventTime << std::endl;
#endif
    snd_seq_event_t ev(*event);
    ev.time.time.tv_sec = eventTime.sec;
    ev.time.time.tv_nsec = eventTime.nsec;
    m_eventBuffer.write(&ev, 1);
}

void
DSSIPluginInstance::run(const RealTime &blockTime)
{
    if (!m_descriptor || !m_descriptor->run_synth) return;
 
#ifdef DEBUG_DSSI_PROCESS
    std::cerr << "DSSIPluginInstance::run(" << blockTime << ")" << std::endl;
#endif
   
    static snd_seq_event_t localEventBuffer[EVENT_BUFFER_SIZE];
    int evCount = 0;

#ifdef DEBUG_DSSI_PROCESS
    if (m_eventBuffer.getReadSpace() > 0) {
	std::cerr << "DSSIPluginInstance::run: event buffer has "
		  << m_eventBuffer.getReadSpace() << " event(s) in it" << std::endl;
    }
#endif

    while (m_eventBuffer.getReadSpace() > 0) {

	snd_seq_event_t *ev = localEventBuffer + evCount;
	*ev = m_eventBuffer.peek();

	RealTime evTime(ev->time.time.tv_sec, ev->time.time.tv_nsec);

	int frameOffset = 0;
	if (evTime > blockTime) {
	    frameOffset = RealTime::realTime2Frame(evTime - blockTime, m_sampleRate);
	}

#ifdef DEBUG_DSSI_PROCESS
	std::cerr << "DSSIPluginInstance::run: evTime " << evTime << ", frameOffset " << frameOffset
		  << ", block size " << m_blockSize << std::endl;
#endif

	if (frameOffset >= int(m_blockSize)) break;
	if (frameOffset < 0) frameOffset = 0;

	ev->time.tick = frameOffset;
	m_eventBuffer.skip(1);
	if (++evCount >= EVENT_BUFFER_SIZE) break;
    }

#ifdef DEBUG_DSSI_PROCESS
    std::cerr << "DSSIPluginInstance::run: running with " << evCount << " events"
	      << std::endl;
#endif

    m_descriptor->run_synth(m_instanceHandle, m_blockSize,
			    localEventBuffer, evCount);

#ifdef DEBUG_DSSI_PROCESS
//    for (int i = 0; i < m_blockSize; ++i) {
//	std::cout << m_outputBuffers[0][i] << " ";
//	if (i % 8 == 0) std::cout << std::endl;
//    }
#endif

    if (m_idealChannelCount < m_audioPortsOut.size()) {
	if (m_idealChannelCount == 1) {
	    // mix down to mono
	    for (size_t ch = 1; ch < m_audioPortsOut.size(); ++ch) {
		for (size_t i = 0; i < m_blockSize; ++i) {
		    m_outputBuffers[0][i] += m_outputBuffers[ch][i];
		}
	    }
	}
    } else if (m_idealChannelCount > m_audioPortsOut.size()) {
	// duplicate
	for (size_t ch = m_audioPortsOut.size(); ch < m_idealChannelCount; ++ch) {
	    size_t sch = (ch - m_audioPortsOut.size()) % m_audioPortsOut.size();
	    for (size_t i = 0; i < m_blockSize; ++i) {
		m_outputBuffers[ch][i] = m_outputBuffers[sch][i];
	    }
	}
    }	
}

void
DSSIPluginInstance::deactivate()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::deactivate" << std::endl;
#endif
    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->deactivate) return;
    m_descriptor->LADSPA_Plugin->deactivate(m_instanceHandle);
}

void
DSSIPluginInstance::cleanup()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::cleanup" << std::endl;
#endif
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


