// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.2
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

#ifndef HAVE_ALSA

#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include <arts/artsflow.h>     // aRts audio subsys
#include <arts/artsmodules.h>  // aRts wav modules

#include "MidiRecord.h"        // local MIDI record implementation
#include "SoundDriver.h"


// Specialisation of SoundDriver to support aRts (http://www.arts-project.org)
// Supports version 0.6.0
//
//

#ifndef _ARTSDRIVER_H_
#define _ARTSDRIVER_H_

namespace Rosegarden
{

class ArtsDriver : public SoundDriver
{
public:
    ArtsDriver(MappedStudio *studio);
    virtual ~ArtsDriver();

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
    
    virtual void processEventsOut(const MappedComposition &mC,
                                  const RealTime &playLatency,
                                  bool now);

    virtual void record(const RecordStatus& recordStatus);

    virtual void processPending(const RealTime &playLatency);

    // Not supported
    //
    virtual unsigned int getSampleRate() const { return 0; }

    void processMidiIn(const Arts::MidiCommand &midiCommand,
                       const Arts::TimeStamp &timeStamp,
                       const Rosegarden::RealTime &playLatency);

    // Some Arts helper methods 'cos the basic Arts::TimeStamp
    // method is a bit unhelpful
    //
    Arts::TimeStamp aggregateTime(const Arts::TimeStamp &ts1,
                                  const Arts::TimeStamp &ts2);

    Arts::TimeStamp deltaTime(const Arts::TimeStamp &ts1,
                              const Arts::TimeStamp &ts2);

    inline Arts::TimeStamp recordTime(Arts::TimeStamp const &ts)
    {
        return (aggregateTime(deltaTime(ts, m_artsRecordStartTime),
                Arts::TimeStamp(m_playStartPosition.sec,
                m_playStartPosition.usec)));
    }

    // Plugin instance management - do nothing for the moment
    //
    virtual void setPluginInstance(InstrumentId id,
                                   unsigned long pluginId,
                                   int position) {;}

    virtual void removePluginInstance(InstrumentId id, int position) {;}


protected:
    virtual void generateInstruments();
    virtual void processAudioQueue(const RealTime &playLatency,
                                   bool now);
    virtual void processMidiOut(const MappedComposition &mC,
                                const RealTime &playLatency,
                                bool now);

    void sendDeviceController(MidiByte controller, MidiByte value);

private:
    // aRts sound server reference
    //
    Arts::SoundServerV2      m_soundServer;

    // aRts MIDI devices
    //
    Arts::MidiManager        m_midiManager;
    Arts::Dispatcher         m_dispatcher;
    Arts::MidiClient         m_midiPlayClient;
    Arts::MidiClient         m_midiRecordClient;
    RosegardenMidiRecord     m_midiRecordPort;
    Arts::MidiPort           m_midiPlayPort;

    // aRts Audio devices
    //
    Arts::Synth_AMAN_PLAY    m_amanPlay;
    Arts::Synth_AMAN_RECORD  m_amanRecord;

    Arts::TimeStamp          m_artsPlayStartTime;
    Arts::TimeStamp          m_artsRecordStartTime;

};

}

#endif // _ARTSDRIVER_H_

#endif // HAVE_ALSA
