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

// ALSA
#include <alsa/asoundlib.h>
#include <alsa/version.h>  // SND_LIB_VERSION_STR


namespace Rosegarden
{

AlsaDriver::AlsaDriver():
    SoundDriver(std::string("ALSA ") + std::string(SND_LIB_VERSION_STR)),
    m_runningId(0)
{
    std::cout << "Rosegarden AlsaDriver - " << m_name << std::endl;
    generateInstruments();
}

AlsaDriver::~AlsaDriver()
{
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
                     SND_SEQ_OPEN_INPUT,
                     SND_SEQ_NONBLOCK) < 0)
    {
        std::cout << "AlsaDriver::generateInstruments() - "
                  << "couldn't open sequencer - " << snd_strerror(errno)
                  << std::endl;
        exit(1);
    }

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
                for (int i = 0; i < snd_seq_port_info_get_midi_channels(pinfo);
                                i++)
                {
                    addInstrument(
                            Instrument::Midi,
                            std::string(snd_seq_port_info_get_name(pinfo)),
                            snd_seq_port_info_get_client(pinfo),
                            snd_seq_port_info_get_port(pinfo),
                            i);

                }

                std::cout << "MIDI VOICE = "
                          << snd_seq_port_info_get_midi_voices(pinfo)
                          << std::endl;

                std::cout << "SYNTH VOICES = "
                          << snd_seq_port_info_get_synth_voices(pinfo)
                          << std::endl;
                /*
                std::cout << snd_seq_port_info_get_client(pinfo) << " : "
                          << snd_seq_port_info_get_port(pinfo) << " : "
                          << snd_seq_client_info_get_name(cinfo) << " : "
                          << snd_seq_port_info_get_name(pinfo) << " : CH = "
                          << snd_seq_port_info_get_midi_channels(pinfo)
                          << std::endl;


                instrumentId++:
                */
            }

        }
    }

    snd_seq_close(m_handle);

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

    Instrument *instr = new Instrument(m_runningId,
                                       type,
                                       name,
                                       channel,
                                       0);

    m_instruments.push_back(instr);
                                   
    m_runningId++;
}


void
AlsaDriver::initialiseMidi()
{ 
    if (snd_seq_open(&m_handle,
                     "hw",   // why hw always?
                     SND_SEQ_OPEN_DUPLEX,
                     SND_SEQ_NONBLOCK) < 0)
    {
        std::cerr
            << "AlsaDriver::initialiseMidi() - couldn't open ALSA sequencer"
            << std::endl;
        exit(1);
    }

    // Create queue, client and port
    //
    m_queue = snd_seq_alloc_queue(m_handle);
    m_client = snd_seq_client_id(m_handle);

    m_port = snd_seq_create_simple_port(m_handle,
                                        NULL,
                                        SND_SEQ_PORT_CAP_WRITE |
                                        SND_SEQ_PORT_CAP_SUBS_WRITE |
                                        SND_SEQ_PORT_CAP_READ,
                                        SND_SEQ_PORT_TYPE_MIDI_GENERIC);


}

void
AlsaDriver::initialiseAudio()
{
}

void
AlsaDriver::initialisePlayback()
{
    // Set a default tempo
    //
    double tempo = 120.0;
    long resolution = 960; // ppq

    snd_seq_queue_tempo_t *qtempo;
    snd_seq_queue_tempo_alloca(&qtempo);
    memset(qtempo, 0, snd_seq_queue_tempo_sizeof());
    snd_seq_queue_tempo_set_ppq(qtempo, resolution);
    snd_seq_queue_tempo_set_tempo(qtempo, 60*1000000/tempo);
    int ret = snd_seq_set_queue_tempo(m_handle, m_queue, qtempo);
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
AlsaDriver::processMidiOut(const MappedComposition &/*mC*/,
                           const RealTime &/*playLatency*/,
                           bool /*now*/)
{
}

}


