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


#include <vector>
#include <alsa/asoundlib.h> // ALSA

#include "Sequencer.h" // temporary - for NoteOffQueue

#include "SoundDriver.h"
#include "Instrument.h"
#include "Device.h"


// Specialisation of SoundDriver to support ALSA
//
//


#ifndef _ALSADRIVER_H_
#define _ALSADRIVER_H_

namespace Rosegarden
{


class AlsaInstrument
{
public:
    AlsaInstrument(InstrumentId id,
                   const std::string &name,
                   int client,
                   int port,
                   int channel):
        m_id(id),
        m_name(name),
        m_client(client),
        m_port(port),
        m_channel(channel),
        m_input(false),
        m_type(0) {;}

    ~AlsaInstrument() {;}

    InstrumentId m_id;
    std::string  m_name;
    int          m_client;
    int          m_port;
    int          m_channel;
    bool         m_input;    // is an input port (recording?)
    unsigned int m_type;     // MIDI, synth or what?
};


class AlsaDriver : public SoundDriver
{
public:
    AlsaDriver();
    virtual ~AlsaDriver();

    virtual void generateInstruments();
    virtual void initialiseMidi();
    virtual void initialiseAudio();
    virtual void initialisePlayback();
    virtual void stopPlayback();
    virtual void resetPlayback();
    virtual void allNotesOff();
    virtual void processNotesOff(const RealTime &time);
    virtual void processAudioQueue();

    virtual RealTime getSequencerTime();

    virtual void
        immediateProcessEventsOut(MappedComposition &mC);

    virtual MappedComposition*
        getMappedComposition(const RealTime &playLatency);
    
    virtual void processMidiOut(const MappedComposition &mC,
                                const RealTime &playLatency,
                                bool now);

    void addInstrument(Instrument::InstrumentType type,
                       const std::string &name, 
                       int client,
                       int port,
                       int channel);


    // Some stuff to help us debug this
    //
    void getSystemInfo();
    void showQueueStatus(int queue);

private:
    std::vector<AlsaInstrument*> m_alsaInstruments;
    InstrumentId                 m_runningId;

    // ALSA lib stuff
    //
    snd_seq_t                   *m_handle;
    int                          m_client;
    int                          m_port;
    int                          m_queue;
    int                          m_maxClients;
    int                          m_maxPorts;
    int                          m_maxQueues;


    NoteOffQueue                 m_noteOffQueue;


};

}

#endif // _SOUNDDRIVER_H_

