
// -*- c-basic-offset: 4 -*-

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

#include "JackDriver.h"
#include "AlsaDriver.h"
#include "MappedStudio.h"
#include "AudioProcess.h"
#include "Profiler.h"
#include "AudioLevel.h"

#ifdef HAVE_ALSA
#ifdef HAVE_LIBJACK

#define DEBUG_ALSA 1

//!!! need to get this merged with alsa driver's audit trail

static std::string _audit;

// I was going to prevent output to std::cout if NDEBUG was set, but actually
// it's probably a good idea to continue to send to std::cout as well -- it
// shouldn't even get noticed unless someone's actually trying to debug,
// and it's necessary if the sequencer crashes altogether (because the
// status button in the GUI won't work if the sequencer's crashed).

#define AUDIT_STREAM _auditStream
#if (__GNUC__ < 3)
#include <strstream>
#define AUDIT_START std::strstream _auditStream
#ifdef NOT_DEFINED_NDEBUG
#define AUDIT_UPDATE _auditStream << std::ends; _audit += _auditStream.str();
#else
#define AUDIT_UPDATE _auditStream << std::ends; \
                     std::string _auditChunk = _auditStream.str(); \
                     _audit += _auditChunk; std::cerr << _auditChunk;
#endif
#else
#include <sstream>
#define AUDIT_START std::stringstream _auditStream
#ifdef NOT_DEFINED_NDEBUG
#define AUDIT_UPDATE _audit += _auditStream.str();
#else
#define AUDIT_UPDATE std::string _auditChunk = _auditStream.str(); \
                     _audit += _auditChunk; std::cerr << _auditChunk;
#endif
#endif

namespace Rosegarden
{

JackDriver::JackDriver(AlsaDriver *alsaDriver) :
    m_client(0),
    m_bufferSize(0),
    m_sampleRate(0),
    m_tempOutBuffer(0),
    m_jackTransportEnabled(false),
    m_jackTransportMaster(false),
    m_audioPlayLatency(0, 0),
    m_audioRecordLatency(0, 0),
    m_waiting(false),
    m_waitingState(JackTransportStopped),
    m_waitingToken(0),
    m_bussMixer(0),
    m_instrumentMixer(0),
    m_fileReader(0),
    m_fileWriter(0),
    m_alsaDriver(alsaDriver),
    m_masterLevel(1.0),
    m_masterPan(0.0),
    m_ok(false)
{
    assert(sizeof(sample_t) == sizeof(float));
    initialise();
}

JackDriver::~JackDriver()
{
    std::cerr << "JackDriver::~JackDriver" << std::endl;

    m_ok = false; // prevent any more work in process()

    if (m_client)
    {
#ifdef DEBUG_ALSA
        std::cerr << "JackDriver::shutdown - closing JACK client"
                  << std::endl;
#endif
	std::cerr << "deactivating" << std::endl;
        if (jack_deactivate(m_client))
	{
#ifdef DEBUG_ALSA
	    std::cerr << "JackDriver::shutdown - deactivation failed"
		      << std::endl;
#endif
	}

        for (unsigned int i = 0; i < m_inputPorts.size(); ++i)
        {
	    std::cerr << "unregistering input " << i << std::endl;
            if (jack_port_unregister(m_client, m_inputPorts[i]))
            {
#ifdef DEBUG_ALSA
                std::cerr << "JackDriver::shutdown - "
                          << "can't unregister input port " << i + 1
                          << std::endl;
#endif
            }
        }

	for (unsigned int i = 0; i < m_outputSubmasters.size(); ++i)
	{
	    std::cerr << "unregistering output sub " << i << std::endl;
	    if (jack_port_unregister(m_client, m_outputSubmasters[i]))
	    {
#ifdef DEBUG_ALSA
		std::cerr << "JackDriver::shutdown - "
			  << "can't unregister output submaster " << i+1 << std::endl;
#endif
	    }
	}

	for (unsigned int i = 0; i < m_outputMonitors.size(); ++i)
	{
	    std::cerr << "unregistering output mon " << i << std::endl;
	    if (jack_port_unregister(m_client, m_outputMonitors[i]))
	    {
#ifdef DEBUG_ALSA
		std::cerr << "JackDriver::shutdown - "
			  << "can't unregister output monitor " << i+1 << std::endl;
#endif
	    }
	}
	
	for (unsigned int i = 0; i < m_outputMasters.size(); ++i)
	{
	    std::cerr << "unregistering output master " << i << std::endl;
	    if (jack_port_unregister(m_client, m_outputMasters[i]))
	    {
#ifdef DEBUG_ALSA
		std::cerr << "JackDriver::shutdown - "
			  << "can't unregister output master " << i+1 << std::endl;
#endif
	    }
	}
                
	std::cerr << "closing client" << std::endl;
        jack_client_close(m_client);
        m_client = 0;
    }

    std::cerr << "JackDriver::~JackDriver: deleting disk and mix managers" << std::endl;
    delete m_fileReader;
    delete m_fileWriter;
    delete m_instrumentMixer;
    delete m_bussMixer;

    std::cerr << "JackDriver::~JackDriver exiting" << std::endl;
}

void
JackDriver::initialise()
{
    AUDIT_START;
    AUDIT_STREAM << std::endl;

    std::string jackClientName = "rosegarden";

    // attempt connection to JACK server
    //
    if ((m_client = jack_client_new(jackClientName.c_str())) == 0)
    {
        AUDIT_STREAM << "JackDriver::initialiseAudio - "
                     << "JACK server not running"
                     << std::endl;
        return;
    }

    // set callbacks
    //
    jack_set_process_callback(m_client, jackProcessStatic, this);
    jack_set_buffer_size_callback(m_client, jackBufferSize, this);
    jack_set_sample_rate_callback(m_client, jackSampleRate, this);
    jack_on_shutdown(m_client, jackShutdown, this);
    jack_set_graph_order_callback(m_client, jackGraphOrder, this);
    //jack_set_xrun_callback(m_client, jackXRun, this);
    jack_set_sync_callback(m_client, jackSyncCallback, this);

    // get and report the sample rate and buffer size
    //
    m_sampleRate = jack_get_sample_rate(m_client);
    m_bufferSize = jack_get_buffer_size(m_client);

    AUDIT_STREAM << "JackDriver::initialiseAudio - JACK sample rate = "
                 << m_sampleRate << "Hz, buffer size = " << m_bufferSize
		 << std::endl;

    // Get the initial buffer size before we activate the client
    //

    // create processing buffer(s)
    //
    m_tempOutBuffer = new sample_t[m_bufferSize];
/*!!!
    // Create output ports
    //
    for (int i = 0; i < 4; ++i) {//!!!

	char namebuffer[20];

	snprintf(namebuffer, 19, "submaster %d out L", i+1);
	port = jack_port_register
	    (m_client, namebuffer,
	     JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	m_outputSubmasters.push_back(port);

	snprintf(namebuffer, 19, "submaster %d out R", i+1);
	port = jack_port_register
	    (m_client, namebuffer,
	     JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	m_outputSubmasters.push_back(port);
    }
*/
    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "creating disk thread" << std::endl;

    m_fileReader = new AudioFileReader(m_alsaDriver, m_sampleRate);
    m_fileWriter = new AudioFileWriter(m_alsaDriver, m_sampleRate);
    m_instrumentMixer = new AudioInstrumentMixer
	(m_alsaDriver, m_fileReader, m_sampleRate,
	 m_bufferSize < 1024 ? 1024 : m_bufferSize);
    m_bussMixer = new AudioBussMixer
	(m_alsaDriver, m_instrumentMixer, m_sampleRate,
	 m_bufferSize < 1024 ? 1024 : m_bufferSize);
    m_instrumentMixer->setBussMixer(m_bussMixer);

    // Create and connect the default numbers of ports.  We always create
    // one stereo pair each of master and monitor outs, and then we create
    // record ins, fader outs and submaster outs according to the user's
    // preferences.  Since we don't know the user's preferences yet, we'll
    // start by creating one pair of record ins and no fader or submaster
    // outs.
    //
    createMainOutputs(); // one stereo pair master, one pair monitor
    createRecordInputs(1);

    if (jack_activate(m_client))
    {
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "client activation failed" << std::endl;
	AUDIT_UPDATE;
	return;
    }

    // no submaster outs by default
//    createInputPorts(2, false);

//    AUDIT_STREAM << "JackDriver::initialiseAudio - "
//                 << "created input ports" << std::endl;

    // Now set up the default connections.

    std::string playback_1, playback_2;

    const char **ports =
	jack_get_ports(m_client, NULL, NULL,
		       JackPortIsPhysical | JackPortIsInput);
    
    if (ports)
    {
	if (ports[0]) playback_1 = std::string(ports[0]);
	if (ports[1]) playback_2 = std::string(ports[1]);
	
	// count ports
	unsigned int i = 0;
	for (i = 0; ports[i]; i++);
	AUDIT_STREAM << "JackDriver::initialiseAudio - "
		     << "found " << i << " JACK physical outputs"
		     << std::endl;
    }
    else
	AUDIT_STREAM << "JackDriver::createJackInputPorts - "
		     << "no JACK physical outputs found"
		     << std::endl;
    free(ports);
    
    if (playback_1 != "")
    {
        AUDIT_STREAM << "JackDriver::initialiseAudio - "
                     << "connecting from "
                     << "\"" << jack_port_name(m_outputMasters[0])
                     << "\" to \"" << playback_1.c_str() << "\""
                     << std::endl;

        // connect our client up to the ALSA ports - first left output
        //
        if (jack_connect(m_client, jack_port_name(m_outputMasters[0]),
                         playback_1.c_str()))
        {
            AUDIT_STREAM << "JackDriver::initialiseAudio - "
                         << "cannot connect to JACK output port" << std::endl;
	    AUDIT_UPDATE;
            return;
        }

        if (jack_connect(m_client, jack_port_name(m_outputMonitors[0]),
                         playback_1.c_str()))
        {
            AUDIT_STREAM << "JackDriver::initialiseAudio - "
                         << "cannot connect to JACK output port" << std::endl;
	    AUDIT_UPDATE;
            return;
        }
    }

    if (playback_2 != "")
    {
        AUDIT_STREAM << "JackDriver::initialiseAudio - "
                     << "connecting from "
                     << "\"" << jack_port_name(m_outputMasters[1])
                     << "\" to \"" << playback_2.c_str() << "\""
                     << std::endl;

        if (jack_connect(m_client, jack_port_name(m_outputMasters[1]),
                         playback_2.c_str()))
        {
            AUDIT_STREAM << "JackDriver::initialiseAudio - "
                         << "cannot connect to JACK output port" << std::endl;
        }

        if (jack_connect(m_client, jack_port_name(m_outputMonitors[1]),
                         playback_2.c_str()))
        {
            AUDIT_STREAM << "JackDriver::initialiseAudio - "
                         << "cannot connect to JACK output port" << std::endl;
        }
    }

    // Get the latencies from JACK and set them as RealTime
    //
    jack_nframes_t outputLatency =
        jack_port_get_total_latency(m_client, m_outputMasters[0]);

    jack_nframes_t inputLatency = 
        jack_port_get_total_latency(m_client, m_inputPorts[0]);

    double latency = double(outputLatency) / double(m_sampleRate);

    // Set the audio latencies ready for collection by the GUI
    //
    m_audioPlayLatency = RealTime(int(latency),
				  int((latency - int(latency)) * 1e9));

    latency = double(inputLatency) / double(m_sampleRate);

    m_audioRecordLatency = RealTime(int(latency),
				    int((latency - int(latency)) * 1e9));

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "JACK playback latency " << m_audioPlayLatency << std::endl;

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "JACK record latency " << m_audioRecordLatency << std::endl;

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "initialised JACK audio subsystem"
                 << std::endl;

    m_ok = true;

    AUDIT_UPDATE;
}

void
JackDriver::createMainOutputs()
{
    jack_port_t *port =  jack_port_register
	(m_client, "master out L",
	 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    m_outputMasters.push_back(port);

    port = jack_port_register
	(m_client, "master out R",
	 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    m_outputMasters.push_back(port);

    port = jack_port_register
	(m_client, "record monitor out L",
	 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    m_outputMonitors.push_back(port);

    port = jack_port_register
	(m_client, "record monitor out R",
	 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    m_outputMonitors.push_back(port);
}

void
JackDriver::createFaderOutputs(int pairs)
{
    int pairsNow = m_outputInstruments.size() / 2;
    if (pairs == pairsNow) return;

    for (int i = pairsNow; i < pairs; ++i) {

	char namebuffer[22];

	snprintf(namebuffer, 21, "fader %d out L", i+1);
	m_outputInstruments.push_back(jack_port_register(m_client,
							 namebuffer,
							 JACK_DEFAULT_AUDIO_TYPE,
							 JackPortIsOutput,
							 0));
	snprintf(namebuffer, 21, "fader %d out R", i+1);
	m_outputInstruments.push_back(jack_port_register(m_client,
							 namebuffer,
							 JACK_DEFAULT_AUDIO_TYPE,
							 JackPortIsOutput,
							 0));
    }

    while ((int)m_outputInstruments.size() > pairs * 2) {
	std::vector<jack_port_t *>::iterator itr = m_outputInstruments.end();
	--itr;
	jack_port_unregister(m_client, *itr);
	m_outputInstruments.erase(itr);
    }
}
  
void
JackDriver::createSubmasterOutputs(int pairs)
{
    int pairsNow = m_outputSubmasters.size() / 2;
    if (pairs == pairsNow) return;

    for (int i = pairsNow; i < pairs; ++i) {

	char namebuffer[22];

	snprintf(namebuffer, 21, "submaster %d out L", i+1);
	m_outputSubmasters.push_back(jack_port_register(m_client,
							 namebuffer,
							 JACK_DEFAULT_AUDIO_TYPE,
							 JackPortIsOutput,
							 0));
	snprintf(namebuffer, 21, "submaster %d out R", i+1);
	m_outputSubmasters.push_back(jack_port_register(m_client,
							 namebuffer,
							 JACK_DEFAULT_AUDIO_TYPE,
							 JackPortIsOutput,
							 0));
    }

    while ((int)m_outputSubmasters.size() > pairs * 2) {
	std::vector<jack_port_t *>::iterator itr = m_outputSubmasters.end();
	--itr;
	jack_port_unregister(m_client, *itr);
	m_outputSubmasters.erase(itr);
    }
}

void
JackDriver::createRecordInputs(int pairs)
{
    int pairsNow = m_inputPorts.size() / 2;
    if (pairs == pairsNow) return;

//!!! need to check return values from jack_port_register in this
// and preceding two functions
    for (int i = pairsNow; i < pairs; ++i) {

	char namebuffer[22];

	snprintf(namebuffer, 21, "record in %d", i * 2);
	m_inputPorts.push_back(jack_port_register(m_client,
						  namebuffer,
						  JACK_DEFAULT_AUDIO_TYPE,
						  JackPortIsInput,
						  0));
	snprintf(namebuffer, 21, "record in %d", i * 2 + 1);
	m_inputPorts.push_back(jack_port_register(m_client,
						  namebuffer,
						  JACK_DEFAULT_AUDIO_TYPE,
						  JackPortIsInput,
						  0));
    }

    while ((int)m_outputSubmasters.size() > pairs * 2) {
	std::vector<jack_port_t *>::iterator itr = m_outputSubmasters.end();
	--itr;
	jack_port_unregister(m_client, *itr);
	m_outputSubmasters.erase(itr);
    }
/*!!!
    while (m_outputSubmasters.size() > pairs * 2) {
	std::vector<jack_port_t *>::iterator itr = m_outputSubmasters.end();
	--itr;
	jack_port_unregister(m_client, *itr);
	m_outputSubmasters.erase(itr);
    }
    jack_port_t *inputPort;
    char portName[10];
    for (unsigned int i = 0; i < totalPorts; ++i)
    {
        sprintf(portName, "record in %d", i + 1);
        inputPort = jack_port_register(m_client,
                                       portName,
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsInput|JackPortIsTerminal,
                                       0);
	if (!inputPort) {
	    AUDIT_STREAM << "JackDriver::createJackInputPorts: failed to create input port " << portName << std::endl;
	    return;
	}
	m_inputPorts.push_back(inputPort);


	AUDIT_STREAM << "JackDriver::createJackInputPorts - "
		     << "adding input port " << i + 1 << std::endl;
    }
*/
}
    

void
JackDriver::setAudioPortCounts(int inputs, int submasters)
{
    std::cerr << "JackDriver::setAudioPortCounts(" << inputs << "," << submasters << ")" << std::endl;
    //!!! ignore for now.
}

void
JackDriver::setAudioPorts(bool faderOuts, bool submasterOuts)
{
    std::cerr << "JackDriver::setAudioPorts(" << faderOuts << "," << submasterOuts << ")" << std::endl;

    if (faderOuts) {
	InstrumentId instrumentBase;
	int instruments;
	m_alsaDriver->getAudioInstrumentNumbers(instrumentBase, instruments);
	createFaderOutputs(instruments);
    } else {
	createFaderOutputs(0);
    }

    if (submasterOuts) {
	createSubmasterOutputs(4); //!!! for now
    } else {
	createSubmasterOutputs(0);
    }
}
/*!!!
void
JackDriver::createInputPorts(unsigned int totalPorts, bool deactivate)
{
    if (!m_client) return;

    AUDIT_START;

    // Out port connections if we need them
    //
    const char **outLeftPort = 0, **outRightPort = 0;

    // deactivate client
    //
    if (deactivate)
    {
        // store output connections for reconnect after port mods
        //
        outLeftPort = jack_port_get_connections(m_outputMasters[0]);
        outRightPort = jack_port_get_connections(m_outputMasters[1]);

        if (jack_deactivate(m_client))
        {
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "client deactivation failed" << std::endl;
        }
    }

    // Unregister any we already have connected
    //
    for (unsigned int i = 0; i < m_inputPorts.size(); ++i)
        jack_port_unregister(m_client, m_inputPorts[i]);
    m_inputPorts.clear();

    jack_port_t *inputPort;
    char portName[10];
    for (unsigned int i = 0; i < totalPorts; ++i)
    {
        sprintf(portName, "record in %d", i + 1);
        inputPort = jack_port_register(m_client,
                                       portName,
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsInput|JackPortIsTerminal,
                                       0);
	if (!inputPort) {
	    AUDIT_STREAM << "JackDriver::createJackInputPorts: failed to create input port " << portName << std::endl;
	    return;
	}
	m_inputPorts.push_back(inputPort);


	AUDIT_STREAM << "JackDriver::createJackInputPorts - "
		     << "adding input port " << i + 1 << std::endl;
    }
    
    // reactivate client
    //
    if (jack_activate(m_client))
    {
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "client deactivation failed" << std::endl;
    }

    std::string capture_1, capture_2;

    // Assign port directly if they were specified
    //
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "getting ports" << std::endl;
        const char **ports =
            jack_get_ports(m_client, NULL, NULL,
                           JackPortIsPhysical|JackPortIsOutput);

        if (ports)
        {
            if (ports[0]) capture_1 = std::string(ports[0]);
            if (ports[1]) capture_2 = std::string(ports[1]);

            // count ports
            unsigned int i = 0;
            for (i = 0; ports[i]; i++);
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "found " << i << " JACK physical inputs"
                         << std::endl;
        }
        else
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "no JACK physical inputs found"
                         << std::endl;
        free(ports);


    if (capture_1 != "")
    {
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "connecting from "
                     << "\"" << capture_1.c_str() << "\" to \""
                     << jack_port_name(m_inputPorts[0]) << "\""
                     << std::endl;

        // now input
        if (jack_connect(m_client, capture_1.c_str(),
                         jack_port_name(m_inputPorts[0])))
        {
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "cannot connect to JACK input port" << std::endl;
        }
    }

    if (capture_2 != "")
    {
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "connecting from "
                     << "\"" << capture_2.c_str() << "\" to \""
                     << jack_port_name(m_inputPorts[1]) << "\""
                     << std::endl;

        if (jack_connect(m_client, capture_2.c_str(),
                         jack_port_name(m_inputPorts[1])))
        {
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "cannot connect to JACK input port" << std::endl;
        }
    }

//!!!
    // Reconnect out ports
    if (deactivate)
    {

        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "reconnecting JACK output port (left)"
                     << std::endl;

        if (outLeftPort && outLeftPort[0])
        {
            if (jack_connect(m_client,
                             jack_port_name(m_outputMasters[0]),
                             outLeftPort[0]))
            {
                AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                             << "cannot reconnect JACK output port (left)"
                             << std::endl;
            }
        }

        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "reconnecting JACK output port (right)"
                     << std::endl;

        if (outRightPort && outRightPort[0])
        {
            if (jack_connect(m_client, 
                             jack_port_name(m_outputMasters[1]),
                             outRightPort[0]))
            {
                AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                             << "cannot reconnect JACK output port (right)"
                             << std::endl;
            }
        }
    }

    AUDIT_UPDATE;
}
*/

int
JackDriver::jackProcessStatic(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);
    if (inst) return inst->jackProcess(nframes);
    else return 0;
}

int
JackDriver::jackProcess(jack_nframes_t nframes)
{
    if (!m_ok) return;

    if (!m_bussMixer) {
	return jackProcessEmpty(nframes);
    }

    SequencerDataBlock *sdb = m_alsaDriver->getSequencerDataBlock();
    
//    Rosegarden::Profiler profiler("JackProcess, clocks running");

    jack_position_t position;
    if (m_jackTransportEnabled) {
	jack_transport_state_t state = jack_transport_query(m_client, &position);

//	std::cout << "process: jack state is " << state << std::endl;
	if (state == JackTransportStopped) {
	    if (m_alsaDriver->isPlaying() &&
		m_alsaDriver->areClocksRunning()) {
		ExternalTransport *transport = 
		    m_alsaDriver->getExternalTransportControl();
		if (transport) {
		    m_waitingToken =
			transport->transportJump
			(ExternalTransport::TransportStopAtTime,
			 RealTime::frame2RealTime(position.frame,
						  position.frame_rate));
		}
	    }
	    return jackProcessEmpty(nframes);
	} else if (state == JackTransportStarting) {
	    return jackProcessEmpty(nframes);
	} else if (state == JackTransportRolling) {
	    if (m_waiting) {
		m_alsaDriver->startClocksApproved();
		m_waiting = false;
	    }
	}
    } else if (!m_alsaDriver->areClocksRunning()) {
	return jackProcessEmpty(nframes);
    }

    InstrumentId instrumentBase;
    int instruments;
    m_alsaDriver->getAudioInstrumentNumbers(instrumentBase, instruments);

    jackProcessRecord(nframes);

    // We always have the master out

    sample_t *master[2] = {
	static_cast<sample_t *>
	(jack_port_get_buffer(m_outputMasters[0], nframes)),
	static_cast<sample_t *>
	(jack_port_get_buffer(m_outputMasters[1], nframes))
    };

    memset(master[0], 0, nframes * sizeof(sample_t));
    memset(master[1], 0, nframes * sizeof(sample_t));
	
    int bussCount = m_bussMixer->getBussCount();
    
    // If we have any busses, then we just mix from them (but we still
    // need to keep ourselves up to date by reading and monitoring the
    // instruments).  If we have no busses, mix direct from instruments.

    for (int buss = 0; buss < bussCount; ++buss) {

	sample_t *submaster[2] = { 0, 0 };
	sample_t peak[2] = { 0.0, 0.0 };

	if ((int)m_outputSubmasters.size() > buss * 2 + 1) {
	    submaster[0] = static_cast<sample_t *>
		(jack_port_get_buffer(m_outputSubmasters[buss * 2], nframes));
	    submaster[1] = static_cast<sample_t *>
		(jack_port_get_buffer(m_outputSubmasters[buss * 2 + 1], nframes));
	}

	if (!submaster[0]) submaster[0] = m_tempOutBuffer;
	if (!submaster[1]) submaster[1] = m_tempOutBuffer;

	for (int ch = 0; ch < 2; ++ch) {

	    RingBuffer<AudioBussMixer::sample_t> *rb =
		m_bussMixer->getRingBuffer(buss, ch);

	    if (!rb || m_bussMixer->isBussDormant(buss)) {
		if (rb) rb->skip(nframes);
		if (submaster[ch])
		    memset(submaster[ch], 0, nframes * sizeof(sample_t));
	    } else {
		size_t actual = rb->read(submaster[ch], nframes);
		if (actual < nframes) {
		    std::cerr << "WARNING: buffer underrun in buss ringbuffer " << buss << ":" << ch << std::endl;
		}
		for (size_t i = 0; i < nframes; ++i) {
		    sample_t sample = submaster[ch][i];
		    if (sample > peak[ch]) peak[ch] = sample;
		    master[ch][i] += sample;
		}
	    }
	}

	if (sdb) {
	    Rosegarden::LevelInfo info;
	    info.level = AudioLevel::multiplier_to_fader
		(peak[0], 127, AudioLevel::LongFader);
	    info.levelRight = AudioLevel::multiplier_to_fader
		(peak[1], 127, AudioLevel::LongFader);

	    sdb->setSubmasterLevel(buss, info);
	}
    }
    
    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instruments; ++id) {
	
	sample_t *instrument[2] = { 0, 0 };
	sample_t peak[2] = { 0.0, 0.0 };

	unsigned int index = id - instrumentBase;
	if (m_outputInstruments.size() > index * 2 + 1) {
	    instrument[0] = static_cast<sample_t *>
		(jack_port_get_buffer(m_outputInstruments[index * 2], nframes));
	    instrument[1] = static_cast<sample_t *>
		(jack_port_get_buffer(m_outputInstruments[index * 2 + 1], nframes));
	}

	if (!instrument[0]) instrument[0] = m_tempOutBuffer;
	if (!instrument[1]) instrument[1] = m_tempOutBuffer;

	for (int ch = 0; ch < 2; ++ch) {

	    RingBuffer<AudioInstrumentMixer::sample_t, 2> *rb =
		m_instrumentMixer->getRingBuffer(id, ch);

	    if (!rb || m_instrumentMixer->isInstrumentDormant(id)) {
		if (rb) rb->skip(nframes);
		if (instrument[ch])
		    memset(instrument[ch], 0, nframes * sizeof(sample_t));
	    } else {
		size_t actual = rb->read(instrument[ch], nframes);
		if (actual < nframes) {
		    std::cerr << "WARNING: buffer underrun in instrument ringbuffer " << id << ":" << ch << std::endl;
		}
		for (size_t i = 0; i < nframes; ++i) {
		    sample_t sample = instrument[ch][i];
		    if (sample > peak[ch]) peak[ch] = sample;
		    if (bussCount == 0) master[ch][i] += sample;
		}
	    }
	}

	if (sdb) {
	    Rosegarden::LevelInfo info;
	    info.level = AudioLevel::multiplier_to_fader
		(peak[0], 127, AudioLevel::LongFader);
	    info.levelRight = AudioLevel::multiplier_to_fader
		(peak[1], 127, AudioLevel::LongFader);

	    sdb->setInstrumentLevel(id, info);
	}
    }

    // Get master fader levels.

    float volume = AudioLevel::dB_to_multiplier(m_masterLevel);
    float pan = m_masterPan;

    float masterGain[2] = {
	volume * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0),
	volume * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0)
    };

    float masterPeak[2] = { 0.0, 0.0 };

    for (int ch = 0; ch < 2; ++ch) {
	for (size_t i = 0; i < nframes; ++i) {
	    sample_t sample = master[ch][i] * masterGain[ch];
	    if (sample > masterPeak[ch]) masterPeak[ch] = sample;
	    master[ch][i] = sample;
	}
    }

    if (sdb) {
	Rosegarden::LevelInfo info;
	info.level = AudioLevel::multiplier_to_fader
	    (masterPeak[0], 127, AudioLevel::LongFader);
	info.levelRight = AudioLevel::multiplier_to_fader
	    (masterPeak[1], 127, AudioLevel::LongFader);
	
	sdb->setMasterLevel(info);
    }

    if (m_alsaDriver->isPlaying()) {
	m_bussMixer->signal();
    }

    return 0;
}

int
JackDriver::jackProcessEmpty(jack_nframes_t nframes)
{
    sample_t *buffer;

    buffer = static_cast<sample_t *>
	(jack_port_get_buffer(m_outputMasters[0], nframes));
    if (buffer) memset(buffer, 0, nframes * sizeof(sample_t));

    buffer = static_cast<sample_t *>
	(jack_port_get_buffer(m_outputMasters[1], nframes));
    if (buffer) memset(buffer, 0, nframes * sizeof(sample_t));

    buffer = static_cast<sample_t *>
	(jack_port_get_buffer(m_outputMonitors[0], nframes));
    if (buffer) memset(buffer, 0, nframes * sizeof(sample_t));

    buffer = static_cast<sample_t *>
	(jack_port_get_buffer(m_outputMonitors[1], nframes));
    if (buffer) memset(buffer, 0, nframes * sizeof(sample_t));

    for (unsigned int i = 0; i < m_outputSubmasters.size(); ++i) {
	buffer = static_cast<sample_t *>
	    (jack_port_get_buffer(m_outputSubmasters[i], nframes));
	if (buffer) memset(buffer, 0, nframes * sizeof(sample_t));
    }
    
    for (unsigned int i = 0; i < m_outputInstruments.size(); ++i) {
	buffer = static_cast<sample_t *>
	    (jack_port_get_buffer(m_outputInstruments[i], nframes));
	if (buffer) memset(buffer, 0, nframes * sizeof(sample_t));
    }

    return 0;
}
    
int
JackDriver::jackProcessRecord(jack_nframes_t nframes)
{
    SequencerDataBlock *sdb = m_alsaDriver->getSequencerDataBlock();
    bool wroteSomething = false;

    //!!! This absolutely should not be done here, as it involves
    //taking out a pthread lock -- either do it in the record
    //thread only, or cache it

    int channels = 2;
    int connection = 0;
/*
    MappedAudioFader *fader =
	m_alsaDriver->getMappedStudio()->
	getAudioFader(m_alsaDriver->getAudioMonitoringInstrument());
    
    
    if (fader) {
	float f = 2;
	//!!! nah, this is just going to return 1 for the moment because
	// we don't have the proper correlation between instruments and recording
//	(void)fader->getProperty(MappedAudioFader::Channels, f);
	int channels = (int)f;
    }
*/  
    // Get input buffer
    //
    sample_t *inputBufferLeft = 0, *inputBufferRight = 0;
    
    inputBufferLeft = static_cast<sample_t*>
	(jack_port_get_buffer(m_inputPorts[connection * channels], nframes));
    
//    if (channels == 2) {
	inputBufferRight = static_cast<sample_t*>
	    (jack_port_get_buffer(m_inputPorts[connection * channels + 1], nframes));
//    }
    
    //!!! want an actual instrument id
    //!!! want file writer to apply volume from fader?
    
    if (m_alsaDriver->getRecordStatus() == RECORD_AUDIO &&
	m_alsaDriver->areClocksRunning()) {

	if (inputBufferLeft) {
	    m_fileWriter->write(m_alsaDriver->getAudioMonitoringInstrument(),
				inputBufferLeft, 0, nframes);
	}
    
	if (channels == 2) {
	    if (inputBufferRight) {
		m_fileWriter->write(m_alsaDriver->getAudioMonitoringInstrument(),
				    inputBufferRight, 1, nframes);
	    }
	} else {
	    if (inputBufferLeft) {
		m_fileWriter->write(m_alsaDriver->getAudioMonitoringInstrument(),
				    inputBufferLeft, 1, nframes);
	    }
	}
    
	wroteSomething = true;
    }

    if (m_outputMonitors.size() > 0) {

	sample_t *buf = 
	    static_cast<sample_t *>
	    (jack_port_get_buffer(m_outputMonitors[0], nframes));
	memcpy(buf, inputBufferLeft, nframes * sizeof(sample_t));

	if (channels == 2 && m_outputMonitors.size() > 1) {
	    buf =
		static_cast<sample_t *>
		(jack_port_get_buffer(m_outputMonitors[1], nframes));
	    memcpy(buf, inputBufferRight, nframes * sizeof(sample_t));
	}
    }
  
    sample_t peakLeft = 0.0, peakRight = 0.0;
  
    for (size_t i = 0; i < nframes; ++i) {
	if (inputBufferLeft[i] > peakLeft) peakLeft = inputBufferLeft[i];
    }
    if (channels == 2) {
	for (size_t i = 0; i < nframes; ++i) {
	    if (inputBufferRight[i] > peakRight) peakRight = inputBufferRight[i];
	}
    } else {
	peakRight = peakLeft;
    }

    if (sdb) {
	Rosegarden::LevelInfo info;
	info.level = AudioLevel::multiplier_to_fader
	    (peakLeft, 127, AudioLevel::LongFader);
	info.levelRight = AudioLevel::multiplier_to_fader
	    (peakRight, 127, AudioLevel::LongFader);
	sdb->setRecordLevel(info);
    }

    if (wroteSomething) {
	m_fileWriter->signal();
    }

    return 0;
}


int
JackDriver::jackSyncCallback(jack_transport_state_t state,
			     jack_position_t *position,
			     void *arg)
{
    JackDriver *inst = (JackDriver *)arg;
    if (!inst) return true; // or rather, return "huh?"

    if (!inst->m_jackTransportEnabled) return true; // ignore

    ExternalTransport *transport =
	inst->m_alsaDriver->getExternalTransportControl();
    if (!transport) return true;

    std::cout << "jackSyncCallback(" << state << ", " << position->frame << "), m_waiting " << inst->m_waiting << ", playing " << inst->m_alsaDriver->isPlaying() << std::endl;

    ExternalTransport::TransportRequest request =
	ExternalTransport::TransportNoChange;

    if (inst->m_alsaDriver->isPlaying()) {

	if (state == JackTransportStarting) {
	    request = ExternalTransport::TransportJumpToTime;
	} else if (state == JackTransportStopped) {
	    request = ExternalTransport::TransportStop;
	}

    } else {

	if (state == JackTransportStarting) {
	    request = ExternalTransport::TransportStartAtTime;
	} else if (state == JackTransportStopped) {
	    request = ExternalTransport::TransportNoChange;
	}
    }

    if (!inst->m_waiting || inst->m_waitingState != state) {

	if (request == ExternalTransport::TransportJumpToTime ||
	    request == ExternalTransport::TransportStartAtTime) {

	    RealTime rt = RealTime::frame2RealTime(position->frame,
						   position->frame_rate);

	    std::cout << "Requesting jump to " << rt << std::endl;
	    
	    inst->m_waitingToken = transport->transportJump(request, rt);

	    std::cout << "My token is " << inst->m_waitingToken << std::endl;

	} else if (request == ExternalTransport::TransportStop) {

	    std::cout << "Requesting state change to " << request << std::endl;
	    
	    inst->m_waitingToken = transport->transportChange(request);

	    std::cout << "My token is " << inst->m_waitingToken << std::endl;

	} else if (request == ExternalTransport::TransportNoChange) {

	    std::cout << "Requesting no state change!" << std::endl;

	    inst->m_waitingToken = transport->transportChange(request);

	    std::cout << "My token is " << inst->m_waitingToken << std::endl;
	}

	inst->m_waiting = true;
	inst->m_waitingState = state;
	return 0;

    } else {

	if (transport->isTransportSyncComplete(inst->m_waitingToken)) {
	    std::cout << "Sync complete" << std::endl;
	    return 1;
	} else {
	    std::cout << "Sync not complete" << std::endl;
	    return 0;
	}
    }
}

bool
JackDriver::start()
{
    if (!m_client) return true;

    prebufferAudio();

    // m_waiting is true if we are waiting for the JACK transport
    // to finish a change of state.

    if (m_jackTransportEnabled) {

	// Where did this request come from?  Are we just responding
	// to an external sync?

	ExternalTransport *transport =
	m_alsaDriver->getExternalTransportControl();
	
	if (transport) {
	    if (transport->isTransportSyncComplete(m_waitingToken)) {

		// Nope, this came from Rosegarden

		std::cout << "start: asking for start, setting waiting" << std::endl;
		m_waiting = true;
		m_waitingState = JackTransportStarting;
		jack_transport_locate(m_client,
				      RealTime::realTime2Frame
				      (m_alsaDriver->getSequencerTime(),
				       m_sampleRate));
		jack_transport_start(m_client);
	    } else {
		std::cout << "start: waiting already" << std::endl;
	    }
	}
	return false;
    }
    
    std::cout << "start: not on transport" << std::endl;
    return true;
}

void
JackDriver::stop()
{
    if (!m_client) return;

    flushAudio();

    if (m_jackTransportEnabled) {

	// Where did this request come from?  Is this a result of our
	// sync to a transport that has in fact already stopped?

	ExternalTransport *transport =
	    m_alsaDriver->getExternalTransportControl();

	if (transport) {
	    if (transport->isTransportSyncComplete(m_waitingToken)) {
		// No, we have no outstanding external requests; this
		// must have genuinely been requested from within
		// Rosegarden, so:
		jack_transport_stop(m_client);
	    } else {
		// Nothing to do
	    }
	}
    }

    if (m_instrumentMixer) m_instrumentMixer->resetAllPlugins();
}


// Pick up any change of buffer size
//
int
JackDriver::jackBufferSize(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);

#ifdef DEBUG_ALSA
    std::cerr << "JackDriver::jackBufferSize - buffer size changed to "
              << nframes << std::endl;
#endif 

    inst->m_bufferSize = nframes;

    // Recreate our temporary mix buffers to the new size
    //
    //!!! need buffer size change callbacks on plugins (so long as they
    // have internal buffers) and the mix manager, with locks acquired
    // appropriately

    delete [] inst->m_tempOutBuffer;
    inst->m_tempOutBuffer = new sample_t[inst->m_bufferSize];

    return 0;
}

// Sample rate change
//
int
JackDriver::jackSampleRate(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);

#ifdef DEBUG_ALSA
    std::cerr << "JackDriver::jackSampleRate - sample rate changed to "
               << nframes << std::endl;
#endif
    inst->m_sampleRate = nframes;

    return 0;
}

void
JackDriver::jackShutdown(void * /*arg*/)
{
#ifdef DEBUG_ALSA
    std::cerr << "JackDriver::jackShutdown() - callback received - doing nothing yet" << std::endl;
#endif

    /*
    JackDriver *inst = static_cast<JackDriver*>(arg);
    if (inst) inst->shutdown();
    delete inst;
    */
}

int
JackDriver::jackGraphOrder(void *)
{
    //std::cerr << "JackDriver::jackGraphOrder" << std::endl;
    return 0;
}

int
JackDriver::jackXRun(void *)
{
#ifdef DEBUG_ALSA
    std::cerr << "JackDriver::jackXRun" << std::endl;
#endif
    return 0;
}

void
JackDriver::prebufferAudio()
{
    if (!m_instrumentMixer) return;

    // For JACK transport sync, the next slice could start anywhere.
    // We need to query it.
//!!! urch, no, seems this is being called before the transport has actually
// repositioned
/*
    if (m_jackTransportEnabled) {
	jack_position_t position;
	jack_transport_query(m_client, &position);
	m_instrumentMixer->fillBuffers
	    (RealTime::frame2RealTime(position.frame, m_sampleRate));
	m_bussMixer->fillBuffers();
    } else {
*/
	m_instrumentMixer->fillBuffers
	    (getNextSliceStart(m_alsaDriver->getSequencerTime()));
	m_bussMixer->fillBuffers();
//    }
}

void
JackDriver::flushAudio()
{
    if (!m_instrumentMixer) return;

    m_instrumentMixer->emptyBuffers();
    m_bussMixer->emptyBuffers();
}
 
void
JackDriver::kickAudio()
{
    if (m_fileReader) m_fileReader->kick();
    if (m_instrumentMixer) m_instrumentMixer->kick();
    if (m_bussMixer) m_bussMixer->kick();
    if (m_fileWriter) m_fileWriter->kick();
}

void
JackDriver::updateAudioLevels()
{
    MappedAudioBuss *mbuss =
	m_alsaDriver->getMappedStudio()->getAudioBuss(0);

    if (mbuss) {
	float level = 0.0;
	(void)mbuss->getProperty(MappedAudioBuss::Level, level);
	m_masterLevel = level;

//!!!	float pan = 0.0;
//!!!	(void)mbuss->getProperty(MappedAudioBuss::Pan, pan);
//!!!   m_masterPan = pan;
    }
}

RealTime
JackDriver::getNextSliceStart(const RealTime &now) const
{
    jack_nframes_t frame = RealTime::realTime2Frame(now, m_sampleRate);
    jack_nframes_t rounded = frame;
    rounded /= m_bufferSize;
    rounded *= m_bufferSize;

    if (rounded == frame)
	return RealTime::frame2RealTime(rounded, m_sampleRate);
    else
	return RealTime::frame2RealTime(rounded + m_bufferSize, m_sampleRate);
}


int
JackDriver::getAudioQueueLocks()
{
    // We have to lock the mixers first, because the mixers can try to
    // lock the disk manager from within a locked section -- so if we
    // locked the disk manager first we would risk deadlock when
    // trying to acquire the instrument mixer lock

    int rv = 0;
    if (m_bussMixer) {
	std::cerr << "JackDriver::getAudioQueueLocks: trying to lock buss mixer" << std::endl;
	rv = m_bussMixer->getLock();
	if (rv) return rv;
    }
    if (m_instrumentMixer) {
	std::cerr << "ok, now trying for instrument mixer" << std::endl;
	rv = m_instrumentMixer->getLock();
	if (rv) return rv;
    }
    if (m_fileReader) {
	std::cerr << "ok, now trying for disk reader" << std::endl;
	rv = m_fileReader->getLock();
	if (rv) return rv;
    }
    if (m_fileWriter) {
	std::cerr << "ok, now trying for disk writer" << std::endl;
	rv = m_fileWriter->getLock();
    }
    std::cerr << "ok" << std::endl;
    return rv;
}

int
JackDriver::tryAudioQueueLocks()
{
    int rv = 0;
    if (m_bussMixer) {
	rv = m_bussMixer->tryLock();
	if (rv) return rv;
    }
    if (m_instrumentMixer) {
	rv = m_instrumentMixer->tryLock();
	if (rv) {
	    if (m_bussMixer) {
		m_bussMixer->releaseLock();
	    }
	}
    }
    if (m_fileReader) {
	rv = m_fileReader->tryLock();
	if (rv) {
	    if (m_instrumentMixer) {
		m_instrumentMixer->releaseLock();
	    }
	    if (m_bussMixer) {
		m_bussMixer->releaseLock();
	    }
	}
    }
    if (m_fileWriter) {
	rv = m_fileWriter->tryLock();
	if (rv) {
	    if (m_fileReader) {
		m_fileReader->releaseLock();
	    }
	    if (m_instrumentMixer) {
		m_instrumentMixer->releaseLock();
	    }
	    if (m_bussMixer) {
		m_bussMixer->releaseLock();
	    }
	}
    }
    return rv;
}

int
JackDriver::releaseAudioQueueLocks()
{
    int rv = 0;
    std::cerr << "JackDriver::releaseAudioQueueLocks" << std::endl;
    if (m_fileWriter) rv = m_fileWriter->releaseLock();
    if (m_fileReader) rv = m_fileReader->releaseLock();
    if (m_instrumentMixer) rv = m_instrumentMixer->releaseLock();
    if (m_bussMixer) rv = m_bussMixer->releaseLock();
    return rv;
}

//!!! should really eliminate these and call on mixer directly from
// driver I s'pose

void
JackDriver::setPluginInstance(InstrumentId id, unsigned long pluginId,
			      int position)
{
    if (m_instrumentMixer) m_instrumentMixer->setPlugin(id, position, pluginId);
}

void
JackDriver::removePluginInstance(InstrumentId id, int position)
{
    if (m_instrumentMixer) m_instrumentMixer->removePlugin(id, position);
}

void
JackDriver::removePluginInstances()
{
    if (m_instrumentMixer) m_instrumentMixer->removeAllPlugins();
}

void
JackDriver::setPluginInstancePortValue(InstrumentId id, int position,
				       unsigned long portNumber,
				       float value)
{
    if (m_instrumentMixer) m_instrumentMixer->setPluginPortValue(id, position, portNumber, value);
}

void
JackDriver::setPluginInstanceBypass(InstrumentId id, int position, bool value)
{
    if (m_instrumentMixer) m_instrumentMixer->setPluginBypass(id, position, value);
}

//!!! and these
bool
JackDriver::createRecordFile(const std::string &filename)
{
    //!!! want an actual instrument
    if (m_fileWriter) {
	return m_fileWriter->createRecordFile(m_alsaDriver->getAudioMonitoringInstrument(), filename);
    } else return false;
}

bool
JackDriver::closeRecordFile(AudioFileId &returnedId)
{
    if (m_fileWriter) {
	return m_fileWriter->closeRecordFile(m_alsaDriver->getAudioMonitoringInstrument(), returnedId);
    } else return false;
}

}

#endif // HAVE_LIBJACK
#endif // HAVE_ALSA
