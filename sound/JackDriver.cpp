
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

#include "JackDriver.h"
#include "AlsaDriver.h"
#include "MappedStudio.h"
#include "AudioProcess.h"
#include "Profiler.h"
#include "AudioLevel.h"
#include "Audit.h"
#include "PluginFactory.h"

#ifdef HAVE_ALSA
#ifdef HAVE_LIBJACK

//#define DEBUG_JACK_DRIVER 1
//#define DEBUG_JACK_TRANSPORT 1
//#define DEBUG_JACK_PROCESS 1

namespace Rosegarden
{

#if (defined(DEBUG_JACK_DRIVER) || defined(DEBUG_JACK_PROCESS) || defined(DEBUG_JACK_TRANSPORT))
static unsigned long framesThisPlay = 0;
static RealTime startTime;
#endif

JackDriver::JackDriver(AlsaDriver *alsaDriver) :
    m_client(0),
    m_bufferSize(0),
    m_sampleRate(0),
    m_tempOutBuffer(0),
    m_jackTransportEnabled(false),
    m_jackTransportMaster(false),
    m_waiting(false),
    m_waitingState(JackTransportStopped),
    m_waitingToken(0),
    m_bussMixer(0),
    m_instrumentMixer(0),
    m_fileReader(0),
    m_fileWriter(0),
    m_alsaDriver(alsaDriver),
    m_masterLevel(1.0),
    m_directMasterAudioInstruments(0L),
    m_directMasterSynthInstruments(0L),
    m_haveAsyncAudioEvent(false),
    m_recordInput(1000),
    m_recordInputChannel(-1),
    m_recordLevel(0.0),
    m_kickedOutAt(0),
    m_framesProcessed(0),
    m_ok(false)
{
    assert(sizeof(sample_t) == sizeof(float));
    initialise();
}

JackDriver::~JackDriver()
{
#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::~JackDriver" << std::endl;
#endif
    m_ok = false; // prevent any more work in process()

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::~JackDriver: terminating buss mixer" << std::endl;
#endif
    AudioBussMixer *bussMixer = m_bussMixer;
    m_bussMixer = 0;
    if (bussMixer) bussMixer->terminate();

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::~JackDriver: terminating instrument mixer" << std::endl;
#endif
    AudioInstrumentMixer *instrumentMixer = m_instrumentMixer;
    m_instrumentMixer = 0;
    if (instrumentMixer) {
	instrumentMixer->terminate();
	instrumentMixer->destroyAllPlugins();
    }

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::~JackDriver: terminating file reader" << std::endl;
#endif
    AudioFileReader *reader = m_fileReader;
    m_fileReader = 0;
    if (reader) reader->terminate();

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::~JackDriver: terminating file writer" << std::endl;
#endif
    AudioFileWriter *writer = m_fileWriter;
    m_fileWriter = 0;
    if (writer) writer->terminate();

    if (m_client)
    {
#ifdef DEBUG_JACK_DRIVER
        std::cerr << "JackDriver::shutdown - closing JACK client"
                  << std::endl;
#endif
        if (jack_deactivate(m_client))
	{
	    std::cerr << "JackDriver::shutdown - deactivation failed"
		      << std::endl;
	}
        for (unsigned int i = 0; i < m_inputPorts.size(); ++i)
        {
#ifdef DEBUG_JACK_DRIVER
	    std::cerr << "unregistering input " << i << std::endl;
#endif
            if (jack_port_unregister(m_client, m_inputPorts[i]))
            {
                std::cerr << "JackDriver::shutdown - "
                          << "can't unregister input port " << i + 1
                          << std::endl;
            }
        }

	for (unsigned int i = 0; i < m_outputSubmasters.size(); ++i)
	{
#ifdef DEBUG_JACK_DRIVER
	    std::cerr << "unregistering output sub " << i << std::endl;
#endif
	    if (jack_port_unregister(m_client, m_outputSubmasters[i]))
	    {
		std::cerr << "JackDriver::shutdown - "
			  << "can't unregister output submaster " << i+1 << std::endl;
	    }
	}

	for (unsigned int i = 0; i < m_outputMonitors.size(); ++i)
	{
#ifdef DEBUG_JACK_DRIVER
	    std::cerr << "unregistering output mon " << i << std::endl;
#endif
	    if (jack_port_unregister(m_client, m_outputMonitors[i]))
	    {
		std::cerr << "JackDriver::shutdown - "
			  << "can't unregister output monitor " << i+1 << std::endl;
	    }
	}
	
	for (unsigned int i = 0; i < m_outputMasters.size(); ++i)
	{
#ifdef DEBUG_JACK_DRIVER
	    std::cerr << "unregistering output master " << i << std::endl;
#endif
	    if (jack_port_unregister(m_client, m_outputMasters[i]))
	    {
		std::cerr << "JackDriver::shutdown - "
			  << "can't unregister output master " << i+1 << std::endl;
	    }
	}

#ifdef DEBUG_JACK_DRIVER
	std::cerr << "closing client" << std::endl;
#endif
        jack_client_close(m_client);
	std::cerr << "done" << std::endl;
        m_client = 0;
    }

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver: deleting mixers etc" << std::endl;
#endif
    delete bussMixer;
    delete instrumentMixer;
    delete reader;
    delete writer;

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::~JackDriver exiting" << std::endl;
#endif
}

void
JackDriver::initialise(bool reinitialise)
{
    m_ok = false;

    Audit audit;
    audit << std::endl;

    std::string jackClientName = "rosegarden";

    // attempt connection to JACK server
    //
    if ((m_client = jack_client_new(jackClientName.c_str())) == 0)
    {
        audit << "JackDriver::initialiseAudio - "
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
    jack_set_xrun_callback(m_client, jackXRun, this);
    jack_set_sync_callback(m_client, jackSyncCallback, this);

    // get and report the sample rate and buffer size
    //
    m_sampleRate = jack_get_sample_rate(m_client);
    m_bufferSize = jack_get_buffer_size(m_client);

    audit << "JackDriver::initialiseAudio - JACK sample rate = "
	  << m_sampleRate << "Hz, buffer size = " << m_bufferSize
	  << std::endl;

    PluginFactory::setSampleRate(m_sampleRate);

    // Get the initial buffer size before we activate the client
    //

    if (!reinitialise) {
	
	// create processing buffer(s)
	//
	m_tempOutBuffer = new sample_t[m_bufferSize];
	
	audit << "JackDriver::initialiseAudio - "
	      << "creating disk thread" << std::endl;
	
	m_fileReader = new AudioFileReader(m_alsaDriver, m_sampleRate);
	m_fileWriter = new AudioFileWriter(m_alsaDriver, m_sampleRate);
	m_instrumentMixer = new AudioInstrumentMixer
	    (m_alsaDriver, m_fileReader, m_sampleRate, m_bufferSize);
	m_bussMixer = new AudioBussMixer
	    (m_alsaDriver, m_instrumentMixer, m_sampleRate, m_bufferSize);
	m_instrumentMixer->setBussMixer(m_bussMixer);

	// We run the file reader whatever, but we only run the other
	// threads (instrument mixer, buss mixer, file writer) when we
	// actually need them.  (See updateAudioData and createRecordFile.)
	m_fileReader->run();
    }

    // Create and connect the default numbers of ports.  We always create
    // one stereo pair each of master and monitor outs, and then we create
    // record ins, fader outs and submaster outs according to the user's
    // preferences.  Since we don't know the user's preferences yet, we'll
    // start by creating one pair of record ins and no fader or submaster
    // outs.
    //
    m_outputMasters.clear();
    m_outputMonitors.clear();
    m_outputSubmasters.clear();
    m_outputInstruments.clear();
    m_inputPorts.clear();

    if (!createMainOutputs()) { // one stereo pair master, one pair monitor
        audit << "JackDriver::initialise - "
	      << "failed to create main outputs!" << std::endl;
	return;
    }
	
    if (!createRecordInputs(1)) {
        audit << "JackDriver::initialise - "
	      << "failed to create record inputs!" << std::endl;
	return;
    }

    if (jack_activate(m_client))
    {
        audit << "JackDriver::initialise - "
	      << "client activation failed" << std::endl;
	return;
    }

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
	audit << "JackDriver::initialiseAudio - "
	      << "found " << i << " JACK physical outputs"
	      << std::endl;
    }
    else
	audit << "JackDriver::initialiseAudio - "
	      << "no JACK physical outputs found"
	      << std::endl;
    free(ports);
    
    if (playback_1 != "")
    {
        audit << "JackDriver::initialiseAudio - "
	      << "connecting from "
	      << "\"" << jack_port_name(m_outputMasters[0])
	      << "\" to \"" << playback_1.c_str() << "\""
	      << std::endl;

        // connect our client up to the ALSA ports - first left output
        //
        if (jack_connect(m_client, jack_port_name(m_outputMasters[0]),
                         playback_1.c_str()))
        {
            audit << "JackDriver::initialiseAudio - "
		  << "cannot connect to JACK output port" << std::endl;
	                return;
        }

/*
        if (jack_connect(m_client, jack_port_name(m_outputMonitors[0]),
                         playback_1.c_str()))
        {
            audit << "JackDriver::initialiseAudio - "
		  << "cannot connect to JACK output port" << std::endl;
	    return;
        }
*/
    }

    if (playback_2 != "")
    {
        audit << "JackDriver::initialiseAudio - "
	      << "connecting from "
	      << "\"" << jack_port_name(m_outputMasters[1])
	      << "\" to \"" << playback_2.c_str() << "\""
	      << std::endl;

        if (jack_connect(m_client, jack_port_name(m_outputMasters[1]),
                         playback_2.c_str()))
        {
            audit << "JackDriver::initialiseAudio - "
		  << "cannot connect to JACK output port" << std::endl;
        }

/*
        if (jack_connect(m_client, jack_port_name(m_outputMonitors[1]),
                         playback_2.c_str()))
        {
            audit << "JackDriver::initialiseAudio - "
		  << "cannot connect to JACK output port" << std::endl;
        }
*/
    }


    std::string capture_1, capture_2;

    ports =
	jack_get_ports(m_client, NULL, NULL,
		       JackPortIsPhysical | JackPortIsOutput);
    
    if (ports)
    {
	if (ports[0]) capture_1 = std::string(ports[0]);
	if (ports[1]) capture_2 = std::string(ports[1]);
	
	// count ports
	unsigned int i = 0;
	for (i = 0; ports[i]; i++);
	audit << "JackDriver::initialiseAudio - "
	      << "found " << i << " JACK physical inputs"
	      << std::endl;
    }
    else
	audit << "JackDriver::initialiseAudio - "
	      << "no JACK physical inputs found"
	      << std::endl;
    free(ports);
    
    if (capture_1 != "") {

        audit << "JackDriver::initialiseAudio - "
	      << "connecting from "
	      << "\"" << capture_1.c_str()
	      << "\" to \"" << jack_port_name(m_inputPorts[0]) << "\""
	      << std::endl;

        if (jack_connect(m_client, capture_1.c_str(),
			 jack_port_name(m_inputPorts[0])))
        {
            audit << "JackDriver::initialiseAudio - "
		  << "cannot connect to JACK input port" << std::endl;
        }
    }
    
    if (capture_2 != "") {

        audit << "JackDriver::initialiseAudio - "
	      << "connecting from "
	      << "\"" << capture_2.c_str()
	      << "\" to \"" << jack_port_name(m_inputPorts[1]) << "\""
	      << std::endl;

        if (jack_connect(m_client, capture_2.c_str(),
			 jack_port_name(m_inputPorts[1])))
        {
            audit << "JackDriver::initialiseAudio - "
		  << "cannot connect to JACK input port" << std::endl;
        }
    }

    audit << "JackDriver::initialiseAudio - "
	  << "initialised JACK audio subsystem"
	  << std::endl;

    m_ok = true;
}

bool
JackDriver::createMainOutputs()
{
    if (!m_client) return false;

    jack_port_t *port =  jack_port_register
	(m_client, "master out L",
	 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port) return false;
    m_outputMasters.push_back(port);

    port = jack_port_register
	(m_client, "master out R",
	 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port) return false;
    m_outputMasters.push_back(port);

    port = jack_port_register
	(m_client, "record monitor out L",
	 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port) return false;
    m_outputMonitors.push_back(port);

    port = jack_port_register
	(m_client, "record monitor out R",
	 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port) return false;
    m_outputMonitors.push_back(port);

    return true;
}

bool
JackDriver::createFaderOutputs(int pairs)
{
    if (!m_client) return false;

    int pairsNow = m_outputInstruments.size() / 2;
    if (pairs == pairsNow) return true;

    for (int i = pairsNow; i < pairs; ++i) {

	char namebuffer[22];
	jack_port_t *port;

	snprintf(namebuffer, 21, "fader %d out L", i+1);

	port = jack_port_register(m_client,
				  namebuffer,
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsOutput,
				  0);
	if (!port) return false;
	m_outputInstruments.push_back(port);

	snprintf(namebuffer, 21, "fader %d out R", i+1);
	port = jack_port_register(m_client,
				  namebuffer,
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsOutput,
				  0);
	if (!port) return false;
	m_outputInstruments.push_back(port);
    }

    while ((int)m_outputInstruments.size() > pairs * 2) {
	std::vector<jack_port_t *>::iterator itr = m_outputInstruments.end();
	--itr;
	jack_port_unregister(m_client, *itr);
	m_outputInstruments.erase(itr);
    }

    return true;
}
  
bool
JackDriver::createSubmasterOutputs(int pairs)
{
    if (!m_client) return false;

    int pairsNow = m_outputSubmasters.size() / 2;
    if (pairs == pairsNow) return true;

    for (int i = pairsNow; i < pairs; ++i) {

	char namebuffer[22];
	jack_port_t *port;

	snprintf(namebuffer, 21, "submaster %d out L", i+1);
	port = jack_port_register(m_client,
				  namebuffer,
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsOutput,
				  0);
	if (!port) return false;
	m_outputSubmasters.push_back(port);

	snprintf(namebuffer, 21, "submaster %d out R", i+1);
	port = jack_port_register(m_client,
				  namebuffer,
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsOutput,
				  0);
	if (!port) return false;
	m_outputSubmasters.push_back(port);
    }

    while ((int)m_outputSubmasters.size() > pairs * 2) {
	std::vector<jack_port_t *>::iterator itr = m_outputSubmasters.end();
	--itr;
	jack_port_unregister(m_client, *itr);
	m_outputSubmasters.erase(itr);
    }

    return true;
}

bool
JackDriver::createRecordInputs(int pairs)
{
    if (!m_client) return false;

    int pairsNow = m_inputPorts.size() / 2;
    if (pairs == pairsNow) return true;

    for (int i = pairsNow; i < pairs; ++i) {

	char namebuffer[22];
	jack_port_t *port;

	snprintf(namebuffer, 21, "record in %d L", i + 1);
	port = jack_port_register(m_client,
				  namebuffer,
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsInput,
				  0);
	if (!port) return false;
	m_inputPorts.push_back(port);

	snprintf(namebuffer, 21, "record in %d R", i + 1);
	port = jack_port_register(m_client,
				  namebuffer,
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsInput,
				  0);
	if (!port) return false;
	m_inputPorts.push_back(port);
    }

    while ((int)m_outputSubmasters.size() > pairs * 2) {
	std::vector<jack_port_t *>::iterator itr = m_outputSubmasters.end();
	--itr;
	jack_port_unregister(m_client, *itr);
	m_outputSubmasters.erase(itr);
    }
    
    return true;
}
    

void
JackDriver::setAudioPorts(bool faderOuts, bool submasterOuts)
{
    if (!m_client) return;

    Audit audit;
#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::setAudioPorts(" << faderOuts << "," << submasterOuts << ")" << std::endl;
#endif

    if (!m_client) {
	std::cerr << "JackDriver::setAudioPorts(" << faderOuts << "," << submasterOuts << "): no client yet" << std::endl;
	return;
    }

    if (faderOuts) {
	InstrumentId instrumentBase;
	int instruments;
	m_alsaDriver->getAudioInstrumentNumbers(instrumentBase, instruments);
	if (!createFaderOutputs(instruments)) {
	    m_ok = false;
	    audit << "Failed to create fader outs!" << std::endl;
	    return;
	}
    } else {
	createFaderOutputs(0);
    }

    if (submasterOuts) {
	
	// one fewer than returned here, because the master has a buss object too
	if (!createSubmasterOutputs
	    (m_alsaDriver->getMappedStudio()->getObjectCount
	     (MappedObject::AudioBuss) - 1)) {
	    m_ok = false;
	    audit << "Failed to create submaster outs!" << std::endl;
	    return;
	}

    } else {
	createSubmasterOutputs(0);
    }
}

RealTime
JackDriver::getAudioPlayLatency() const
{
    if (!m_client) return RealTime::zeroTime;

    jack_nframes_t latency =
        jack_port_get_total_latency(m_client, m_outputMasters[0]);

    return RealTime::frame2RealTime(latency, m_sampleRate);
}

RealTime
JackDriver::getAudioRecordLatency() const
{
    if (!m_client) return RealTime::zeroTime;
    
    jack_nframes_t latency =
        jack_port_get_total_latency(m_client, m_inputPorts[0]);

    return RealTime::frame2RealTime(latency, m_sampleRate);
}

RealTime
JackDriver::getInstrumentPlayLatency(InstrumentId id) const
{
    if (m_instrumentLatencies.find(id) == m_instrumentLatencies.end()) {
	return RealTime::zeroTime;
    } else {
	return m_instrumentLatencies.find(id)->second;
    }
}

RealTime
JackDriver::getMaximumPlayLatency() const
{
    return m_maxInstrumentLatency;
}

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
    if (!m_ok || !m_client) {
#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcess: not OK" << std::endl;
#endif
	return 0;
    }

    if (!m_bussMixer) {
#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcess: no buss mixer" << std::endl;
#endif
	return jackProcessEmpty(nframes);
    }

    bool lowLatencyMode = m_alsaDriver->getLowLatencyMode();
    bool clocksRunning = m_alsaDriver->areClocksRunning();
    bool playing = m_alsaDriver->isPlaying();
    bool asyncAudio = m_haveAsyncAudioEvent;
 
#ifdef DEBUG_JACK_PROCESS
    Rosegarden::Profiler profiler("jackProcess", true);
#endif

    if (lowLatencyMode) {
	if (clocksRunning) {
	    if (playing || asyncAudio) {

		if (m_instrumentMixer->tryLock() == 0) {
		    m_instrumentMixer->kick(false);
		    m_instrumentMixer->releaseLock();
//#ifdef DEBUG_JACK_PROCESS
		} else {
		    std::cerr << "JackDriver::jackProcess: no instrument mixer lock available" << std::endl;
//#endif
		}
		if (m_bussMixer->getBussCount() > 0) {
		    if (m_bussMixer->tryLock() == 0) {
			m_bussMixer->kick(false);
			m_bussMixer->releaseLock();
//#ifdef DEBUG_JACK_PROCESS
		    } else {
			std::cerr << "JackDriver::jackProcess: no buss mixer lock available" << std::endl;
//#endif
		    }
		}
	    }
	}
    }

    if (jack_cpu_load(m_client) > 98.0) {
	reportFailure(Rosegarden::MappedEvent::FailureCPUOverload);
	return jackProcessEmpty(nframes);
    }

#ifdef DEBUG_JACK_PROCESS
    Rosegarden::Profiler profiler2("jackProcess post mix", true);
#endif

    SequencerDataBlock *sdb = m_alsaDriver->getSequencerDataBlock();
    
    jack_position_t position;
    jack_transport_state_t state = JackTransportRolling;

    if (m_jackTransportEnabled) {

	state = jack_transport_query(m_client, &position);

#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcess: JACK transport state is " << state << std::endl;
#endif
	if (state == JackTransportStopped) {
	    if (playing && clocksRunning) {
		ExternalTransport *transport = 
		    m_alsaDriver->getExternalTransportControl();
		if (transport) {
#ifdef DEBUG_JACK_TRANSPORT
		    std::cerr << "JackDriver::jackProcess: JACK transport stopped externally at " << position.frame << std::endl;
#endif
		    m_waitingToken =
			transport->transportJump
			(ExternalTransport::TransportStopAtTime,
			 RealTime::frame2RealTime(position.frame,
						  position.frame_rate));
		}
	    } else if (clocksRunning) {
		if (!asyncAudio) {
#ifdef DEBUG_JACK_PROCESS
		    std::cerr << "JackDriver::jackProcess: no interesting async events" << std::endl;
#endif
		    // do this before record monitor, otherwise we lost monitor out
		    jackProcessEmpty(nframes);
		}
		return jackProcessRecord(nframes, 0, 0, clocksRunning); // for monitoring
	    } else {
		return jackProcessEmpty(nframes);
	    }
	} else if (state == JackTransportStarting) {
	    return jackProcessEmpty(nframes);
	} else if (state != JackTransportRolling) {
	    std::cerr << "JackDriver::jackProcess: unexpected JACK transport state " << state << std::endl;
	}
    }

    if (state == JackTransportRolling) { // also covers not-on-transport case
	if (m_waiting) {
#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackProcess: transport rolling, telling ALSA driver to go!" << std::endl;
#endif
	    m_alsaDriver->startClocksApproved();
	    m_waiting = false;
	}

#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcess (rolling or not on JACK transport)" << std::endl;
#endif
	if (!clocksRunning) {
#ifdef DEBUG_JACK_PROCESS
	    std::cerr << "JackDriver::jackProcess: clocks stopped" << std::endl;
#endif
	    return jackProcessEmpty(nframes);

	} else if (!playing) {
#ifdef DEBUG_JACK_PROCESS
	    std::cerr << "JackDriver::jackProcess: not playing" << std::endl;
#endif
	    if (!asyncAudio) {
#ifdef DEBUG_JACK_PROCESS
		std::cerr << "JackDriver::jackProcess: no interesting async events" << std::endl;
#endif
		// do this before record monitor, otherwise we lost monitor out
		jackProcessEmpty(nframes);
	    }

	    int rv = jackProcessRecord(nframes, 0, 0, clocksRunning); // for monitoring

	    if (!asyncAudio) {
		return rv;
	    }
	}
    }

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_alsaDriver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_alsaDriver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    bool doneRecord = false;

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
		    reportFailure(Rosegarden::MappedEvent::FailureBussMixUnderrun);
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

	if (buss + 1 == m_recordInput) {
	    jackProcessRecord(nframes, submaster[0], submaster[1], clocksRunning);
	    doneRecord = true;
	}
    }

#ifdef DEBUG_JACK_PROCESS
    std::cerr << "JackDriver::jackProcess: have " << audioInstruments << " audio and " << synthInstruments << " synth instruments" << std::endl;
#endif

    bool allInstrumentsDormant = true;
    static RealTime dormantTime = RealTime::zeroTime;

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {
	
	InstrumentId id;
	if (i < audioInstruments) id = audioInstrumentBase + i;
	else id = synthInstrumentBase + (i - audioInstruments);

	if (m_instrumentMixer->isInstrumentEmpty(id)) continue;

	sample_t *instrument[2] = { 0, 0 };
	sample_t peak[2] = { 0.0, 0.0 };

	if (int(m_outputInstruments.size()) > i * 2 + 1) {
	    instrument[0] = static_cast<sample_t *>
		(jack_port_get_buffer(m_outputInstruments[i * 2], nframes));
	    instrument[1] = static_cast<sample_t *>
		(jack_port_get_buffer(m_outputInstruments[i * 2 + 1], nframes));
	}

	if (!instrument[0]) instrument[0] = m_tempOutBuffer;
	if (!instrument[1]) instrument[1] = m_tempOutBuffer;

	for (int ch = 0; ch < 2; ++ch) {

	    // We always need to read from an instrument's ring buffer
	    // to keep the instrument moving along, as well as for
	    // monitoring.  If the instrument is connected straight to
	    // the master, then we also need to mix from it.  (We have
	    // that information cached courtesy of updateAudioData.)

	    bool directToMaster = false;
	    if (i < audioInstruments) {
		directToMaster = (m_directMasterAudioInstruments & (1 << i));
	    } else {
		directToMaster = (m_directMasterSynthInstruments &
				  (1 << (i - audioInstruments)));
	    }

#ifdef DEBUG_JACK_PROCESS
	    if (id == 1000 || id == 10000) {
		std::cerr << "JackDriver::jackProcess: instrument id " << id << ", base " << audioInstrumentBase << ", direct masters " << m_directMasterAudioInstruments << ": " << directToMaster << std::endl;
	    }
#endif

	    RingBuffer<AudioInstrumentMixer::sample_t, 2> *rb =
		m_instrumentMixer->getRingBuffer(id, ch);

	    if (!rb || m_instrumentMixer->isInstrumentDormant(id)) {
#ifdef DEBUG_JACK_PROCESS
		if (id == 1000 || id == 10000) {
		    if (rb) {
			std::cerr << "JackDriver::jackProcess: instrument " << id << " dormant" << std::endl;
		    } else {
			std::cerr << "JackDriver::jackProcess: instrument " << id << " has no ring buffer for channel " << ch << std::endl;
		    }
		}
#endif
		if (rb) rb->skip(nframes);
		if (instrument[ch])
		    memset(instrument[ch], 0, nframes * sizeof(sample_t));

	    } else {

		allInstrumentsDormant = false;

		size_t actual = rb->read(instrument[ch], nframes);

#ifdef DEBUG_JACK_PROCESS
		if (id == 1000) {
		    std::cerr << "JackDriver::jackProcess: read " << actual << " of " << nframes << " frames for instrument " << id << " channel " << ch << std::endl;
		}
#endif

		if (actual < nframes) {

		    std::cerr << "JackDriver::jackProcess: read " << actual << " of " << nframes << " frames for " << id << " ch " << ch << " (pl " << playing << ", cl " << clocksRunning << ", aa " << asyncAudio << ")" << std::endl;

		    reportFailure(Rosegarden::MappedEvent::FailureMixUnderrun);
		}
		for (size_t f = 0; f < nframes; ++f) {
		    sample_t sample = instrument[ch][f];
		    if (sample > peak[ch]) peak[ch] = sample;
		    if (directToMaster) master[ch][f] += sample;
		}
	    }

	    // If the instrument is connected straight to master we
	    // also need to skip() on the buss mixer's reader for it,
	    // otherwise it'll block because the buss mixer isn't
	    // needing to read it.

	    if (rb && directToMaster) {
		rb->skip(nframes, 1); // 1 is the buss mixer's reader (magic)
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

    if (asyncAudio) {
	if (!allInstrumentsDormant) {
	    dormantTime = RealTime::zeroTime;
	} else {
	    dormantTime = dormantTime +
		RealTime::frame2RealTime(m_bufferSize, m_sampleRate);
	    if (dormantTime > RealTime(10, 0)) {
		m_haveAsyncAudioEvent = false;
	    }
	}
    }

    // Get master fader levels.  There's no pan on the master.
    float gain = AudioLevel::dB_to_multiplier(m_masterLevel);
    float masterPeak[2] = { 0.0, 0.0 };

    for (int ch = 0; ch < 2; ++ch) {
	for (size_t i = 0; i < nframes; ++i) {
	    sample_t sample = master[ch][i] * gain;
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

    if (m_recordInput == 0) {
	jackProcessRecord(nframes, master[0], master[1], clocksRunning);
    } else if (!doneRecord) {
	jackProcessRecord(nframes, 0, 0, clocksRunning);
    }

    if (playing) {
	if (!lowLatencyMode) {
	    if (m_bussMixer->getBussCount() == 0) {
		m_instrumentMixer->signal();
	    } else {
		m_bussMixer->signal();
	    }
	}
    }

    m_framesProcessed += nframes;

#if (defined(DEBUG_JACK_DRIVER) || defined(DEBUG_JACK_PROCESS) || defined(DEBUG_JACK_TRANSPORT))
    framesThisPlay += nframes; //!!!
#endif
#ifdef DEBUG_JACK_PROCESS
    std::cerr << "JackDriver::jackProcess: " << nframes << " frames, " << framesThisPlay << " this play, " << m_framesProcessed << " total" << std::endl;
#endif
    
    return 0;
}

int
JackDriver::jackProcessEmpty(jack_nframes_t nframes)
{
    sample_t *buffer;

#ifdef DEBUG_JACK_PROCESS
    std::cerr << "JackDriver::jackProcessEmpty" << std::endl;
#endif

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

    m_framesProcessed += nframes;

#if (defined(DEBUG_JACK_DRIVER) || defined(DEBUG_JACK_PROCESS) || defined(DEBUG_JACK_TRANSPORT))
    framesThisPlay += nframes;
#endif
#ifdef DEBUG_JACK_PROCESS
    std::cerr << "JackDriver::jackProcess: " << nframes << " frames, " << framesThisPlay << " this play, " << m_framesProcessed << " total" << std::endl;
#endif

    return 0;
}
    
int
JackDriver::jackProcessRecord(jack_nframes_t nframes,
			      sample_t *sourceBufferLeft,
			      sample_t *sourceBufferRight,
			      bool clocksRunning)
{
    SequencerDataBlock *sdb = m_alsaDriver->getSequencerDataBlock();
    bool wroteSomething = false;
    sample_t peakLeft = 0.0, peakRight = 0.0;

#ifdef DEBUG_JACK_PROCESS
    std::cerr << "JackDriver::jackProcessRecord" << std::endl;
#endif

    // Get input buffers
    //
    sample_t *inputBufferLeft = 0, *inputBufferRight = 0;

    int channel = m_recordInputChannel;
    int channels = (channel == -1 ? 2 : 1);
    if (channels == 2) channel = 0;

    if (sourceBufferLeft) {

#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcessRecord: buss input provided" << std::endl;
#endif
	inputBufferLeft = sourceBufferLeft;
	if (sourceBufferRight) inputBufferRight = sourceBufferRight;

    } else if (m_recordInput < 1000) {

#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcessRecord: no known input" << std::endl;
#endif
	return 0;

    } else {

#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcessRecord: record input " << m_recordInput << std::endl;
#endif
	int input = m_recordInput - 1000;

	int port = input * channels + channel;
	int portRight = input * channels + 1;

	if (port < int(m_inputPorts.size())) {
	    inputBufferLeft = static_cast<sample_t*>
		(jack_port_get_buffer(m_inputPorts[port], nframes));
	}
    
	if (channels == 2 && portRight < int(m_inputPorts.size())) {
	    inputBufferRight = static_cast<sample_t*>
		(jack_port_get_buffer(m_inputPorts[portRight], nframes));
	}
    }

    float gain = AudioLevel::dB_to_multiplier(m_recordLevel);
    
    if (m_alsaDriver->getRecordStatus() == RECORD_AUDIO &&
	clocksRunning) {

#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcessRecord: recording" << std::endl;
#endif
	memset(m_tempOutBuffer, 0, nframes * sizeof(sample_t));

	if (inputBufferLeft) {
	    for (size_t i = 0; i < nframes; ++i) {
		sample_t sample = inputBufferLeft[i] * gain;
		if (sample > peakLeft) peakLeft = sample;
		m_tempOutBuffer[i] = sample;
	    }

	    if (m_outputMonitors.size() > 0) {
		sample_t *buf = 
		    static_cast<sample_t *>
		    (jack_port_get_buffer(m_outputMonitors[0], nframes));
		memcpy(buf, m_tempOutBuffer, nframes * sizeof(sample_t));
	    }

	    m_fileWriter->write(m_alsaDriver->getAudioMonitoringInstrument(),
				m_tempOutBuffer, 0, nframes);
	}

	if (channels == 2) {

	    if (inputBufferRight) {
		for (size_t i = 0; i < nframes; ++i) {
		    sample_t sample = inputBufferRight[i] * gain;
		    if (sample > peakRight) peakRight = sample;
		    m_tempOutBuffer[i] = sample;
		}
		if (m_outputMonitors.size() > 1) {
		    sample_t *buf =
			static_cast<sample_t *>
			(jack_port_get_buffer(m_outputMonitors[1], nframes));
		    memcpy(buf, m_tempOutBuffer, nframes * sizeof(sample_t));
		}
	    } 

	    m_fileWriter->write(m_alsaDriver->getAudioMonitoringInstrument(),
				m_tempOutBuffer, 1, nframes);
	}
	    
	wroteSomething = true;

    } else {

	// want peak levels and monitors anyway, even if not recording

#ifdef DEBUG_JACK_PROCESS
	std::cerr << "JackDriver::jackProcessRecord: monitoring only" << std::endl;
#endif

	if (inputBufferLeft) {

	    sample_t *buf = 0;
	    if (m_outputMonitors.size() > 0) {
		buf = static_cast<sample_t *>
		    (jack_port_get_buffer(m_outputMonitors[0], nframes));
	    }

	    for (size_t i = 0; i < nframes; ++i) {
		sample_t sample = inputBufferLeft[i] * gain;
		if (sample > peakLeft) peakLeft = sample;
		if (buf) buf[i] = sample;
	    }

	    if (channels == 2 && inputBufferRight) {

		buf = 0;
		if (m_outputMonitors.size() > 1) {
		    buf = static_cast<sample_t *>
			(jack_port_get_buffer(m_outputMonitors[1], nframes));
		}

		for (size_t i = 0; i < nframes; ++i) {
		    sample_t sample = inputBufferRight[i] * gain;
		    if (sample > peakRight) peakRight = sample;
		    if (buf) buf[i] = sample;
		}
	    }
	}
    }

    if (channels < 2) peakRight = peakLeft;

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

#ifdef DEBUG_JACK_TRANSPORT
    std::cerr << "JackDriver::jackSyncCallback: state " << state << ", frame " << position->frame << ", m_waiting " << inst->m_waiting << ", playing " << inst->m_alsaDriver->isPlaying() << std::endl;

    std::cerr << "JackDriver::jackSyncCallback: unique_1 " << position->unique_1 << ", unique_2 " << position->unique_2 << std::endl;
    
    std::cerr << "JackDriver::jackSyncCallback: rate " << position->frame_rate << ", bar " << position->bar << ", beat " << position->beat << ", tick " << position->tick << ", bpm " << position->beats_per_minute << std::endl;

#endif

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

#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackSyncCallback: Requesting jump to " << rt << std::endl;
#endif
	    
	    inst->m_waitingToken = transport->transportJump(request, rt);

#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackSyncCallback: My token is " << inst->m_waitingToken << std::endl;
#endif

	} else if (request == ExternalTransport::TransportStop) {

#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackSyncCallback: Requesting state change to " << request << std::endl;
#endif
	    
	    inst->m_waitingToken = transport->transportChange(request);

#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackSyncCallback: My token is " << inst->m_waitingToken << std::endl;
#endif

	} else if (request == ExternalTransport::TransportNoChange) {

#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackSyncCallback: Requesting no state change!" << std::endl;
#endif

	    inst->m_waitingToken = transport->transportChange(request);

#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackSyncCallback: My token is " << inst->m_waitingToken << std::endl;
#endif
	}

	inst->m_waiting = true;
	inst->m_waitingState = state;
	return 0;

    } else {

	if (transport->isTransportSyncComplete(inst->m_waitingToken)) {
#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackSyncCallback: Sync complete" << std::endl;
#endif
	    return 1;
	} else {
#ifdef DEBUG_JACK_TRANSPORT
	    std::cerr << "JackDriver::jackSyncCallback: Sync not complete" << std::endl;
#endif
	    return 0;
	}
    }
}

bool
JackDriver::start()
{
    if (!m_client) return true;

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::start" << std::endl;
#endif

//!!!    prebufferAudio();

    // m_waiting is true if we are waiting for the JACK transport
    // to finish a change of state.

    if (m_jackTransportEnabled) {

	// If on the transport, we never return true here -- instead
	// the JACK process calls startClocksApproved() to signal to
	// the ALSA driver that it's time to go.  But we do use this
	// to manage our JACK transport state requests.

	// Where did this request come from?  Are we just responding
	// to an external sync?

	ExternalTransport *transport =
	m_alsaDriver->getExternalTransportControl();
	
	if (transport) {
	    if (transport->isTransportSyncComplete(m_waitingToken)) {

		// Nope, this came from Rosegarden

#ifdef DEBUG_JACK_TRANSPORT
		std::cerr << "JackDriver::start: asking JACK transport to start, setting wait state" << std::endl;
#endif
		m_waiting = true;
		m_waitingState = JackTransportStarting;
		jack_transport_locate(m_client,
				      RealTime::realTime2Frame
				      (m_alsaDriver->getSequencerTime(),
				       m_sampleRate));
		jack_transport_start(m_client);
	    } else {
#ifdef DEBUG_JACK_TRANSPORT
		std::cerr << "JackDriver::start: waiting already" << std::endl;
#endif
	    }
	}
	return false;
    }
    
#if (defined(DEBUG_JACK_DRIVER) || defined(DEBUG_JACK_PROCESS) || defined(DEBUG_JACK_TRANSPORT))
    framesThisPlay = 0; //!!!
    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    startTime = RealTime(tv.tv_sec, tv.tv_usec * 1000); //!!!
#endif
#ifdef DEBUG_JACK_TRANSPORT
    std::cerr << "JackDriver::start: not on JACK transport, accepting right away" << std::endl;
#endif
    return true;
}

void
JackDriver::stop()
{
    if (!m_client) return;

    m_haveAsyncAudioEvent = false;

#ifdef DEBUG_JACK_TRANSPORT
    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    RealTime endTime = RealTime(tv.tv_sec, tv.tv_usec * 1000);//!!!
    std::cerr << "\nJackDriver::stop: frames this play: " << framesThisPlay << ", elapsed " << (endTime - startTime) << std::endl;
#endif

//!!!    flushAudio();

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

#ifdef DEBUG_JACK_TRANSPORT
		std::cerr << "JackDriver::stop: internal request, asking JACK transport to stop" << std::endl;
#endif

		jack_transport_stop(m_client);

	    } else {
		// Nothing to do

#ifdef DEBUG_JACK_TRANSPORT
		std::cerr << "JackDriver::stop: external request, JACK transport is already stopped" << std::endl;
#endif
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

#ifdef DEBUG_JACK_DRIVER
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

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::jackSampleRate - sample rate changed to "
               << nframes << std::endl;
#endif
    inst->m_sampleRate = nframes;

    return 0;
}

void
JackDriver::jackShutdown(void *arg)
{
#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::jackShutdown() - callback received - " 
              << "informing GUI" << std::endl;
#endif

    JackDriver *inst = static_cast<JackDriver*>(arg);
    inst->m_ok = false;
    inst->m_kickedOutAt = time(0);

    inst->reportFailure(Rosegarden::MappedEvent::FailureJackDied);
    if (inst->m_instrumentMixer) inst->m_instrumentMixer->resetAllPlugins();
}

int
JackDriver::jackGraphOrder(void *)
{
#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::jackGraphOrder" << std::endl;
#endif
    return 0;
}

int
JackDriver::jackXRun(void *arg)
{
#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::jackXRun" << std::endl;
#endif

    // Report to GUI
    //
    JackDriver *inst = static_cast<JackDriver*>(arg);
    inst->reportFailure(Rosegarden::MappedEvent::FailureXRuns);

    return 0;
}


void

JackDriver::restoreIfRestorable()
{
    if (m_kickedOutAt == 0) return;

    if (m_client) {
        jack_client_close(m_client);
	std::cerr << "closed client" << std::endl;
	m_client = 0;
    }
    
    time_t now = time(0);

    if (now < m_kickedOutAt || now >= m_kickedOutAt + 3) {
    
	initialise(true);
	
	if (m_ok) {
	    reportFailure(Rosegarden::MappedEvent::FailureJackRestart);
	} else {
	    reportFailure(Rosegarden::MappedEvent::FailureJackRestartFailed);
	}

	m_kickedOutAt = 0;
    }
}	

void
JackDriver::prebufferAudio()
{
    if (!m_instrumentMixer) return;

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::prebufferAudio: sequencer time is "
	      << m_alsaDriver->getSequencerTime() << std::endl;
#endif

    // For JACK transport sync, the next slice could start anywhere.
    // We need to query it.
//!!! urch, no, seems this is being called before the transport has actually
// repositioned
/*
    if (m_jackTransportEnabled) {
	jack_position_t position;
	jack_transport_query(m_client, &position);
	m_bussMixer->fillBuffers
	    (RealTime::frame2RealTime(position.frame, m_sampleRate));
    } else {
*/

    RealTime sliceStart = getNextSliceStart(m_alsaDriver->getSequencerTime());

    m_fileReader->fillBuffers(sliceStart);
    m_instrumentMixer->fillBuffers(sliceStart);

    if (m_bussMixer->getBussCount() > 0) {
	m_bussMixer->fillBuffers(sliceStart);
    }
}

void
JackDriver::flushAudio()
{
    if (!m_instrumentMixer) return;

#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::flushAudio" << std::endl;
#endif
    
    //!!! experimental to get async synth events agogo
    m_fileReader->fillBuffers(RealTime::zeroTime);

    if (m_bussMixer->getBussCount() > 0) {
	m_instrumentMixer->fillBuffers(RealTime::zeroTime);
	m_bussMixer->fillBuffers(RealTime::zeroTime);
    } else {
	m_instrumentMixer->fillBuffers(RealTime::zeroTime);
    }
}
 
void
JackDriver::kickAudio()
{
#ifdef DEBUG_JACK_PROCESS
    std::cerr << "JackDriver::kickAudio" << std::endl;
#endif

    if (m_fileReader) m_fileReader->kick();
    if (m_instrumentMixer) m_instrumentMixer->kick();
    if (m_bussMixer) m_bussMixer->kick();
    if (m_fileWriter) m_fileWriter->kick();
}

void
JackDriver::updateAudioData()
{
    if (!m_ok || !m_client) return;

    MappedAudioBuss *mbuss =
	m_alsaDriver->getMappedStudio()->getAudioBuss(0);

    if (mbuss) {
	float level = 0.0;
	(void)mbuss->getProperty(MappedAudioBuss::Level, level);
	m_masterLevel = level;
    }

    unsigned long directMasterAudioInstruments = 0L;
    unsigned long directMasterSynthInstruments = 0L;

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_alsaDriver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_alsaDriver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    RealTime jackLatency = getAudioPlayLatency();
    RealTime maxLatency = RealTime::zeroTime;

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

	InstrumentId id;
	if (i < audioInstruments) id = audioInstrumentBase + i;
	else id = synthInstrumentBase + (i - audioInstruments);

	MappedAudioFader *fader = m_alsaDriver->getMappedStudio()->getAudioFader(id);
	if (!fader) continue;

	if (id == m_alsaDriver->getAudioMonitoringInstrument()) {

	    float f = 2;
	    (void)fader->getProperty(MappedAudioFader::Channels, f);
	    int channels = (int)f;
	    
	    int inputChannel = -1;
	    if (channels == 1) {
		float f = 0;
		(void)fader->getProperty(MappedAudioFader::InputChannel, f);
		inputChannel = (int)f;
	    }
	    m_recordInputChannel = inputChannel;
	    
	    float level = 0.0;
	    (void)fader->getProperty(MappedAudioFader::FaderRecordLevel, level);
	    m_recordLevel = level;

	    // At the moment we only record one track at a time, and
	    // we record to the so-called audio monitoring instrument.
	    // So we just have the one field to set, to note which
	    // input is connected to it.  Like in base/Instrument.h,
	    // we use numbers < 1000 to mean buss numbers and >= 1000
	    // to mean record ins.
	    
	    MappedObjectValueList connections = fader->getConnections
		(MappedConnectableObject::In);

	    if (connections.empty()) {
		
		std::cerr << "No connections in for record instrument "
			  << (id) << " (mapped id " << fader->getId() << ")" << std::endl;

		// oh dear.
		m_recordInput = 1000;

	    } else if (*connections.begin() == mbuss->getId()) {

		m_recordInput = 0;
		
	    } else {
		
		MappedObject *obj = m_alsaDriver->getMappedStudio()->
		    getObjectById(MappedObjectId(*connections.begin()));

		if (!obj) {

		    std::cerr << "No such object as " << *connections.begin() << std::endl;
		    m_recordInput = 1000;
		} else if (obj->getType() == MappedObject::AudioBuss) {
		    m_recordInput = (int)((MappedAudioBuss *)obj)->getBussId();
		} else if (obj->getType() == MappedObject::AudioInput) {
		    m_recordInput = (int)((MappedAudioInput *)obj)->getInputNumber()
			+ 1000;
		} else {
		    std::cerr << "Object " << *connections.begin() << " is not buss or input" << std::endl;
		    m_recordInput = 1000;
		}
	    }
	}

	size_t pluginLatency = 0;
	bool empty = m_instrumentMixer->isInstrumentEmpty(id);

	if (!empty) {
	    pluginLatency = m_instrumentMixer->getPluginLatency(id);
	}

	// If we find the object is connected to no output, or to buss
	// number 0 (the master), then we set the bit appropriately.

	MappedObjectValueList connections = fader->getConnections
	    (MappedConnectableObject::Out);

	if (connections.empty() || (*connections.begin() == mbuss->getId())) {
	    if (i < audioInstruments) {
		directMasterAudioInstruments |= (1 << i);
	    } else {
		directMasterSynthInstruments |= (1 << (i - audioInstruments));
	    }		
	} else if (!empty) {
	    pluginLatency +=
		m_instrumentMixer->getPluginLatency((unsigned int)*connections.begin());
	}

	if (empty) {
	    m_instrumentLatencies[id] = RealTime::zeroTime;
	} else {
	    m_instrumentLatencies[id] = jackLatency +
		RealTime::frame2RealTime(pluginLatency, m_sampleRate);
	    if (m_instrumentLatencies[id] > maxLatency) {
		maxLatency = m_instrumentLatencies[id];
	    }
	}
    }

    m_directMasterAudioInstruments = directMasterAudioInstruments;
    m_directMasterSynthInstruments = directMasterSynthInstruments;
    m_maxInstrumentLatency = maxLatency;
    
    int inputs = m_alsaDriver->getMappedStudio()->
	getObjectCount(MappedObject::AudioInput);

    if (m_client) {
	// this will return with no work if the inputs are already correct:
	createRecordInputs(inputs);
    }

    m_bussMixer->updateInstrumentConnections();

    if (m_bussMixer->getBussCount() == 0 || m_alsaDriver->getLowLatencyMode()) {
	if (m_bussMixer->running()) {
	    m_bussMixer->terminate();
	}
    } else {
	if (!m_bussMixer->running()) {
	    m_bussMixer->run();
	}
    }

    if (m_alsaDriver->getLowLatencyMode()) {
	if (m_instrumentMixer->running()) {
	    m_instrumentMixer->terminate();
	}
    } else {
	if (!m_instrumentMixer->running()) {
	    m_instrumentMixer->run();
	}
    }
}

void
JackDriver::setAudioBussLevels(int bussNo, float dB, float pan)
{
    if (m_bussMixer) {
	m_bussMixer->setBussLevels(bussNo, dB, pan);
    }
}

void
JackDriver::setAudioInstrumentLevels(InstrumentId instrument, float dB, float pan)
{
    if (m_instrumentMixer) {
	m_instrumentMixer->setInstrumentLevels(instrument, dB, pan);
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
#ifdef DEBUG_JACK_DRIVER
	std::cerr << "JackDriver::getAudioQueueLocks: trying to lock buss mixer" << std::endl;
#endif
	rv = m_bussMixer->getLock();
	if (rv) return rv;
    }
    if (m_instrumentMixer) {
#ifdef DEBUG_JACK_DRIVER
	std::cerr << "JackDriver::getAudioQueueLocks: ok, now trying for instrument mixer" << std::endl;
#endif
	rv = m_instrumentMixer->getLock();
	if (rv) return rv;
    }
    if (m_fileReader) {
#ifdef DEBUG_JACK_DRIVER
	std::cerr << "JackDriver::getAudioQueueLocks: ok, now trying for disk reader" << std::endl;
#endif
	rv = m_fileReader->getLock();
	if (rv) return rv;
    }
    if (m_fileWriter) {
#ifdef DEBUG_JACK_DRIVER
	std::cerr << "JackDriver::getAudioQueueLocks: ok, now trying for disk writer" << std::endl;
#endif
	rv = m_fileWriter->getLock();
    }
#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::getAudioQueueLocks: ok" << std::endl;
#endif
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
#ifdef DEBUG_JACK_DRIVER
    std::cerr << "JackDriver::releaseAudioQueueLocks" << std::endl;
#endif
    if (m_fileWriter) rv = m_fileWriter->releaseLock();
    if (m_fileReader) rv = m_fileReader->releaseLock();
    if (m_instrumentMixer) rv = m_instrumentMixer->releaseLock();
    if (m_bussMixer) rv = m_bussMixer->releaseLock();
    return rv;
}


void
JackDriver::setPluginInstance(InstrumentId id, QString identifier,
			      int position)
{
    if (m_instrumentMixer) m_instrumentMixer->setPlugin(id, position, identifier);
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

float
JackDriver::getPluginInstancePortValue(InstrumentId id, int position,
				       unsigned long portNumber)
{
    if (m_instrumentMixer)
	return m_instrumentMixer->getPluginPortValue(id, position, portNumber);
    return 0;
}

void
JackDriver::setPluginInstanceBypass(InstrumentId id, int position, bool value)
{
    if (m_instrumentMixer) m_instrumentMixer->setPluginBypass(id, position, value);
}

QStringList
JackDriver::getPluginInstancePrograms(InstrumentId id, int position)
{
    if (m_instrumentMixer) return m_instrumentMixer->getPluginPrograms(id, position);
    return QStringList();
}

QString
JackDriver::getPluginInstanceProgram(InstrumentId id, int position)
{
    if (m_instrumentMixer) return m_instrumentMixer->getPluginProgram(id, position);
    return QString();
}

QString
JackDriver::getPluginInstanceProgram(InstrumentId id, int position,
				     int bank, int program)
{
    if (m_instrumentMixer) return m_instrumentMixer->getPluginProgram(id, position, bank, program);
    return QString();
}

unsigned long
JackDriver::getPluginInstanceProgram(InstrumentId id, int position, QString name)
{
    if (m_instrumentMixer) return m_instrumentMixer->getPluginProgram(id, position, name);
    return 0;
}

void 
JackDriver::setPluginInstanceProgram(InstrumentId id, int position, QString program)
{
    if (m_instrumentMixer) m_instrumentMixer->setPluginProgram(id, position, program);
}

QString
JackDriver::configurePlugin(InstrumentId id, int position, QString key, QString value)
{
    if (m_instrumentMixer) return m_instrumentMixer->configurePlugin(id, position, key, value);
    return QString();
}

RunnablePluginInstance *
JackDriver::getSynthPlugin(InstrumentId id)
{
    if (m_instrumentMixer) return m_instrumentMixer->getSynthPlugin(id);
    else return 0;
}

bool
JackDriver::createRecordFile(const std::string &filename)
{
    if (m_fileWriter) {
	if (!m_fileWriter->running()) {
	    m_fileWriter->run();
	}
	return m_fileWriter->createRecordFile(m_alsaDriver->getAudioMonitoringInstrument(), filename);
    } else {
	std::cerr << "JackDriver::createRecordFile: No file writer available!" << std::endl;
	return false;
    }
}

bool
JackDriver::closeRecordFile(AudioFileId &returnedId)
{
    if (m_fileWriter) {
	return m_fileWriter->closeRecordFile(m_alsaDriver->getAudioMonitoringInstrument(), returnedId);
	if (m_fileWriter->running()) {
	    m_fileWriter->terminate();
	}
    } else return false;
}


void 
JackDriver::reportFailure(Rosegarden::MappedEvent::FailureCode code)
{ 
    if (m_alsaDriver) m_alsaDriver->reportFailure(code); 
}


}

#endif // HAVE_LIBJACK
#endif // HAVE_ALSA
