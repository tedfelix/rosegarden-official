/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "DSSIPluginInstance.h"
#include "PluginIdentifier.h"
#include "LADSPAPluginFactory.h"

#include <misc/Strings.h>

#include <QtGlobal>

#include <cstdlib>


#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <alloca.h>
#endif

//#define DEBUG_DSSI 1
//#define DEBUG_DSSI_PROCESS 1

namespace Rosegarden
{

#define EVENT_BUFFER_SIZE 1023

DSSIPluginInstance::GroupMap DSSIPluginInstance::m_groupMap;
snd_seq_event_t **DSSIPluginInstance::m_groupLocalEventBuffers = nullptr;
size_t DSSIPluginInstance::m_groupLocalEventBufferCount = 0;
Scavenger<ScavengerArrayWrapper<snd_seq_event_t *> > DSSIPluginInstance::m_bufferScavenger(2, 10);


DSSIPluginInstance::DSSIPluginInstance(PluginFactory *factory,
                                       InstrumentId instrument,
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
        m_programCacheValid(false),
        m_eventBuffer(EVENT_BUFFER_SIZE),
        m_blockSize(blockSize),
        m_idealChannelCount(idealChannelCount),
        m_sampleRate(sampleRate),
        m_latencyPort(nullptr),
        m_run(false),
        m_runSinceReset(false),
        m_bypassed(false),
        m_grouped(false)
{
    pthread_mutex_t initialisingMutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&m_processLock, &initialisingMutex, sizeof(pthread_mutex_t));

#ifdef DEBUG_DSSI

    std::cerr << "DSSIPluginInstance::DSSIPluginInstance(" << identifier << ")"
    << std::endl;
#endif

    init();

    m_inputBuffers = new sample_t * [m_audioPortsIn.size()];
    m_outputBuffers = new sample_t * [m_outputBufferCount];

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        m_inputBuffers[i] = new sample_t[blockSize];
    }
    for (size_t i = 0; i < m_outputBufferCount; ++i) {
        m_outputBuffers[i] = new sample_t[blockSize];
    }

    m_ownBuffers = true;

    m_pending.lsb = m_pending.msb = m_pending.program = -1;

    instantiate(sampleRate);
    if (isOK()) {
        connectPorts();
        activate();
        initialiseGroupMembership();
    }
}

DSSIPluginInstance::DSSIPluginInstance(PluginFactory *factory,
                                       InstrumentId instrument,
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
        m_latencyPort(nullptr),
        m_run(false),
        m_runSinceReset(false),
        m_bypassed(false),
        m_grouped(false)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::DSSIPluginInstance[buffers supplied](" << identifier << ")"
    << std::endl;
#endif

    init();

    m_pending.lsb = m_pending.msb = m_pending.program = -1;

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

    for (unsigned long i = 0; i < descriptor->PortCount; ++i) {
        if (LADSPA_IS_PORT_AUDIO(descriptor->PortDescriptors[i])) {
            if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[i])) {
                m_audioPortsIn.push_back(i);
            } else {
                m_audioPortsOut.push_back(i);
            }
        } else
            if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[i])) {
                if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[i])) {

                    LADSPA_Data *data = new LADSPA_Data(0.0);

                    m_controlPortsIn.push_back(std::pair<unsigned long, LADSPA_Data*>
                                               (i, data));

                    m_backupControlPortsIn.push_back(0.0);
                    m_portChangedSinceProgramChange.push_back(false);

                } else {
                    LADSPA_Data *data = new LADSPA_Data(0.0);
                    m_controlPortsOut.push_back(
                        std::pair<unsigned long, LADSPA_Data*>(i, data));
                    if (!strcmp(descriptor->PortNames[i], "latency") ||
                            !strcmp(descriptor->PortNames[i], "_latency")) {
#ifdef DEBUG_DSSI
                        std::cerr << "Wooo! We have a latency port!" << std::endl;
#endif

                        m_latencyPort = data;
                    }
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

size_t
DSSIPluginInstance::getLatency()
{
#ifdef DEBUG_DSSI 
    //    std::cerr << "DSSIPluginInstance::getLatency(): m_latencyPort " << m_latencyPort << ", m_run " << m_run << std::endl;
#endif

    if (m_latencyPort) {
        if (!m_run) {
            for (unsigned int i = 0; i < getAudioInputCount(); ++i) {
                for (unsigned int j = 0; j < m_blockSize; ++j) {
                    m_inputBuffers[i][j] = 0.f;
                }
            }
            run(RealTime::zeroTime);
	}
#ifdef DEBUG_DSSI

        std::cerr << "DSSIPluginInstance::getLatency(): latency is " << (size_t)(*m_latencyPort + 0.1) << std::endl;
#endif

        return (size_t)(*m_latencyPort + 0.1);
    }
    return 0;
}

void
DSSIPluginInstance::silence()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::silence: m_run " << m_run << ", m_runSinceReset " << m_runSinceReset << std::endl;
#endif

    if (m_run && !m_runSinceReset) {
        return ;
    }
    if (m_instanceHandle != nullptr) {
        deactivate();
        activate();
    }
    m_runSinceReset = false;
}

void
DSSIPluginInstance::discardEvents()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::discardEvents" << std::endl;
#endif

    m_eventBuffer.reset();
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
        return ;
    }

    if (m_instanceHandle != nullptr) {
        deactivate();
    }

    m_idealChannelCount = channels;

    if (channels > m_outputBufferCount) {

        for (size_t i = 0; i < m_outputBufferCount; ++i) {
            delete[] m_outputBuffers[i];
        }

        delete[] m_outputBuffers;

        m_outputBufferCount = channels;

        m_outputBuffers = new sample_t * [m_outputBufferCount];

        for (size_t i = 0; i < m_outputBufferCount; ++i) {
            m_outputBuffers[i] = new sample_t[m_blockSize];
        }

        connectPorts();
    }

    if (m_instanceHandle != nullptr) {
        activate();
    }
}

void
DSSIPluginInstance::detachFromGroup()
{
    if (!m_grouped)
        return ;
    m_groupMap[m_identifier].erase(this);
    m_grouped = false;
}

void
DSSIPluginInstance::initialiseGroupMembership()
{
    if (!m_descriptor->run_multiple_synths) {
        m_grouped = false;
        return ;
    }

    //!!! GroupMap is not actually thread-safe.

    size_t pluginsInGroup = m_groupMap[m_identifier].size();

    if (++pluginsInGroup > m_groupLocalEventBufferCount) {

        size_t nextBufferCount = pluginsInGroup * 2;

        snd_seq_event_t **eventLocalBuffers = new snd_seq_event_t * [nextBufferCount];

        for (size_t i = 0; i < m_groupLocalEventBufferCount; ++i) {
            eventLocalBuffers[i] = m_groupLocalEventBuffers[i];
        }
        for (size_t i = m_groupLocalEventBufferCount; i < nextBufferCount; ++i) {
            eventLocalBuffers[i] = new snd_seq_event_t[EVENT_BUFFER_SIZE];
        }

        if (m_groupLocalEventBuffers) {
            m_bufferScavenger.claim(new ScavengerArrayWrapper<snd_seq_event_t *>
                                    (m_groupLocalEventBuffers));
        }

        m_groupLocalEventBuffers = eventLocalBuffers;
        m_groupLocalEventBufferCount = nextBufferCount;
    }

    m_grouped = true;
    m_groupMap[m_identifier].insert(this);
}

DSSIPluginInstance::~DSSIPluginInstance()
{
//    std::cerr << "DSSIPluginInstance::~DSSIPluginInstance" << std::endl;

    detachFromGroup();

    if (m_instanceHandle != nullptr) {
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

    if (!m_descriptor)
        return ;

    const LADSPA_Descriptor *descriptor = m_descriptor->LADSPA_Plugin;

    if (!descriptor->instantiate) {
        std::cerr << "Bad plugin: plugin id " << descriptor->UniqueID
        << ":" << descriptor->Label
        << " has no instantiate method!" << std::endl;
        return ;
    }

    m_instanceHandle = descriptor->instantiate(descriptor, sampleRate);

    if (m_instanceHandle) {

        if (m_descriptor->get_midi_controller_for_port) {

            for (unsigned long i = 0; i < descriptor->PortCount; ++i) {

                if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[i]) &&
                        LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[i])) {

                    int controller = m_descriptor->get_midi_controller_for_port
                                     (m_instanceHandle, i);

                    if (controller != 0 && controller != 32 &&
                            DSSI_IS_CC(controller)) {

                        m_controllerMap[DSSI_CC_NUMBER(controller)] = i;
                    }
                }
            }
        }
    }
}

void
DSSIPluginInstance::checkProgramCache()
{
    if (m_programCacheValid)
        return ;
    m_cachedPrograms.clear();

#ifdef DEBUG_DSSI

    std::cerr << "DSSIPluginInstance::checkProgramCache" << std::endl;
#endif

    if (!m_descriptor || !m_descriptor->get_program) {
        m_programCacheValid = true;
        return ;
    }

    unsigned long index = 0;
    const DSSI_Program_Descriptor *programDescriptor;
    while ((programDescriptor = m_descriptor->get_program(m_instanceHandle, index))) {
        ++index;
        ProgramDescriptor d;
        d.bank = programDescriptor->Bank;
        d.program = programDescriptor->Program;
        d.name = QString("%1. %2").arg(index).arg(programDescriptor->Name);
        m_cachedPrograms.push_back(d);
    }

#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::checkProgramCache: have " << m_cachedPrograms.size() << " programs" << std::endl;
#endif

    m_programCacheValid = true;
}

QStringList
DSSIPluginInstance::getPrograms()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::getPrograms" << std::endl;
#endif

    if (!m_descriptor)
        return QStringList();

    checkProgramCache();

    QStringList programs;

    for (std::vector<ProgramDescriptor>::iterator i = m_cachedPrograms.begin();
            i != m_cachedPrograms.end(); ++i) {
        programs.push_back(i->name);
    }

    return programs;
}

QString
DSSIPluginInstance::getProgram(int bank, int program)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::getProgram(" << bank << "," << program << ")" << std::endl;
#endif

    if (!m_descriptor)
        return QString();

    checkProgramCache();

    for (std::vector<ProgramDescriptor>::iterator i = m_cachedPrograms.begin();
            i != m_cachedPrograms.end(); ++i) {
        if (i->bank == bank && i->program == program)
            return i->name;
    }

    return QString();
}

unsigned long
DSSIPluginInstance::getProgram(QString name)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::getProgram(" << name << ")" << std::endl;
#endif

    if (!m_descriptor)
        return 0;

    checkProgramCache();

    unsigned long rv;

    for (std::vector<ProgramDescriptor>::iterator i = m_cachedPrograms.begin();
            i != m_cachedPrograms.end(); ++i) {
        if (i->name == name) {
            rv = i->bank;
            rv = (rv << 16) + i->program;
            return rv;
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
    selectProgramAux(program, true);
}

void
DSSIPluginInstance::selectProgramAux(QString program, bool backupPortValues)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance[" << this << "]::selectProgram(" << program << ", " << backupPortValues << ")" << std::endl;
#endif

    if (!m_descriptor)
        return ;

    checkProgramCache();

    if (!m_descriptor->select_program)
        return ;

    bool found = false;
    unsigned long bankNo = 0, programNo = 0;

    for (std::vector<ProgramDescriptor>::iterator i = m_cachedPrograms.begin();
            i != m_cachedPrograms.end(); ++i) {

        if (i->name == program) {

            bankNo = i->bank;
            programNo = i->program;
            found = true;

#ifdef DEBUG_DSSI

            std::cerr << "DSSIPluginInstance::selectProgram(" << program << "): found at bank " << bankNo << ", program " << programNo << std::endl;
#endif

            break;
        }
    }

    if (!found)
        return ;
    m_program = program;

    // DSSI select_program is an audio context call
    pthread_mutex_lock(&m_processLock);
    m_descriptor->select_program(m_instanceHandle, bankNo, programNo);
    pthread_mutex_unlock(&m_processLock);

#ifdef DEBUG_DSSI

    std::cerr << "DSSIPluginInstance::selectProgram(" << program << "): made select_program(" << bankNo << "," << programNo << ") call" << std::endl;
#endif

    if (backupPortValues) {
        for (size_t i = 0; i < m_backupControlPortsIn.size(); ++i) {
            m_backupControlPortsIn[i] = *m_controlPortsIn[i].second;
            m_portChangedSinceProgramChange[i] = false;
        }
    }
}

void
DSSIPluginInstance::activate()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance[" << this << "]::activate" << std::endl;
#endif

    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->activate)
        return ;
    m_descriptor->LADSPA_Plugin->activate(m_instanceHandle);

    for (size_t i = 0; i < m_backupControlPortsIn.size(); ++i) {
        if (m_portChangedSinceProgramChange[i]) {
#ifdef DEBUG_DSSI
            std::cerr << "DSSIPluginInstance::activate: setting port " << m_controlPortsIn[i].first << " to " << m_backupControlPortsIn[i] << std::endl;
#endif

            *m_controlPortsIn[i].second = m_backupControlPortsIn[i];
        }
    }

    if (!m_program.isEmpty()) {
#ifdef DEBUG_DSSI
        std::cerr << "DSSIPluginInstance::activate: restoring program " << m_program << std::endl;
#endif

        selectProgramAux(m_program, false);

        for (size_t i = 0; i < m_backupControlPortsIn.size(); ++i) {
            if (m_portChangedSinceProgramChange[i]) {
#ifdef DEBUG_DSSI
                std::cerr << "DSSIPluginInstance::activate: setting port " << m_controlPortsIn[i].first << " to " << m_backupControlPortsIn[i] << std::endl;
#endif

                *m_controlPortsIn[i].second = m_backupControlPortsIn[i];
            }
        }
    }
}

void
DSSIPluginInstance::connectPorts()
{
    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->connect_port)
        return ;
#ifdef DEBUG_DSSI

    std::cerr << "DSSIPluginInstance::connectPorts: " << m_audioPortsIn.size()
    << " audio ports in, " << m_audioPortsOut.size() << " out, "
    << m_outputBufferCount << " output buffers" << std::endl;
#endif

    Q_ASSERT(sizeof(LADSPA_Data) == sizeof(float));
    Q_ASSERT(sizeof(sample_t) == sizeof(float));

    size_t inbuf = 0, outbuf = 0;

    for (size_t i = 0; i < m_audioPortsIn.size(); ++i) {
        m_descriptor->LADSPA_Plugin->connect_port
        (m_instanceHandle,
         m_audioPortsIn[i],
         (LADSPA_Data *)m_inputBuffers[inbuf]);
        ++inbuf;
    }

    for (size_t i = 0; i < m_audioPortsOut.size(); ++i) {
        m_descriptor->LADSPA_Plugin->connect_port
        (m_instanceHandle,
         m_audioPortsOut[i],
         (LADSPA_Data *)m_outputBuffers[outbuf]);
        ++outbuf;
    }

    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        m_descriptor->LADSPA_Plugin->connect_port
        (m_instanceHandle,
         m_controlPortsIn[i].first,
         m_controlPortsIn[i].second);
    }

    for (size_t i = 0; i < m_controlPortsOut.size(); ++i) {
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
    std::cerr << "DSSIPluginInstance[" << this << "]::setPortValue(" << portNumber << ") to " << value << std::endl;
#endif

    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        if (m_controlPortsIn[i].first == portNumber) {
            LADSPAPluginFactory *f = dynamic_cast<LADSPAPluginFactory *>(m_factory);
            if (f) {
                if (value < f->getPortMinimum(m_descriptor->LADSPA_Plugin, portNumber)) {
                    value = f->getPortMinimum(m_descriptor->LADSPA_Plugin, portNumber);
                }
                if (value > f->getPortMaximum(m_descriptor->LADSPA_Plugin, portNumber)) {
                    value = f->getPortMaximum(m_descriptor->LADSPA_Plugin, portNumber);
                }
            }
            (*m_controlPortsIn[i].second) = value;
            m_backupControlPortsIn[i] = value;
            m_portChangedSinceProgramChange[i] = true;
        }
    }
}

void
DSSIPluginInstance::setPortValueFromController(unsigned int port, int cv)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::setPortValueFromController(" << port << ") to " << cv << std::endl;
#endif

    const LADSPA_Descriptor *p = m_descriptor->LADSPA_Plugin;
    LADSPA_PortRangeHintDescriptor d = p->PortRangeHints[port].HintDescriptor;
    LADSPA_Data lb = p->PortRangeHints[port].LowerBound;
    LADSPA_Data ub = p->PortRangeHints[port].UpperBound;

    float value = (float)cv;

    if (!LADSPA_IS_HINT_BOUNDED_BELOW(d)) {
        if (!LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
            /* unbounded: might as well leave the value alone. */
        } else {
            /* bounded above only. just shift the range. */
            value = ub - 127.0f + value;
        }
    } else {
        if (!LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
            /* bounded below only. just shift the range. */
            value = lb + value;
        } else {
            /* bounded both ends.  more interesting. */
            /* XXX !!! todo: fill in logarithmic, sample rate &c */
            value = lb + ((ub - lb) * value / 127.0f);
        }
    }

    setPortValue(port, value);
}

float
DSSIPluginInstance::getPortValue(unsigned int portNumber)
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::getPortValue(" << portNumber << ")" << std::endl;
#endif

    for (size_t i = 0; i < m_controlPortsIn.size(); ++i) {
        if (m_controlPortsIn[i].first == portNumber) {
            return (*m_controlPortsIn[i].second);
        }
    }

    return 0.0;
}

QString
DSSIPluginInstance::configure(QString key,
                              QString value)
{
    if (!m_descriptor || !m_descriptor->configure)
        return QString();

    if (key == PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY) {
#ifdef DSSI_PROJECT_DIRECTORY_KEY
        key = DSSI_PROJECT_DIRECTORY_KEY;
#else

        return QString();
#endif

    }


#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::configure(" << key << "," << value << ")" << std::endl;
#endif

    char *message = m_descriptor->configure
	(m_instanceHandle, key.toLocal8Bit().data(), value.toLocal8Bit().data());

    m_programCacheValid = false;

    QString qm;

    // Ignore return values from reserved key configuration calls such
    // as project directory
#ifdef DSSI_RESERVED_CONFIGURE_PREFIX

    if (key.startsWith(DSSI_RESERVED_CONFIGURE_PREFIX)) {
        return qm;
    }
#endif

    if (message) {
        if (m_descriptor->LADSPA_Plugin && m_descriptor->LADSPA_Plugin->Label) {
            qm = QString(m_descriptor->LADSPA_Plugin->Label) + ": ";
        }
        qm = qm + QString(message);
        
	free(message);
    }

    return qm;
}

void
DSSIPluginInstance::sendEvent(const RealTime &eventTime,
                              const void *e)
{
    snd_seq_event_t *event = (snd_seq_event_t *)e;
#ifdef DEBUG_DSSI_PROCESS

    std::cerr << "DSSIPluginInstance::sendEvent at " << eventTime << std::endl;
#endif

    snd_seq_event_t ev(*event);

    ev.time.time.tv_sec = eventTime.sec;
    ev.time.time.tv_nsec = eventTime.nsec;

    // DSSI doesn't use MIDI channels, it uses run_multiple_synths instead.
    ev.data.note.channel = 0;

    m_eventBuffer.write(&ev, 1);
}

bool
DSSIPluginInstance::handleController(snd_seq_event_t *ev)
{
    int controller = ev->data.control.param;

#ifdef DEBUG_DSSI_PROCESS

    std::cerr << "DSSIPluginInstance::handleController " << controller << std::endl;
#endif

    if (controller == 0) { // bank select MSB

        m_pending.msb = ev->data.control.value;

    } else if (controller == 32) { // bank select LSB

        m_pending.lsb = ev->data.control.value;

    } else if (controller > 0 && controller < 128) {

        if (m_controllerMap.find(controller) != m_controllerMap.end()) {
            int port = m_controllerMap[controller];
            setPortValueFromController(port, ev->data.control.value);
        } else {
            return true; // pass through to plugin
        }
    }

    return false;
}

void
DSSIPluginInstance::run(const RealTime &blockTime)
{
    static snd_seq_event_t localEventBuffer[EVENT_BUFFER_SIZE];
    int evCount = 0;
    unsigned int evDeferred = 0;

    bool needLock = false;
    if (m_descriptor->select_program)
        needLock = true;

    if (needLock) {
        if (pthread_mutex_trylock(&m_processLock) != 0) {
            for (size_t ch = 0; ch < m_audioPortsOut.size(); ++ch) {
                memset(m_outputBuffers[ch], 0, m_blockSize * sizeof(sample_t));
            }
            return ;
        }
    }

    if (m_grouped) {
        runGrouped(blockTime);
        goto done;
    }

    if (!m_descriptor || !m_descriptor->run_synth) {
        m_eventBuffer.skip(m_eventBuffer.getReadSpace());
        if (m_descriptor->LADSPA_Plugin->run) {
            m_descriptor->LADSPA_Plugin->run(m_instanceHandle, m_blockSize);
        } else {
            for (size_t ch = 0; ch < m_audioPortsOut.size(); ++ch) {
                memset(m_outputBuffers[ch], 0, m_blockSize * sizeof(sample_t));
            }
        }
        m_run = true;
        m_runSinceReset = true;
        if (needLock)
            pthread_mutex_unlock(&m_processLock);
        return ;
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

    while (m_eventBuffer.getReadSpace() > evDeferred) {

        snd_seq_event_t *ev = localEventBuffer + evCount;
        *ev = m_eventBuffer.peek();
        bool accept = true;

        RealTime evTime(ev->time.time.tv_sec, ev->time.time.tv_nsec);

        int frameOffset = 0;
        if (evTime > blockTime) {
            frameOffset = (int)RealTime::realTime2Frame(evTime - blockTime, m_sampleRate);
        }

#ifdef DEBUG_DSSI_PROCESS
        std::cerr << "DSSIPluginInstance::run: evTime " << evTime << ", blockTime " << blockTime << ", frameOffset " << frameOffset
        << ", blockSize " << m_blockSize << std::endl;
        std::cerr << "Type: " << int(ev->type) << ", pitch: " << int(ev->data.note.note) << ", velocity: " << int(ev->data.note.velocity) << std::endl;
#endif

        if (frameOffset >= int(m_blockSize)) {
            if (m_eventBuffer.getReadSpace() == 1) {
                break;
            }
            // The events are possibly unsorted because we can merge events played in
            // realtime with recorded events (generally sorted but not immediate), so
            // we reinsert the current event at the last position and continue to read
            // the other events.
            m_eventBuffer.skip(1);
            m_eventBuffer.write(ev, 1);
            if (m_eventBuffer.getReadSpace() == ++evDeferred) {
                // No other events to read.
                break;
            }
            continue;
        }
        if (frameOffset < 0)
            frameOffset = 0;

        ev->time.tick = (unsigned int)frameOffset;
        m_eventBuffer.skip(1);

        if (ev->type == SND_SEQ_EVENT_CONTROLLER) {
            accept = handleController(ev);
        } else if (ev->type == SND_SEQ_EVENT_PGMCHANGE) {
            m_pending.program = ev->data.control.value;
            accept = false;
        }

        if (accept) {
            if (++evCount >= EVENT_BUFFER_SIZE)
                break;
        }
    }

    if (m_pending.program >= 0 && m_descriptor->select_program) {

        int program = m_pending.program;
        int bank = m_pending.lsb + 128 * m_pending.msb;

#ifdef DEBUG_DSSI

        std::cerr << "DSSIPluginInstance::run: making select_program(" << bank << "," << program << " call" << std::endl;
#endif

        m_pending.lsb = m_pending.msb = m_pending.program = -1;
        m_descriptor->select_program(m_instanceHandle, bank, program);

#ifdef DEBUG_DSSI

        std::cerr << "DSSIPluginInstance::run: made select_program(" << bank << "," << program << " call" << std::endl;
#endif

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
    if (needLock)
        pthread_mutex_unlock(&m_processLock);

    if (m_audioPortsOut.empty()) {
        // copy inputs to outputs
        for (size_t ch = 0; ch < m_idealChannelCount; ++ch) {
            size_t sch = ch % m_audioPortsIn.size();
            for (size_t i = 0; i < m_blockSize; ++i) {
                m_outputBuffers[ch][i] = m_inputBuffers[sch][i];
            }
        }
    } else if (m_idealChannelCount < m_audioPortsOut.size()) {
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
    m_run = true;
    m_runSinceReset = true;
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

        return ;
    }

#ifdef DEBUG_DSSI_PROCESS
    std::cerr << "DSSIPluginInstance::runGrouped(" << blockTime << "): I'm the first, running" << std::endl;
#endif

    size_t index = 0;
    unsigned long *counts = (unsigned long *)
                            alloca(m_groupLocalEventBufferCount * sizeof(unsigned long));
    unsigned long *evDeferred = (unsigned long *)
                                alloca(m_groupLocalEventBufferCount * sizeof(unsigned long));
    LADSPA_Handle *instances = (LADSPA_Handle *)
                               alloca(m_groupLocalEventBufferCount * sizeof(LADSPA_Handle));

    for (PluginSet::iterator i = s.begin(); i != s.end(); ++i) {

        if (index >= m_groupLocalEventBufferCount)
            break;

        DSSIPluginInstance *instance = *i;
        counts[index] = 0;
        evDeferred[index] = 0;
        instances[index] = instance->m_instanceHandle;

#ifdef DEBUG_DSSI_PROCESS

        std::cerr << "DSSIPluginInstance::runGrouped(" << blockTime << "): running " << instance << std::endl;
#endif

        if (instance->m_pending.program >= 0 &&
                instance->m_descriptor->select_program) {
            int program = instance->m_pending.program;
            int bank = instance->m_pending.lsb + 128 * instance->m_pending.msb;
            instance->m_pending.lsb = instance->m_pending.msb = instance->m_pending.program = -1;
            instance->m_descriptor->select_program
            (instance->m_instanceHandle, bank, program);
        }

        while (instance->m_eventBuffer.getReadSpace() > evDeferred[index]) {

            snd_seq_event_t *ev = m_groupLocalEventBuffers[index] + counts[index];
            *ev = instance->m_eventBuffer.peek();
            bool accept = true;

            RealTime evTime(ev->time.time.tv_sec, ev->time.time.tv_nsec);

            int frameOffset = 0;
            if (evTime > blockTime) {
                frameOffset = (int)RealTime::realTime2Frame(evTime - blockTime, m_sampleRate);
            }

#ifdef DEBUG_DSSI_PROCESS
            std::cerr << "DSSIPluginInstance::runGrouped: evTime " << evTime << ", frameOffset " << frameOffset
            << ", block size " << m_blockSize << std::endl;
#endif

            if (frameOffset >= int(m_blockSize)) {
                if (instance->m_eventBuffer.getReadSpace() == 1) {
                    break;
                }
                // The events are possibly unsorted because we can merge events played in
                // realtime with recorded events (generally sorted but not immediate), so
                // we reinsert the current event at the last position and continue to read
                // the other events.
                instance->m_eventBuffer.skip(1);
                instance->m_eventBuffer.write(ev, 1);
                if (instance->m_eventBuffer.getReadSpace() == ++evDeferred[index]) {
                    // No other events to read.
                    break;
                }
                continue;
            }

            if (frameOffset < 0)
                frameOffset = 0;

            ev->time.tick = (unsigned int)frameOffset;
            instance->m_eventBuffer.skip(1);

            if (ev->type == SND_SEQ_EVENT_CONTROLLER) {
                accept = instance->handleController(ev);
            } else if (ev->type == SND_SEQ_EVENT_PGMCHANGE) {
                instance->m_pending.program = ev->data.control.value;
                accept = false;
            }

            if (accept) {
                if (++counts[index] >= EVENT_BUFFER_SIZE)
                    break;
            }
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

    if (!m_descriptor || !m_descriptor->LADSPA_Plugin->deactivate)
        return ;

    for (size_t i = 0; i < m_backupControlPortsIn.size(); ++i) {
        m_backupControlPortsIn[i] = *m_controlPortsIn[i].second;
    }

    m_descriptor->LADSPA_Plugin->deactivate(m_instanceHandle);
#ifdef DEBUG_DSSI

    std::cerr << "DSSIPluginInstance::deactivate " << m_identifier << " done" << std::endl;
#endif

    m_bufferScavenger.scavenge();
}

void
DSSIPluginInstance::cleanup()
{
#ifdef DEBUG_DSSI
    std::cerr << "DSSIPluginInstance::cleanup " << m_identifier << std::endl;
#endif

    if (!m_descriptor)
        return ;

    if (!m_descriptor->LADSPA_Plugin->cleanup) {
        std::cerr << "Bad plugin: plugin id "
        << m_descriptor->LADSPA_Plugin->UniqueID
        << ":" << m_descriptor->LADSPA_Plugin->Label
        << " has no cleanup method!" << std::endl;
        return ;
    }

    m_descriptor->LADSPA_Plugin->cleanup(m_instanceHandle);
    m_instanceHandle = nullptr;
#ifdef DEBUG_DSSI

    std::cerr << "DSSIPluginInstance::cleanup " << m_identifier << " done" << std::endl;
#endif
}


}


