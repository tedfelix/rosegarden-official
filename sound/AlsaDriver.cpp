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

#ifdef HAVE_LIBJACK
#include <jack/types.h>
#include <unistd.h> // for usleep
#include <cmath>
#endif

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

using std::cout;
using std::cerr;
using std::endl;


namespace Rosegarden
{

#ifdef HAVE_LIBJACK

static jack_nframes_t    _jackBufferSize;
static unsigned int      _jackSampleRate;
static bool              _usingAudioQueueVector;
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

static Rosegarden::MappedEvent _jackMappedEvent;

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
    m_currentPair(-1, -1),
    m_addedMetronome(false),
    m_audioMeterSent(false)


#ifdef HAVE_LIBJACK
    ,m_audioClient(0)
    ,m_transportPosition(0)
#endif

{
    std::cout << "Rosegarden AlsaDriver - " << m_name << std::endl;

#ifdef HAVE_LIBJACK
    _jackBufferSize = 0;
    _jackSampleRate = 0;
    _usingAudioQueueVector = false;
    _passThroughCounter = 0;

    _jackTransportEnabled = false;
    _jackTransportMaster = false;

    // Make sure we keep this MappedEvent hanging around - 
    // setting this property means that MappedComposition::clear()
    // won't attempt to delete it.
    //
    _jackMappedEvent.setPersistent(true);

#endif

    _threadAlsaClosing = false;
    _threadJackClosing = false;
}

AlsaDriver::~AlsaDriver()
{
    std::cout << "AlsaDriver::~AlsaDriver - shutting down" << std::endl;

    if (_threadAlsaClosing == false && m_midiHandle)
    {
        _threadAlsaClosing = true;
        snd_seq_stop_queue(m_midiHandle, m_queue, 0);
        snd_seq_close(m_midiHandle);
        m_midiHandle = 0;
    }

#ifdef HAVE_LADSPA
    m_studio->unloadAllPluginLibraries();
#endif // HAVE_LADSPA

#ifdef HAVE_LIBJACK

    if (_threadJackClosing == false && m_audioClient)
    {
        _threadJackClosing = true;
        std::cout << "AlsaDriver::~AlsaDriver - closing JACK client"
                  << std::endl;
        jack_port_unregister(m_audioClient, m_audioInputPortLeft);
        jack_port_unregister(m_audioClient, m_audioInputPortRight);
        jack_port_unregister(m_audioClient, m_audioOutputPortLeft);
        jack_port_unregister(m_audioClient, m_audioOutputPortRight);

        if (jack_deactivate(m_audioClient))
        {
            std::cerr << "AlsaDriver::~AlsaDriver - deactivation failed"
                      << std::endl;
        }

        jack_client_close(m_audioClient);
        m_audioClient = 0;
    }

#endif // HAVE_LIBJACK
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
        std::cerr << "System info error: " <<  snd_strerror(err)
                  << std::endl;
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

            std::cerr << "Client " << idx << " info error: "
                      << snd_strerror(err) << std::endl;
            exit(EXIT_FAILURE);
        }

        std::cout << "Queue " << snd_seq_queue_status_get_queue(status)
                  << std::endl;

        std::cout << "Tick       = "
                  << snd_seq_queue_status_get_tick_time(status)
                  << std::endl;

        std::cout << "Realtime   = "
                  << snd_seq_queue_status_get_real_time(status)->tv_sec
                  << "."
                  << snd_seq_queue_status_get_real_time(status)->tv_nsec
                  << std::endl;

        std::cout << "Flags      = 0x"
                  << snd_seq_queue_status_get_status(status)
                  << std::endl;
    }

}


void
AlsaDriver::generateInstruments()
{
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int  client;
    unsigned int writeCap = SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE;
    unsigned int readCap = SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_READ;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    // Reset these before each Device/Instrument hunt
    //
    m_deviceRunningId = 0;
    m_addedMetronome = false;
    m_audioRunningId = Rosegarden::AudioInstrumentBase;
    m_midiRunningId = Rosegarden::MidiInstrumentBase;
    m_currentPair  = std::pair<int, int>(-1, -1);

    // Clear these
    //
    m_instruments.clear();
    m_devices.clear();
    m_alsaPorts.clear();

    std::cout << std::endl << "  ALSA Client information:"
              << std::endl << std::endl;

    std::vector<AlsaPortDescription*> alsaPorts;

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
            if (((snd_seq_port_info_get_capability(pinfo) & writeCap)
                    == writeCap) ||
                ((snd_seq_port_info_get_capability(pinfo) & readCap)
                 == readCap))
            {
                std::cout << "    "
                          << snd_seq_port_info_get_client(pinfo) << ","
                          << snd_seq_port_info_get_port(pinfo) << " - ("
                          << snd_seq_client_info_get_name(cinfo) << ", "
                          << snd_seq_port_info_get_name(pinfo) << ")";

                PortDirection direction;

                if (snd_seq_port_info_get_capability(pinfo) &
                    SND_SEQ_PORT_CAP_DUPLEX)
                {
                    direction = Duplex;
                    std::cout << "\t\t\t(DUPLEX)";
                }
                else if (snd_seq_port_info_get_capability(pinfo) &
                         SND_SEQ_PORT_CAP_WRITE)
                {
                    direction = WriteOnly;
                    std::cout << "\t\t(WRITE ONLY)";
                }
                else
                {
                    direction = ReadOnly;
                    std::cout << "\t\t(READ ONLY)";
                }

                // Generate a unique name using the client id
                //
                char clientId[10];
                sprintf(clientId,
                        "%d ",
                        snd_seq_port_info_get_client(pinfo));

                std::string fullClientName = 
                    std::string(snd_seq_client_info_get_name(cinfo));

                std::string clientName =
                    std::string(clientId) + fullClientName;

                AlsaPortDescription *portDescription = 
                    new AlsaPortDescription(
                            Instrument::Midi,
                            clientName,
                            snd_seq_port_info_get_client(pinfo),
                            snd_seq_port_info_get_port(pinfo),
                            direction);

                alsaPorts.push_back(portDescription);

                std::cout << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Ok now sort by duplexicity
    //
    std::sort(alsaPorts.begin(), alsaPorts.end(), AlsaPortCmp());

    std::vector<AlsaPortDescription*>::iterator it = alsaPorts.begin();
    for (; it != alsaPorts.end(); it++)
    {
        /*
        cout << "installing device " << (*it)->m_name
             << " client = " << (*it)->m_client
             << " port = " << (*it)->m_port << endl;
             */

        addInstrumentsForPort((*it)->m_type,
                              (*it)->m_name,
                              (*it)->m_client,
                              (*it)->m_port,
                              (*it)->m_direction);
    }

#ifdef HAVE_LIBJACK

    // Create a number of audio Instruments - these are just
    // logical Instruments anyway and so we can create as 
    // many as we like and then use them as Tracks.
    //

    // New device
    //
    m_deviceRunningId++;

    MappedInstrument *instr;
    char number[100];
    std::string audioName;

    if (m_driverStatus & AUDIO_OK)
    {
        for (int channel = 0; channel < 16; ++channel)
        {
            sprintf(number, " #%d", channel + 1);
            audioName = "JACK Audio" + std::string(number);
            instr = new MappedInstrument(Instrument::Audio,
                                         channel,
                                         m_audioRunningId,
                                         0, // sub ordering is 0
                                         audioName,
                                         m_deviceRunningId);
            m_instruments.push_back(instr);

            // Create a fader with a matching id - this is the starting
            // point for all audio faders.
            //
            m_studio->createObject(Rosegarden::MappedObject::AudioFader,
                                   m_audioRunningId);

            /*
            std::cout  << "AlsaDriver::generateInstruments - "
                       << "added audio fader (id=" << m_audioRunningId
                       << ")" << std::endl;
                       */
    
            m_audioRunningId++;
        }

        // Create audio device
        //
        MappedDevice *device =
                        new MappedDevice(m_deviceRunningId,
                                         Rosegarden::Device::Audio,
                                         "JACK Audio");
        m_devices.push_back(device);
    }

#endif

}

// Create a local ALSA port for reference purposes
// and create a GUI Instrument for transmission upwards.
//
void
AlsaDriver::addInstrumentsForPort(Instrument::InstrumentType type,
                                  const std::string &name, 
                                  int client,
                                  int port,
                                  PortDirection direction)
{
    // only increment device number if we're on a new client
    //
    if (client != m_currentPair.first && m_currentPair.first != -1)
        m_deviceRunningId++;
 
    AlsaPort *alsaInstr;
    MappedInstrument *instr;
    std::string channelName;
    char number[100];


    if (type == Instrument::Midi)
    {

        // If we haven't added a metronome then add one to the first
        // instrument we add.   This is accomplished by adding a
        // MappedInstrument for Instrument #0
        //
        if(m_addedMetronome == false)
        {
            alsaInstr = new AlsaPort(0,
                                     0,
                                     name,
                                     client,
                                     port,
                                     direction);  // port direction

            m_alsaPorts.push_back(alsaInstr);

            for (int channel = 0; channel < 16; ++channel)
            {
                sprintf(number, " #%d", channel);
                channelName = "Metronome" + std::string(number);
                instr = new MappedInstrument(type,
                                             9, // always the drum channel
                                             channel,
                                             port,
                                             channelName,
                                             m_deviceRunningId);

                instr->setDirection(direction);

                m_instruments.push_back(instr);
            }
            m_addedMetronome = true;
        }

        // Create AlsaPort with the start and end MappedInstrument
        // indexes.
        //
        alsaInstr = new AlsaPort(m_midiRunningId,
                                 m_midiRunningId + 15,
                                 name,
                                 client,
                                 port,
                                 direction);

        m_alsaPorts.push_back(alsaInstr);

        for (int channel = 0; channel < 16; ++channel)
        {
            // Create MappedInstrument for export to GUI
            //
            sprintf(number, " #%d", channel + 1);
            channelName = name + std::string(number);

            if (channel == 9)
                channelName = name + std::string(" #10[D]");
            MappedInstrument *instr = new MappedInstrument(type,
                                                           channel,
                                                           m_midiRunningId++,
                                                           port,
                                                           channelName,
                                                           m_deviceRunningId);
            instr->setDirection(direction);

            m_instruments.push_back(instr);
        }

        MappedDevice *device =
                new MappedDevice(m_deviceRunningId,
                                 Rosegarden::Device::Midi,
                                 name);
        m_devices.push_back(device);
    }

    // Store these numbers for next time through
    m_currentPair.first = client;
    m_currentPair.second = port;
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
    int result;

    // Create a non-blocking handle.
    // ("hw" will possibly give in to other handles in future?)
    //
    if (snd_seq_open(&m_midiHandle,
                     "hw",                
                     SND_SEQ_OPEN_DUPLEX,
                     SND_SEQ_NONBLOCK) < 0)
    {
        std::cout << "AlsaDriver::initialiseMidi - "
                  << "couldn't open sequencer - " << snd_strerror(errno)
                  << std::endl;
        return;
    }

    generateInstruments();

    // Create a queue
    //
    if((m_queue = snd_seq_alloc_named_queue(m_midiHandle,
                                                "Rosegarden queue")) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi - can't allocate queue"
                  << std::endl;
        return;
    }


    // Create a client
    //
    snd_seq_set_client_name(m_midiHandle, "Rosegarden sequencer");
    if((m_client = snd_seq_client_id(m_midiHandle)) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi - can't create client"
                  << std::endl;
        return;
    }

    // Create a port
    //
    m_port = snd_seq_create_simple_port(m_midiHandle,
                                        NULL,
                                        SND_SEQ_PORT_CAP_WRITE |
                                        SND_SEQ_PORT_CAP_SUBS_WRITE |
                                        SND_SEQ_PORT_CAP_READ |
                                        SND_SEQ_PORT_CAP_SUBS_READ,
                                        SND_SEQ_PORT_TYPE_MIDI_GENERIC);
    if (m_port < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi - can't create port"
                  << std::endl;
        return;
    }

    ClientPortPair inputDevice = getFirstDestination(true); // duplex = true

    std::cout << "    Record client set to (" << inputDevice.first
              << ", "
              << inputDevice.second
              << ")" << std::endl << std::endl;

    std::vector<AlsaPort*>::iterator it;

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
        std::cerr << "AlsaDriver::initialiseMidi - "
                  << "can't subscribe input client/port"
                  << std::endl;
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
        std::cerr << "AlsaDriver::initialiseMidi - "
                  << "can't modify pool parameters"
                  << std::endl;
        return;
    }

    getSystemInfo();

    // Modify status with MIDI success
    //
    m_driverStatus |= MIDI_OK;

    // Start the timer
    if ((result = snd_seq_start_queue(m_midiHandle, m_queue, NULL)) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi - couldn't start queue - "
                  << snd_strerror(result)
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    // process anything pending
    snd_seq_drain_output(m_midiHandle);

    std::cout << "AlsaDriver::initialiseMidi -  initialised MIDI subsystem"
              << std::endl;
}

// We don't even attempt to use ALSA audio.  We just use JACK instead.
// See comment at the top of this file and jackProcess() for further
// information on how we use this.
//
void
AlsaDriver::initialiseAudio()
{
#ifdef HAVE_LIBJACK
    // Using JACK instead
    //

    std::string jackClientName = "rosegarden";

    // attempt connection to JACK server
    //
    if ((m_audioClient = jack_client_new(jackClientName.c_str())) == 0)
    {
        std::cerr << "AlsaDriver::initialiseAudio - "
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

    std::cout << "AlsaDriver::initialiseAudio - JACK sample rate = "
              << _jackSampleRate << std::endl;

    m_audioInputPortLeft =
        jack_port_register(m_audioClient,
                           "in_1",
                           JACK_DEFAULT_AUDIO_TYPE,
                           JackPortIsInput|JackPortIsTerminal,
                           0);
    m_audioInputPortRight =
        jack_port_register(m_audioClient,
                           "in_2",
                           JACK_DEFAULT_AUDIO_TYPE,
                           JackPortIsInput|JackPortIsTerminal,
                           0);

    m_audioOutputPortLeft = jack_port_register(m_audioClient,
                                               "out_1",
                                               JACK_DEFAULT_AUDIO_TYPE,
                                               JackPortIsOutput,
                                               0);

    m_audioOutputPortRight = jack_port_register(m_audioClient,
                                                "out_2",
                                                JACK_DEFAULT_AUDIO_TYPE,
                                                JackPortIsOutput,
                                                0);

    // set some latencies - these don't appear to do anything yet
    //
    /*
    jack_port_set_latency(m_audioOutputPortLeft, 1024);
    jack_port_set_latency(m_audioOutputPortRight, 1024);
    jack_port_set_latency(m_audioInputPort, 1024);
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

    // Activate the client
    //
    if (jack_activate(m_audioClient))
    {
        std::cerr << "AlsaDriver::initialiseAudio - "
                  << "cannot activate JACK client" << std::endl;
        return;
    }

    std::cout << "AlsaDriver::initialiseAudio - "
              << "initialised JACK audio subsystem"
              << std::endl;

    std::string playback_1, playback_2, capture_1, capture_2;

    // Assign port directly if they were specified
    //
    if (m_args.size() == 4)
    {
        playback_1 = std::string(m_args[0].data());
        playback_2 = std::string(m_args[1].data());
        capture_1 = std::string(m_args[2].data());
        capture_2 = std::string(m_args[3].data());
    }
    else // match from JACK
    {
        const char **ports =
            jack_get_ports(m_audioClient, NULL, NULL,
                           JackPortIsPhysical|JackPortIsInput);

        playback_1 = std::string(ports[0]);
        playback_2 = std::string(ports[1]);
        free(ports);

        ports = jack_get_ports(m_audioClient, NULL, NULL,
                               JackPortIsPhysical|JackPortIsOutput);
        capture_1 = std::string(ports[0]);
        capture_2 = std::string(ports[1]);
        free(ports);
    }


    // connect our client up to the ALSA ports - first left output
    //
    if (jack_connect(m_audioClient, jack_port_name(m_audioOutputPortLeft),
                     playback_1.c_str()))
    {
        std::cerr << "AlsaDriver::initialiseAudio - "
                  << "cannot connect to JACK output port" << std::endl;
        return;
    }

    if (jack_connect(m_audioClient, jack_port_name(m_audioOutputPortRight),
                     playback_2.c_str()))
    {
        std::cerr << "AlsaDriver::initialiseAudio - "
                  << "cannot connect to JACK output port" << std::endl;
        return;
    }

    // now input
    if (jack_connect(m_audioClient, capture_1.c_str(),
                     jack_port_name(m_audioInputPortLeft)))
    {
        std::cerr << "AlsaDriver::initialiseAudio - "
                  << "cannot connect to JACK input port" << std::endl;
    }

    if (jack_connect(m_audioClient, capture_2.c_str(),
                     jack_port_name(m_audioInputPortRight)))
    {
        std::cerr << "AlsaDriver::initialiseAudio - "
                  << "cannot connect to JACK input port" << std::endl;
    }

    // Get the latencies from JACK and set them as RealTime
    //
    jack_nframes_t outputLatency =
        jack_port_get_total_latency(m_audioClient, m_audioOutputPortLeft);

    jack_nframes_t inputLatency = 
        jack_port_get_total_latency(m_audioClient, m_audioInputPortLeft);

    double latency = double(outputLatency) / double(_jackSampleRate);

    // Set the audio latencies ready for collection by the GUI
    //
    m_audioPlayLatency = RealTime(int(latency), int(latency * 1000000.0));

    latency = double(inputLatency) / double(_jackSampleRate);

    m_audioRecordLatency = RealTime(int(latency), int(latency * 1000000.0));

    std::cout << "AlsaDriver::initialiseAudio - "
              << "JACK playback latency " << m_audioPlayLatency << std::endl;

    std::cout << "AlsaDriver::initialiseAudio - "
              << "JACK record latency " << m_audioRecordLatency << std::endl;

    // ok with audio driver
    //
    m_driverStatus |= AUDIO_OK;

#endif
}

void
AlsaDriver::initialisePlayback(const RealTime &position,
                               const RealTime &playLatency)
{
    std::cout << "AlsaDriver - initialisePlayback" << std::endl;
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
        cout << "START ADJUST FROM " << m_alsaPlayStartTime
             << " to " << alsaClockSent << endl;
             */

        m_alsaPlayStartTime = alsaClockSent;

        if (position == RealTime(0, 0))
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

}


void
AlsaDriver::stopPlayback()
{
    allNotesOff();
    m_playing = false;

    // reset the clock send time
    //
    m_midiClockSendTime = Rosegarden::RealTime(0, 0);

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
    std::vector<AlsaPort*>::iterator it;
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

    // Wait for the queue vector to become available so we can
    // clear it down.  We're careful with this sleep.
    //
    int sleepPeriod = 500; // microseconds
    int totalSleep = 0;

    while (_usingAudioQueueVector)
    {
        usleep(sleepPeriod);
        totalSleep += sleepPeriod;

        if (totalSleep > 1000000)
        {
            // Bound to be uncaught but at least we'll break out rather
            // than just hanging here.
            //
            throw(std::string("AlsaDriver::stopPlayback - slept too long waiting for audio queue vector to free"));
        }
    }

    clearAudioPlayQueue();
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
        if (modifyNoteOff <= RealTime(0, 0))
        {
            (*i)->setRealTime((*i)->getRealTime() + modifyNoteOff);
        }
        else // we're rewinding - kill the note immediately
        {
            (*i)->setRealTime(m_playStartPosition);
        }
    }
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
            std::cout << "AlsaDriver::allNotesOff - "
                      << "can't send event" << std::endl;
        }

        delete(*it);
    }
    
    m_noteOffQueue.erase(m_noteOffQueue.begin(), m_noteOffQueue.end());

    /*
    std::cout << "AlsaDriver::allNotesOff - "
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
      std::cout << "AlsaDriver::processNotesOff - "
      << " queue size = " << m_noteOffQueue.size() << std::endl;
    */
}

void
AlsaDriver::processAudioQueue(const RealTime &playLatency, bool now)
{
    std::vector<PlayableAudioFile*>::iterator it;
    RealTime currentTime = getSequencerTime() - playLatency;

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
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
    }

}

// Get the queue time and convert it to RealTime for the gui
// to use.
//
RealTime
AlsaDriver::getSequencerTime()
{
    if(m_playing)
       return getAlsaTime() + m_playStartPosition - m_alsaPlayStartTime;

    return RealTime(0, 0);
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
        std::cerr << "AlsaDriver::getSequencerTime - can't get queue status"
                  << std::endl;
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

    Rosegarden::RealTime eventTime(0, 0);

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

                    assert(duration >= RealTime(0, 0));

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
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiKeyPressure);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.value >> 7);
                    mE->setData2(event->data.control.value & 0x7f);
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
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiPitchBend);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.value >> 7);
                    mE->setData2(event->data.control.value & 0x7f);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CHANPRESS:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiChannelPressure);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.value >> 7);
                    mE->setData2(event->data.control.value & 0x7f);
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

                   if ((MidiByte)(data[0]) == MIDI_SYSEX_RT)
                   {
                       cout << "REALTIME SYSEX" << endl;
                   }

                   MappedEvent *mE = new MappedEvent();
                   mE->setType(MappedEvent::MidiSystemExclusive);
                   mE->setDataBlock(data);
                   m_recordComposition.insert(mE);
               }
               break;


            case SND_SEQ_EVENT_SENSING: // MIDI device is still there
               break;

            case SND_SEQ_EVENT_CLOCK:
               /*
               std::cout << "AlsaDriver::getMappedComposition - "
                         << "got realtime MIDI clock" << std::endl;
                         */
               break;

            case SND_SEQ_EVENT_START:
               std::cout << "AlsaDriver::getMappedComposition - "
                         << "START" << std::endl;
               break;

            case SND_SEQ_EVENT_CONTINUE:
               std::cout << "AlsaDriver::getMappedComposition - "
                         << "CONTINUE" << std::endl;
               break;

            case SND_SEQ_EVENT_STOP:
               std::cout << "AlsaDriver::getMappedComposition - "
                         << "STOP" << std::endl;
               break;

            case SND_SEQ_EVENT_SONGPOS:
               std::cout << "AlsaDriver::getMappedComposition - "
                         << "SONG POSITION" << std::endl;
               break;

               // these cases are handled by slotCheckForNewClients
               //
            case SND_SEQ_EVENT_PORT_SUBSCRIBED:
            case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
                   break;

            case SND_SEQ_EVENT_TICK:
            default:
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "got unhandled MIDI event type from ALSA sequencer"
                         << "(" << int(event->type) << ")" << std::endl;
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
    Rosegarden::RealTime midiRelativeTime;
    Rosegarden::RealTime midiRelativeStopTime;
    Rosegarden::MappedInstrument *instrument;
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

        snd_seq_ev_set_dest(event,
                            outputDevice.first,
                            outputDevice.second);

        /*
        cout << "INSTRUMENT = " << (*i)->getInstrument() << endl;

        cout << "TIME = " << time.tv_sec << " : " << time.tv_nsec * 1000
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
            std::cerr << "processMidiOut() - couldn't get Instrument for Event"
                      << std::endl;
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
                if ((*i)->getDuration() == Rosegarden::RealTime(-1, 0))
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
                    int value = ((*i)->getData1() << 7) |
                                ((*i)->getData2() & 0x7F);

                    // keep within -8192 to +8192
                    //
                    if (value & 0x4000)
                        value -= 0x8000;

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

                    data += (*i)->getDataBlock();

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
                std::cout << "AlsaDriver::processMidiOut - "
                          << "skipping unrecognised or invalid MappedEvent type"
                          << std::endl;
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

        if (error < 0)
        {
            std::cerr << "AlsaDriver::processMidiOut - "
                      << "failed to send ALSA event ("
                      << error << ")" <<  std::endl;
        }

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
                             const Rosegarden::RealTime &playLatency,
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
                Rosegarden::RealTime adjustedEventTime =
                    (*i)->getEventTime();

                /*
                    adjustedEventTime == RealTime(0, 0) &&
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
                        adjustedEventTime = RealTime(0, 0);
                }


                PlayableAudioFile *audioFile =
                    new PlayableAudioFile((*i)->getInstrument(),
                                          getAudioFile((*i)->getAudioID()),
                                          adjustedEventTime - playLatency,
                                          (*i)->getAudioStartMarker(),
                                          (*i)->getDuration());

                // If the start index is non-zero then we must scan to
                // the required starting position in the sample
                if (audioFile->getStartIndex() != RealTime(0, 0))
                {
                    // If there's a problem with the scan (most likely
                    // we've moved beyond the end of the sample) then
                    // cast it away.
                    //
                    if(audioFile->scanTo(audioFile->getStartIndex()) == false)
                    {
                        std::cerr << "AlsaDriver::processEventsOut - "
                                  << "skipping audio file" << std::endl;
                        delete audioFile;
                        continue;
                    }
                }

                queueAudio(audioFile);
            }
            else
            {
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "can't find audio file reference" 
                          << std::endl;

                std::cerr << "AlsaDriver::processEventsOut - "
                          << "try reloading the current Rosegarden file"
                          << std::endl;
            }
        }

        // Cancel a playing audio file
        //
        if ((*i)->getType() == MappedEvent::AudioCancel)
        {
            cancelAudioFile(Rosegarden::InstrumentId((*i)->getInstrument()),
                             Rosegarden::AudioFileId((*i)->getData1()));
        }

        if ((*i)->getType() == MappedEvent::SystemMIDIClock)
        {
            if ((*i)->getData1())
            {
                m_midiClockEnabled = true;
                std::cout << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI CLOCK ENABLED"
                          << std::endl;
            }
            else
            {
                m_midiClockEnabled = false;
                std::cout << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI CLOCK DISABLED"
                          << std::endl;
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
                    std::cout << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is JACK Transport MASTER"
                              << std::endl;
                    break;

                case 1:
                    _jackTransportEnabled = true;
                    std::cout << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is JACK Transport SLAVE"
                              << std::endl;
                    break;

                case 0:
                default:
                    std::cout << "AlsaDriver::processEventsOut - "
                              << "Rosegarden JACK Transport DISABLED"
                              << std::endl;
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
                    std::cout << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MMC MASTER"
                              << std::endl;
                    m_mmcMaster = true;
                    m_mmcEnabled = true;
                    break;

                case 1:
                    m_mmcEnabled = true;
                    std::cout << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MMC SLAVE"
                              << std::endl;
                    break;

                case 0:
                default:
                    std::cout << "AlsaDriver::processEventsOut - "
                              << "Rosegarden MMC Transport DISABLED"
                              << std::endl;
                    break;
            }
        }

        if ((*i)->getType() == MappedEvent::SystemRecordDevice)
        {
            Rosegarden::DeviceId recordDevice =
               (Rosegarden::DeviceId)((*i)->getData1());

            setRecordDevice(recordDevice);
        }
    }

    // Process Midi and Audio
    //
    processMidiOut(mC, playLatency, now);
    processAudioQueue(playLatency, now);

}


bool
AlsaDriver::record(const RecordStatus& recordStatus)
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
        std::cerr << "AlsaDriver::record - can't record audio without JACK"
                  << std::endl;
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
    else
    {
        std::cerr << "ArtsDriver::record - unsupported recording mode"
                  << std::endl;
    }

    return true;
}

ClientPortPair
AlsaDriver::getFirstDestination(bool duplex)
{
    ClientPortPair destPair(-1, -1);
    std::vector<AlsaPort*>::iterator it;

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
    ClientPortPair matchPair(-1, -1);

    std::vector<AlsaPort*>::iterator it;

    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); ++it)
    {
        if (id >= (*it)->m_startId && id <= (*it)->m_endId)
        {
            matchPair.first = (*it)->m_client;
            matchPair.second = (*it)->m_port;
            return matchPair;
        }
    }

    return matchPair;
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
    std::cout << "AlsaDriver::setPluginInstance id = " << pluginId << std::endl;

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
        std::cout << "AlsaDriver::setPluginInstance - "
                  << "activate and connect plugin" << std::endl;

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
    else
        std::cerr << "AlsaDriver::setPluginInstance - "
                  << "can't initialise plugin descriptor" << std::endl;

#endif // HAVE_LADSPA

}


void
AlsaDriver::removePluginInstance(InstrumentId id, int position)
{
#ifdef HAVE_LADSPA
    std::cout << "AlsaDriver::removePluginInstance" << std::endl;

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
            break;
        }
    }


#endif // HAVE_LADSPA
}

void
AlsaDriver::setPluginInstancePortValue(InstrumentId id,
                                       int position,
                                       unsigned long portNumber,
                                       float value)
{
#ifdef HAVE_LADSPA

    /*
    std::cout << "AlsaDriver::setPluginInstancePortValue" << std::endl;
    */

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

PluginInstances
AlsaDriver::getUnprocessedPlugins()
{
    PluginInstances list;
    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
    {
        if (!(*it)->hasBeenProcessed() && !(*it)->isBypassed())
            list.push_back(*it);
    }
    return list;
}

// Return an ordered list of plugins for an instrument
//
PluginInstances
AlsaDriver::getInstrumentPlugins(InstrumentId id)
{
    OrderedPluginList orderedList;
    PluginIterator it = m_pluginInstances.begin();
    for (; it != m_pluginInstances.end(); ++it)
    {
        if ((*it)->getInstrument() == id)
            orderedList.insert(*it);
    }

    PluginInstances list;
    OrderedPluginIterator oIt = orderedList.begin();
    for (; oIt != orderedList.end(); ++oIt)
        list.push_back(*oIt);

    return list;
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
        if (inst->getRecordStatus() == RECORD_AUDIO ||
            inst->getRecordStatus() == ASYNCHRONOUS_AUDIO)
        {
            // Get input buffer
            //
            sample_t *inputBufferLeft = static_cast<sample_t*>
                (jack_port_get_buffer(inst->getJackInputPortLeft(),
                                      nframes));

            /*
            sample_t *inputBufferRight = static_cast<sample_t*>
                (jack_port_get_buffer(inst->getJackInputPortRight(),
                                      nframes));
                                      */

            /*
            std::cout << "AlsaDriver::jackProcess - recording samples"
                      << std::endl;
                      */

            // Turn buffer into a string
            //
            std::string buffer;
            unsigned char b1, b2;
            float inputLevel = 0.0f;

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
                inputLevel += fabs(inputBufferLeft[i]);

            }

            inputLevel /= float(nframes);

            // Return an event to inform the GUI that AudioFileId has
            // now stopped playing.
            //
            _jackMappedEvent.setInstrument((inst)->
                    getAudioMonitoringInstrument());
            _jackMappedEvent.setType(MappedEvent::AudioLevel);
            _jackMappedEvent.setData1(0);
            _jackMappedEvent.setData2(int(inputLevel * 127.0));
            inst->insertMappedEventForReturn(&_jackMappedEvent);

            if (inst->getRecordStatus() == RECORD_AUDIO)
            {
               // append the sample string
               inst->appendToAudioFile(buffer);
            }
        }

        // Ok, we're playing - so clear the temporary buffers ready
        // for writing, get the audio queue, grab the queue vector
        // for usage and prepate a string for storage of the sample
        // slice.
        //

        // Clear temporary buffers
        //
        for (unsigned int i = 0 ; i < nframes; ++i)
        {
            _tempOutBuffer1[i] = 0.0f;
            _tempOutBuffer2[i] = 0.0f;
        }

        std::vector<PlayableAudioFile*> &audioQueue = inst->getAudioPlayQueue();
        std::vector<PlayableAudioFile*>::iterator it;

        // From here on in protect the audio queue vector from
        // outside intervention until we've finished with it.
        // Don't forget to relinquish it at the end of this block.
        //
        _usingAudioQueueVector = true;

        // Store returned samples in this string
        //
        std::string samples;

        // Define a huge number of counters and helpers
        //
        int samplesIn = 0;
        int oldSamplesIn = 0;
        double dSamplesIn = 0.0;
        double dInSamplesInc = 0.0;
        jack_nframes_t samplesOut = 0;

        std::vector<float> peakLevels;
        float peakLevel;

        for (it = audioQueue.begin(); it != audioQueue.end(); ++it)
        {
            // Another thread could've cleared down all the
            // PlayableAudioFiles already.  As we've already
            // noted we must be careful.
            //
            if ( /*inst->isPlaying() &&*/
                (*it)->getStatus() == PlayableAudioFile::PLAYING)
            {

                // make sure we haven't stopped playing because if we
                // have the iterator is invalid
                //

                /*
                if (!inst->isPlaying())
                    continue;
                    */

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

                    /*
                    std::cout << "RATE MISMATCH : will fetch "
                               << fetchFrames << " frames" << std::endl;
                               */
                }

                // Get the number of frames according to the number of
                // channels.
                //
                try
                {
                    samples = (*it)->getSampleFrames(fetchFrames);
                }
                catch(std::string es)
                {
                    // We've run out of samples in the PlayableAudioFile
                    // - throw it away
                    //
                    std::cerr << "jackProcess - EXCEPTION \"" << es << "\""
                              << std::endl;
                }

                // If we can't fetch everything then we must be at
                // the end of the AudioFile.
                //
                if (samples.length() !=
                    (fetchFrames * (*it)->getBytesPerSample()))
                {
                    (*it)->setStatus(PlayableAudioFile::DEFUNCT);

                    // Simple event to inform that AudioFileId has
                    // now stopped playing.
                    //
                    _jackMappedEvent.setInstrument((*it)->getInstrument());
                    _jackMappedEvent.setType(MappedEvent::AudioStopped);
                    _jackMappedEvent.setData1((*it)->getAudioFile()->getId());
                    _jackMappedEvent.setData2(0);
                    inst->insertMappedEventForReturn(&_jackMappedEvent);
                }


                // How do we convert the audio files into a format that
                // JACK can understand?
                // 
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
                // The Left and Rightness of these ports isn't in any
                // way correct I don't think.  Still a bit of a mystery
                // that bit.
                //
                // 13.05.2002 - on the fly sample rate conversions are
                //              now in place.  Clunky and non interpolated
                //              but it'll do for the moment until some of
                //              the other bugs are ironed out.
                //
                // 11.09.2002 - attempting to add plugin support will mean
                //              major restructuring of this code.  Overdue
                //              as it is anyway.
                //

                // Hmm, so many counters
                //
                samplesIn = 0;
                oldSamplesIn = 0;
                dSamplesIn = 0.0;
                dInSamplesInc = ((double)fetchFrames)/((double)nframes);
                samplesOut = 0;

                // Now point to the string
                char *samplePtr = (char *)(samples.c_str());
                char *origSamplePtr = samplePtr;
                float outBytes;

                peakLevel = 0.0f;

                // Get the volume of the audio fader and set a modifier
                // accordingly.
                //
                float volume = 1.0f;

                Rosegarden::MappedAudioFader *fader =
                    dynamic_cast<Rosegarden::MappedAudioFader*>
                        (inst->getMappedStudio()->
                             getAudioFader((*it)->getInstrument()));
                
                if (fader)
                {
                    MappedObjectPropertyList result =
                        fader->getPropertyList(MappedAudioFader::FaderLevel);
                    volume = float(result[0].toFloat())/127.0;
                }

                while (samplesOut < nframes)
                {
                    switch(bytes)
                    {
                        case 1: // for 8-bit samples
                            outBytes =
                                ((short)(*(unsigned char *)samplePtr)) /
                                         _8bitSampleMax;

                            _pluginBufferIn1[samplesOut] = outBytes * volume;
                            //_tempOutBuffer1[samplesOut] += outBytes * volume;

                            if (fabs(outBytes) > peakLevel)
                                peakLevel = fabs(outBytes);

                            if (channels == 2)
                            {
                                outBytes =
                                    ((short)
                                     (*((unsigned char *)samplePtr + 1))) /
                                          _8bitSampleMax;
                            }

                            _pluginBufferIn2[samplesOut] = outBytes * volume;
                            //_tempOutBuffer2[samplesOut] += outBytes * volume;
                            break;

                        case 2: // for 16-bit samples
                            outBytes = (*((short*)(samplePtr))) /
                                           _16bitSampleMax;

                            _pluginBufferIn1[samplesOut] = outBytes * volume;

                            //_tempOutBuffer1[samplesOut] += outBytes * volume;

                            if (fabs(outBytes) > peakLevel)
                                peakLevel = fabs(outBytes);

                            // Get other sample if we have one
                            //
                            if (channels == 2)
                            {
                                outBytes = (*((short*)(samplePtr + 2))) /
                                           _16bitSampleMax;
                            }
                            _pluginBufferIn2[samplesOut] = outBytes * volume;
                            //_tempOutBuffer2[samplesOut] += outBytes * volume;
                            break;

                        case 3: // for 24-bit samples
                        default:
                            std::cerr << "jackProcess() - sample size "
                                      << "not supported" << std::endl;
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

                    if ((*it)->getStatus() == PlayableAudioFile::DEFUNCT)
                    {
                        if (samplePtr - origSamplePtr > int(samples.length()))
                        {
                            /*
                            cout << "FINAL FRAME BREAKING - "
                                 << "calc = " << samplesOut * bytes * channels
                                 << " : samples = " << samples.length()
                                 << endl;
                                 */
                            break;
                        }
                        /*
                        else
                        {
                            cout << "FINAL FRAME OUT" << endl;
                        }
                        */
                    }
                }

                peakLevels.push_back(peakLevel);

                // Insert plugins go here
                //

                // At this point check for plugins on this
                // instrument and step through them in the
                // order returned.
                //
#ifdef HAVE_LADSPA
                PluginInstances list =
                    inst->getInstrumentPlugins((*it)->getInstrument());


                if (list.size())
                {
                    //cout << "GOT " << list.size() << " PLUGINS" << endl;
                    //int c = 0;

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
                            /*
                            cout << "RUN = " << c++ << " : "
                                 << "LASPA ID = " << (*pIt)->getLADSPAId()
                                 << " : POS = " << (*pIt)->getPosition() 
                                 << endl;
                                 */
                        }

                        // Now mix the signal
                        //
                        for (unsigned int i = 0; i < nframes; ++i)
                        {
                            _tempOutBuffer1[i] += _pluginBufferOut1[i];
                            
                            if ((*pIt)->getAudioChannelsOut() >= 2)
                            {
                                _tempOutBuffer2[i] += _pluginBufferOut2[i];
                            }
                            else
                            {
                                _tempOutBuffer2[i] += _pluginBufferOut1[i];
                            }
                        }

                        // If we've got more than one plugin then now copy
                        // back the out buffer into the input for reprocessing
                        // by the next plugin.
                        // 
                        if ((*pIt) != list.back())
                        {
                            for (unsigned int i = 0; i < nframes; ++i)
                            {
                                _pluginBufferIn1[i] = _pluginBufferOut1[i];
                                _pluginBufferIn2[i] = _pluginBufferOut2[i];
                            }
                        }
                    }
                }
#else
                if (0) {}
#endif // HAVE_LADSPA
                else // straight through without plugins
                {
                    for (unsigned int i = 0; i < nframes; ++i)
                    {
                        _tempOutBuffer1[i] += _pluginBufferIn1[i];
                        _tempOutBuffer2[i] += _pluginBufferIn2[i];
                    }
                }
            }
        }

        // Playing plugins that have no current audio input but
        // nonetheless could still have audio output.
        //
#ifdef HAVE_LADSPA
        if (inst->isPlaying())
        {

            PluginInstances list = inst->getUnprocessedPlugins();
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
                    float volume = 1.0f;
    
                    Rosegarden::MappedAudioFader *fader =
                        dynamic_cast<Rosegarden::MappedAudioFader*>
                            (inst->getMappedStudio()->
                                 getAudioFader((*it)->getInstrument()));
                
                    if (fader)
                    {
                        MappedObjectPropertyList result =
                            fader->getPropertyList(MappedAudioFader::FaderLevel);
                        volume = float(result[0].toFloat())/127.0;
                    }

                    /*
                    cout << "UNPROC INSTRUMENT = " << (*it)->getInstrument()
                         << " : POS = " << (*it)->getPosition()
                         << endl;
                         */

                    // Run the plugin
                    //
                    (*it)->run(_jackBufferSize);

                    // Now mix the signal in from the plugin output
                    //
                    for (unsigned int i = 0; i < nframes; ++i)
                    {
                        _tempOutBuffer1[i] += _pluginBufferOut1[i] * volume;
                            
                        if ((*it)->getAudioChannelsOut() >= 2)
                        {
                            _tempOutBuffer2[i] += _pluginBufferOut2[i] * volume;
                        }
                        else
                        {
                            _tempOutBuffer2[i] += _pluginBufferOut1[i] * volume;
                        }
                    }
                }
            }
        }

#endif // HAVE_LADSPA
        

        // Transfer the sum of the samples
        //
        for (unsigned int i = 0 ; i < nframes; ++i)
        {
            *(leftBuffer++) = _tempOutBuffer1[i];
            *(rightBuffer++) = _tempOutBuffer2[i];
        }

        // Process any queue vector operations now it's safe to do so
        //
        inst->pushPlayableAudioQueue();


        // Every time times through we report the current audio level
        // back to the gui.
        //
        if (audioQueue.size() > 0 && _passThroughCounter++ > 5)
        {
            int i = 0;
            for (it = audioQueue.begin(); it != audioQueue.end(); ++it)
            {
                if ((*it)->getStatus() == PlayableAudioFile::PLAYING)
                {
                    _jackMappedEvent.setInstrument((*it)->getInstrument());
                    _jackMappedEvent.setType(MappedEvent::AudioLevel);
                    _jackMappedEvent.setData1((*it)->getAudioFile()->getId());
                    _jackMappedEvent.setData2(int(peakLevels[i++] * 127.0));
                    inst->insertMappedEventForReturn(&_jackMappedEvent);
                }
            }

            _passThroughCounter = 0;
        }

        _usingAudioQueueVector = false;

#ifdef HAVE_LADSPA
        // Reset all plugins so they're processed next time
        //
        inst->setAllPluginsToUnprocessed();
#endif // HAVE_LADSPA

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
    std::cout << "AlsaDriver::jackBufferSize - buffer size changed to "
              << nframes << std::endl;
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
    std::cout << "AlsaDriver::jackSampleRate - sample rate changed to "
               << nframes << std::endl;
    _jackSampleRate = nframes;

    return 0;
}

void
AlsaDriver::jackShutdown(void * /*arg*/)
{
    std::cout << "AlsaDriver::jackShutdown() - received" << std::endl;

    // Old restart code can be abandoned
    //
    /*
    AlsaDriver *inst = static_cast<AlsaDriver*>(arg);
    if (inst)
    {
        // Shutdown audio references
        //
        inst->shutdownAudio();

        // And then restart Audio
        //
        std::cout << "AlsaDriver::jackShutdown() - received, " 
                  << "restarting..." << std::endl;

        inst->initialiseAudio();
    }
    */
}

void
AlsaDriver::shutdownAudio()
{
    std::cout << "AlsaDriver::shutdownAudio - JACK shutdown callback"
              << std::endl;
    /*
    if (m_audioClient)
    {
        std::cout << "AlsaDriver::shutdownAudio - "
                  << "shutting down JACK client" << std::endl;
        jack_deactivate(m_audioClient);
        m_audioClient = 0;

        // Hmm, this sometimes just hangs our subsequent JACK calls -
        // probably leave it out for the moment.
        //
        //jack_client_close(m_audioClient);
        //
    }
    */
}

int
AlsaDriver::jackGraphOrder(void *)
{
    //std::cout << "AlsaDriver::jackGraphOrder" << std::endl;
    return 0;
}

int
AlsaDriver::jackXRun(void *)
{
    std::cout << "AlsaDriver::jackXRun" << std::endl;
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


    cout << "AlsaDriver::createAudioFile - creating \"" 
         << fileName << "\"" << std::endl;

    // we use JACK_DEFAULT_AUDIO_TYPE for all ports currently so
    // we're recording 32 bit float MONO audio.
    //
    _recordFile =
        new Rosegarden::WAVAudioFile(fileName,
                                     1,                    // channels
                                     _jackSampleRate,      // bits per second
                                     _jackSampleRate/16,   // bytes per second
                                     2,                    // bytes per sample
                                     16);                  // bits per sample
    // open the file for writing
    //
    return (_recordFile->write());
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

    jack_transport_info_t info;

    // Only reset the position if we're playing
    if (m_playing == true)
    {
        m_transportPosition = getJACKFrame(getSequencerTime());
    }

    info.position = m_transportPosition;

    // Get the transport position directly from the ALSA Sequencer 
    // - hopefully this is the right one allowing for all those
    // latencies etc.
    // 
    //info.position = getJACKFrame(getSequencerTime());

    if (m_playing)
    {
        if (m_looping)
        {
            //cout << "LOOPING (frame = " << info.position << ")" << endl;
            info.state = JackTransportLooping;
            info.loop_start = getJACKFrame(m_loopStartTime);
            info.loop_end = getJACKFrame(m_loopEndTime);
            info.valid = jack_transport_bits_t(JackTransportPosition |
                                               JackTransportState |
                                               JackTransportLoop);
        }
        else
        {
            //cout << "PLAYING (frame = " << info.position << ")" << endl;
            info.state = JackTransportRolling;
            info.valid = jack_transport_bits_t(JackTransportPosition |
                                               JackTransportState);
        }
    }
    else
    {
        //cout << "STOPPED (frame = " << info.position << ")" << endl;
        info.state = JackTransportStopped;
        info.valid = jack_transport_bits_t(JackTransportPosition |
                                           JackTransportState);
    }

    jack_set_transport_info(m_audioClient, &info);
}



#endif // HAVE_LIBJACK


// At some point make this check for just different numbers of clients
//
bool
AlsaDriver::checkForNewClients()
{
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int  client;
    unsigned int currentClientCount = 0,
                 oldClientCount = 0;

    unsigned int writeCap = SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE;
    unsigned int readCap = SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_READ;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    std::vector<MappedDevice*>::iterator it = m_devices.begin();
    for (; it != m_devices.end(); ++it)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
            oldClientCount++;
    }

    // Count current clients
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
                currentClientCount++;
        }
    }

    // Have we got a different number of clients than we had before?
    //
    if (oldClientCount != currentClientCount)
    {
        generateInstruments();

        MappedEvent *mE =
            new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
                            0, 0);
        // send completion event
        insertMappedEventForReturn(mE);
        return true;
    }

    return false;
}

void
AlsaDriver::setPluginInstanceBypass(InstrumentId id,
                                    int position,
                                    bool value)
{
    std::cout << "AlsaDriver::setPluginInstanceBypass - "
              << value << std::endl;

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
AlsaDriver::setRecordDevice(Rosegarden::DeviceId id)
{
    Rosegarden::InstrumentId typicalId = 0;

    // Find a typical InstrumentId for this device
    //
    std::vector<MappedInstrument*>::iterator mIt = m_instruments.begin();
    for (; mIt != m_instruments.end(); ++mIt)
    {
        if ((*mIt)->getDevice() == id)
        {
            typicalId = (*mIt)->getId();
            break;
        }
    }

    // Locate a suitable port
    //
    snd_seq_addr_t sender, dest;
    dest.client = m_client;
    dest.port = m_port;

    std::vector<AlsaPort*>::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        sender.client = (*it)->m_client;
        sender.port = (*it)->m_port;

        if (typicalId >= (*it)->m_startId &&
            typicalId <= (*it)->m_endId) break;
    }

    if (it == m_alsaPorts.end())
    {
        std::cerr << "AlsaDriver::setRecordDevice - "
                  << "couldn't match device id (" << id << ") to ALSA port"
                  << std::endl;
        return;
    }

    if ((*it)->m_direction == WriteOnly)
    {
        std::cerr << "AlsaDriver::setRecordDevice - "
                  << "attempting to set non-duplex device (" << id 
                  << ") to record device" << std::endl;
        return;
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

        if (error < 0)
        {
            std::cerr << "AlsaDriver::setRecordDevice - "
                      << "can't unsubscribe record port" << std::endl;

        }

        snd_seq_query_subscribe_set_index(qSubs,
                snd_seq_query_subscribe_get_index(qSubs) + 1);
    }

    if (snd_seq_subscribe_port(m_midiHandle, subs) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi - "
                  << "can't subscribe input client/port"
                  << std::endl;
        // Not the end of the world if this fails but we
        // have to flag it internally.
        //
        m_midiInputPortConnected = false;
        std::cerr << "AlsaDriver::setRecordDevice - "
                  << "failed to subscribe device " 
                  << id << " as record port" << std::endl;
    }
    else
    {
        m_midiInputPortConnected = true;
        std::cerr << "AlsaDriver::setRecordDevice - "
                  << "successfully subscribed device "
                  << id << " as record port" << std::endl;
    }

}

// We send 
void
AlsaDriver::sendMMC(Rosegarden::MidiByte deviceId,
                    Rosegarden::MidiByte instruction,
                    bool isCommand,
                    const std::string &data)
{
    MappedComposition mC;
    MappedEvent *mE;

    std::vector<AlsaPort*>::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per duplex device
        //
        if ((*it)->m_port == 0 && (*it)->m_direction == Duplex)
        {
            try
            {
                // Create a plain SysEx
                //
                 mE = new MappedEvent((*it)->m_startId,
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
                std::cerr << "AlsaDriver::sendMMC - "
                          << "couldn't create MMC message" << std::endl;
                return;
            }

            mC.insert(mE);
        }
    }

    processMidiOut(mC, RealTime(0, 0), true);
}

// Send a system real-time message
//
void
AlsaDriver::sendSystemDirect(MidiByte command, const std::string &args)
{
    snd_seq_addr_t sender, dest;
    sender.client = m_client;
    sender.port = m_port;

    std::vector<AlsaPort*>::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per duplex device
        //
        if ((*it)->m_port == 0 && (*it)->m_direction == Duplex)
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

            if (error < 0)
            {
                std::cerr << "AlsaDriver::sendSystemDirect - "
                          << "can't send event (" << int(command) << ")"
                          << std::endl;
            }
        }
    }

    snd_seq_drain_output(m_midiHandle);
}


void
AlsaDriver::sendSystemQueued(Rosegarden::MidiByte command,
                             const std::string &args,
                             const Rosegarden::RealTime &time)
{
    snd_seq_addr_t sender, dest;
    sender.client = m_client;
    sender.port = m_port;
    snd_seq_real_time_t sendTime = { time.sec, time.usec * 1000 };


    std::vector<AlsaPort*>::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per duplex device
        //
        if ((*it)->m_port == 0 && (*it)->m_direction == Duplex)
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

            if (error < 0)
            {
                std::cerr << "AlsaDriver::sendSystemQueued - "
                          << "can't send event (" << int(command) << ")"
                          << " - error = (" << error << ")"
                          << std::endl;
            }
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
        (unsigned int)(Rosegarden::RealTime(10, 0)/
                       Rosegarden::RealTime(0, m_midiClockInterval));

    // First time through set the clock send time - this will also
    // ensure we send the first batch of clock events
    //
    if (m_midiClockSendTime == Rosegarden::RealTime(0, 0))
    {
        m_midiClockSendTime = getAlsaTime() + playLatency;
        /*
        cout << "INITIAL ALSA TIME = " << m_midiClockSendTime << endl;
        */
    }

    // If we're within a tenth of a second of running out of clock
    // then send a new batch of clock signals.
    //
    if ((getAlsaTime() + playLatency) >
        (m_midiClockSendTime - Rosegarden::RealTime(0, 100000)))
    {
        /*
        cout << "SENDING " << numTicks
             << " CLOCK TICKS @ " << m_midiClockSendTime << endl;
             */

        for (unsigned int i = 0; i < numTicks; i++)
        {
            sendSystemQueued(SND_SEQ_EVENT_CLOCK, "", m_midiClockSendTime);

            // increment send time
            m_midiClockSendTime = m_midiClockSendTime +
                Rosegarden::RealTime(0, m_midiClockInterval);
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
                         Rosegarden::RealTime(0, m_midiClockInterval)) / 6.0);

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

}


#endif // HAVE_ALSA
