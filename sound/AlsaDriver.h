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

#include <vector>
#include <alsa/asoundlib.h> // ALSA

#include "SoundDriver.h"
#include "Instrument.h"
#include "Device.h"


// Specialisation of SoundDriver to support ALSA (http://www.alsa-project.org)
// Currently supports version 0.9beta12
//
//


#ifndef _ALSADRIVER_H_
#define _ALSADRIVER_H_

namespace Rosegarden
{

// An AlsaPort represents one or more (usually 16) MappedInstruments.
//
//

class AlsaPort
{
public:
    AlsaPort(InstrumentId startId,
             InstrumentId endId,
             const std::string &name,
             int client,
             int port,
             bool duplex):
        m_startId(startId),
        m_endId(endId),
        m_name(name),
        m_midiClient(client),
        m_midiPort(port),
        m_duplex(duplex),
        m_type(0) {;}

    ~AlsaPort() {;}

    InstrumentId m_startId;
    InstrumentId m_endId;
    std::string  m_name;
    int          m_midiClient;
    int          m_midiPort;
    bool         m_duplex;   // is an input port as well?
    unsigned int m_type;     // MIDI, synth or what?
};


typedef std::pair<int, int> ClientPortPair;

class AlsaDriver : public SoundDriver
{
public:
    AlsaDriver();
    virtual ~AlsaDriver();

    virtual void initialiseMidi();
    virtual void initialiseAudio();
    virtual void initialisePlayback(const RealTime &position);
    virtual void stopPlayback();
    virtual void resetPlayback(const RealTime &position,
                               const RealTime &latency);
    virtual void allNotesOff();
    virtual void processNotesOff(const RealTime &time);

    virtual RealTime getSequencerTime();

    virtual MappedComposition*
        getMappedComposition(const RealTime &playLatency);
    
    virtual void record(const RecordStatus& recordStatus);

    void addInstrumentsForPort(Instrument::InstrumentType type,
                               const std::string &name, 
                               int client,
                               int port,
                               bool duplex);

    virtual void processEventsOut(const MappedComposition &mC,
                                  const Rosegarden::RealTime &playLatency, 
                                  bool now);

    // Some stuff to help us debug this
    //
    void getSystemInfo();
    void showQueueStatus(int queue);

protected:
    ClientPortPair getFirstDestination(bool duplex);

    virtual void generateInstruments();
    virtual void processMidiOut(const MappedComposition &mC,
                                const RealTime &playLatency,
                                bool now);

    virtual void processAudioQueue();

private:
    std::vector<AlsaPort*>       m_alsaPorts;


    // Counters for assignment of Instruments at the driver level.
    //
    InstrumentId                 m_midiRunningId; 
    InstrumentId                 m_audioRunningId;

    // ALSA MIDI/Sequencer stuff
    //
    snd_seq_t                   *m_midiHandle;
    int                          m_midiClient;
    int                          m_midiPort;
    int                          m_midiQueue;
    int                          m_maxClients;
    int                          m_maxPorts;
    int                          m_maxQueues;

    // ALSA Audio/PCM stuff
    //
    snd_pcm_t                   *m_audioHandle;
    snd_pcm_stream_t             m_audioStream;

    NoteOffQueue                 m_noteOffQueue;

    // Because can fail even if the driver's up
    bool                         m_midiInputPortConnected;
    bool                         m_midiOutputPortConnected;

    //m_alsaRecordStartTime;

};

}

#endif // _SOUNDDRIVER_H_

