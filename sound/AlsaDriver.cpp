// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.1
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

#ifdef HAVE_ALSA

// ALSA
#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>
#include <alsa/version.h>

#include "AlsaDriver.h"
#include "MappedInstrument.h"
#include "Midi.h"


namespace Rosegarden
{

AlsaDriver::AlsaDriver():
    SoundDriver(std::string("ALSA ") + std::string(SND_LIB_VERSION_STR)),
    m_client(-1),
    m_port(-1),
    m_queue(-1),
    m_maxClients(-1),
    m_maxPorts(-1),
    m_maxQueues(-1),
    m_midiInputPortConnected(false),
    m_alsaPlayStartTime(0, 0),
    m_alsaRecordStartTime(0, 0)

{
    std::cout << "Rosegarden AlsaDriver - " << m_name << std::endl;
}

AlsaDriver::~AlsaDriver()
{
    snd_seq_close(m_midiHandle);
}

void
AlsaDriver::getSystemInfo()
{
    int err;
    snd_seq_system_info_t *sysinfo;

    snd_seq_system_info_alloca(&sysinfo);

    if ((err = snd_seq_system_info(m_midiHandle, sysinfo))<0)
    {
        std::cerr << "System info error: " <<  snd_strerror(err)
                  << std::endl;
        exit(1);
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

    for (idx = min; idx < max; idx++)
    {
        if ((err = snd_seq_get_queue_status(m_midiHandle, idx, status))<0)
        {
            if (err == -ENOENT)
                continue;

            std::cerr << "Client " << idx << " info error: "
                      << snd_strerror(err) << std::endl;
            exit(0);
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
    unsigned int cap;


    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    m_instruments.clear();
    m_alsaPorts.clear();

    std::cout << std::endl << "  ALSA Client information:"
              << std::endl << std::endl;

    bool duplex = false;

    int synthCount = 0;

    // Use these to store ONE input (duplex) port
    // which we push onto the Instrument list last
    //
    std::string inputName;
    int inputClient = -1;
    int inputPort = -1;

    // Get only the client ports we're interested in 
    //
    while (snd_seq_query_next_client(m_midiHandle, cinfo) >= 0
            && synthCount < 2)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(m_midiHandle, pinfo) >= 0
                && synthCount < 2)
        {
            cap = (SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE);

            if ((snd_seq_port_info_get_capability(pinfo) & cap) == cap)
            {
                std::cout << "    "
                          << snd_seq_port_info_get_client(pinfo) << ","
                          << snd_seq_port_info_get_port(pinfo) << " - ("
                          << snd_seq_client_info_get_name(cinfo) << ", "
                          << snd_seq_port_info_get_name(pinfo) << ")";
                          /*
                          << snd_seq_port_info_get_midi_channels(pinfo)
                          << " : MIDI VOICES = "
                          << snd_seq_port_info_get_midi_voices(pinfo)
                          << " : SYNTH VOICES = "
                          << snd_seq_port_info_get_synth_voices(pinfo)
                          */
                if ((snd_seq_port_info_get_capability(pinfo) &
                     SND_SEQ_PORT_TYPE_SYNTH) == SND_SEQ_PORT_TYPE_SYNTH)
                    std::cout << " (SYNTH)";

                if (snd_seq_port_info_get_capability(pinfo) &
                    SND_SEQ_PORT_CAP_DUPLEX)
                {
                    std::cout << "\t\t(DUPLEX)" << std::endl;
                    inputName = std::string(snd_seq_port_info_get_name(pinfo));
                    inputClient = snd_seq_port_info_get_client(pinfo);
                    inputPort = snd_seq_port_info_get_port(pinfo);
                    continue;
                }
                else
                {
                    std::cout << "\t\t(WRITE ONLY)";
                }

                // For the moment limit to two strictly synth devices
                //
                addInstrumentsForPort(
                            Instrument::Midi,
                            std::string(snd_seq_port_info_get_name(pinfo)),
                            snd_seq_port_info_get_client(pinfo),
                            snd_seq_port_info_get_port(pinfo),
                            false);

                synthCount++;

                std::cout << std::endl;
            }
        }
    }

    if (inputPort != -1 && inputClient != -1)
    {
        addInstrumentsForPort(
                    Instrument::Midi,
                    inputName,
                    inputClient,
                    inputPort,
                    true);
    }

    std::cout << std::endl;

}

// Create a local ALSA port for reference purposes
// and create a GUI Instrument for transmission upwards.
//
void
AlsaDriver::addInstrumentsForPort(Instrument::InstrumentType type,
                                  const std::string &name, 
                                  int client,
                                  int port,
                                  bool duplex)
{
 
    if (type == Instrument::Midi)
    {
        // Create AlsaPort with the start and end MappedInstrumnet
        //
        AlsaPort *alsaInstr = new AlsaPort(m_midiRunningId,
                                           m_midiRunningId + 15,
                                           name,
                                           client,
                                           port,
                                           duplex);  // a duplex port?

        m_alsaPorts.push_back(alsaInstr);


        std::string channelName;
        char number[100];

        for (int channel = 0; channel < 16; channel++)
        {
            // Create MappedInstrument for export to GUI
            //
            sprintf(number, ", Channel %d", channel);
            channelName = name + std::string(number);

            if (channel == 9)
                channelName = name + std::string(", Drums (9)");
            MappedInstrument *instr = new MappedInstrument(type,
                                                           channel,
                                                           m_midiRunningId++,
                                                           channelName);
            m_instruments.push_back(instr);
        }
    }
    else  // audio
    {
        m_audioRunningId++;
    }
}


// Set up queue, client and port
//
void
AlsaDriver::initialiseMidi()
{ 
    // Create a non-blocking handle.
    // ("hw" will possibly give in to other handles in future?)
    //
    if (snd_seq_open(&m_midiHandle,
                     "hw",                
                     SND_SEQ_OPEN_DUPLEX,
                     SND_SEQ_NONBLOCK) < 0)
    {
        std::cout << "AlsaDriver::generateInstruments() - "
                  << "couldn't open sequencer - " << snd_strerror(errno)
                  << std::endl;
        m_driverStatus = NO_DRIVER;
        return;
    }

    // Now we have a handle generate some possible destinations
    // through the Instrument metaphor
    //
    generateInstruments();


    // Create a queue
    //
    if((m_queue = snd_seq_alloc_named_queue(m_midiHandle,
                                                "Rosegarden queue")) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - can't allocate queue"
                  << std::endl;
        m_driverStatus = NO_DRIVER;
        return;
    }


    // Create a client
    //
    snd_seq_set_client_name(m_midiHandle, "Rosegarden sequencer");
    if((m_client = snd_seq_client_id(m_midiHandle)) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - can't create client"
                  << std::endl;
        m_driverStatus = NO_DRIVER;
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
        std::cerr << "AlsaDriver::initialiseMidi() - can't create port"
                  << std::endl;
        m_driverStatus = NO_DRIVER;
        return;
    }

    ClientPortPair inputDevice = getFirstDestination(true); // duplex = true

    cout << "    INPUT PAIR  = " << inputDevice.first << ", "
                                << inputDevice.second << endl;

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
            std::cerr << "AlsaDriver::initialiseMidi() - "
                      << "can't subscribe output client/port"
                      << std::endl;
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

    // set so we realtime timestamps
    //
    snd_seq_port_subscribe_set_time_real(subs, 1);

    if (snd_seq_subscribe_port(m_midiHandle, subs) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - "
                  << "can't subscribe input client/port"
                  << std::endl;
        // Not the end of the world if this fails but we
        // have to flag it internally.
        //
        m_midiInputPortConnected = false;
    }
    else
        m_midiInputPortConnected = true;

    // Erm?
    //
    if (snd_seq_set_client_pool_output(m_midiHandle, 200) < 0 ||
        snd_seq_set_client_pool_input(m_midiHandle, 20) < 0 ||
        snd_seq_set_client_pool_output_room(m_midiHandle, 20) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - "
                  << "can't modify pool parameters"
                  << std::endl;
        m_driverStatus = NO_DRIVER;
        return;
    }

    getSystemInfo();

    // Modify status with success
    //
    if (m_driverStatus == AUDIO_OK)
        m_driverStatus = MIDI_AND_AUDIO_OK;
    else if (m_driverStatus == NO_DRIVER)
        m_driverStatus = MIDI_OK;

    std::cout << "AlsaDriver - initialised MIDI subsystem" << std::endl;
}

// Thanks to Matthias Nagorni's ALSA 0.9.0 HOWTO:
//
//    http://www.suse.de/~mana/alsa090_howto.html
//
//
void
AlsaDriver::initialiseAudio()
{
    m_audioStream = SND_PCM_STREAM_PLAYBACK;

    // Contains information about hardware
    //
    snd_pcm_hw_params_t *hwParams;
    snd_pcm_hw_params_alloca(&hwParams);

    // Name of the PCM device, like plughw:0,0 
    // The first number is the number of the soundcard,
    // the second number is the number of the device.
    //
    char *pcmName;
    pcmName = strdup("plughw:0,0");

    if (snd_pcm_open(&m_audioHandle, pcmName, m_audioStream, 0) < 0)
    {
        std::cerr << "AlsaDriver::initialiseAudio - can't open PCM device"
                  << std::endl;

        return;
    }

    if (snd_pcm_hw_params_any(m_audioHandle, hwParams) < 0)
    {
        std::cerr << "AlsaDriver::initialiseAudio - "
                  << "can't configure this audio device" << std::endl;

        // Adjust status
        //
        if (m_driverStatus == MIDI_AND_AUDIO_OK)
            m_driverStatus = MIDI_OK;
        else
            m_driverStatus = NO_DRIVER;

        return;
    }

    int rate = 44100; /* Sample rate */
    int periods = 2;     /* Number of periods */
    int periodsize = 8192; /* Periodsize (bytes) */


    // Adjust driver status according to our success here
    //
    if (m_driverStatus == MIDI_OK)
        m_driverStatus = MIDI_AND_AUDIO_OK;
    else if (m_driverStatus == NO_DRIVER)
        m_driverStatus = AUDIO_OK;

    std::cout << "AlsaDriver - initialised Audio (PCM) subsystem" << std::endl;
}

void
AlsaDriver::initialisePlayback(const RealTime &position)
{
    std::cout << "AlsaDriver - initialisePlayback" << std::endl;
    int result;

    m_alsaPlayStartTime = RealTime(0, 0);
    m_playStartPosition = position;
    m_startPlayback = true;

    // Start the timer
    if ((result = snd_seq_start_queue(m_midiHandle, m_queue, NULL)) < 0)
    {
        std::cerr << "AlsaDriver::initialisePlayback - couldn't start queue - "
                  << snd_strerror(result)
                  << std::endl;
        exit(1);
    }

    snd_seq_drain_output(m_midiHandle);
}


void
AlsaDriver::stopPlayback()
{
    allNotesOff();
    snd_seq_drain_output(m_midiHandle);
    snd_seq_stop_queue(m_midiHandle, m_queue, 0);
    m_playing = false;

    // send sounds-off to all client port pairs
    //
    std::vector<AlsaPort*>::iterator it;
    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); it++)
    {
        sendDeviceController(ClientPortPair((*it)->m_client,
                                            (*it)->m_port),
                             MIDI_CONTROLLER_SOUNDS_OFF,
                             0);
    }

}



void
AlsaDriver::resetPlayback(const RealTime &position, const RealTime &latency)
{
    allNotesOff();
    m_playStartPosition = position;
    m_alsaPlayStartTime = getAlsaTime() - latency;
}

void
AlsaDriver::allNotesOff()
{
    snd_seq_event_t *event = new snd_seq_event_t();
    Rosegarden::RealTime now = getAlsaTime();
    ClientPortPair outputDevice;

    // prepare the event
    snd_seq_ev_clear(event);
    snd_seq_ev_set_source(event, m_port);

    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
                                i != m_noteOffQueue.end(); ++i)
    {
        // Set destination according to instrument mapping to port
        //
        outputDevice = getPairForMappedInstrument((*i)->getInstrument());

        snd_seq_ev_set_dest(event,
                            outputDevice.first,
                            outputDevice.second);

        snd_seq_real_time_t time = { now.sec,
                                     now.usec * 1000 };

        snd_seq_ev_schedule_real(event, m_queue, 0, &time);
        snd_seq_ev_set_noteoff(event,
                               (*i)->getChannel(),
                               (*i)->getPitch(),
                               127);
        // send note off directly
        snd_seq_event_output_direct(m_midiHandle, event);

        delete(*i);
        m_noteOffQueue.erase(i);
    }

    // drop - does this work?
    //
    snd_seq_drop_output(m_midiHandle);
    snd_seq_drop_output_buffer(m_midiHandle);

    // and flush them
    snd_seq_drain_output(m_midiHandle);
}

void
AlsaDriver::processNotesOff(const RealTime &time)
{
    snd_seq_event_t *event = new snd_seq_event_t();

    ClientPortPair outputDevice;

    // prepare the event
    snd_seq_ev_clear(event);
    snd_seq_ev_set_source(event, m_port);

    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
                      i != m_noteOffQueue.end(); ++i)
    {
        if ((*i)->getRealTime() <= time)
        {
            // Set destination according to instrument mapping to port
            //
            outputDevice = getPairForMappedInstrument((*i)->getInstrument());

            snd_seq_ev_set_dest(event,
                                outputDevice.first,
                                outputDevice.second);

            snd_seq_real_time_t time = { (*i)->getRealTime().sec,
                                         (*i)->getRealTime().usec * 1000 };

            snd_seq_ev_schedule_real(event, m_queue, 0, &time);
            snd_seq_ev_set_noteoff(event,
                                   (*i)->getChannel(),
                                   (*i)->getPitch(),
                                   127);
            // send note off
            snd_seq_event_output(m_midiHandle, event);

            delete(*i);
            m_noteOffQueue.erase(i);
        }
    }

    // and flush them
    snd_seq_drain_output(m_midiHandle);

}

void
AlsaDriver::processAudioQueue()
{
}

// Get the queue time and convert it to RealTime for the gui
// to use.
//
RealTime
AlsaDriver::getSequencerTime()
{
    return getAlsaTime() + m_playStartPosition - m_alsaPlayStartTime;
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
    sequencerTime.usec = snd_seq_queue_status_get_real_time(status)->tv_nsec
                         / 1000;

    snd_seq_queue_status_free(status);

    return sequencerTime;
}


// Get all pending input events and turn them into a MappedComposition.
//
//
MappedComposition*
AlsaDriver::getMappedComposition(const RealTime & playLatency)
{
    m_recordComposition.clear();

    // If the input port hasn't connected we shouldn't poll it
    //
    if(m_midiInputPortConnected == false)
        return &m_recordComposition;

    Rosegarden::RealTime eventTime;

    do
    {
        snd_seq_event_t *event;

        // Check for input events and return if nothing returned
        //
        if (snd_seq_event_input(m_midiHandle, &event) < 0)
            return &m_recordComposition;

        MidiByte channel = event->data.note.channel;
        unsigned int chanNoteKey = ( channel << 8 ) + event->data.note.note;

        eventTime.sec = event->time.time.tv_sec;
        eventTime.usec = event->time.time.tv_nsec / 1000;
        eventTime = eventTime + m_alsaRecordStartTime;

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

                    /*
                    std::cout << "NOTE ON TIMESTAMP = "
                              << event->time.time.tv_sec
                              << " . "
                              << event->time.time.tv_nsec
                              << std::endl;
                              */
                    break;
                }

                // fall through (velocity 0 == NOTEOFF)

            case SND_SEQ_EVENT_NOTEOFF:
                if (m_noteOnMap[chanNoteKey] != 0)
                {
                    RealTime duration = eventTime -
                         m_noteOnMap[chanNoteKey]->getEventTime();

                    assert(duration >= RealTime(0, 0));

                    m_noteOnMap[chanNoteKey]->setDuration(duration);

                    std::cout << "Inserted NOTE at "
                              << m_noteOnMap[chanNoteKey]->getEventTime()
                              << " with duration "
                              << m_noteOnMap[chanNoteKey]->getDuration()
                              << std::endl;

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
                    //mE->setEventTime(guiTimeStamp);
                    mE->setData1(event->data.control.value >> 7);
                    mE->setData2(event->data.control.value & 0x7f);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CONTROLLER:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiController);
                    //mE->setEventTime(guiTimeStamp);
                    mE->setData1(event->data.control.param);
                    mE->setData2(event->data.control.value);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_PGMCHANGE:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiProgramChange);
                    //mE->setEventTime(guiTimeStamp);
                    mE->setData1(event->data.control.value);
                    m_recordComposition.insert(mE);

                }
                break;

            case SND_SEQ_EVENT_PITCHBEND:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiPitchWheel);
                    //mE->setEventTime(guiTimeStamp);
                    mE->setData1(event->data.control.value >> 7);
                    mE->setData2(event->data.control.value & 0x7f);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CHANPRESS:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiChannelPressure);
                    //mE->setEventTime(guiTimeStamp);
                    mE->setData1(event->data.control.value >> 7);
                    mE->setData2(event->data.control.value & 0x7f);
                    m_recordComposition.insert(mE);
                }
               break;

            default:
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "got unrecognised MIDI event" << std::endl;
               break;

            case SND_SEQ_EVENT_SYSEX:
                std::cout << "AlsaDriver - SYSTEM EXCLUSIVE EVENT not supported"
                          << std::endl;

                break;

        }

        snd_seq_free_event(event);
    }
    while (snd_seq_event_input_pending(m_midiHandle, 0) > 0);

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
        if ((*i)->getType() == MappedEvent::Audio)
            continue;


        midiRelativeTime = (*i)->getEventTime() - m_playStartPosition +
                           playLatency + m_alsaPlayStartTime;

        // Second and nanoseconds for ALSA
        //
        snd_seq_real_time_t time = { midiRelativeTime.sec,
                                     midiRelativeTime.usec * 1000 };

        // Set destination according to Instrument mapping
        //
        outputDevice = getPairForMappedInstrument((*i)->getInstrument());

        snd_seq_ev_set_dest(event,
                            outputDevice.first,
                            outputDevice.second);

        /*
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
            channel = 0;

        switch((*i)->getType())
        {
            case MappedEvent::MidiNote:
                // Could use just snd_seq_ev_set_note with duration instead
                //
                snd_seq_ev_set_noteon(event,
                                      channel,
                                      (*i)->getPitch(),
                                      (*i)->getVelocity());
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

            case MappedEvent::MidiPitchWheel:
                snd_seq_ev_set_pitchbend(event,
                                         channel,
                                         (*i)->getData1());
                break;

            case MappedEvent::MidiController:
                snd_seq_ev_set_controller(event,
                                          channel,
                                          (*i)->getData1(),
                                          (*i)->getData2());
                break;

            default:
                std::cout << "AlsaDriver::processMidiOut - "
                          << "unrecognised event type"
                          << std::endl;
                break;
        }

        // Add note to note off stack
        //
        if ((*i)->getType() == MappedEvent::MidiNote)
        {
            NoteOffEvent *noteOffEvent =
                new NoteOffEvent(midiRelativeStopTime, // already calculated
                                 (*i)->getPitch(),
                                 channel,
                                 (*i)->getInstrument());
            m_noteOffQueue.insert(noteOffEvent);
        }

        snd_seq_event_output(m_midiHandle, event);
        //snd_seq_event_output_buffer(m_midiHandle, event);
        //snd_seq_event_output_direct(m_midiHandle, event);

    }

    /*
    cout << "PENDING EVENTS = " << snd_seq_event_output_pending(m_midiHandle)
         << endl;
         */

    //showQueueStatus(m_queue);
    snd_seq_drain_output(m_midiHandle); // the new "flush" it seems
    //snd_seq_sync_output_queue(m_midiHandle);

    //printSystemInfo();
    processNotesOff(midiRelativeTime);

    delete event;
}

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

    // No audio for the moment, just the Midi events
    //
    processMidiOut(mC, playLatency, now);
}


void
AlsaDriver::record(const RecordStatus& recordStatus)
{
    if (recordStatus == RECORD_MIDI)
    {
        // start recording
        m_recordStatus = RECORD_MIDI;
        m_alsaRecordStartTime = getSequencerTime();
    }
    else if (recordStatus == RECORD_AUDIO)
    {
        std::cerr << "ArtsDriver - record() - AUDIO RECORDING not yet supported"
                  << std::endl;
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
        std::cerr << "ArtsDriver  - record() - Unsupported recording mode"
                  << std::endl;
    }
}

ClientPortPair
AlsaDriver::getFirstDestination(bool duplex)
{
    ClientPortPair destPair(-1, -1);
    std::vector<AlsaPort*>::iterator it;

    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); it++)
    {
        destPair.first = (*it)->m_client;
        destPair.second = (*it)->m_port;

        // If duplex port is required then choose first one
        //
        if (duplex)
        {
            if ((*it)->m_duplex == true)
                return destPair;
        }
        else
        {
            // If duplex port isn't required then choose first
            // specifically non-duplex port (should be a synth)
            //
            if ((*it)->m_duplex == false)
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

    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); it++)
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

void
AlsaDriver::processPending()
{
    processNotesOff(getSequencerTime());
}


}

#endif // HAVE_ALSA
