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

// EXPERIMENTAL - SHOULDN'T EVEN BE IN THE BUILD YET!!!!
//
// rwb 04.04.2002
//

#include "AlsaDriver.h"
#include "MappedInstrument.h"

// ALSA
//#include <linux/asequencer.h>
#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>
#include <alsa/version.h>  // SND_LIB_VERSION_STR


namespace Rosegarden
{

AlsaDriver::AlsaDriver():
    SoundDriver(std::string("ALSA ") + std::string(SND_LIB_VERSION_STR)),
    m_runningId(0),
    m_client(-1),
    m_port(-1),
    m_queue(-1),
    m_maxClients(-1),
    m_maxPorts(-1),
    m_maxQueues(-1)
{
    std::cout << "Rosegarden AlsaDriver - " << m_name << std::endl;
    generateInstruments();
}

AlsaDriver::~AlsaDriver()
{
    snd_seq_close(m_handle);
}

void
AlsaDriver::getSystemInfo()
{
    int err;
    snd_seq_system_info_t *sysinfo;

    snd_seq_system_info_alloca(&sysinfo);

    if ((err = snd_seq_system_info(m_handle, sysinfo))<0)
    {
        std::cerr << "System info error: " <<  snd_strerror(err)
                  << std::endl;
        exit(1);
    }

    m_maxQueues = snd_seq_system_info_get_queues(sysinfo); 
    m_maxClients = snd_seq_system_info_get_clients(sysinfo);
    m_maxPorts = snd_seq_system_info_get_ports(sysinfo);

    std::cout << "Max queues   = " << m_maxQueues << std::endl;
    std::cout << "Max clients  = " << m_maxClients << std::endl;
    std::cout << "Max ports    = " << m_maxPorts << std::endl;
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
        if ((err = snd_seq_get_queue_status(m_handle, idx, status))<0)
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
    //snd_seq_system_info_t *sysinfo;
    //int  port;
    int  client;
    unsigned int cap;

    if (snd_seq_open(&m_handle,
                     "hw",   // why hw always?
                     SND_SEQ_OPEN_DUPLEX,
                     SND_SEQ_NONBLOCK) < 0)
    {
        std::cout << "AlsaDriver::generateInstruments() - "
                  << "couldn't open sequencer - " << snd_strerror(errno)
                  << std::endl;
        exit(1);
    }

    snd_seq_set_client_name(m_handle, "Rosegarden sequencer");

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    m_instruments.clear();
    m_alsaInstruments.clear();

    // reset running id
    //
    m_runningId = MidiInstrumentBase;

    while (snd_seq_query_next_client(m_handle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(m_handle, pinfo) >= 0)
        {
            cap = (SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE);

            if ((snd_seq_port_info_get_capability(pinfo) & cap) == cap)
            {
                /*
                for (int i = 0; i < snd_seq_port_info_get_midi_channels(pinfo);
                                i++)
                {
                */
                    addInstrument(
                            Instrument::Midi,
                            std::string(snd_seq_port_info_get_name(pinfo)),
                            snd_seq_port_info_get_client(pinfo),
                            snd_seq_port_info_get_port(pinfo),
                            0);
                //}

                /*
                std::cout << "MIDI VOICE = "
                          << snd_seq_port_info_get_midi_voices(pinfo)
                          << std::endl;

                std::cout << "SYNTH VOICES = "
                          << snd_seq_port_info_get_synth_voices(pinfo)
                          << std::endl;
                          */

                std::cout << snd_seq_port_info_get_client(pinfo) << " : "
                          << snd_seq_port_info_get_port(pinfo) << " : "
                          << snd_seq_client_info_get_name(cinfo) << " : "
                          << snd_seq_port_info_get_name(pinfo) << " : CH = "
                          << snd_seq_port_info_get_midi_channels(pinfo)
                          << std::endl;
            }

        }
    }

}

// Create a local ALSA instrument for reference purposes
// and create a GUI Instrument for transmission upwards.
//
void
AlsaDriver::addInstrument(Instrument::InstrumentType type,
                          const std::string &name, 
                          int client,
                          int port,
                          int channel)
{
    AlsaInstrument *alsaInstr =
        new AlsaInstrument(m_runningId,
                           name,
                           client,
                           port,
                           channel);

    m_alsaInstruments.push_back(alsaInstr);

    MappedInstrument *instr = new MappedInstrument(type,
                                                   channel,
                                                   m_runningId);

    m_instruments.push_back(instr);
                                   
    m_runningId++;
}


// Set up queue, client and port
//
void
AlsaDriver::initialiseMidi()
{ 
    if((m_queue = snd_seq_alloc_named_queue(m_handle, "Rosegarden queue")) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - can't allocate queue"
                  << std::endl;
        exit(1);
    }

    if((m_client = snd_seq_client_id(m_handle)) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - can't create client"
                  << std::endl;
        exit(1);
    }

    // create our port
    //
    //snd_seq_port_info_t *port_info;
    //std::memset(port_info, 0, sizeof(port_info));
    /*
    std::strcpy(port_info->name,  "Rosegarden");
    //std::strcpy(port_info->group, SND_SEQ_GROUP_APPLICATION);
    port_info->capability = SND_SEQ_PORT_CAP_READ
                         | SND_SEQ_PORT_CAP_SUBS_READ
                         | SND_SEQ_PORT_CAP_WRITE
                         | SND_SEQ_PORT_CAP_SUBS_WRITE
                        | SND_SEQ_PORT_CAP_DUPLEX;
    */
    //port_info->cap_group  = port_info->capability;
    //port_info->type       = SND_SEQ_PORT_TYPE_APPLICATION;
    //m_port = snd_seq_create_port(m_handle, port_info);

    m_port = snd_seq_create_simple_port(m_handle,
                                        NULL,
                                        SND_SEQ_PORT_CAP_WRITE |
                                        SND_SEQ_PORT_CAP_SUBS_WRITE |
                                        SND_SEQ_PORT_CAP_READ,
                                        SND_SEQ_PORT_TYPE_MIDI_GENERIC);
    if (m_port < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - can't create port"
                  << std::endl;
        exit(1);
    }

    if (snd_seq_connect_to(m_handle, m_port, m_alsaInstruments.back()->m_client,
                                         m_alsaInstruments.back()->m_port) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - can't subscribe"
                  << std::endl;
        exit(1);
    }

    if (snd_seq_set_client_pool_output(m_handle, 200) < 0 ||
        snd_seq_set_client_pool_input(m_handle, 20) < 0 ||
        snd_seq_set_client_pool_output_room(m_handle, 20) < 0)
    {
        std::cerr << "AlsaDriver::initialiseMidi() - "
                  << "can't modify pool parameters"
                  << std::endl;
        exit(1);

    }
    getSystemInfo();

}

void
AlsaDriver::initialiseAudio()
{
}

void
AlsaDriver::initialisePlayback()
{
    std::cout << "AlsaDriver::initialisePlayback" << std::endl;

    // Queue timer - do we need this?
    //
    //
    /*
    snd_timer_id_t *timerId;
    snd_seq_queue_timer_t *queueTimer;

    snd_seq_queue_timer_alloca(&queueTimer);
    snd_timer_id_alloca(&timerId);

    snd_timer_id_set_class(timerId, SND_TIMER_CLASS_PCM);
    snd_timer_id_set_card(timerId, 0) ; // pcm_card = 0
    snd_timer_id_set_device(timerId, 0) ; // pcm_device = 0
    snd_timer_id_set_subdevice(timerId, 0);
    snd_seq_queue_timer_set_type(queueTimer, SND_SEQ_TIMER_ALSA);
    snd_seq_queue_timer_set_id(queueTimer, timerId);

    if (snd_seq_set_queue_timer(m_handle, m_queue, queueTimer) < 0)
    {
        std::cerr << "AlsaDriver::initialisePlayback - "
                  << "can't assign queue timer"
                  << std::endl;
        exit(1);
    }




    // Set a default tempo
    //
    snd_seq_queue_tempo_t *tempo;
    snd_seq_queue_tempo_alloca(&tempo);
    std::memset(tempo, 0, snd_seq_queue_tempo_sizeof());
    int r = snd_seq_get_queue_tempo(m_handle, m_queue, tempo);
    snd_seq_queue_tempo_set_tempo(tempo, 10);
    snd_seq_queue_tempo_set_ppq(tempo, 960);
    r = snd_seq_set_queue_tempo(m_handle, m_queue, tempo);

    snd_seq_event_t *ev = snd_seq_create_event();
    //ev->queue             = m_handle->queue;
    ev->dest.client       = SND_SEQ_CLIENT_SYSTEM;
    ev->dest.port         = SND_SEQ_PORT_SYSTEM_TIMER;
    //ev->data.queue.queue  = m_handle->queue;
    ev->flags             = SND_SEQ_TIME_STAMP_REAL
                           | SND_SEQ_TIME_MODE_REL;

    ev->time.time.tv_sec  = 0;
    ev->time.time.tv_nsec = 0;
    ev->type              = SND_SEQ_EVENT_START;
    snd_seq_event_output(m_handle, ev);
    //snd_seq_flush_output(m_handle);
    snd_seq_drain_output(m_handle);
    */

    /*
    double tempo = 120.0;

    snd_seq_queue_tempo_t *qtempo;
    snd_seq_queue_tempo_alloca(&qtempo);
    memset(qtempo, 0, snd_seq_queue_tempo_sizeof());
    snd_seq_queue_tempo_set_ppq(qtempo, resolution);
    */


    long resolution = 960; // ppq
    double tempo = 90.0;

    snd_seq_queue_tempo_t *qtempo;
    snd_seq_queue_tempo_alloca(&qtempo);
    snd_seq_queue_tempo_set_ppq(qtempo, resolution);
    snd_seq_queue_tempo_set_tempo(qtempo, (unsigned int)(60.0*1000000.0/tempo));
    if (snd_seq_set_queue_tempo(m_handle, m_queue, qtempo) < 0)
    {
        std::cerr << "AlsaDriver::initialisePlayback - "
                  << "couldn't set queue tempo"
                  << std::endl;
    }


    int result;

    // Start the timer
    if ((result = snd_seq_start_queue(m_handle, m_queue, NULL)) < 0)
    {
        std::cerr << "AlsaDriver::initialisePlayback - couldn't start queue - "
                  << snd_strerror(result)
                  << std::endl;
        exit(1);
    }

    snd_seq_drain_output(m_handle);
}


void
AlsaDriver::stopPlayback()
{
    snd_seq_stop_queue(m_handle, m_queue, 0);
    snd_seq_drain_output(m_handle);

}



void
AlsaDriver::resetPlayback()
{
}

void
AlsaDriver::allNotesOff()
{
}

void
AlsaDriver::processNotesOff(const RealTime & /*time*/)
{
}

void
AlsaDriver::processAudioQueue()
{
}

RealTime
AlsaDriver::getSequencerTime()
{
    RealTime rT;
    return rT;
}

void
AlsaDriver::immediateProcessEventsOut(MappedComposition & /*mC*/)
{
}

MappedComposition*
AlsaDriver::getMappedComposition(const RealTime & /*playLatency*/)
{
    MappedComposition *mC = new MappedComposition();
    return mC;
}
    
void
AlsaDriver::processMidiOut(const MappedComposition &mC,
                           const RealTime &playLatency,
                           bool now)
{



    Rosegarden::RealTime midiTime;
    //Rosegarden::RealTime midiRelativeStopTime;
    Rosegarden::MappedInstrument *instrument;
    MidiByte channel;

    for (MappedComposition::iterator i = mC.begin(); i != mC.end(); ++i)
    {
        if ((*i)->getType() == MappedEvent::Audio)
            continue;


        midiTime = (*i)->getEventTime() + playLatency;

        // Second and nanoseconds!
        //snd_seq_real_time_t time = { midiTime.sec, midiTime.usec * 1000 };
        snd_seq_real_time_t time = { midiTime.sec, midiTime.usec * 1000 };
        cout << "TIME = " << midiTime.sec << " : " << midiTime.usec * 1000
              << endl;

        // initialise event
        //
        snd_seq_event_t *event= new snd_seq_event_t();
        snd_seq_ev_clear(event);
        snd_seq_ev_set_dest(event,
                            m_alsaInstruments.back()->m_client,
                            m_alsaInstruments.back()->m_port);

        std::cout << "EVENT to " << m_alsaInstruments.back()->m_client
                  << " : " 
                  << m_alsaInstruments.back()->m_port << std::endl;

        snd_seq_ev_set_source(event, m_port);
        snd_seq_ev_schedule_real(event, m_queue, 0, &time); // last is "time"

        instrument = getMappedInstrument((*i)->getInstrument());
 
        if (instrument != 0)
            channel = instrument->getChannel();
        else
            channel = 0;

        switch((*i)->getType())
        {
            case MappedEvent::MidiNote:
                /*
                event->data.note.note = (*i)->getPitch();
                event->data.note.velocity = (*i)->getVelocity();
                event->data.note.duration = 1000; //(*i)->getDuration();
                */
                snd_seq_ev_set_noteon(event,
                                      channel,
                                      (*i)->getPitch(),
                                      (*i)->getVelocity());
                break;

            case MappedEvent::MidiProgramChange:
            case MappedEvent::MidiKeyPressure:
            case MappedEvent::MidiChannelPressure:
            case MappedEvent::MidiPitchWheel:
            case MappedEvent::MidiController:
            default:
                break;
        }

        // Add note to note off stack
        //
        if ((*i)->getType() == MappedEvent::MidiNote)
        {
            ;/*
            NoteOffEvent *noteOffEvent =
                new NoteOffEvent(midiRelativeStopTime,
                (Rosegarden::MidiByte)event->data.note.note,
                (Rosegarden::MidiByte)event.command.status);
                */
        }


        snd_seq_event_output(m_handle, event);
        //snd_seq_event_output_buffer(m_handle, event);
        //snd_seq_event_output_direct(m_handle, event);

    }

    cout << "PENDING EVENTS = " << snd_seq_event_output_pending(m_handle)
         << endl;

    //showQueueStatus(m_queue);
    snd_seq_drain_output(m_handle); // the new "flush" it seems
    //snd_seq_sync_output_queue(m_handle);

    //printSystemInfo();
    //processNotesOff(time);
}

}


