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

#include "config.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

#ifdef HAVE_ALSA

// ALSA
#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>
#include <alsa/version.h>
#include <alsa/seq.h>

#include "AlsaDriver.h"
#include "AlsaPort.h"
#include "MappedInstrument.h"
#include "Midi.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"
#include "rosestrings.h"
#include "MappedCommon.h"

#include <qregexp.h>

#ifdef HAVE_LIBJACK
#include <jack/types.h>
#include <unistd.h> // for usleep
#include <cmath>
#endif

#define DEBUG_ALSA 1

// This driver implements MIDI in and out via the ALSA (www.alsa-project.org)
// sequencer interface.
//
// We use JACK (http://jackit.sourceforge.net/) for audio.  It's complicated
// by the fact there's not yet any simple app-level documentation to explain
// what's going on.  Attempting to rectify that here.  So to enable JACK
// for your app:
//
// o Create at least one input port and output port - we might need
//   more but I'm still not quite sure about the port -> audio channel
//   relationship yet.
//
// o Register callbacks that JACK uses to push data about and tell the
//   client what's happening.
//
// o Get and store initial sample size
//
// o Activate client
//
// o Get a list of JACK ports available (probably get away with hardcoding
//   for the moment)
//
// o Connect like-to-like ports for client
//
// o Start throwing correct sized chunks of samples at the output
//   port.  See the jackProcess() method for more information on
//   how this bit works.
//
// o Get samples coming in and store them somewhere.
//
// For the moment we access the sample file handles directly and
// don't cache any data anywhere.  Performance is actually pretty
// good so far (for pretty small sample sizes obviously) but we'll
// see how it fails when the audio files get big.
//
// updated 10.05.2002, Richard Bown
//
//

using std::cerr;
using std::endl;


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

#ifdef HAVE_LIBJACK

static jack_nframes_t    _jackBufferSize;
static unsigned int      _jackSampleRate;
static sample_t         *_tempOutBuffer1;
static sample_t         *_tempOutBuffer2;

static sample_t         *_pluginBufferIn1;
static sample_t         *_pluginBufferIn2;

static sample_t         *_pluginBufferOut1;
static sample_t         *_pluginBufferOut2;

static int               _passThroughCounter;

static const float  _8bitSampleMax  = (float)(0xff/2);
static const float  _16bitSampleMax = (float)(0xffff/2);

static bool _jackTransportEnabled;
static bool _jackTransportMaster;

// We use a matrix of pre-allocated MappedEvents to return information
// to the GUI every so often from the jackProcess loop (where need to
// be as efficient as possible).
//
static const int     _jackMappedEventsMax = 100;
static MappedEvent*  _jackMappedEvent[_jackMappedEventsMax];
static int           _jackMappedEventCounter;

// How many passes through JACK's process loop before reporting
// audio level.  So we avoid flooding.
//
static const int reportPasses = 5;

pthread_t            _diskThread;
pthread_mutex_t      _diskThreadLock = PTHREAD_MUTEX_INITIALIZER;

// A set of RingBuffers that we can reuse
//
static std::vector<std::pair<bool, RingBuffer*> > _ringBuffer;

#endif

static bool              _threadJackClosing;
static bool              _threadAlsaClosing;

// a global AudioFile works for the threads - it didn't want to
// work out of the instance returned to the JACK callbacks.
//
AudioFile *_recordFile;

AlsaDriver::AlsaDriver(MappedStudio *studio):
    SoundDriver(studio, std::string("alsa-lib version ") +
                        std::string(SND_LIB_VERSION_STR)),
    m_client(-1),
    m_port(-1),
    m_queue(-1),
    m_maxClients(-1),
    m_maxPorts(-1),
    m_maxQueues(-1),
    m_midiInputPortConnected(false),
    m_alsaPlayStartTime(0, 0),
    m_alsaRecordStartTime(0, 0),
    m_loopStartTime(0, 0),
    m_loopEndTime(0, 0),
    m_looping(false),
    m_audioMeterSent(false)


#ifdef HAVE_LIBJACK
    ,m_audioClient(0)
    ,m_transportPosition(0)
#endif

{
    AUDIT_START;
    AUDIT_STREAM << "Rosegarden " << VERSION << " - AlsaDriver - " 
                 << m_name << std::endl;

#ifdef HAVE_LIBJACK
    _jackBufferSize = 0;
    _jackSampleRate = 0;
    _passThroughCounter = 0;

    _jackTransportEnabled = false;
    _jackTransportMaster = false;

    // Make sure we keep this MappedEvent hanging around - 
    // setting this property means that MappedComposition::clear()
    // won't attempt to delete it.
    //
    for (int i = 0; i < _jackMappedEventsMax; ++i)
    {
        _jackMappedEvent[i] = new MappedEvent();
        _jackMappedEvent[i]->setPersistent(true);
    }

    _jackMappedEventCounter = 0;

    // Setup RingBuffer vector
    //
    for (unsigned int i = 0; i < 24; ++i)
    {
        RingBuffer *ringBuffer  = new RingBuffer(65535);
        ringBuffer->lock(); // lock the ringbuffer into physical memory
        _ringBuffer.push_back(std::pair<bool, RingBuffer*>(false, ringBuffer));
    }

#endif

    _threadAlsaClosing = false;
    _threadJackClosing = false;
    AUDIT_UPDATE;
}

AlsaDriver::~AlsaDriver()
{
    shutdown();
}

void
AlsaDriver::shutdown()
{
    AUDIT_START;
    AUDIT_STREAM << "AlsaDriver::~AlsaDriver - shutting down" << std::endl;

#ifdef HAVE_LIBJACK
    // This thread should already have completed at this point
    //
    if (_diskThread)
    {
        pthread_cancel(_diskThread);
        _diskThread = 0;
    }
#endif

    // to ensure that only one thread does the closing
    if (_threadAlsaClosing) return;

    if (_threadAlsaClosing == false && m_midiHandle)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::shutdown - closing MIDI client" << std::endl;
#endif

        _threadAlsaClosing = true;
        snd_seq_stop_queue(m_midiHandle, m_queue, 0);
        snd_seq_close(m_midiHandle);
        m_midiHandle = 0;
    }

#ifdef HAVE_LADSPA
#ifdef DEBUG_ALSA
    std::cout << "AlsaDriver::shutdown - unloading LADSPA" << std::endl;
#endif
    m_studio->unloadAllPluginLibraries();
#endif // HAVE_LADSPA

#ifdef HAVE_LIBJACK

    // lock this and terminate safely
    pthread_mutex_lock(&_diskThreadLock);
    _threadJackClosing = true;

    if (m_audioClient)
    {

#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::shutdown - closing JACK client"
                  << std::endl;
#endif

        if (jack_deactivate(m_audioClient))
        {
#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::shutdown - deactivation failed"
                      << std::endl;
#endif
        }

        for (unsigned int i = 0; i < m_jackInputPorts.size(); ++i)
        {
            if (jack_port_unregister(m_audioClient, m_jackInputPorts[i]))
            {
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::shutdown - "
                          << "can't unregister input port " << i + 1
                          << std::endl;
#endif
            }
        }

        if (jack_port_unregister(m_audioClient, m_jackOutputPortLeft))
        {
#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::shutdown - "
                      << "can't unregister output port left" << std::endl;
#endif
        }

        if (jack_port_unregister(m_audioClient, m_jackOutputPortRight))
        {
#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::shutdown - "
                      << "can't unregister output port right" << std::endl;
#endif
        }
                
        jack_client_close(m_audioClient);
        m_audioClient = 0;
    }

    // Get rid of all those static events
    for (int i = 0; i < _jackMappedEventsMax; ++i)
        delete _jackMappedEvent[i];

#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::shutdown - cancelling disk thread" << std::endl;
#endif

    // Clear the mutex
    //
    pthread_mutex_destroy(&_diskThreadLock);

    // clear ring buffers
    //
    for (unsigned int i = 0; i < _ringBuffer.size(); ++i)
        delete _ringBuffer[i].second;
    _ringBuffer.clear();

#endif // HAVE_LIBJACK
    AUDIT_UPDATE;
}

void
AlsaDriver::setLoop(const RealTime &loopStart, const RealTime &loopEnd)
{
    m_loopStartTime = loopStart;
    m_loopEndTime = loopEnd;

    // currently we use this simple test for looping - it might need
    // to get more sophisticated in the future.
    //
    if (m_loopStartTime != m_loopEndTime)
        m_looping = true;
    else
        m_looping = false;
}

void
AlsaDriver::getSystemInfo()
{
    int err;
    snd_seq_system_info_t *sysinfo;

    snd_seq_system_info_alloca(&sysinfo);

    if ((err = snd_seq_system_info(m_midiHandle, sysinfo)) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "System info error: " <<  snd_strerror(err)
                  << std::endl;
#endif
        exit(EXIT_FAILURE);
    }

    m_maxQueues = snd_seq_system_info_get_queues(sysinfo); 
    m_maxClients = snd_seq_system_info_get_clients(sysinfo);
    m_maxPorts = snd_seq_system_info_get_ports(sysinfo);

}

void
AlsaDriver::showQueueStatus(int queue)
{
    int err, idx, min, max;
    snd_seq_queue_status_t *status;

    snd_seq_queue_status_alloca(&status);
    min = queue < 0 ? 0 : queue;
    max = queue < 0 ? m_maxQueues : queue + 1;

    for (idx = min; idx < max; ++idx)
    {
        if ((err = snd_seq_get_queue_status(m_midiHandle, idx, status))<0)
        {
            if (err == -ENOENT)
                continue;

#ifdef DEBUG_ALSA
            std::cerr << "Client " << idx << " info error: "
                      << snd_strerror(err) << std::endl;
#endif
            exit(EXIT_FAILURE);
        }

#ifdef DEBUG_ALSA
        std::cerr << "Queue " << snd_seq_queue_status_get_queue(status)
                  << std::endl;

        std::cerr << "Tick       = "
                  << snd_seq_queue_status_get_tick_time(status)
                  << std::endl;

        std::cerr << "Realtime   = "
                  << snd_seq_queue_status_get_real_time(status)->tv_sec
                  << "."
                  << snd_seq_queue_status_get_real_time(status)->tv_nsec
                  << std::endl;

        std::cerr << "Flags      = 0x"
                  << snd_seq_queue_status_get_status(status)
                  << std::endl;
#endif
    }

}


void
AlsaDriver::generatePortList(AlsaPortList *newPorts)
{
    AUDIT_START;
    AlsaPortList alsaPorts;

    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int  client;
    unsigned int writeCap = SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE;
    unsigned int readCap = SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_READ;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    AUDIT_STREAM << std::endl << "  ALSA Client information:"
		 << std::endl << std::endl;

    // Get only the client ports we're interested in and store them
    // for sorting and then device creation.
    //
    while (snd_seq_query_next_client(m_midiHandle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        // Ignore ourselves and the system client
        //
        if (client == m_client || client == 0) continue;

        while (snd_seq_query_next_port(m_midiHandle, pinfo) >= 0)
        {
	    int client = snd_seq_port_info_get_client(pinfo);
	    int port = snd_seq_port_info_get_port(pinfo);
	    unsigned int clientType = snd_seq_client_info_get_type(cinfo);
	    unsigned int portType = snd_seq_port_info_get_type(pinfo);
	    unsigned int capability = snd_seq_port_info_get_capability(pinfo);

            if (((capability & writeCap) == writeCap) ||
                ((capability &  readCap) ==  readCap))
            {
                AUDIT_STREAM << "    "
			     << client << ","
			     << port << " - ("
			     << snd_seq_client_info_get_name(cinfo) << ", "
			     << snd_seq_port_info_get_name(pinfo) << ")";

                PortDirection direction;

                if (capability & SND_SEQ_PORT_CAP_DUPLEX)
                {
                    direction = Duplex;
                    AUDIT_STREAM << "\t\t\t(DUPLEX)";
                }
                else if (capability & SND_SEQ_PORT_CAP_WRITE)
                {
                    direction = WriteOnly;
                    AUDIT_STREAM << "\t\t(WRITE ONLY)";
                }
                else
                {
                    direction = ReadOnly;
                    AUDIT_STREAM << "\t\t(READ ONLY)";
                }

		AUDIT_STREAM << " [ctype " << clientType << ", ptype " << portType << ", cap " << capability << "]";

                // Generate a unique name using the client id
                //
		char portId[40];
		sprintf(portId, "%d:%d ", client, port);

                std::string fullClientName = 
                    std::string(snd_seq_client_info_get_name(cinfo));

                std::string fullPortName = 
                    std::string(snd_seq_port_info_get_name(pinfo));

		std::string name;

		// If the first part of the client name is the same as the 
                // start of the port name, just use the port name.  otherwise
                // concatenate.
                //
                int firstSpace = fullClientName.find(" ");

                // If no space is found then we try to match the whole string
                //
                if (firstSpace < 0) firstSpace = fullClientName.length();

                if (firstSpace &&
		    int(fullPortName.length()) >= firstSpace &&
		    fullPortName.substr(0, firstSpace) ==
		    fullClientName.substr(0, firstSpace)) {
		    name = portId + fullPortName;
		} else {
		    name = portId + fullClientName + ": " + fullPortName;
		}

                // Sanity check for length
                //
                if (name.length() > 25) name = portId + fullPortName;

		if (direction == WriteOnly) {
		    name += " (write)";
		} else if (direction == ReadOnly) {
		    name += " (read)";
		} else if (direction == Duplex) {
		    name += " (duplex)";
		}

                AlsaPortDescription *portDescription = 
                    new AlsaPortDescription(
                            Instrument::Midi,
                            name,
                            client,
			    port,
			    clientType,
			    portType,
			    capability,
                            direction);

		if (newPorts &&
		    (getPortName(ClientPortPair(client, port)) == "")) {
		    newPorts->push_back(portDescription);
		}

                alsaPorts.push_back(portDescription);

                AUDIT_STREAM << std::endl;
            }
        }
    }

    AUDIT_STREAM << std::endl;

    // Ok now sort by duplexicity
    //
    std::sort(alsaPorts.begin(), alsaPorts.end(), AlsaPortCmp());
    m_alsaPorts = alsaPorts;

    AUDIT_UPDATE;
}


void
AlsaDriver::generateInstruments()
{
    // Reset these before each Instrument hunt
    //
    m_audioRunningId = AudioInstrumentBase;
    m_midiRunningId = MidiInstrumentBase;

    // Clear these
    //
    m_instruments.clear();
    m_devices.clear();
    m_devicePortMap.clear();
    m_suspendedPortMap.clear();

    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); it++)
    {
        /*
        std::cout << "installing device " << (*it)->m_name
             << " client = " << (*it)->m_client
             << " port = " << (*it)->m_port << endl;
             */

	if ((*it)->isWriteable()) {
	    MappedDevice *device = createMidiDevice(*it, MidiDevice::Play);
	    if (!device) {
#ifdef DEBUG_ALSA
		std::cerr << "WARNING: Failed to create play device" << std::endl;
#else
                ;
#endif
	    } else {
		addInstrumentsForDevice(device);
		m_devices.push_back(device);
	    }
	}
	if ((*it)->isReadable()) {
	    MappedDevice *device = createMidiDevice(*it, MidiDevice::Record);
	    if (!device) {
#ifdef DEBUG_ALSA
		std::cerr << "WARNING: Failed to create record device" << std::endl;
#else
                ;
#endif
	    } else {
		m_devices.push_back(device);
	    }
	}
    }

#ifdef HAVE_LIBJACK

    // Create a number of audio Instruments - these are just
    // logical Instruments anyway and so we can create as 
    // many as we like and then use them as Tracks.
    //

    MappedInstrument *instr;
    char number[100];
    std::string audioName;

    DeviceId audioDeviceId = getSpareDeviceId();

    if (m_driverStatus & AUDIO_OK)
    {
        for (int channel = 0; channel < 16; ++channel)
        {
            sprintf(number, " #%d", channel + 1);
            audioName = "JACK Audio" + std::string(number);
            instr = new MappedInstrument(Instrument::Audio,
                                         channel,
                                         m_audioRunningId,
                                         audioName,
                                         audioDeviceId);
            m_instruments.push_back(instr);

            // Create a fader with a matching id - this is the starting
            // point for all audio faders.
            //
            m_studio->createObject(MappedObject::AudioFader,
                                   m_audioRunningId);

            /*
            std::cerr  << "AlsaDriver::generateInstruments - "
                       << "added audio fader (id=" << m_audioRunningId
                       << ")" << std::endl;
                       */
    
            m_audioRunningId++;
        }

        // Create audio device
        //
        MappedDevice *device =
                        new MappedDevice(audioDeviceId,
                                         Device::Audio,
                                         "JACK Audio",
                                         "Audio connection");
        m_devices.push_back(device);
    }

#endif
}

MappedDevice *
AlsaDriver::createMidiDevice(AlsaPortDescription *port,
			     MidiDevice::DeviceDirection reqDirection)
{
    char deviceName[100];
    std::string connectionName("");
    AUDIT_START;

    static int unknownCounter;

    static int counters[3][2]; // [system/hardware/software][out/in]
    const int SYSTEM = 0, HARDWARE = 1, SOFTWARE = 2;
    static const char *firstNames[4][2] = {
	{ "MIDI output system device", "MIDI input system device" },
	{ "MIDI external device", "MIDI hardware input device" },
	{ "MIDI software device", "MIDI software input" }
    };
    static const char *countedNames[4][2] = {
	{ "MIDI output system device %d", "MIDI input system device %d" },
	{ "MIDI external device %d", "MIDI hardware input device %d" },
	{ "MIDI software device %d", "MIDI software input %d" }
    };

    static int specificCounters[2];
    static const char *specificNames[2] = {
	"MIDI soundcard synth", "MIDI soft synth",
    };
    static const char *specificCountedNames[2] = {
	"MIDI soundcard synth %d", "MIDI soft synth %d",
    };

    DeviceId deviceId = getSpareDeviceId();

    if (port) {

	if (reqDirection == MidiDevice::Record && !port->isReadable())  return 0;
	if (reqDirection == MidiDevice::Play   && !port->isWriteable()) return 0;

	int category = (port->m_client <  64 ? SYSTEM :
			port->m_client < 128 ? HARDWARE : SOFTWARE);

	bool haveName = false;

	if (category != SYSTEM && reqDirection == MidiDevice::Play) {

	    // No way to query whether a port is a MIDI synth, as
	    // PORT_TYPE_SYNTH actually indicates something different
	    // (ability to do direct wavetable synthesis -- nothing
	    // to do with MIDI).  But we assume GM/GS/XG/MT32 devices
	    // are synths.

	    bool isSynth = (port->m_portType &
			    (SND_SEQ_PORT_TYPE_MIDI_GM |
			     SND_SEQ_PORT_TYPE_MIDI_GS |
			     SND_SEQ_PORT_TYPE_MIDI_XG |
			     SND_SEQ_PORT_TYPE_MIDI_MT32));

	    // Because we can't discover through the API whether a
	    // port is a synth, we are instead reduced to this
	    // disgusting hack:

	    if (!isSynth &&
		(port->m_name.find("ynth") < port->m_name.length())) isSynth = true;
	    
	    if (category == SYSTEM) isSynth = false;

	    if (isSynth) {
		int clientType = (category == SOFTWARE) ? 1 : 0;
		if (specificCounters[clientType] == 0) {
		    sprintf(deviceName, specificNames[clientType]);
		    ++specificCounters[clientType];
		} else {
		    sprintf(deviceName,
			    specificCountedNames[clientType],
			    ++specificCounters[clientType]);
		}
		haveName = true;
	    }
	}

	if (!haveName) {

	    if (counters[category][reqDirection] == 0) {
		sprintf(deviceName, firstNames[category][reqDirection]);
		++counters[category][reqDirection];
	    } else {
		sprintf(deviceName,
			countedNames[category][reqDirection],
			++counters[category][reqDirection]);
	    }
	}

	m_devicePortMap[deviceId] = ClientPortPair(port->m_client,
						   port->m_port);

	connectionName = port->m_name;

	AUDIT_STREAM << "Creating device " << deviceId << " in "
		     << (reqDirection == MidiDevice::Play ? "Play" : "Record")
		     << " mode for connection " << connectionName
		     << "\nDefault device name for this device is "
		     << deviceName << std::endl;

    } else {

	sprintf(deviceName, "Anonymous MIDI device %d", ++unknownCounter);

	AUDIT_STREAM << "Creating device " << deviceId << " in "
		     << (reqDirection == MidiDevice::Play ? "Play" : "Record")
		     << " mode -- no connection available "
		     << "\nDefault device name for this device is "
		     << deviceName << std::endl;
    }
	
    MappedDevice *device = new MappedDevice(deviceId,
					    Device::Midi,
					    deviceName,
					    connectionName);
    device->setDirection(reqDirection);
    AUDIT_UPDATE;
    return device;
}

DeviceId
AlsaDriver::getSpareDeviceId()
{
    std::set<DeviceId> ids;
    for (unsigned int i = 0; i < m_devices.size(); ++i) {
	ids.insert(m_devices[i]->getId());
    }

    DeviceId id = 0;
    while (ids.find(id) != ids.end()) ++id;
    return id;
}

void
AlsaDriver::addInstrumentsForDevice(MappedDevice *device)
{
    std::string channelName;
    char number[100];

    for (int channel = 0; channel < 16; ++channel)
    {
	// Create MappedInstrument for export to GUI
	//
	// name is just number, derive rest from device at gui
	sprintf(number, "#%d", channel + 1);
	channelName = std::string(number);
	
	if (channel == 9) channelName = std::string("#10[D]");
	MappedInstrument *instr = new MappedInstrument(Instrument::Midi,
						       channel,
						       m_midiRunningId++,
						       channelName,
						       device->getId());
	m_instruments.push_back(instr);
    }
}
    

bool
AlsaDriver::canReconnect(Device::DeviceType type)
{
    return (type == Device::Midi);
}

DeviceId
AlsaDriver::addDevice(Device::DeviceType type,
		      MidiDevice::DeviceDirection direction)
{
    if (type == Device::Midi) {

	MappedDevice *device = createMidiDevice(0, direction);
	if (!device) {
#ifdef DEBUG_ALSA
	    std::cerr << "WARNING: Device creation failed" << std::endl;
#else
            ;
#endif
	} else {
	    addInstrumentsForDevice(device);
	    m_devices.push_back(device);

	    MappedEvent *mE =
		new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
				0, 0);
	    insertMappedEventForReturn(mE);

	    return device->getId();
	}
    }

    return Device::NO_DEVICE;
}

void
AlsaDriver::removeDevice(DeviceId id)
{
    for (MappedDeviceList::iterator i = m_devices.end();
	 i != m_devices.begin(); ) {
	
	--i;

	if ((*i)->getId() == id) {
	    delete *i;
	    m_devices.erase(i);
	}
    }

    for (MappedInstrumentList::iterator i = m_instruments.end();
	 i != m_instruments.begin(); ) {

	--i;
	
	if ((*i)->getDevice() == id) {
	    delete *i;
	    m_instruments.erase(i);
	}
    }

    MappedEvent *mE =
	new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
			0, 0);
    insertMappedEventForReturn(mE);
}

ClientPortPair
AlsaDriver::getPortByName(std::string name)
{
    for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {
	if (m_alsaPorts[i]->m_name == name) {
	    return ClientPortPair(m_alsaPorts[i]->m_client,
				  m_alsaPorts[i]->m_port);
	}
    }
    return ClientPortPair(-1, -1);
}

std::string
AlsaDriver::getPortName(ClientPortPair port)
{
    for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {
	if (m_alsaPorts[i]->m_client == port.first &&
	    m_alsaPorts[i]->m_port == port.second) {
	    return m_alsaPorts[i]->m_name;
	}
    }
    return "";
}
    

unsigned int
AlsaDriver::getConnections(Device::DeviceType type,
			   MidiDevice::DeviceDirection direction)
{
    if (type != Device::Midi) return 0;

    int count = 0;
    for (unsigned int j = 0; j < m_alsaPorts.size(); ++j) {
	if ((direction == MidiDevice::Play && m_alsaPorts[j]->isWriteable()) ||
	    (direction == MidiDevice::Record && m_alsaPorts[j]->isReadable())) {
	    ++count;
	}
    }
    
    return count;
}

QString
AlsaDriver::getConnection(Device::DeviceType type,
			  MidiDevice::DeviceDirection direction,
			  unsigned int connectionNo)
{
    if (type != Device::Midi) return "";
    
    AlsaPortList tempList;
    for (unsigned int j = 0; j < m_alsaPorts.size(); ++j) {
	if ((direction == MidiDevice::Play && m_alsaPorts[j]->isWriteable()) ||
	    (direction == MidiDevice::Record && m_alsaPorts[j]->isReadable())) {
	    tempList.push_back(m_alsaPorts[j]);
	}
    }
    
    if (connectionNo < tempList.size()) {
	return tempList[connectionNo]->m_name.c_str();
    }

    return "";
}

void
AlsaDriver::setConnection(DeviceId id, QString connection)
{
    AUDIT_START;
    ClientPortPair port(getPortByName(connection.data()));

    if (port.first != -1 && port.second != -1) {

	m_devicePortMap[id] = port;

	for (unsigned int i = 0; i < m_devices.size(); ++i) {

	    if (m_devices[i]->getId() == id) {
		m_devices[i]->setConnection(connection.data());

		MappedEvent *mE =
		    new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
				    0, 0);
		insertMappedEventForReturn(mE);

		break;
	    }
	}
    }

    AUDIT_UPDATE;
}

void
AlsaDriver::setPlausibleConnection(DeviceId id, QString idealConnection)
{
    AUDIT_START;
    ClientPortPair port(getPortByName(idealConnection.data()));

    AUDIT_STREAM << "AlsaDriver::setPlausibleConnection: connection like "
		 << idealConnection << " requested for device " << id << std::endl;

    if (port.first != -1 && port.second != -1) {

	m_devicePortMap[id] = port;

	for (unsigned int i = 0; i < m_devices.size(); ++i) {

	    if (m_devices[i]->getId() == id) {
		m_devices[i]->setConnection(idealConnection.data());
		break;
	    }
	}

	AUDIT_STREAM << "AlsaDriver::setPlausibleConnection: exact match available"
		     << std::endl;
	AUDIT_UPDATE;
	return;
    }

    // What we want is a connection that:
    // 
    //  * is in the right "class" (the 0-63/64-127/128+ range of client id)
    //  * has at least some text in common
    //  * is not yet in use for any device.
    // 
    // To do this, we exploit our privileged position as part of AlsaDriver
    // and use our knowledge of how connection strings are made (see
    // AlsaDriver::generatePortList above) to pick out the relevant parts
    // of the requested string.
    
    int client = 0;
    int colon = idealConnection.find(":");
    if (colon >= 0) client = idealConnection.left(colon).toInt();

    int firstSpace = idealConnection.find(" ");
    int endOfText  = idealConnection.find(QRegExp("[^\\w ]"), firstSpace);

    QString text;
    if (endOfText < 2) {
	text = idealConnection.mid(firstSpace + 1);
    } else {
	text = idealConnection.mid(firstSpace + 1, endOfText - firstSpace - 2);
    }

    for (int testName = 1; testName >= 0; --testName) {

	for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {

	    AlsaPortDescription *port = m_alsaPorts[i];

	    if (client > 0 && (port->m_client / 64 != client / 64)) continue;
	    
	    if (testName && text != "" &&
		!QString(port->m_name.c_str()).contains(text)) continue;
	    
	    bool used = false;
	    for (DevicePortMap::iterator dpmi = m_devicePortMap.begin();
		 dpmi != m_devicePortMap.end(); ++dpmi) {
		if (dpmi->second.first  == port->m_client &&
		    dpmi->second.second == port->m_port) {
		    used = true;
		    break;
		}
	    }
	    if (used) continue;

	    // OK, this one will do

	    AUDIT_STREAM << "AlsaDriver::setPlausibleConnection: fuzzy match "
			 << port->m_name << " available" << std::endl;

	    m_devicePortMap[id] = ClientPortPair(port->m_client, port->m_port);

	    for (unsigned int i = 0; i < m_devices.size(); ++i) {

		if (m_devices[i]->getId() == id) {
		    m_devices[i]->setConnection(port->m_name);
		    
		    // in this case we don't request a device resync,
		    // because this is only invoked at times such as
		    // file load when the GUI is well aware that the
		    // whole situation is in upheaval anyway
		    
		    AUDIT_UPDATE;
		    return;
		}
	    }
	}
    }

    AUDIT_STREAM << "AlsaDriver::setPlausibleConnection: nothing suitable available"
		 << std::endl;
    AUDIT_UPDATE;
}

void
AlsaDriver::initialise()
{
    initialiseAudio();
    initialiseMidi();
}



// Set up queue, client and port
//
void
AlsaDriver::initialiseMidi()
{ 
    AUDIT_START;

    int result;

    // Create a non-blocking handle.
    // ("hw" will possibly give in to other handles in future?)
    //
    if (snd_seq_open(&m_midiHandle,
                     "default",
                     SND_SEQ_OPEN_DUPLEX,
                     SND_SEQ_NONBLOCK) < 0)
    {
        AUDIT_STREAM << "AlsaDriver::initialiseMidi - "
                  << "couldn't open sequencer - " << snd_strerror(errno)
                  << std::endl;
        return;
    }

    generatePortList();
    generateInstruments();

    // Create a queue
    //
    if((m_queue = snd_seq_alloc_named_queue(m_midiHandle,
                                                "Rosegarden queue")) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - can't allocate queue"
                  << std::endl;
#endif
        return;
    }


    // Create a client
    //
    snd_seq_set_client_name(m_midiHandle, "Rosegarden sequencer");
    if((m_client = snd_seq_client_id(m_midiHandle)) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - can't create client"
                  << std::endl;
#endif
        return;
    }

    // Create a port
    //
    m_port = snd_seq_create_simple_port(m_midiHandle,
					"Rosegarden",
                                        SND_SEQ_PORT_CAP_WRITE |
                                        SND_SEQ_PORT_CAP_READ  |
					SND_SEQ_PORT_CAP_SUBS_WRITE |
                                        SND_SEQ_PORT_CAP_SUBS_READ,
                                        //SND_SEQ_PORT_CAP_NO_EXPORT,
                                        SND_SEQ_PORT_TYPE_APPLICATION);
    if (m_port < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - can't create port"
                  << std::endl;
#endif
        return;
    }

    ClientPortPair inputDevice = getFirstDestination(true); // duplex = true

    AUDIT_STREAM << "    Record port set to (" << inputDevice.first
              << ", "
              << inputDevice.second
              << ")" << std::endl << std::endl;

    AlsaPortList::iterator it;

    // Connect to all available output client/ports
    //
    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); it++)
    {
        if (snd_seq_connect_to(m_midiHandle,
                               m_port,
                               (*it)->m_client,
                               (*it)->m_port) < 0)
        {
            /*
            std::cerr << "AlsaDriver::initialiseMidi - "
                      << "can't subscribe output client/port ("
                      << (*it)->m_client << ", "
                      << (*it)->m_port << ")"
                      << std::endl;
                      */
        }
    }

    // Connect input port - enabling timestamping on the way through.
    // We have to fill out the subscription information as follows:
    //
    snd_seq_addr_t sender, dest;
    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca(&subs);

    sender.client = inputDevice.first;
    sender.port = inputDevice.second;
    dest.client = m_client;
    dest.port = m_port;

    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);

    snd_seq_port_subscribe_set_queue(subs, m_queue);

    // enable time-stamp-update mode 
    //
    snd_seq_port_subscribe_set_time_update(subs, 1);

    // set so we get realtime timestamps
    //
    snd_seq_port_subscribe_set_time_real(subs, 1);

    if (snd_seq_subscribe_port(m_midiHandle, subs) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - "
                  << "can't subscribe input client:port "
		  << int(sender.client) << ":" << int(sender.port)
                  << std::endl;
#endif
        // Not the end of the world if this fails but we
        // have to flag it internally.
        //
        m_midiInputPortConnected = false;
    }
    else
        m_midiInputPortConnected = true;

    // Set the input queue size
    //
    if (snd_seq_set_client_pool_output(m_midiHandle, 2000) < 0 ||
        snd_seq_set_client_pool_input(m_midiHandle, 2000) < 0 ||
        snd_seq_set_client_pool_output_room(m_midiHandle, 2000) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - "
                  << "can't modify pool parameters"
                  << std::endl;
#endif
        return;
    }

    getSystemInfo();

    // Modify status with MIDI success
    //
    m_driverStatus |= MIDI_OK;

    // Start the timer
    if ((result = snd_seq_start_queue(m_midiHandle, m_queue, NULL)) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - couldn't start queue - "
                  << snd_strerror(result)
                  << std::endl;
#endif
        exit(EXIT_FAILURE);
    }

    // process anything pending
    snd_seq_drain_output(m_midiHandle);

    AUDIT_STREAM << "AlsaDriver::initialiseMidi -  initialised MIDI subsystem"
              << std::endl << std::endl;
    AUDIT_UPDATE;
}

#ifdef HAVE_LIBJACK
void
AlsaDriver::createJackInputPorts(unsigned int totalPorts, bool deactivate)
{
    if (!m_audioClient) return;

    AUDIT_START;

    // Out port connetions if we need them
    //
    const char **outLeftPort = 0, **outRightPort = 0;

    // deactivate client
    //
    if (deactivate)
    {
        // store output connections for reconnect after port mods
        //
        outLeftPort = jack_port_get_connections(m_jackOutputPortLeft);
        outRightPort = jack_port_get_connections(m_jackOutputPortRight);

        if (jack_deactivate(m_audioClient))
        {
            AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                         << "client deactivation failed" << std::endl;
        }
    }

    // Unregister any we already have connected
    //
    for (unsigned int i = 0; i < m_jackInputPorts.size(); ++i)
        jack_port_unregister(m_audioClient, m_jackInputPorts[i]);
    m_jackInputPorts.clear();

    jack_port_t *inputPort;
    char portName[10];
    for (unsigned int i = 0; i < totalPorts; ++i)
    {
        sprintf(portName, "in_%d", i + 1);
        inputPort = jack_port_register(m_audioClient,
                                       portName,
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsInput|JackPortIsTerminal,
                                       0);
         m_jackInputPorts.push_back(inputPort);


         AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                      << "adding input port " << i + 1 << std::endl;
    }
    
    // reactivate client
    //
    if (jack_activate(m_audioClient))
    {
        AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                     << "client deactivation failed" << std::endl;
    }

    std::string capture_1, capture_2;

    // Assign port directly if they were specified
    //
    if (m_args.size() >= 4)
    {
        capture_1 = std::string(m_args[2].data());
        capture_2 = std::string(m_args[3].data());
    }
    else // match from JACK
    {
        AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                     << "getting ports" << std::endl;
        const char **ports =
            jack_get_ports(m_audioClient, NULL, NULL,
                           JackPortIsPhysical|JackPortIsOutput);

        if (ports)
        {
            if (ports[0]) capture_1 = std::string(ports[0]);
            if (ports[1]) capture_2 = std::string(ports[1]);

            // count ports
            unsigned int i = 0;
            for (i = 0; ports[i]; i++);
            AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                         << "found " << i << " JACK physical inputs"
                         << std::endl;
        }
        else
            AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                         << "no JACK physical inputs found"
                         << std::endl;
        free(ports);
    }

    if (capture_1 != "")
    {
        AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                     << "connecting from "
                     << "\"" << capture_1.c_str() << "\" to \""
                     << jack_port_name(m_jackInputPorts[0]) << "\""
                     << std::endl;

        // now input
        if (jack_connect(m_audioClient, capture_1.c_str(),
                         jack_port_name(m_jackInputPorts[0])))
        {
            AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                         << "cannot connect to JACK input port" << std::endl;
        }
    }

    if (capture_2 != "")
    {
        AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                     << "connecting from "
                     << "\"" << capture_2.c_str() << "\" to \""
                     << jack_port_name(m_jackInputPorts[1]) << "\""
                     << std::endl;

        if (jack_connect(m_audioClient, capture_2.c_str(),
                         jack_port_name(m_jackInputPorts[1])))
        {
            AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                         << "cannot connect to JACK input port" << std::endl;
        }
    }

    // Reconnect out ports
    if (deactivate)
    {

        AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                     << "reconnecting JACK output port (left)"
                     << std::endl;

        if (outLeftPort && outLeftPort[0])
        {
            if (jack_connect(m_audioClient,
                             jack_port_name(m_jackOutputPortLeft),
                             outLeftPort[0]))
            {
                AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                             << "cannot reconnect JACK output port (left)"
                             << std::endl;
            }
        }

        AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                     << "reconnecting JACK output port (right)"
                     << std::endl;

        if (outRightPort && outRightPort[0])
        {
            if (jack_connect(m_audioClient, 
                             jack_port_name(m_jackOutputPortRight),
                             outRightPort[0]))
            {
                AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                             << "cannot reconnect JACK output port (right)"
                             << std::endl;
            }
        }
    }

    AUDIT_UPDATE;
}

#endif


// We don't even attempt to use ALSA audio.  We just use JACK instead.
// See comment at the top of this file and jackProcess() for further
// information on how we use this.
//
void
AlsaDriver::initialiseAudio()
{
#ifdef HAVE_LIBJACK
    AUDIT_START;
    AUDIT_STREAM << std::endl;

    // Using JACK instead
    //
    std::string jackClientName = "rosegarden";

    // attempt connection to JACK server
    //
    if ((m_audioClient = jack_client_new(jackClientName.c_str())) == 0)
    {
        AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                     << "JACK server not running"
                     << std::endl;
        return;
    }

    // set callbacks
    //
    jack_set_process_callback(m_audioClient, jackProcess, this);

    // deprecated
    //jack_set_buffer_size_callback(m_audioClient, jackBufferSize, this);

    jack_set_sample_rate_callback(m_audioClient, jackSampleRate, this);
    jack_on_shutdown(m_audioClient, jackShutdown, this);
    jack_set_graph_order_callback(m_audioClient, jackGraphOrder, this);
    //jack_set_xrun_callback(m_audioClient, jackXRun, this);

    // get and report the sample rate
    //
    _jackSampleRate = jack_get_sample_rate(m_audioClient);

    AUDIT_STREAM << "AlsaDriver::initialiseAudio - JACK sample rate = "
                 << _jackSampleRate << "Hz" << std::endl;



    // set some latencies - these don't appear to do anything yet
    //
    /*
    jack_port_set_latency(m_audioOutputPortLeft, 1024);
    jack_port_set_latency(m_audioOutputPortRight, 1024);
    jack_port_set_latency(m_jackInputPort, 1024);
    */

    // Get the initial buffer size before we activate the client
    //
    _jackBufferSize = jack_get_buffer_size(m_audioClient);

    // create buffers
    //
    _tempOutBuffer1 = new sample_t[_jackBufferSize];
    _tempOutBuffer2 = new sample_t[_jackBufferSize];
    _pluginBufferIn1 = new sample_t[_jackBufferSize];
    _pluginBufferIn2 = new sample_t[_jackBufferSize];
    _pluginBufferOut1 = new sample_t[_jackBufferSize];
    _pluginBufferOut2 = new sample_t[_jackBufferSize];

    // Create output ports
    //
    m_jackOutputPortLeft = jack_port_register(m_audioClient,
                                              "out_1",
                                              JACK_DEFAULT_AUDIO_TYPE,
                                              JackPortIsOutput,
                                              0);

    AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                 << "added output port 1 (left)" << std::endl;

    m_jackOutputPortRight = jack_port_register(m_audioClient,
                                               "out_2",
                                               JACK_DEFAULT_AUDIO_TYPE,
                                               JackPortIsOutput,
                                               0);
    // Create and connect (default) two audio inputs - activating as
    // we go
    //
    createJackInputPorts(2, false);

    AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                 << "added output port 2 (right)" << std::endl;

    std::string playback_1, playback_2;

    // Assign port directly if they were specified
    //
    if (m_args.size() >= 2)
    {
        playback_1 = std::string(m_args[0].data());
        playback_2 = std::string(m_args[1].data());
    }
    else // match from JACK
    {
        const char **ports =
            jack_get_ports(m_audioClient, NULL, NULL,
                           JackPortIsPhysical|JackPortIsInput);

        if (ports)
        {
            if (ports[0]) playback_1 = std::string(ports[0]);
            if (ports[1]) playback_2 = std::string(ports[1]);

            // count ports
            unsigned int i = 0;
            for (i = 0; ports[i]; i++);
            AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                         << "found " << i << " JACK physical outputs"
                         << std::endl;
        }
        else
            AUDIT_STREAM << "AlsaDriver::createJackInputPorts - "
                         << "no JACK physical outputs found"
                         << std::endl;
        free(ports);
    }

    if (playback_1 != "")
    {
        AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                     << "connecting from "
                     << "\"" << jack_port_name(m_jackOutputPortLeft)
                     << "\" to \"" << playback_1.c_str() << "\""
                     << std::endl;

        // connect our client up to the ALSA ports - first left output
        //
        if (jack_connect(m_audioClient, jack_port_name(m_jackOutputPortLeft),
                         playback_1.c_str()))
        {
            AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                         << "cannot connect to JACK output port" << std::endl;
            return;
        }
    }

    if (playback_2 != "")
    {
        AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                     << "connecting from "
                     << "\"" << jack_port_name(m_jackOutputPortLeft)
                     << "\" to \"" << playback_2.c_str() << "\""
                     << std::endl;

        if (jack_connect(m_audioClient, jack_port_name(m_jackOutputPortRight),
                         playback_2.c_str()))
        {
            AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                         << "cannot connect to JACK output port" << std::endl;
            return;
        }
    }

    // Get the latencies from JACK and set them as RealTime
    //
    jack_nframes_t outputLatency =
        jack_port_get_total_latency(m_audioClient, m_jackOutputPortLeft);

    jack_nframes_t inputLatency = 
        jack_port_get_total_latency(m_audioClient, m_jackInputPorts[0]);

    double latency = double(outputLatency) / double(_jackSampleRate);

    // Set the audio latencies ready for collection by the GUI
    //
    m_audioPlayLatency = RealTime(int(latency), int(latency * 1000000.0));

    latency = double(inputLatency) / double(_jackSampleRate);

    m_audioRecordLatency = RealTime(int(latency), int(latency * 1000000.0));

    AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                 << "JACK playback latency " << m_audioPlayLatency << std::endl;

    AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                 << "JACK record latency " << m_audioRecordLatency << std::endl;

    // ok with audio driver
    //
    m_driverStatus |= AUDIO_OK;

    AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                 << "initialised JACK audio subsystem"
                 << std::endl;

    AUDIT_STREAM << "AlsaDriver::initialiseAudio - "
                 << "creating disk thread" << std::endl;

    // setup disk thread
    //
    pthread_create(&_diskThread, NULL, jackDiskThread, this);
    pthread_detach(_diskThread);

    AUDIT_UPDATE;
#endif
}

void
AlsaDriver::initialisePlayback(const RealTime &position,
                               const RealTime &playLatency)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver - initialisePlayback" << std::endl;
#endif

    m_alsaPlayStartTime = getAlsaTime();
    m_playStartPosition = position;
    m_startPlayback = true;

    // If the clock is enabled then adjust for the MIDI Clock to 
    // synchronise the sequencer with the clock.
    //
    if (m_midiClockEnabled)
    {
        // Last clock sent should always be ahead of current
        // ALSA time - adjust for latency and find nearest
        // clock for start time.
        //
        RealTime alsaClockSent = m_midiClockSendTime - playLatency;

        while (alsaClockSent > m_alsaPlayStartTime)
            alsaClockSent = alsaClockSent - RealTime(0, m_midiClockInterval);

        /*
        std::cout << "START ADJUST FROM " << m_alsaPlayStartTime
             << " to " << alsaClockSent << endl;
             */

        m_alsaPlayStartTime = alsaClockSent;

        if (position == RealTime::zeroTime)
            sendSystemQueued(SND_SEQ_EVENT_START, "",
                             m_alsaPlayStartTime + playLatency);
        else
            sendSystemQueued(SND_SEQ_EVENT_CONTINUE, "",
                             m_alsaPlayStartTime + playLatency);

    }

    if (isMMCMaster())
    {
        sendMMC(127, MIDI_MMC_PLAY, true, "");
    }

#ifdef HAVE_LIBJACK
    // stop transport master
    if (_jackTransportMaster && _jackTransportEnabled && m_audioClient)
        jack_transport_start(m_audioClient);
#endif

}


void
AlsaDriver::stopPlayback()
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver - stopPlayback" << std::endl;
#endif

    allNotesOff();
    m_playing = false;

    // reset the clock send time
    //
    m_midiClockSendTime = RealTime::zeroTime;

    // Flush the output and input queues
    //
    snd_seq_remove_events_t *info;
    snd_seq_remove_events_alloca(&info);
    snd_seq_remove_events_set_condition(info, SND_SEQ_REMOVE_INPUT|
                                              SND_SEQ_REMOVE_OUTPUT);
    snd_seq_remove_events(m_midiHandle, info);

    // Send system stop to duplex MIDI devices
    //
    if (m_midiClockEnabled) sendSystemDirect(SND_SEQ_EVENT_STOP, "");

    // send sounds-off to all client port pairs
    //
    AlsaPortList::iterator it;
    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); ++it)
    {
        sendDeviceController(ClientPortPair((*it)->m_client,
                                            (*it)->m_port),
                             MIDI_CONTROLLER_SOUNDS_OFF,
                             0);
    }

    // Close any recording file
    if (m_recordStatus == RECORD_AUDIO && _recordFile)
    {
        _recordFile->close();

        // Create event to return to gui to say that we've completed
        // an audio file and we can generate a preview for it now.
        //
        try
        {
            MappedEvent *mE =
                new MappedEvent(_recordFile->getId(),
                                MappedEvent::AudioGeneratePreview,
                                0);

            // send completion event
            insertMappedEventForReturn(mE);
        }
        catch(...) {;}

        _recordFile = 0;
        m_recordStatus = ASYNCHRONOUS_AUDIO;

    }
        

    // Change recorded state if any set
    //
    if (m_recordStatus == RECORD_MIDI)
        m_recordStatus = ASYNCHRONOUS_MIDI;


    // Sometimes we don't "process" again before we actually
    // stop

#ifdef HAVE_LIBJACK

    // stop transport master
    if (_jackTransportMaster && _jackTransportEnabled && m_audioClient)
        jack_transport_stop(m_audioClient);

    pthread_mutex_lock(&_diskThreadLock);
    clearAudioPlayQueue();
    pthread_mutex_unlock(&_diskThreadLock);

    // free all RingBuffers
    //
    for (unsigned int j = 0; j < _ringBuffer.size(); ++j)
    {
        _ringBuffer[j].first = false;
        _ringBuffer[j].second->reset();
    }

#endif

#ifdef HAVE_LADSPA
    // Reset all plugins dumping any remnant audio
    //
    resetAllPlugins();

#endif // HAVE_LADSPA


}

void
AlsaDriver::resetPlayback(const RealTime &position, const RealTime &latency)
{
    // Reset note offs to correct positions
    //
    RealTime modifyNoteOff = m_playStartPosition - m_alsaPlayStartTime;

    // set new
    m_playStartPosition = position;
    m_alsaPlayStartTime = getAlsaTime() - latency;

    // add
    modifyNoteOff = modifyNoteOff - m_playStartPosition + m_alsaPlayStartTime;

    // modify the note offs that exist as they're relative to the
    // playStartPosition terms.
    //
    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
                                i != m_noteOffQueue.end(); ++i)
    {

        // if we're fast forwarding then we bring the note off closer
        if (modifyNoteOff <= RealTime::zeroTime)
        {
            (*i)->setRealTime((*i)->getRealTime() + modifyNoteOff);
        }
        else // we're rewinding - kill the note immediately
        {
            (*i)->setRealTime(m_playStartPosition);
        }
    }

#ifdef HAVE_LIBJACK

    // Clear down all playing audio files
    //
    if(pthread_mutex_trylock(&_diskThreadLock) != EBUSY)
    {
        std::vector<PlayableAudioFile*>::iterator it;
        for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
        {
#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::resetPlayback - resetting audio file" << std::endl;
#endif // DEBU_ALSA

            // only interested in playing files
            if ((*it)->getStatus() != PlayableAudioFile::PLAYING)
                continue;

            if (modifyNoteOff <= RealTime::zeroTime) // if ffwding
                (*it)->setStatus(PlayableAudioFile::DEFUNCT); // for the moment do the same
            else // rewinding - mark this file as DEFUNCT
                (*it)->setStatus(PlayableAudioFile::DEFUNCT);
        }

        pthread_mutex_unlock(&_diskThreadLock);
    }

#endif
}


void
AlsaDriver::allNotesOff()
{
    snd_seq_event_t *event = new snd_seq_event_t();
    ClientPortPair outputDevice;
    RealTime offTime;

    // drop any pending notes
    snd_seq_drop_output_buffer(m_midiHandle);
    snd_seq_drop_output(m_midiHandle);

    // prepare the event
    snd_seq_ev_clear(event);
    snd_seq_ev_set_source(event, m_port);
    offTime = getAlsaTime();

    for (NoteOffQueue::iterator it = m_noteOffQueue.begin();
                                it != m_noteOffQueue.end(); ++it)
    {
        // Set destination according to instrument mapping to port
        //
        outputDevice = getPairForMappedInstrument((*it)->getInstrument());
	if (outputDevice.first < 0 || outputDevice.second < 0) continue;

        snd_seq_ev_set_dest(event,
                            outputDevice.first,
                            outputDevice.second);


        /*
        snd_seq_real_time_t alsaOffTime = { offTime.sec,
                                            offTime.usec * 1000 };

        snd_seq_ev_schedule_real(event, m_queue, 0, &alsaOffTime);
        */

        snd_seq_ev_set_noteoff(event,
                               (*it)->getChannel(),
                               (*it)->getPitch(),
                               127);
        //snd_seq_event_output(m_midiHandle, event);
        int error = snd_seq_event_output_direct(m_midiHandle, event);

        if (error < 0)
        {
#ifdef DEBUG_ALSA
	    std::cerr << "AlsaDriver::allNotesOff - "
                      << "can't send event" << std::endl;
#endif
        }

        delete(*it);
    }
    
    m_noteOffQueue.erase(m_noteOffQueue.begin(), m_noteOffQueue.end());

    /*
    std::cerr << "AlsaDriver::allNotesOff - "
              << " queue size = " << m_noteOffQueue.size() << std::endl;
              */

    // flush
    snd_seq_drain_output(m_midiHandle);
    delete event;
}

void
AlsaDriver::processNotesOff(const RealTime &time)
{
    static snd_seq_event_t event;

    ClientPortPair outputDevice;
    RealTime offTime;

    // prepare the event
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_port);

    NoteOffQueue::iterator it = m_noteOffQueue.begin();

    for (;it != m_noteOffQueue.end() && (*it)->getRealTime() <= time; ++it)
    {
        // Set destination according to instrument mapping to port
        //
        outputDevice = getPairForMappedInstrument((*it)->getInstrument());
	if (outputDevice.first < 0 || outputDevice.second < 0) continue;

        snd_seq_ev_set_dest(&event,
                            outputDevice.first,
                            outputDevice.second);

        offTime = (*it)->getRealTime();

        snd_seq_real_time_t alsaOffTime = { offTime.sec,
                                            offTime.usec * 1000 };

        snd_seq_ev_schedule_real(&event, m_queue, 0, &alsaOffTime);
        snd_seq_ev_set_noteoff(&event,
                               (*it)->getChannel(),
                               (*it)->getPitch(),
                               127);
        // send note off
        snd_seq_event_output(m_midiHandle, &event);
        delete(*it);
        m_noteOffQueue.erase(it);
    }

    // and flush them
    snd_seq_drain_output(m_midiHandle);

    /*
      std::cerr << "AlsaDriver::processNotesOff - "
      << " queue size = " << m_noteOffQueue.size() << std::endl;
    */
}

void
AlsaDriver::processAudioQueue(const RealTime &playLatency, bool now)
{
    pthread_mutex_lock(&_diskThreadLock);

    std::vector<PlayableAudioFile*>::iterator it;
    RealTime currentTime = getSequencerTime() - playLatency;

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
        // buffer up, buttercup
        if ((*it)->getStatus() == PlayableAudioFile::IDLE)
            (*it)->fillRingBuffer();

        if ((currentTime >= (*it)->getStartTime() || now) &&
            (*it)->getStatus() == PlayableAudioFile::IDLE)
        {
            (*it)->setStatus(PlayableAudioFile::PLAYING);
        }

        if (currentTime >= (*it)->getEndTime() &&
            (*it)->getStatus() == PlayableAudioFile::PLAYING)
        {
            (*it)->setStatus(PlayableAudioFile::DEFUNCT);

            // Simple event to inform that AudioFileId has
            // now stopped playing.
            //
            try
            {
                MappedEvent *mE =
                    new MappedEvent((*it)->getAudioFile()->getId(),
                                    MappedEvent::AudioStopped,
                                    0);

                // send completion event
                insertMappedEventForReturn(mE);
            }
            catch(...) {;}
        }
        
#ifdef HAVE_LIBJACK
        // Free up any used RingBuffers for reuse
        //
        if ((*it)->getStatus() == PlayableAudioFile::DEFUNCT)
        {
            for (unsigned int j = 0; j < _ringBuffer.size(); ++j)
            {
                if (_ringBuffer[j].second == (*it)->getRingBuffer())
                {
                    _ringBuffer[j].first = false;
                    _ringBuffer[j].second->zero(); // zero the buffer
                    break;
                }
            }
        }
#endif

    }

    pthread_mutex_unlock(&_diskThreadLock);

}

// Get the queue time and convert it to RealTime for the gui
// to use.
//
RealTime
AlsaDriver::getSequencerTime()
{
    RealTime t(0, 0);

    if (m_playing)
       t = getAlsaTime() + m_playStartPosition - m_alsaPlayStartTime;

    return t;
}

// Gets the time of the ALSA queue
//
RealTime
AlsaDriver::getAlsaTime()
{
    RealTime sequencerTime(0, 0);

    snd_seq_queue_status_t *status;
    snd_seq_queue_status_malloc(&status);

    if (snd_seq_get_queue_status(m_midiHandle, m_queue, status) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::getSequencerTime - can't get queue status"
                  << std::endl;
#endif
        return sequencerTime;
    }

    sequencerTime.sec = snd_seq_queue_status_get_real_time(status)->tv_sec;

    double microSeconds = snd_seq_queue_status_get_real_time(status)->tv_nsec
                          /1000.0;

    sequencerTime.usec = (int)microSeconds;

    snd_seq_queue_status_free(status);

    return sequencerTime;
}


// Get all pending input events and turn them into a MappedComposition.
//
//
MappedComposition*
AlsaDriver::getMappedComposition(const RealTime &playLatency)
{

#ifdef HAVE_LIBJACK
    // Reset this counter for return MappedEvent usage on every pass through.
    //
    _jackMappedEventCounter = 0;
#endif

    // If we're already inserted some audio VU meter events then
    // don't clear them from the composition.
    //
    if (m_audioMeterSent == false)
        m_recordComposition.clear();

    if (m_recordStatus != RECORD_MIDI &&
        m_recordStatus != RECORD_AUDIO &&
        m_recordStatus != ASYNCHRONOUS_MIDI &&
        m_recordStatus != ASYNCHRONOUS_AUDIO)
    {
        m_audioMeterSent = false; // reset this always
        return &m_recordComposition;
    }

    // If the input port hasn't connected we shouldn't poll it
    //
    if(m_midiInputPortConnected == false)
    {
        m_audioMeterSent = false; // reset this always
        return &m_recordComposition;
    }

    RealTime eventTime(0, 0);

    snd_seq_event_t *event;

    while(snd_seq_event_input(m_midiHandle, &event) > 0)
    {
        unsigned int channel = (unsigned int)event->data.note.channel;
        unsigned int chanNoteKey = ( channel << 8 ) +
                                   (unsigned int) event->data.note.note;

        eventTime.sec = event->time.time.tv_sec;
        eventTime.usec = event->time.time.tv_nsec / 1000;
        eventTime = eventTime - m_alsaRecordStartTime + m_playStartPosition
                              - playLatency;

        switch(event->type)
        {

            case SND_SEQ_EVENT_NOTE:
            case SND_SEQ_EVENT_NOTEON:
                if (event->data.note.velocity > 0)
                {
                    m_noteOnMap[chanNoteKey] = new MappedEvent();
                    m_noteOnMap[chanNoteKey]->setPitch(event->data.note.note);
                    m_noteOnMap[chanNoteKey]->
                        setVelocity(event->data.note.velocity);
                    m_noteOnMap[chanNoteKey]->setEventTime(eventTime);

                    // Negative duration - we need to hear the NOTE ON
                    // so we must insert it now with a negative duration
                    // and pick and mix against the following NOTE OFF
                    // when we create the recorded segment.
                    //
                    m_noteOnMap[chanNoteKey]->setDuration(RealTime(-1, 0));

                    // Create a copy of this when we insert the NOTE ON -
                    // keeping a copy alive on the m_noteOnMap.
                    //
                    // We shake out the two NOTE Ons after we've recorded
                    // them.
                    //
                    m_recordComposition.insert(
                            new MappedEvent(m_noteOnMap[chanNoteKey]));

                    break;
                }

            case SND_SEQ_EVENT_NOTEOFF:
                if (m_noteOnMap[chanNoteKey] != 0)
                {
                    // Set duration correctly on the NOTE OFF
                    //
                    RealTime duration = eventTime -
                             m_noteOnMap[chanNoteKey]->getEventTime();

                    if (duration < RealTime::zeroTime) break;

                    // Velocity 0 - NOTE OFF.  Set duration correctly
                    // for recovery later.
                    //
                    m_noteOnMap[chanNoteKey]->setVelocity(0);
                    m_noteOnMap[chanNoteKey]->setDuration(duration);

                    // force shut off of note
                    m_recordComposition.insert(m_noteOnMap[chanNoteKey]);

                    // reset the reference
                    //
                    m_noteOnMap[chanNoteKey] = 0;

                }
                break;

            case SND_SEQ_EVENT_KEYPRESS:
                {
                    // Fix for 632964 by Pedro Lopez-Cabanillas (20030523)
                    //
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiKeyPressure);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.note.note);
                    mE->setData2(event->data.note.velocity);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CONTROLLER:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiController);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.param);
                    mE->setData2(event->data.control.value);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_PGMCHANGE:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiProgramChange);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.value);
                    m_recordComposition.insert(mE);

                }
                break;

            case SND_SEQ_EVENT_PITCHBEND:
                {
                    // Fix for 711889 by Pedro Lopez-Cabanillas (20030523)
                    //
                    int s = event->data.control.value + 8192;
                    int d1 = (s >> 7) & 0x7f; // data1 = MSB
                    int d2 = s & 0x7f; // data2 = LSB
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiPitchBend);
                    mE->setEventTime(eventTime);
                    mE->setData1(d1);
                    mE->setData2(d2);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CHANPRESS:
                {
                    // Fixed by Pedro Lopez-Cabanillas (20030523)
                    //
                    int s = event->data.control.value & 0x7f;
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiChannelPressure);
                    mE->setEventTime(eventTime);
                    mE->setData1(s);
                    m_recordComposition.insert(mE);
                }
               break;

            case SND_SEQ_EVENT_SYSEX:
               {
                   // Bundle up the data into a block on the MappedEvent
                   //
                   std::string data;
                   char *ptr = (char*)(event->data.ext.ptr);
                   for (unsigned int i = 0; i < event->data.ext.len; ++i)
                       data += *(ptr++);

#ifdef DEBUG_ALSA
                   if ((MidiByte)(data[0]) == MIDI_SYSEX_RT)
                   {
                       std::cerr << "REALTIME SYSEX" << endl;
                   }
#endif

                   MappedEvent *mE = new MappedEvent();
                   mE->setType(MappedEvent::MidiSystemExclusive);
                   // chop off SYX and EOX bytes from data block
                   // Fix for 674731 by Pedro Lopez-Cabanillas (20030601)
                   DataBlockRepository::setDataBlockForEvent(mE, data.substr(1, data.length() - 2));
                   mE->setEventTime(eventTime);
                   m_recordComposition.insert(mE);
               }
               break;


            case SND_SEQ_EVENT_SENSING: // MIDI device is still there
               break;

            case SND_SEQ_EVENT_CLOCK:
               /*
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "got realtime MIDI clock" << std::endl;
                         */
               break;

            case SND_SEQ_EVENT_START:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "START" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_CONTINUE:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "CONTINUE" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_STOP:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "STOP" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_SONGPOS:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "SONG POSITION" << std::endl;
#endif
               break;

               // these cases are handled by checkForNewClients
               //
            case SND_SEQ_EVENT_PORT_SUBSCRIBED:
            case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
                   break;

            case SND_SEQ_EVENT_TICK:
            default:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "got unhandled MIDI event type from ALSA sequencer"
                         << "(" << int(event->type) << ")" << std::endl;
#endif
               break;


        }

        snd_seq_free_event(event);
    }

    // reset this always
    m_audioMeterSent = false;

    return &m_recordComposition;
}
    
void
AlsaDriver::processMidiOut(const MappedComposition &mC,
                           const RealTime &playLatency,
                           bool now)
{
    if (_threadAlsaClosing == true) return;

    RealTime midiRelativeTime;
    RealTime midiRelativeStopTime;
    MappedInstrument *instrument;
    ClientPortPair outputDevice;
    MidiByte channel;
    snd_seq_event_t *event = new snd_seq_event_t();

    // These won't change in this slice
    //
    snd_seq_ev_clear(event);
    snd_seq_ev_set_source(event, m_port);

    for (MappedComposition::iterator i = mC.begin(); i != mC.end(); ++i)
    {
        if ((*i)->getType() >= MappedEvent::Audio)
            continue;


        midiRelativeTime = (*i)->getEventTime() - m_playStartPosition +
                           playLatency + m_alsaPlayStartTime;

        // Second and nanoseconds for ALSA
        //
        snd_seq_real_time_t time = { midiRelativeTime.sec,
                                     midiRelativeTime.usec * 1000 };

        // millisecond note duration
        //
        unsigned int eventDuration = (*i)->getDuration().sec * 1000
             + (*i)->getDuration().usec / 1000;

        // Set destination according to Instrument mapping
        //
        outputDevice = getPairForMappedInstrument((*i)->getInstrument());
	if (outputDevice.first < 0 && outputDevice.second < 0) continue;

        snd_seq_ev_set_dest(event,
                            outputDevice.first,
                            outputDevice.second);

        /*
        std::cout << "INSTRUMENT = " << (*i)->getInstrument() << endl;

        std::cout << "TIME = " << time.tv_sec << " : " << time.tv_nsec * 1000
              << endl;


        std::cout << "EVENT to " << (int)event->dest.client
                  << " : " 
                  << (int)event->dest.port << endl;
        */

        snd_seq_ev_schedule_real(event, m_queue, 0, &time);
        instrument = getMappedInstrument((*i)->getInstrument());

        // set the stop time for Note Off
        //
        midiRelativeStopTime = midiRelativeTime + (*i)->getDuration();
 
        if (instrument != 0)
            channel = instrument->getChannel();
        else
        {
#ifdef DEBUG_ALSA
            std::cerr << "processMidiOut() - couldn't get Instrument for Event"
                      << std::endl;
#endif
            channel = 0;
        }

        switch((*i)->getType())
        {
            case MappedEvent::MidiNoteOneShot:
                {
                    // Just an arbitrary duration for one-shot Notes
                    // for the moment until we work out the timing
                    // conversion.
                    //
                    int duration = 100;
                    snd_seq_ev_set_note(event,
                                        channel,
                                        (*i)->getPitch(),
                                        (*i)->getVelocity(),
                                        duration);
                }
                break;

            case MappedEvent::MidiNote:
                // If we've got an "infinite" note then just noteon -
                // else send the duration.
                //
                if ((*i)->getDuration() == RealTime(-1, 0))
                {
                    snd_seq_ev_set_noteon(event,
                                          channel,
                                          (*i)->getPitch(),
                                          (*i)->getVelocity());
                }
                else
                {
                    if ((*i)->getVelocity() == 0)
                    {
                        snd_seq_ev_set_noteoff(event,
                                               channel,
                                               (*i)->getPitch(),
                                               (*i)->getVelocity());
                    }
                    else
                    {
                        snd_seq_ev_set_note(event,
                                            channel,
                                            (*i)->getPitch(),
                                            (*i)->getVelocity(),
                                            eventDuration);

                        /*
                        std::cerr << "NOTE OUT : pitch = " 
                                  << int((*i)->getPitch())
                                  << ", velocity = "
                                  << int((*i)->getVelocity())
                                  << " at time " 
                                  << (*i)->getEventTime() << std::endl;
                                  */
                    }
                }
                break;

            case MappedEvent::MidiProgramChange:
                snd_seq_ev_set_pgmchange(event,
                                         channel,
                                         (*i)->getData1());
                break;

            case MappedEvent::MidiKeyPressure:
                snd_seq_ev_set_keypress(event,
                                        channel,
                                        (*i)->getData1(),
                                        (*i)->getData2());
                break;

            case MappedEvent::MidiChannelPressure:
                snd_seq_ev_set_chanpress(event,
                                         channel,
                                         (*i)->getData1());
                break;

            case MappedEvent::MidiPitchBend:
                {
                    int d1 = (int)((*i)->getData1());
                    int d2 = (int)((*i)->getData2());
                    int value = ((d1 << 7) | d2) - 8192;

                    // keep within -8192 to +8192
                    //
                    // if (value & 0x4000)
                    //    value -= 0x8000;

                    snd_seq_ev_set_pitchbend(event,
                                             channel,
                                             value);
                }
                break;

            case MappedEvent::MidiSystemExclusive:
                {
                    // pack data between start and end blocks
                    //

                    char out[2];
                    sprintf(out, "%c", MIDI_SYSTEM_EXCLUSIVE);
                    std::string data = out;

                    data += DataBlockRepository::getDataBlockForEvent((*i));

                    sprintf(out, "%c", MIDI_END_OF_EXCLUSIVE);
                    data += out;

                    snd_seq_ev_set_sysex(event,
                                         data.length(),
                                         (char*)(data.c_str()));
                }
                break;

            case MappedEvent::MidiController:
                snd_seq_ev_set_controller(event,
                                          channel,
                                          (*i)->getData1(),
                                          (*i)->getData2());
                break;

            case MappedEvent::Audio:
            case MappedEvent::AudioCancel:
            case MappedEvent::AudioLevel:
            case MappedEvent::AudioStopped:
            case MappedEvent::SystemUpdateInstruments:
            case MappedEvent::SystemJackTransport:
            case MappedEvent::SystemMMCTransport:
            case MappedEvent::SystemMIDIClock:
                break;

            default:
            case MappedEvent::InvalidMappedEvent:
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processMidiOut - "
                          << "skipping unrecognised or invalid MappedEvent type"
                          << std::endl;
#endif
                continue;
        }

        int error = 0;
        if (now || m_playing == false)
        {
            RealTime nowTime = getAlsaTime();
            snd_seq_real_time_t outTime = { nowTime.sec,
                                         nowTime.usec * 1000 };
            snd_seq_ev_schedule_real(event, m_queue, 0, &outTime);
            error = snd_seq_event_output_direct(m_midiHandle, event);
        }
        else
            error = snd_seq_event_output(m_midiHandle, event);

#ifdef DEBUG_ALSA
        if (error < 0)
        {
            std::cerr << "AlsaDriver::processMidiOut - "
                      << "failed to send ALSA event ("
                      << error << ")" <<  std::endl;
        }
#endif

        // Add note to note off stack
        //
        if ((*i)->getType() == MappedEvent::MidiNote)
        {
            bool extended = false;
            NoteOffQueue::iterator it;

            for (it = m_noteOffQueue.begin(); it != m_noteOffQueue.end(); ++it)
            {
                if ((*it)->getPitch() == (*i)->getPitch() &&
                    (*it)->getChannel() == channel &&
                    (*it)->getInstrument() == (*i)->getInstrument())
                {
                    (*it)->setRealTime(midiRelativeStopTime);
                    extended = true;
                }
            }

            if (!extended)
            {
                NoteOffEvent *noteOffEvent =
                    new NoteOffEvent(midiRelativeStopTime, // already calculated
                                     (*i)->getPitch(),
                                     channel,
                                     (*i)->getInstrument());
                m_noteOffQueue.insert(noteOffEvent);
            }
        }
    }

    snd_seq_drain_output(m_midiHandle);

    delete event;
}


// This is almost identical to the aRts driver version at
// the moment.  Can't see it ever having to change that
// much really.
//
void
AlsaDriver::processEventsOut(const MappedComposition &mC,
                             const RealTime &playLatency,
                             bool now)
{
    if (m_startPlayback)
    {
        m_alsaPlayStartTime = getAlsaTime();
        m_startPlayback= false;
        m_playing = true;
    }

    AudioFile *audioFile = 0;

    // insert audio events if we find them
    for (MappedComposition::iterator i = mC.begin(); i != mC.end(); ++i)
    {
        // Play an audio file
        //
        if ((*i)->getType() == MappedEvent::Audio)
        {
            // Check for existence of file - if the sequencer has died
            // and been restarted then we're not always loaded up with
            // the audio file references we should have.  In the future
            // we could make this just get the gui to reload our files
            // when (or before) this fails.
            //
            audioFile = getAudioFile((*i)->getAudioID());

            if (audioFile)
            { 
                RealTime adjustedEventTime =
                    (*i)->getEventTime();

                /*
                    adjustedEventTime == RealTime::zeroTime &&
                    getSequencerTime() > RealTime(1, 0))
                    */

                // If we're playing, the event time is two minutes or
                // more in the past we've sent an async audio event -
                // if we're playing we have to reset this time to
                // some point in our playing future - otherwise we
                // just reset to zero.
                //
                if (adjustedEventTime <= RealTime(-120, 0))
                {
                    if (m_playing)
                        adjustedEventTime = getAlsaTime() + RealTime(0, 500000);
                    else
                        adjustedEventTime = RealTime::zeroTime;
                }

                // Get a RingBuffer - if there's not one available the zero pointer
                // will generate one inside the PlayableAudioFile.
                //
                RingBuffer *ringBuffer = 0;

#ifdef HAVE_LIBJACK
                for (unsigned int j = 0; j < _ringBuffer.size(); ++j)
                {
                    if (_ringBuffer[j].first == false)
                    {
                        ringBuffer = _ringBuffer[j].second;
                        _ringBuffer[j].first = true;
                        break;
                    }
                }
#endif

                // Create this event in this thread and push it onto audio queue.
                // All initialisation will occur in the disk thread.
                //
                PlayableAudioFile *audioFile =
                    new PlayableAudioFile((*i)->getInstrument(),
                                          getAudioFile((*i)->getAudioID()),
                                          adjustedEventTime - playLatency,
                                          (*i)->getAudioStartMarker(),
                                          (*i)->getDuration(),
                                          4096, // play buffer size
                                          ringBuffer);

                // segment runtime id
                audioFile->setRuntimeSegmentId((*i)->getRuntimeSegmentId());

                // This is thread safe as we push the audio file into a holding queue
                // which is pushed onto the actual audio queue at pushPlayableAudioQueue()
                //
                queueAudio(audioFile);
            }
            else
            {
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "can't find audio file reference" 
                          << std::endl;

                std::cerr << "AlsaDriver::processEventsOut - "
                          << "try reloading the current Rosegarden file"
                          << std::endl;
#else 
                ;
#endif
            }
        }

        // Cancel a playing audio file preview (this is predicated on Instrument
        // and 
        //
        if ((*i)->getType() == MappedEvent::AudioCancel)
        {
            cancelAudioFile(*i);
        }

        if ((*i)->getType() == MappedEvent::SystemMIDIClock)
        {
            if ((*i)->getData1())
            {
                m_midiClockEnabled = true;
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI CLOCK ENABLED"
                          << std::endl;
#endif
            }
            else
            {
                m_midiClockEnabled = false;
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI CLOCK DISABLED"
                          << std::endl;
#endif
            }
        }

#ifdef HAVE_LIBJACK

        // Set the JACK transport 
        if ((*i)->getType() == MappedEvent::SystemJackTransport)
        {
            _jackTransportMaster = false;
            _jackTransportEnabled = false;

            switch ((int)(*i)->getData1())
            {
                case 2:
                    _jackTransportMaster = true;
                    _jackTransportEnabled = true;
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is JACK Transport MASTER"
                              << std::endl;
#endif
                    break;

                case 1:
                    _jackTransportEnabled = true;
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is JACK Transport SLAVE"
                              << std::endl;
#endif
                    break;

                case 0:
                default:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden JACK Transport DISABLED"
                              << std::endl;
#endif
                    break;
            }
        }
#endif // HAVE_LIBJACK


        if ((*i)->getType() == MappedEvent::SystemMMCTransport)
        {
            m_mmcMaster = false;
            m_mmcEnabled = false;

            switch ((int)(*i)->getData1())
            {
                case 2:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MMC MASTER"
                              << std::endl;
#endif
                    m_mmcMaster = true;
                    m_mmcEnabled = true;
                    break;

                case 1:
                    m_mmcEnabled = true;
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MMC SLAVE"
                              << std::endl;
#endif
                    break;

                case 0:
                default:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden MMC Transport DISABLED"
                              << std::endl;
#endif
                    break;
            }
        }

        if ((*i)->getType() == MappedEvent::SystemRecordDevice)
        {
            DeviceId recordDevice =
               (DeviceId)((*i)->getData1());

            // Unset connections
            //
            unsetRecordDevices();

            // Special case to set for all record ports
            //
            if (recordDevice == Device::ALL_DEVICES)
            {
                /* set all record devices */
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "set all record devices - not implemented"
                          << std::endl;
#endif

                /*
		MappedDeviceList::iterator it = m_devices.begin();
                std::vector<int> ports;
                std::vector<int>::iterator pIt;

                for (; it != m_devices.end(); ++it)
                {
                    std::cout << "DEVICE = " << (*it)->getName() << " - DIR = "
                         << (*it)->getDirection() << endl;
                    // ignore ports we can't connect to
                    if ((*it)->getDirection() == MidiDevice::WriteOnly) continue;

                    std::cout << "PORTS = " << ports.size() << endl;
                    ports = (*it)->getPorts();
                    for (pIt = ports.begin(); pIt != ports.end(); ++pIt)
                    {
                        setRecordDevice((*it)->getClient(), *pIt);
                    }
                }
                */
            }
            else
            {
                // Otherwise just for the one device and port
                //
                setRecordDevice(recordDevice);
            }
        }

        if ((*i)->getType() == MappedEvent::SystemAudioInputs)
        {
#ifdef HAVE_LIBJACK
            createJackInputPorts((unsigned int)(*i)->getData1(), true);
#else

#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::processEventsOut - "
                      << "MappedEvent::SystemAudioInputs - no audio subsystem"
                      << std::endl;
#endif
#endif
        }
    }

    // Process Midi and Audio
    //
    processMidiOut(mC, playLatency, now);

#ifdef HAVE_LIBJACK
    processAudioQueue(playLatency, now);
#endif
}

bool
AlsaDriver::record(RecordStatus recordStatus)
{
    if (recordStatus == RECORD_MIDI)
    {
        // start recording
        m_recordStatus = RECORD_MIDI;
        m_alsaRecordStartTime = getAlsaTime();
    }
    else if (recordStatus == RECORD_AUDIO)
    {
#ifdef HAVE_LIBJACK
        if (createAudioFile(m_recordingFilename))
        {
            m_recordStatus = RECORD_AUDIO;
        }
        else
        {
            m_recordStatus = ASYNCHRONOUS_MIDI;
           return false;
        }

#else

#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::record - can't record audio without JACK"
                  << std::endl;
#endif
#endif

    }
    else
    if (recordStatus == ASYNCHRONOUS_MIDI)
    {
        m_recordStatus = ASYNCHRONOUS_MIDI;
    }
    else if (recordStatus == ASYNCHRONOUS_AUDIO)
    {
        m_recordStatus = ASYNCHRONOUS_AUDIO;
    }
#ifdef DEBUG_ALSA
    else
    {
        std::cerr << "ArtsDriver::record - unsupported recording mode"
                  << std::endl;
    }
#endif

    return true;
}

ClientPortPair
AlsaDriver::getFirstDestination(bool duplex)
{
    ClientPortPair destPair(-1, -1);
    AlsaPortList::iterator it;

    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); ++it)
    {
        destPair.first = (*it)->m_client;
        destPair.second = (*it)->m_port;

        // If duplex port is required then choose first one
        //
        if (duplex)
        {
            if ((*it)->m_direction == Duplex)
                return destPair;
        }
        else
        {
            // If duplex port isn't required then choose first
            // specifically non-duplex port (should be a synth)
            //
            if ((*it)->m_direction != Duplex)
                return destPair;
        }
    }

    return destPair;
}


// Sort through the ALSA client/port pairs for the range that
// matches the one we're querying.  If none matches then send
// back -1 for each.
//
ClientPortPair
AlsaDriver::getPairForMappedInstrument(InstrumentId id)
{
    MappedInstrument *instrument = getMappedInstrument(id);
    if (instrument)
    {
	DeviceId device = instrument->getDevice();
	DevicePortMap::iterator i = m_devicePortMap.find(device);
	if (i != m_devicePortMap.end())
	{
	    return i->second;
	}
    }
#ifdef DEBUG_ALSA
    /*
    else
    {
	cerr << "WARNING: AlsaDriver::getPairForMappedInstrument: couldn't find instrument for id " << id << ", falling through" << endl;
    }
    */
#endif

    return ClientPortPair(-1, -1);
}

// Send a direct controller to the specified port/client
//
void
AlsaDriver::sendDeviceController(const ClientPortPair &device,
                                 MidiByte controller,
                                 MidiByte value)
{
    snd_seq_event_t *event = new snd_seq_event_t();


    // These won't change in this slice
    //
    snd_seq_ev_clear(event);
    snd_seq_ev_set_source(event, m_port);

    snd_seq_ev_set_dest(event,
                        device.first,
                        device.second);

    for (int i = 0; i < 16; i++)
    {
        snd_seq_ev_set_controller(event,
                                  i,
                                  controller,
                                  value);
        snd_seq_event_output_direct(m_midiHandle, event);
    }

    snd_seq_drain_output(m_midiHandle);

    delete event;
}

// We only process note offs in this section
//
void
AlsaDriver::processPending(const RealTime &playLatency)
{
    if (!m_playing)
        processAudioQueue(playLatency, true);
    else
        processNotesOff(getAlsaTime() + playLatency);
}

void
AlsaDriver::insertMappedEventForReturn(MappedEvent *mE)
{
    // If we haven't inserted a MappedEvent yet this update period
    // then clear down the composition and flag
    //
    if (m_audioMeterSent == false)
    {
        m_recordComposition.clear();
        m_audioMeterSent = true;
    }

    // Insert the event ready for return at the next opportunity
    //
    m_recordComposition.insert(mE);
}

void
AlsaDriver::setPluginInstance(InstrumentId id,
                              unsigned long pluginId,
                              int position)
{
#ifdef HAVE_LADSPA

#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::setPluginInstance id = " << pluginId << std::endl;
#endif

    // first shut down any running instance
    removePluginInstance(id, position);

    // Get a descriptor - if this fails we can't initialise anyway
    const LADSPA_Descriptor *des = m_studio->createPluginDescriptor(pluginId);

    if (des)
    {
        // create and store
        LADSPAPluginInstance *instance =
            new LADSPAPluginInstance(id, pluginId, position, des);
        m_pluginInstances.push_back(instance);

        // activate and connect
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::setPluginInstance - "
                  << "activate and connect plugin" << std::endl;
#endif

        // create a new instance
        //
        instance->instantiate(getSampleRate());

#ifdef HAVE_LIBJACK

        // connect
        instance->connectPorts(_pluginBufferIn1,
                               _pluginBufferIn2,
                               _pluginBufferOut1,
                               _pluginBufferOut2);

#endif // HAVE_LIBJACK

        // Activate the plugin ready for run()ning
        //
        instance->activate();
    }

#ifdef DEBUG_ALSA
    else
        std::cerr << "AlsaDriver::setPluginInstance - "
                  << "can't initialise plugin descriptor" << std::endl;
#endif

#endif // HAVE_LADSPA

}


void
AlsaDriver::removePluginInstance(InstrumentId id, int position)
{
#ifdef HAVE_LADSPA

#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::removePluginInstance" << std::endl;
#endif

    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
    {
        if ((*it)->getInstrument() == id &&
            (*it)->getPosition() == position)
        {
            // Deactivate and cleanup
            //
            (*it)->deactivate();
            (*it)->cleanup();

            // Potentially unload the shared library in which the plugin
            // came from if none of its siblings are in use.
            //
            m_studio->unloadPlugin((*it)->getLADSPAId());

            delete *it;
            m_pluginInstances.erase(it);

            return;
        }
    }


#endif // HAVE_LADSPA
}

void
AlsaDriver::removePluginInstances()
{
#ifdef HAVE_LADSPA

#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::removePluginInstances" << std::endl;
#endif

    PluginIterator it = m_pluginInstances.begin();

    for (; it != m_pluginInstances.end(); ++it)
    {
        (*it)->deactivate();
        (*it)->cleanup();
        m_studio->unloadPlugin((*it)->getLADSPAId());

        delete (*it);
    }

    m_pluginInstances.clear();
#endif // HAVE_LADSPA
}

void
AlsaDriver::setPluginInstancePortValue(InstrumentId id,
                                       int position,
                                       unsigned long portNumber,
                                       float value)
{
#ifdef HAVE_LADSPA

    //std::cerr << "AlsaDriver::setPluginInstancePortValue" << std::endl;

    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
    {
        if ((*it)->getInstrument() == id &&
            (*it)->getPosition() == position)
        {
            (*it)->setPortValue(portNumber, value);
        }
    }
#endif // HAVE_LADSPA
} 



// Return the sample rate of the JACK driver if we have one installed
//
unsigned int
AlsaDriver::getSampleRate() const
{
#ifdef HAVE_LIBJACK
    return _jackSampleRate;
#else
   return 0;
#endif
}


#ifdef HAVE_LADSPA

PluginInstances&
AlsaDriver::getUnprocessedPlugins()
{
    m_retPluginList.clear();

    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
    {
        if (!(*it)->hasBeenProcessed() && !(*it)->isBypassed())
            m_retPluginList.push_back(*it);
    }
    return m_retPluginList;
}

// Return an ordered list of plugins for an instrument
//
PluginInstances&
AlsaDriver::getInstrumentPlugins(InstrumentId id)
{
    m_orderedPluginList.clear();

    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
    {
        if ((*it)->getInstrument() == id)
            m_orderedPluginList.insert(*it);
    }

    m_retPluginList.clear();
    for (OrderedPluginIterator oIt = m_orderedPluginList.begin(); oIt != m_orderedPluginList.end(); ++oIt)
        m_retPluginList.push_back(*oIt);

    return m_retPluginList;
}


void
AlsaDriver::setAllPluginsToUnprocessed()
{
    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
        (*it)->setProcessed(false);
}

// Turn all plugins on and off to reset their internal states
//
void
AlsaDriver::resetAllPlugins()
{
    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
    {
        (*it)->deactivate();
        (*it)->activate();
    }
}

#endif // HAVE_LADSPA



// ------------ JACK callbacks -----------
//
//

#ifdef HAVE_LIBJACK


// The "process" callback is where we do all the work of turning a sample
// file into a sound.  We de-interleave a WAV and send it out as needs be.
//
// We perform basic mixing at this level - adding the samples together
// using the temp buffers and then read out to the audio buffer at the
// end of the mix stage.  More details supplied within.
//
//
int
AlsaDriver::jackProcess(jack_nframes_t nframes, void *arg)
{
    AlsaDriver *inst = static_cast<AlsaDriver*>(arg);

    if (inst)
    {
        // Clear temporary buffers
        //
        for (unsigned int i = 0 ; i < nframes; ++i)
        {
            _tempOutBuffer1[i] = 0.0f;
            _tempOutBuffer2[i] = 0.0f;
        }

        // Get output buffers
        //
        sample_t *leftBuffer = static_cast<sample_t*>
            (jack_port_get_buffer(inst->getJackOutputPortLeft(),
                                  nframes));
        sample_t *rightBuffer = static_cast<sample_t*>
            (jack_port_get_buffer(inst->getJackOutputPortRight(),
                                  nframes));
        // Are we recording?
        //
        if (inst->getRecordStatus() == RECORD_AUDIO || inst->getRecordStatus() == ASYNCHRONOUS_AUDIO)
        {
            // Get the audio input port from the audio fader
            //
            MappedAudioFader *fader =
                dynamic_cast<MappedAudioFader*>
                    (inst->getMappedStudio()->
                         getAudioFader(inst->getAudioMonitoringInstrument()));

            int channels = 1;
            int connection = 0;

            if (fader)
            {
                channels = fader->getPropertyList(
                        MappedAudioFader::Channels)[0].toInt();

                connection = fader->getPropertyList(
                        MappedAudioObject::ConnectionsIn)[0].toInt();
            }
                
            // Get input buffer
            //
            sample_t *inputBufferLeft = 0, *inputBufferRight = 0;

            inputBufferLeft = static_cast<sample_t*>
              (jack_port_get_buffer(inst->
                 getJackInputPort(connection * channels), nframes));

            if (channels == 2)
            {
                inputBufferRight = static_cast<sample_t*>
                  (jack_port_get_buffer(inst->
                     getJackInputPort(connection * channels + 1), nframes));
            }

            // Turn buffer into a string
            //
            std::string buffer;
            unsigned char b1, b2;
            float inputLevelLeft = 0.0f, inputLevelRight = 0.0f;

            for (unsigned int i = 0; i < nframes; ++i)
            {
                b2 = (unsigned char)((long)
                        (inputBufferLeft[i] * _16bitSampleMax)& 0xff);
                b1 = (unsigned char)((long)
                        (inputBufferLeft[i] * _16bitSampleMax) >> 8);
                buffer += b2;
                buffer += b1;

                // We're monitoring levels here 
                //
                inputLevelLeft += fabs(inputBufferLeft[i]);

                if (inputBufferRight)
                {
                    b2 = (unsigned char)((long)
                            (inputBufferRight[i] * _16bitSampleMax)& 0xff);
                    b1 = (unsigned char)((long)
                            (inputBufferRight[i] * _16bitSampleMax) >> 8);
                    buffer += b2;
                    buffer += b1;

                    inputLevelRight += fabs(inputBufferRight[i]);
                }

                // always append to output stream
                _tempOutBuffer1[i] += inputBufferLeft[i];

                if (inputBufferRight)
                    _tempOutBuffer2[i] += inputBufferRight[i];
                else
                    _tempOutBuffer2[i] += inputBufferLeft[i];
            }

            inputLevelLeft /= float(nframes);
            if (channels == 2) inputLevelRight /= float(nframes);

            if (_passThroughCounter > reportPasses)
            {
                // Report the input level back to the GUI every "reportPasses"
                //
                _jackMappedEvent[_jackMappedEventCounter]
                    ->setInstrument((inst)->getAudioMonitoringInstrument());
                _jackMappedEvent[_jackMappedEventCounter]
                    ->setType(MappedEvent::AudioLevel);
                _jackMappedEvent[_jackMappedEventCounter]
                    ->setData1(int(inputLevelLeft * 127.0));
                _jackMappedEvent[_jackMappedEventCounter]
                    ->setData2(int(inputLevelRight * 127.0));
                inst->insertMappedEventForReturn(_jackMappedEvent[_jackMappedEventCounter]);

                _jackMappedEventCounter++;

                // return
                if (_jackMappedEventCounter >= _jackMappedEventsMax)
                    _jackMappedEventCounter = 0;
            }

            if (inst->getRecordStatus() == RECORD_AUDIO)
            {
               // append the sample string
               inst->appendToAudioFile(buffer);
            }

        }

        // Process any queue vector operations now - making sure the 
        // disk thread doesn't access the same vector.
        //
        pthread_mutex_lock(&_diskThreadLock);

        // Ok, we're playing - so clear the temporary buffers ready
        // for writing, get the audio queue, grab the queue vector
        // for usage and prepate a string for storage of the sample
        // slice.
        //
        std::vector<PlayableAudioFile*> audioQueue = inst->getAudioPlayQueueNotDefunct();
        std::vector<PlayableAudioFile*>::const_iterator it;

        // Store returned samples in this string
        //
        char *samplePtr = 0;

        // Define a huge number of counters and helpers
        //
        int samplesIn = 0;
        int oldSamplesIn = 0;
        double dSamplesIn = 0.0;
        double dInSamplesInc = 0.0;
        jack_nframes_t samplesOut = 0;

        float peakLevelLeft, peakLevelRight;


        //int cnt = 0;

        for (it = audioQueue.begin(); it != audioQueue.end(); ++it)
        {

            // Another (JACK) thread could've cleared down all the
            // PlayableAudioFiles already.  As we've already
            // noted we must be careful.
            //
            if ((*it)->getStatus() == PlayableAudioFile::PLAYING)
            {

                // Get a fader for this instrument - if we can't then this
                // isn't a valid audio track.
                //
                MappedAudioFader *fader =
                    dynamic_cast<MappedAudioFader*>
                        (inst->getMappedStudio()->
                             getAudioFader((*it)->getInstrument()));
    
                if (!fader) continue;
                
                // Do volume and pan processing now and use results after
                // plugins
                //
                MappedObjectPropertyList result =
                        fader->getPropertyList(MappedAudioFader::FaderLevel);
                float volume = float(result[0].toFloat())/100.0;


                // Don't do anything here if the volume is zero
                //
                if (volume == 0.0) continue;

                // Work out the pan
                //
                float pan1 = 1.0f;
                float pan2 = 1.0f;
                result = fader->getPropertyList(MappedAudioFader::Pan);
                float fpan = result[0].toFloat();
                if (fpan < 0.0)
                    pan2 = (fpan + 100.0) / 100.0;
                else
                    pan1 = 1.0 - (fpan / 100.0);

                int channels = (*it)->getChannels();
                int bytes = (*it)->getBitsPerSample() / 8;

                // How many frames to fetch will actually be
                // decided by sampling rate differences.  At 
                // any time we fetch:
                //
                //                 source (file) sample rate
                //     nframes *  ---------------------------
                //                  destination sample rate
                //
                // frames from the sample file.
                //
                jack_nframes_t fetchFrames = nframes;
                unsigned int sampleRate = (*it)->getSampleRate();

                if (sampleRate != _jackSampleRate)
                {
                    fetchFrames = (jack_nframes_t)((double)(nframes) *
                                                 (((double)(sampleRate))/
                                                  ((double)(_jackSampleRate))));
                }

                // Get some frames from the PlayableAudioFile - this
                // data is ringbuffered.  Any data that can't be fetched
                // due to end of file is automatically zeroed out - so
                // we don't need to worry about running off the end of
                // "real" data.
                //
                samplePtr = (*it)->getSampleFrames(fetchFrames);

                // JACK works on a normalised (-1.0 to +1.0) sample basis
                // so all samples read from the WAV file have to be divided
                // by their maximum possible value (0xff/2 for 8-bit and
                // 0xffff/2 for 16-bit samples and so on).
                //
                // Multi-channel WAV files have interleaved data bytes
                // at a given time frame. For a given sample frame there
                // will be n-samples to extract where n is the number of
                // channels.
                //
                // See docs/discussions/riff-wav-format.txt) for a more
                // detailed explanation of interleaving.
                //

                // Hmm, so many counters
                //
                samplesIn = 0;
                oldSamplesIn = 0;
                dSamplesIn = 0.0;
                dInSamplesInc = ((double)fetchFrames)/((double)nframes);
                samplesOut = 0;

                float outBytes;

                peakLevelLeft = 0.0f;
                peakLevelRight = 0.0f;

                while (samplesOut < nframes)
                {
                    switch(bytes)
                    {
                        case 1: // for 8-bit samples
                            outBytes =
                                ((short)(*(unsigned char *)samplePtr)) /
                                         _8bitSampleMax;

                            _pluginBufferIn1[samplesOut] = outBytes;

                            if (fabs(outBytes) > peakLevelLeft)
                                peakLevelLeft = fabs(outBytes);

                            if (channels == 2)
                            {
                                outBytes =
                                    ((short)
                                     (*((unsigned char *)samplePtr + 1))) /
                                          _8bitSampleMax;

                                if (fabs(outBytes) > peakLevelRight)
                                    peakLevelRight = fabs(outBytes);

                            }

                            _pluginBufferIn2[samplesOut] = outBytes;
                            break;

                        case 2: // for 16-bit samples
                            outBytes = (*((short*)(samplePtr))) /
                                           _16bitSampleMax;

                            _pluginBufferIn1[samplesOut] = outBytes;

                            if (fabs(outBytes) > peakLevelLeft)
                                peakLevelLeft = fabs(outBytes);

                            // Get other sample if we have one
                            //
                            if (channels == 2)
                            {
                                outBytes = (*((short*)(samplePtr + 2))) /
                                           _16bitSampleMax;

                                if (fabs(outBytes) > peakLevelRight)
                                    peakLevelRight = fabs(outBytes);

                            }
                            _pluginBufferIn2[samplesOut] = outBytes;
                            break;

                        case 3: // for 24-bit samples
                        default:
#ifdef DEBUG_ALSA
                            std::cerr << "jackProcess() - sample size "
                                      << "not supported" << std::endl;
#endif
                            break;
                    }
                    
                    // next out element
                    dSamplesIn += dInSamplesInc;
                    samplesIn = (int)dSamplesIn;
                    
                    // If we're advancing..
                    //
                    if (oldSamplesIn != samplesIn)
                    {
                        // ..increment pointer by correct amount of bytes
                        //
                        samplePtr += bytes * channels *
                                     (samplesIn - oldSamplesIn);

                    }

                    // store value for next time around
                    oldSamplesIn = samplesIn;

                    // Point to next output sample
                    samplesOut++;
                }

                // At this point check for plugins on this
                // instrument and step through them in the
                // order returned.
                //
#ifdef HAVE_LADSPA
                PluginInstances &list =
                    inst->getInstrumentPlugins((*it)->getInstrument());

                if (list.size())
                {
                    PluginIterator pIt = list.begin();
                    for (; pIt != list.end(); ++pIt)
                    {
                        // Bypass
                        //
                        if ((*pIt)->isBypassed())
                        {
                            for (unsigned int i = 0; i < nframes; ++i)
                            {
                                _pluginBufferOut1[i] = _pluginBufferIn1[i];
                                _pluginBufferOut2[i] = _pluginBufferIn2[i];
                            }
                        }
                        else
                        {
                            (*pIt)->run(_jackBufferSize);
                        }

                        // Copy across the out buffers if we're a channel
                        // short - ensures mono/stereo plugins works with
                        // streams of either nicely.
                        //
                        if ((*pIt)->getAudioChannelsOut() == 1)
                        {
                            for (unsigned int i = 0; i < nframes; ++i)
                                _pluginBufferOut2[i] = _pluginBufferOut1[i];
                        }

                        // If we've got another plugin to process then copy
                        // back the out buffers into the input buffers for
                        // reprocessing in the next pass.
                        // 
                        if (pIt != list.end())
                        {
                            for (unsigned int i = 0; i < nframes; ++i)
                            {
                                _pluginBufferIn1[i] = _pluginBufferOut1[i];
                                _pluginBufferIn2[i] = _pluginBufferOut2[i];
                            }
                        }
                    }

                    // Now mix the post-plugin signal into the out buffer
                    //
                    for (unsigned int i = 0; i < nframes; ++i)
                    {
                        _tempOutBuffer1[i] += 
                            _pluginBufferOut1[i] * volume * pan1;

                        _tempOutBuffer2[i] += 
                            _pluginBufferOut2[i] * volume * pan2;
                    }

                }
#else
                if (0) {}
#endif // HAVE_LADSPA
                else // straight through without plugins
                {
                    for (unsigned int i = 0; i < nframes; ++i)
                    {
                        _tempOutBuffer1[i] += _pluginBufferIn1[i] * volume * pan1;
                        _tempOutBuffer2[i] += _pluginBufferIn2[i] * volume * pan2;
                    }
                }

                if (_passThroughCounter > reportPasses)
                {
                    _jackMappedEvent[_jackMappedEventCounter]
                        ->setInstrument((*it)->getInstrument());
                    _jackMappedEvent[_jackMappedEventCounter]
                        ->setType(MappedEvent::AudioLevel);
                    //_jackMappedEvent[_jackMappedEventCounter]
                        //->setData1((*it)->getAudioFile()->getId());

                    _jackMappedEvent[_jackMappedEventCounter]->setData1(int(peakLevelLeft * 127.0));
                    _jackMappedEvent[_jackMappedEventCounter]->setData2(int(peakLevelRight * 127.0));

                    inst->insertMappedEventForReturn(
                            _jackMappedEvent[_jackMappedEventCounter]);

                    _jackMappedEventCounter++;

                    // return
                    if (_jackMappedEventCounter >= _jackMappedEventsMax)
                        _jackMappedEventCounter = 0;
                }
            }
        }

        // Playing plugins that have no current audio input but
        // nonetheless could still have audio output.  Also
        // process buffers in case we have recorded audio that
        // needs live attention.
        //

#ifdef HAVE_LADSPA
        if (inst->isPlaying())
        {

            PluginInstances &list = inst->getUnprocessedPlugins();
            if (list.size())
            {

                // Clear plugin input buffers to silence
                for (unsigned int i = 0; i < nframes; ++i)
                {
                    _pluginBufferIn1[i] = 0.0f;
                    _pluginBufferIn2[i] = 0.0f;
                }

                // Process plugins
                PluginIterator it = list.begin();
                for (; it != list.end(); ++it)
                {
                    // Run the plugin
                    //
                    (*it)->run(_jackBufferSize);

                    // Now mix the signal in from the plugin output
                    //
                    for (unsigned int i = 0; i < nframes; ++i)
                    {
                        _tempOutBuffer1[i] += _pluginBufferOut1[i]; // * volume;
                            
                        if ((*it)->getAudioChannelsOut() >= 2)
                        {
                            _tempOutBuffer2[i] += _pluginBufferOut2[i]; // * volume;
                        }
                        else
                        {
                            _tempOutBuffer2[i] += _pluginBufferOut1[i]; // * volume;
                        }
                    }
                }
            }
        }
        // Processing for recording or monitoring audio - process live plugins
        //
        else if(inst->getRecordStatus() == RECORD_AUDIO ||
                inst->getRecordStatus() == ASYNCHRONOUS_AUDIO)
        {
            PluginInstances &list = inst->getUnprocessedPlugins();
            if (list.size())
            {
                // initialise plugin input buffer with recorded samples
                //
                for (unsigned int i = 0; i < nframes; ++i)
                {
                    _pluginBufferIn1[i] = _tempOutBuffer1[i];
                    _pluginBufferIn2[i] = _tempOutBuffer2[i];
                }

                // Process plugins
                PluginIterator it = list.begin();
                for (; it != list.end(); ++it)
                {
                    // Run the plugin
                    //
                    (*it)->run(_jackBufferSize);

                    // Now mix the signal in from the plugin output
                    //
                    for (unsigned int i = 0; i < nframes; ++i)
                    {
                        _tempOutBuffer1[i] += _pluginBufferOut1[i]; // * volume;
                            
                        if ((*it)->getAudioChannelsOut() >= 2)
                        {
                            _tempOutBuffer2[i] += _pluginBufferOut2[i]; // * volume;
                        }
                        else
                        {
                            _tempOutBuffer2[i] += _pluginBufferOut1[i]; // * volume;
                        }
                    }

                    // reset input buffer
                    for (unsigned int i = 0; i < nframes; ++i)
                    {
                        _pluginBufferIn1[i] = _pluginBufferOut1[i];
                        _pluginBufferIn2[i] = _pluginBufferOut2[i];
                    }
                }
            }
        }


#endif // HAVE_LADSPA
        

        // Transfer the sum of the samples to the jack output buffers
        //
        for (unsigned int i = 0 ; i < nframes; ++i)
        {
            *(leftBuffer++) = _tempOutBuffer1[i];
            *(rightBuffer++) = _tempOutBuffer2[i];
        }

        // Push Playable onto main queue
        //
        inst->pushPlayableAudioQueue();

        // Clear any defunct files from the queue
        //
        inst->clearDefunctFromAudioPlayQueue();

        // Ok, now we can release the mutex
        //
        pthread_mutex_unlock(&_diskThreadLock);

#ifdef HAVE_LADSPA
        // Reset all plugins so they're processed next time
        //
        inst->setAllPluginsToUnprocessed();
#endif // HAVE_LADSPA

        _passThroughCounter++;

    }

    // Send the current transport status
    inst->sendJACKTransportState();

    return 0;
}

// Pick up any change of buffer size
//
int
AlsaDriver::jackBufferSize(jack_nframes_t nframes, void *)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::jackBufferSize - buffer size changed to "
              << nframes << std::endl;
#endif 

    _jackBufferSize = nframes;

    // Recreate our temporary mix buffers to the new size
    //
    delete [] _tempOutBuffer1;
    delete [] _tempOutBuffer2;
    _tempOutBuffer1 = new sample_t[_jackBufferSize];
    _tempOutBuffer2 = new sample_t[_jackBufferSize];

    delete [] _pluginBufferIn1;
    delete [] _pluginBufferIn2;
    _pluginBufferIn1 = new sample_t[_jackBufferSize];
    _pluginBufferIn2 = new sample_t[_jackBufferSize];

    delete [] _pluginBufferOut1;
    delete [] _pluginBufferOut2;
    _pluginBufferOut1 = new sample_t[_jackBufferSize];
    _pluginBufferOut2 = new sample_t[_jackBufferSize];


    return 0;
}

// Sample rate change
//
int
AlsaDriver::jackSampleRate(jack_nframes_t nframes, void *)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::jackSampleRate - sample rate changed to "
               << nframes << std::endl;
#endif
    _jackSampleRate = nframes;

    return 0;
}

void
AlsaDriver::jackShutdown(void * /*arg*/)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::jackShutdown() - callback received - doing nothing yet" << std::endl;
#endif

    /*
    AlsaDriver *inst = static_cast<AlsaDriver*>(arg);
    if (inst) inst->shutdown();
    delete inst;
    */
}

int
AlsaDriver::jackGraphOrder(void *)
{
    //std::cerr << "AlsaDriver::jackGraphOrder" << std::endl;
    return 0;
}

int
AlsaDriver::jackXRun(void *)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::jackXRun" << std::endl;
#endif
    return 0;
}


bool
AlsaDriver::createAudioFile(const std::string &fileName)
{
    // Already got a recording file - close it first to make
    // sure the data is written and internal totals computed.
    //
    if (_recordFile != 0)
        return false;


#ifdef DEBUG_ALSA
    std::cout << "AlsaDriver::createAudioFile - creating \"" 
         << fileName << "\"" << std::endl;
#endif

    MappedAudioFader *fader =
        dynamic_cast<MappedAudioFader*>
                    (m_studio->getAudioFader(getAudioMonitoringInstrument()));

    if (fader)
    {
        int channels = fader->
            getPropertyList(MappedAudioFader::Channels)[0].toInt();

        //std::cout << "GOT CHANNELS FROM FADER = " << channels << endl;

        // we use JACK_DEFAULT_AUDIO_TYPE for all ports currently so
        // we're recording 32 bit float MONO audio.
        //
        int bytesPerSample = 2 * channels;
        int bitsPerSample = 16;

        _recordFile =
            new WAVAudioFile(fileName,
                             channels,             // channels
                             _jackSampleRate,      // samples per second
                             _jackSampleRate *
                                  bytesPerSample,  // bytes per second
                             bytesPerSample,       // bytes per sample
                             bitsPerSample);       // bits per sample

        // open the file for writing
        //
        return (_recordFile->write());
    }

    return 0;
}


void
AlsaDriver::appendToAudioFile(const std::string &buffer)
{
    if (m_recordStatus != RECORD_AUDIO || _recordFile == 0)
        return;

    // write out
    _recordFile->appendSamples(buffer);
}

 
// Get a JACK frame from a RealTime
//
jack_nframes_t
AlsaDriver::getJACKFrame(const RealTime &time)
{
    jack_nframes_t frame =
        (jack_nframes_t)(((float(time.sec)) +
                           float(time.usec)/1000000.0) *
                           float(_jackSampleRate));
    return frame;
}

// Check out the current transport state and update the JACK
// server with our current position and playing state if we're
// the master timing source.
//
void
AlsaDriver::sendJACKTransportState()
{
    if (!_jackTransportMaster || !_jackTransportEnabled) return;

    // Only reset the position if we're playing
    if (m_playing == true)
    {
        m_transportPosition = getJACKFrame(getSequencerTime());
        jack_transport_locate(m_audioClient, m_transportPosition);
    }

    //info.position = m_transportPosition;

    // Get the transport position directly from the ALSA Sequencer 
    // - hopefully this is the right one allowing for all those
    // latencies etc.
    // 

        /*
    if (m_playing)
    {
    jack_transport_info_t info;

        if (m_looping)
        {
            //std::cout << "LOOPING (frame = " << info.position << ")" << endl;
            //info.state = JackTransportLooping;
            info.loop_start = getJACKFrame(m_loopStartTime);
            info.loop_end = getJACKFrame(m_loopEndTime);
            info.valid = jack_transport_bits_t(JackTransportPosition |
                                               JackTransportState |
                                               JackTransportLoop);
        }
        else
        {
            //std::cout << "PLAYING (frame = " << info.position << ")" << endl;
            //info.state = JackTransportRolling;
            ////info.valid = jack_transport_bits_t(JackTransportPosition |
                                               //JackTransportState);
        }
    else
    {
        //std::cout << "STOPPED (frame = " << info.position << ")" << endl;
        //info.state = JackTransportStopped;
        info.valid = jack_transport_bits_t(JackTransportPosition |
                                           JackTransportState);
    }

    jack_set_transport_info(m_audioClient, &info);
    */
}



#endif // HAVE_LIBJACK


// At some point make this check for just different numbers of clients
//
bool
AlsaDriver::checkForNewClients()
{
    AUDIT_START;
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int  client;
    unsigned int currentPortCount = 0,
                     oldPortCount = m_alsaPorts.size();

    unsigned int writeCap = SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE;
    unsigned int readCap = SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_READ;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    // Count current ports
    //
    while (snd_seq_query_next_client(m_midiHandle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        // Ignore ourselves and the system client
        //
        if (client == m_client || client == 0) continue;

        while (snd_seq_query_next_port(m_midiHandle, pinfo) >= 0)
        {
            if (((snd_seq_port_info_get_capability(pinfo) & writeCap)
                        == writeCap) ||
                ((snd_seq_port_info_get_capability(pinfo) & readCap)
                        == readCap))
                currentPortCount++;
        }
    }

    if (oldPortCount == currentPortCount) return false;

    AUDIT_STREAM << "AlsaDriver: number of ports changed ("
	      << currentPortCount << " now, " << oldPortCount << " before)"
	      << std::endl;
    
    AlsaPortList newPorts;
    generatePortList(&newPorts);

    // If any devices have connections that no longer exist,
    // clear those connections and stick them in the suspended
    // port map in case they come back online later.

    for (MappedDeviceList::iterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {
	
	ClientPortPair pair(m_devicePortMap[(*i)->getId()]);
	
	bool found = false;
	for (AlsaPortList::iterator j = m_alsaPorts.begin();
	     j != m_alsaPorts.end(); ++j) {
	    if ((*j)->m_client == pair.first &&
		(*j)->m_port == pair.second) {
		found = true;
		break;
	    }
	}
	
	if (!found) {
	    m_suspendedPortMap[pair] = (*i)->getId();
	    m_devicePortMap[(*i)->getId()] = ClientPortPair(-1, -1);
	    (*i)->setConnection("");
	}
    }
    
    // If we've increased the number of connections, we need
    // to assign the new connections to existing devices that
    // have none, where possible, and create new devices for
    // any left over.
    
    if (newPorts.size() > 0) {

	AUDIT_STREAM << "New ports:" << std::endl;

	for (AlsaPortList::iterator i = newPorts.begin();
	     i != newPorts.end(); ++i) {

	    AUDIT_STREAM << (*i)->m_name << std::endl;
	    std::string portName = (*i)->m_name;
	    ClientPortPair portPair = ClientPortPair((*i)->m_client,
						     (*i)->m_port);

	    if (m_suspendedPortMap.find(portPair) != m_suspendedPortMap.end()) {
		AUDIT_STREAM << "(Reusing suspended device)" << std::endl;

		Rosegarden::DeviceId id = m_suspendedPortMap[portPair];

		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getId() == id) (*j)->setConnection(portName);
		}

		m_suspendedPortMap.erase(m_suspendedPortMap.find(portPair));
		continue;
	    }
	    
	    bool needPlayDevice = true, needRecordDevice = true;

	    if ((*i)->isReadable()) {
		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getType() == Rosegarden::Device::Midi &&
			(*j)->getConnection() == "" &&
			(*j)->getDirection() == MidiDevice::Record) {
			AUDIT_STREAM << "(Reusing record device " << (*j)->getId()
				  << ")" << std::endl;
			m_devicePortMap[(*j)->getId()] = portPair;
			(*j)->setConnection(portName);
			needRecordDevice = false;
			break;
		    }
		}
	    } else {
		needRecordDevice = false;
	    }

	    if ((*i)->isWriteable()) {
		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getType() == Rosegarden::Device::Midi &&
			(*j)->getConnection() == "" &&
			(*j)->getDirection() == MidiDevice::Play) {
			AUDIT_STREAM << "(Reusing play device " << (*j)->getId()
				  << ")" << std::endl;
			m_devicePortMap[(*j)->getId()] = portPair;
			(*j)->setConnection(portName);
			needPlayDevice = false;
			break;
		    }
		}
	    } else {
		needPlayDevice = false;
	    }

	    if (needRecordDevice) {
		MappedDevice *device = createMidiDevice(*i, MidiDevice::Record);
		if (!device) {
#ifdef DEBUG_ALSA
		    std::cerr << "WARNING: Failed to create record device" << std::endl;
#else 
                    ;
#endif
		} else {
		    AUDIT_STREAM << "(Created new record device " << device->getId() << ")" << std::endl;
		    addInstrumentsForDevice(device);
		    m_devices.push_back(device);
		}
	    }

	    if (needPlayDevice) {
		MappedDevice *device = createMidiDevice(*i, MidiDevice::Play);
		if (!device) {
#ifdef DEBUG_ALSA
		    std::cerr << "WARNING: Failed to create play device" << std::endl;
#else
                    ;
#endif
		} else {
		    AUDIT_STREAM << "(Created new play device " << device->getId() << ")" << std::endl;
		    addInstrumentsForDevice(device);
		    m_devices.push_back(device);
		}
	    }
	}
    }
    
    MappedEvent *mE =
	new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
			0, 0);
    // send completion event
    insertMappedEventForReturn(mE);

    AUDIT_UPDATE;
    return true;
}

void
AlsaDriver::setPluginInstanceBypass(InstrumentId id,
                                    int position,
                                    bool value)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::setPluginInstanceBypass - "
              << value << std::endl;
#endif

#ifdef HAVE_LADSPA

    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
    {
        if ((*it)->getInstrument() == id &&
            (*it)->getPosition() == position)
        {
            (*it)->setBypassed(value);
        }
    }
#endif // HAVE_LADSPA
}


// From a DeviceId get a client/port pair for connecting as the
// MIDI record device.
//
void
AlsaDriver::setRecordDevice(DeviceId id)
{
    AUDIT_START;

    // Locate a suitable port
    //
    if (m_devicePortMap.find(id) == m_devicePortMap.end()) {
        AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
		     << "couldn't match device id (" << id << ") to ALSA port"
		     << std::endl;
        return;
    }

    ClientPortPair pair = m_devicePortMap[id];

    snd_seq_addr_t sender, dest;
    sender.client = pair.first;
    sender.port = pair.second;

    for (MappedDeviceList::iterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {
	if ((*i)->getId() == id) {
	    if ((*i)->getDirection() != MidiDevice::Record) {
		AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
			     << "attempting to set play device (" << id 
			     << ") to record device" << std::endl;
		return;
	    }
	    break;
	}
    }

    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca(&subs);

    dest.client = m_client;
    dest.port = m_port;

    // Set destinations and senders
    //
    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    snd_seq_port_subscribe_set_queue(subs, m_queue);

    // enable time-stamp-update mode 
    //
    snd_seq_port_subscribe_set_time_update(subs, 1);

    // set so we get realtime timestamps
    //
    snd_seq_port_subscribe_set_time_real(subs, 1);

    if (snd_seq_subscribe_port(m_midiHandle, subs) < 0)
    {
        AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
		     << "can't subscribe input client:port"
		     << int(sender.client) << ":" << int(sender.port)
		     << std::endl;

        // Not the end of the world if this fails but we
        // have to flag it internally.
        //
        m_midiInputPortConnected = false;
        AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
		     << "failed to subscribe device " 
		     << id << " as record port" << std::endl;
    }
    else
    {
        m_midiInputPortConnected = true;
        AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
		     << "successfully subscribed device "
		     << id << " as record port" << std::endl;
    }

}

// Clear any record device connections
//
void
AlsaDriver::unsetRecordDevices()
{
    snd_seq_addr_t dest;
    dest.client = m_client;
    dest.port = m_port;

    snd_seq_query_subscribe_t *qSubs;
    snd_seq_addr_t tmp_addr;
    snd_seq_query_subscribe_alloca(&qSubs);

    tmp_addr.client = m_client;
    tmp_addr.port = m_port;

    // Unsubsribe any existing connections
    //
    snd_seq_query_subscribe_set_type(qSubs, SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_index(qSubs, 0);
    snd_seq_query_subscribe_set_root(qSubs, &tmp_addr);

    while (snd_seq_query_port_subscribers(m_midiHandle, qSubs) >= 0)
    {
        tmp_addr = *snd_seq_query_subscribe_get_addr(qSubs);

        snd_seq_port_subscribe_t *dSubs;
        snd_seq_port_subscribe_alloca(&dSubs);

        snd_seq_addr_t dSender;
        dSender.client = tmp_addr.client;
        dSender.port = tmp_addr.port;

        snd_seq_port_subscribe_set_sender(dSubs, &dSender);
        snd_seq_port_subscribe_set_dest(dSubs, &dest);

        int error = snd_seq_unsubscribe_port(m_midiHandle, dSubs);

#ifdef DEBUG_ALSA
        if (error < 0)
        {
            std::cerr << "AlsaDriver::unsetRecordDevices - "
                      << "can't unsubscribe record port" << std::endl;

        }
#endif

        snd_seq_query_subscribe_set_index(qSubs,
                snd_seq_query_subscribe_get_index(qSubs) + 1);
    }
}

void
AlsaDriver::sendMMC(MidiByte deviceArg,
                    MidiByte instruction,
                    bool isCommand,
                    const std::string &data)
{
    MappedComposition mC;
    MappedEvent *mE;

    Rosegarden::DeviceId deviceId = Rosegarden::Device::NO_DEVICE;
    
    for (MappedInstrumentList::iterator i = m_instruments.begin();
	 i != m_instruments.end(); ++i) {

	if ((*i)->getDevice() == deviceId) continue;
	deviceId = (*i)->getDevice();

	if ((*i)->getType() != Rosegarden::Instrument::Midi) continue;

	// Create a plain SysEx
	//
	mE = new MappedEvent((*i)->getId(),
			     MappedEvent::MidiSystemExclusive);

	// Make it a RealTime SysEx
	mE->addDataByte(MIDI_SYSEX_RT);
	
	// Add the destination
	mE->addDataByte(deviceArg);
	
	// Add the command type
	if (isCommand)
	    mE->addDataByte(MIDI_SYSEX_RT_COMMAND);
	else
	    mE->addDataByte(MIDI_SYSEX_RT_RESPONSE);
	
	// Add the command
	mE->addDataByte(instruction);
	
	// Add any data
	mE->addDataString(data);
    
	mC.insert(mE);

#ifdef NOT_DEFINED
    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per writeable port
        //
        if ((*it)->m_port == 0 && (*it)->isWriteable())
        {
            try
            {
                // Create a plain SysEx
                //
		mE = new MappedEvent(0, //!!! should be iterating over devices?  (*it)->m_startId,
				     MappedEvent::MidiSystemExclusive);

		// Make it a RealTime SysEx
		mE->addDataByte(MIDI_SYSEX_RT);
		
		// Add the destination
		mE->addDataByte(deviceId);
		
		// Add the command type
		if (isCommand)
		    mE->addDataByte(MIDI_SYSEX_RT_COMMAND);
		else
		    mE->addDataByte(MIDI_SYSEX_RT_RESPONSE);
		
		// Add the command
		mE->addDataByte(instruction);
		
		// Add any data
		mE->addDataString(data);
            }
            catch(...)
            {
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::sendMMC - "
                          << "couldn't create MMC message" << std::endl;
#endif
                return;
            }

            mC.insert(mE);
        }
#endif
    }

    processMidiOut(mC, RealTime::zeroTime, true);
}

// Send a system real-time message
//
void
AlsaDriver::sendSystemDirect(MidiByte command, const std::string &args)
{
    snd_seq_addr_t sender, dest;
    sender.client = m_client;
    sender.port = m_port;

    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per writeable port
        //
        if ((*it)->m_port == 0 && (*it)->isWriteable())
        {
            snd_seq_event_t event;
            memset(&event, 0, sizeof(&event));

            // Set destination and sender
            dest.client = (*it)->m_client;
            dest.port = (*it)->m_port;
        
            event.dest = dest;
            event.source = sender;
            event.queue = SND_SEQ_QUEUE_DIRECT;

            // set the command
            event.type = command;

            // set args if we have them
            switch(args.length())
            {
                case 1:
                    event.data.control.value = args[0];
                    event.data.control.value = args[0];
                    break;

                case 2:
                    event.data.control.param = args[0];
                    event.data.control.value = args[0];
                    break;

                default: // do nothing
                    break;
            }

            int error = snd_seq_event_output_direct(m_midiHandle, &event);

#ifdef DEBUG_ALSA
            if (error < 0)
            {
                std::cerr << "AlsaDriver::sendSystemDirect - "
                          << "can't send event (" << int(command) << ")"
                          << std::endl;
            }
#endif
        }
    }

    snd_seq_drain_output(m_midiHandle);
}


void
AlsaDriver::sendSystemQueued(MidiByte command,
                             const std::string &args,
                             const RealTime &time)
{
    snd_seq_addr_t sender, dest;
    sender.client = m_client;
    sender.port = m_port;
    snd_seq_real_time_t sendTime = { time.sec, time.usec * 1000 };


    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per writeable port
        //
        if ((*it)->m_port == 0 && (*it)->isWriteable())
        {
            snd_seq_event_t event;
            memset(&event, 0, sizeof(&event));

            // Set destination and sender
            dest.client = (*it)->m_client;
            dest.port = (*it)->m_port;
        
            event.dest = dest;
            event.source = sender;

            // Schedule the command
            //
            event.type = command;

            // useful for debugging
            //snd_seq_ev_set_note(&event, 0, 64, 127, 100);

            snd_seq_ev_schedule_real(&event, m_queue, 0, &sendTime);

            // set args if we have them
            switch(args.length())
            {
                case 1:
                    event.data.control.value = args[0];
                    event.data.control.value = args[0];
                    break;

                case 2:
                    event.data.control.param = args[0];
                    event.data.control.value = args[0];
                    break;

                default: // do nothing
                    break;
            }

            int error = snd_seq_event_output(m_midiHandle, &event);

#ifdef DEBUG_ALSA
            if (error < 0)
            {
                std::cerr << "AlsaDriver::sendSystemQueued - "
                          << "can't send event (" << int(command) << ")"
                          << " - error = (" << error << ")"
                          << std::endl;
            }
#endif
        }
    }

    snd_seq_drain_output(m_midiHandle);
}

// Send the MIDI clock signal
//
void
AlsaDriver::sendMidiClock(const RealTime &playLatency)
{
    // Don't send the clock if it's disabled
    //
    if (!m_midiClockEnabled) return;

    // Get the number of ticks in (say) two seconds
    //
    unsigned int numTicks =
        (unsigned int)(RealTime(10, 0)/
                       RealTime(0, m_midiClockInterval));

    // First time through set the clock send time - this will also
    // ensure we send the first batch of clock events
    //
    if (m_midiClockSendTime == RealTime::zeroTime)
    {
        m_midiClockSendTime = getAlsaTime() + playLatency;
        /*
        std::cout << "INITIAL ALSA TIME = " << m_midiClockSendTime << endl;
        */
    }

    // If we're within a tenth of a second of running out of clock
    // then send a new batch of clock signals.
    //
    if ((getAlsaTime() + playLatency) >
        (m_midiClockSendTime - RealTime(0, 100000)))
    {
        /*
        std::cout << "SENDING " << numTicks
             << " CLOCK TICKS @ " << m_midiClockSendTime << endl;
             */

        for (unsigned int i = 0; i < numTicks; i++)
        {
            sendSystemQueued(SND_SEQ_EVENT_CLOCK, "", m_midiClockSendTime);

            // increment send time
            m_midiClockSendTime = m_midiClockSendTime +
                RealTime(0, m_midiClockInterval);
        }
    }

    // If we're playing then send the song position pointer.
    //
    if (m_playing)
    {
        // Get time from current alsa time to start of alsa timing -
        // add the initial starting point and divide by the total
        // single clock length.  Divide this result by 6 for the SPP
        // position.
        //
        long spp =
          long(((getAlsaTime() - m_alsaPlayStartTime + m_playStartPosition) /
                         RealTime(0, m_midiClockInterval)) / 6.0);

        // Only send if it's changed
        //
        if (m_midiSongPositionPointer != spp)
        {
            m_midiSongPositionPointer = spp;
            MidiByte lsb = spp & 0x7f;
            MidiByte msb = (spp << 8) & 0x7f;
            std::string args;
            args += lsb;
            args += msb;

            sendSystemDirect(SND_SEQ_EVENT_SONGPOS, args);
        }

    }


}

QString
AlsaDriver::getStatusLog()
{
    return QString::fromUtf8(_audit.c_str());
}


#ifdef HAVE_LIBJACK
// Disk thread for audio i/o
//
// This thread ensures that all i/o ringbuffers are full for reading
// or empty (as they can be) on writing.  Frees up the main process
// thread for processing audio from memory only.
//

void *
AlsaDriver::jackDiskThread(void *arg)
{
    AlsaDriver *inst = static_cast<AlsaDriver*>(arg);

//#define DEBUG_DISK_THREAD
#ifdef DEBUG_DISK_THREAD
    std::cerr << "AlsaDriver::jackDiskThread - starting" << std::endl;
#endif 

    // Get the AlsaDriver instance
    //
    if (inst)
    {
        pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

        std::vector<PlayableAudioFile*> audioQueue;
        std::vector<PlayableAudioFile*>::iterator it;

        while(_threadJackClosing == false)
        {
            // Try to lock this mutex but don't worry if it's busy
            //
            if(pthread_mutex_trylock(&_diskThreadLock) != EBUSY)
            {
                audioQueue = inst->getAudioPlayQueueNotDefunct();

                // Clear this down and refill every time
                //
                inst->clearPlayingAudioFiles();

                for (it = audioQueue.begin(); it != audioQueue.end(); ++it)
                {
                    if (!(*it)->isInitialised())
                        (*it)->initialise(); // start audio buffering
                    else
                        (*it)->fillRingBuffer(); // Let the audio file work out if the buffers
                                                 // need filling and if so by how much.

#ifdef FINE_DEBUG_DISK_THREAD
                    std::cerr << ", is now = " << (*it)->getRingBuffer()->readSpace() << std::endl;
#endif 
                    // Add this to the segment audio vector
                    //
                    inst->addPlayingAudioSegmentId((*it)->getRuntimeSegmentId());

                }

                pthread_mutex_unlock(&_diskThreadLock);
            }

            usleep(5000); // sleep for 5 milliseconds

#ifdef DEBUG_DISK_THREAD
            std::cerr << "AlsaDriver::jackDiskThread - continuing" 
                      << std::endl;
#endif

#ifdef DEBUG_DISK_THREAD

            std::cerr << "AlsaDriver::jackDiskThread - audio queue size "
                      << audioQueue.size() << std::endl;
#endif

        }
    }

#ifdef DEBUG_DISK_THREAD
    std::cerr << "AlsaDriver::jackDiskThread - completed" << std::endl;
#endif 

    return 0;
}


#endif // HAVE_LIBJACK

std::vector<int>
AlsaDriver::getPlayingAudioFiles()
{
#ifdef HAVE_LIBJACK
    if (pthread_mutex_trylock(&_diskThreadLock) != EBUSY)
    {
        std::vector<int> tempVector = m_playingAudioSegments;
        pthread_mutex_unlock(&_diskThreadLock);
        return tempVector;
    }

    return std::vector<int>();

#else
    return m_playingAudioSegments;
#endif
}


}



#endif // HAVE_ALSA
