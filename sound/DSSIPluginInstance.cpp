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

DSSIPluginInstance::GroupMap DSSIPluginInstance::m_groupMap;
snd_seq_event_t **DSSIPluginInstance::m_groupLocalEventBuffers = 0;
size_t DSSIPluginInstance::m_groupLocalEventBufferCount = 0;


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
    m_bypassed(false),
    m_grouped(false)
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
    if (isOK()) {
	connectPorts();
	activate();
	initialiseGroupMembership();
    }
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
    m_bypassed(false),
    m_grouped(false)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::DSSIPluginInstance[buffers supplied](" << identifier << ")"
	      << std::endl;
#endif

    init();

    instantiate(sampleRate);
    if (isOK()) {
	connectPorts();
	activate();
	if (m_descriptor->run_multiple_synths) {
	    m_grouped = true;
	    initialiseGroupMembership();
	}
    }
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
DSSIPluginInstance::silence()
{
    if (m_instanceHandle != 0) {
	deactivate();
	activate();
    }
}

void
DSSIPluginInstance::setIdealChannelCount(size_t channels)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::setIdealChannelCount: channel count "
	      << channels << " (was " << m_idealChannelCount << ")" << std::endl;
#endif

    if (channels == m_idealChannelCount) {
	silence();
	return;
    }

    if (m_instanceHandle != 0) {
	deactivate();
    }

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

    if (m_instanceHandle != 0) {
	activate();
    }
}

void
DSSIPluginInstance::detachFromGroup()
{
    if (!m_grouped) return;
    m_groupMap[m_identifier].erase(this);
    m_grouped = false;
}

void
DSSIPluginInstance::initialiseGroupMembership()
{
    if (!m_descriptor->run_multiple_synths) {
	m_grouped = false;
	return;
    }

    //!!! GroupMap is not actually thread-safe.

    size_t pluginsInGroup = m_groupMap[m_identifier].size();

    if (++pluginsInGroup > m_groupLocalEventBufferCount) {

	snd_seq_event_t **eventLocalBuffers = new snd_seq_event_t *[pluginsInGroup];
	for (size_t i = 0; i < m_groupLocalEventBufferCount; ++i) {
	    eventLocalBuffers[i] = m_groupLocalEventBuffers[i];
	}
	for (size_t i = m_groupLocalEventBufferCount; i < pluginsInGroup; ++i) {
	    eventLocalBuffers[i] = new snd_seq_event_t[EVENT_BUFFER_SIZE];
	}

	m_groupLocalEventBuffers = eventLocalBuffers;
	m_groupLocalEventBufferCount = pluginsInGroup;

	//!!! need to scavenge old event buffer thingy
    }

    m_grouped = true;
    m_groupMap[m_identifier].insert(this);
}

DSSIPluginInstance::~DSSIPluginInstance()
{
    std::cerr << "DSSIPluginInstance::~DSSIPluginInstance" << std::endl;

    detachFromGroup();

    if (m_instanceHandle != 0) {
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

#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::getPrograms" << std::endl;
#endif

    if (!m_descriptor || !m_descriptor->get_program) return programs;

    unsigned long index = 0;
    const DSSI_Program_Descriptor *programDescriptor;

    while ((programDescriptor = m_descriptor->get_program(m_instanceHandle, index))) {
	++index;
	programs.append(QString("%1. %2").arg(index).arg(programDescriptor->Name));
    }
    
    return programs;
}

QString
DSSIPluginInstance::getProgram(int bank, int program)
{
    QString programName;

#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::getProgram(" << bank << "," << program << ")" << std::endl;
#endif

    if (!m_descriptor || !m_descriptor->get_program) return programName;

    unsigned long index = 0;
    const DSSI_Program_Descriptor *programDescriptor;

    while ((programDescriptor = m_descriptor->get_program(m_instanceHandle, index))) {
	++index;
	if (int(programDescriptor->Bank) == bank &&
	    int(programDescriptor->Program) == program) {
	    programName = QString("%1. %2").arg(index).arg(programDescriptor->Name);
	    break;
	}
    }
    
    return programName;
}

unsigned long
DSSIPluginInstance::getProgram(QString name)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::getProgram(" << name << ")" << std::endl;
#endif

    if (!m_descriptor || !m_descriptor->get_program) return 0;

    unsigned long index = 0;
    const DSSI_Program_Descriptor *programDescriptor;

    while ((programDescriptor = m_descriptor->get_program(m_instanceHandle, index))) {
	++index;
	QString programName = QString("%1. %2").arg(index).arg(programDescriptor->Name);
	if (programName == name) {
	    return (programDescriptor->Bank << 16) + programDescriptor->Program;
	}
    }
    
    return 0;
}

QString
DSSIPluginInstance::getCurrentProgram()
{
    return m_program;
}

void
DSSIPluginInstance::selectProgram(QString program)
{
    // better if this were more efficient!

#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::selectProgram(" << program << ")" << std::endl;
#endif

    if (!m_descriptor || !m_descriptor->get_program || !m_descriptor->select_program) return;

    unsigned long index = 0;
    const DSSI_Program_Descriptor *programDescriptor;

    bool found = false;
    unsigned long bankNo = 0, programNo = 0;

    while ((programDescriptor = m_descriptor->get_program(m_instanceHandle, index))) {
	++index;
	QString name = QString("%1. %2").arg(index).arg(programDescriptor->Name);
	if (name == program) {
	    bankNo = programDescriptor->Bank;
	    programNo = programDescriptor->Program;

#ifdef DEBUG_DSSI
	    std::cerr << "DSSIPluginInstance::selectProgram(" << program << "): found at bank " << bankNo << ", program " << programNo << std::endl;
#endif

	    found = true;
	    break;
	}
    }

    if (found) {
	//!!! no -- must be scheduled for call from audio context, with run()
	m_descriptor->select_program(m_instanceHandle, bankNo, programNo);
	m_program = program;
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

    if (m_descriptor->get_program && m_descriptor->select_program) {

	if (!m_program) {
	    const DSSI_Program_Descriptor *programDescriptor;
	    if ((programDescriptor = m_descriptor->get_program(m_instanceHandle, 0))) {
		m_program = QString("%1. %2").arg(0).arg(programDescriptor->Name);
	    }
	}

	if (m_program) selectProgram(m_program);
    }
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

QString
DSSIPluginInstance::configure(QString key,
			      QString value)
{
    if (!m_descriptor || !m_descriptor->configure) return QString();
    
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::configure(" << key << "," << value << ")" << std::endl;
#endif

    char *message = m_descriptor->configure(m_instanceHandle, key.data(), value.data());

    QString qm;
    if (message) {
	qm = message;
	free(message);
    }
    return qm;
}

void
DSSIPluginInstance::sendEvent(const RealTime &eventTime,
			      const snd_seq_event_t *event)
{
#ifdef DEBUG_DSSI_PROCESS
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
    static snd_seq_event_t localEventBuffer[EVENT_BUFFER_SIZE];
    int evCount = 0;

    if (m_grouped) {
	runGrouped(blockTime);
	goto done;
    }

    if (!m_descriptor || !m_descriptor->run_synth) {
	for (size_t ch = 0; ch < m_audioPortsOut.size(); ++ch) {
	    memset(m_outputBuffers[ch], 0, m_blockSize * sizeof(sample_t));
	}
	return;
    }

#ifdef DEBUG_DSSI_PROCESS
    std::cerr << "DSSIPluginInstance::run(" << blockTime << ")" << std::endl;
#endif

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

 done:
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

    m_lastRunTime = blockTime;
}

void
DSSIPluginInstance::runGrouped(const RealTime &blockTime)
{
    // If something else in our group has just been called for this
    // block time (but we haven't) then we should just write out the
    // results and return; if we have just been called for this block
    // time or nothing else in the group has been, we should run the
    // whole group.

    bool needRun = true;

    PluginSet &s = m_groupMap[m_identifier];

#ifdef DEBUG_DSSI_PROCESS
    std::cerr << "DSSIPluginInstance::runGrouped(" << blockTime << "): this is " << this << "; " << s.size() << " elements in m_groupMap[" << m_identifier << "]" << std::endl;
#endif

    if (m_lastRunTime != blockTime) {
	for (PluginSet::iterator i = s.begin(); i != s.end(); ++i) {
	    DSSIPluginInstance *instance = *i;
	    if (instance != this && instance->m_lastRunTime == blockTime) {
#ifdef DEBUG_DSSI_PROCESS
		std::cerr << "DSSIPluginInstance::runGrouped(" << blockTime << "): plugin " << instance << " has already been run" << std::endl;
#endif
		needRun = false;
	    }
	}
    }

    if (!needRun) {
#ifdef DEBUG_DSSI_PROCESS
	std::cerr << "DSSIPluginInstance::runGrouped(" << blockTime << "): already run, returning" << std::endl;
#endif
	return;
    }

#ifdef DEBUG_DSSI_PROCESS
    std::cerr << "DSSIPluginInstance::runGrouped(" << blockTime << "): I'm the first, running" << std::endl;
#endif

    size_t index = 0;
    unsigned long *counts = (unsigned long *)
	alloca(m_groupLocalEventBufferCount * sizeof(unsigned long));
    LADSPA_Handle *instances = (LADSPA_Handle *)
	alloca(m_groupLocalEventBufferCount * sizeof(LADSPA_Handle));

    for (PluginSet::iterator i = s.begin(); i != s.end(); ++i) {

	if (index >= m_groupLocalEventBufferCount) break;

	DSSIPluginInstance *instance = *i;
	counts[index] = 0;
	instances[index] = instance->m_instanceHandle;

#ifdef DEBUG_DSSI_PROCESS
	std::cerr << "DSSIPluginInstance::runGrouped(" << blockTime << "): running " << instance << std::endl;
#endif

	while (instance->m_eventBuffer.getReadSpace() > 0) {

	    snd_seq_event_t *ev = m_groupLocalEventBuffers[index] + counts[index];
	    *ev = instance->m_eventBuffer.peek();

	    RealTime evTime(ev->time.time.tv_sec, ev->time.time.tv_nsec);

	    int frameOffset = 0;
	    if (evTime > blockTime) {
		frameOffset = RealTime::realTime2Frame(evTime - blockTime, m_sampleRate);
	    }

#ifdef DEBUG_DSSI_PROCESS
	    std::cerr << "DSSIPluginInstance::runGrouped: evTime " << evTime << ", frameOffset " << frameOffset
		      << ", block size " << m_blockSize << std::endl;
#endif

	    if (frameOffset >= int(m_blockSize)) break;
	    if (frameOffset < 0) frameOffset = 0;

	    ev->time.tick = frameOffset;
	    instance->m_eventBuffer.skip(1);
	    if (++counts[index] >= EVENT_BUFFER_SIZE) break;
	}

	++index;
    }

    m_descriptor->run_multiple_synths(index,
				      instances,
				      m_blockSize,
				      m_groupLocalEventBuffers,
				      counts);
}


void
DSSIPluginInstance::deactivate()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::deactivate " << m_identifier << std::endl;
#endif
    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->deactivate) return;
    m_descriptor->LADSPA_Plugin->deactivate(m_instanceHandle);
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::deactivate " << m_identifier << " done" << std::endl;
#endif
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


