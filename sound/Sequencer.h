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

// From this class we control our sound subsystems - audio
// and MIDI are initialised, playback and recording handles
// are available to the higher levels for sending and 
// retreiving MIDI and audio.  When the Rosegarden sequencer
// (sequencer/) initialises it creates a Rosegarden::Sequencer
// which prepares itself for playback and recording.
//
// At this level we accept MappedCompositions (single point
// representation - NOTE ONs with durations) and turn them
// into MIDI events (generate and segment NOTE OFFs).
//
// Recording wise we take aRTS events and turn them into
// a MappedComposition before sending it up to the gui.
// Timing is normalised by the GUI and returned as
// Rosegarden::RealTime timestamps that can be easily
// converted into the relevant absolute positions.
//
// We don't have any measure of tempo or resolution at
// this level - all we see are Arts::TimeStamps and
// Rosegarden::RealTimes.
//
//

#ifndef _ROSEGARDEN_SEQUENCER_H_
#define _ROSEGARDEN_SEQUENCER_H_

#include <map>
#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include <arts/artsflow.h>     // audio subsys
#include <arts/artsmodules.h>  // handling wavs
#include "MappedComposition.h"
#include "Midi.h"
#include "MidiRecord.h"
#include "MidiEvent.h"


namespace Rosegarden
{

// NOTE OFF queue. This holds a time ordered set of
// pending NOTE OFF events.
//
class NoteOffEvent
{
public:
    NoteOffEvent() {;}
    NoteOffEvent(const Rosegarden::RealTime &realTime,
                 const unsigned int &pitch,
                 const Rosegarden::MidiByte &status):
        m_realTime(realTime),
        m_pitch(pitch),
        m_status(status) {;}
    ~NoteOffEvent() {;}

    struct NoteOffEventCmp
    {
        bool operator()(NoteOffEvent *nO1, NoteOffEvent *nO2)
        { 
            return nO1->getRealTime() < nO2->getRealTime();
        }
    };
    
    Rosegarden::RealTime getRealTime() { return m_realTime; }
    Rosegarden::MidiByte getPitch() { return m_pitch; }

private: 
    Rosegarden::RealTime m_realTime;
    Rosegarden::MidiByte m_pitch;
    Rosegarden::MidiByte m_status;

};

class NoteOffQueue : public std::multiset<NoteOffEvent *,
                     NoteOffEvent::NoteOffEventCmp>
{
public:
    NoteOffQueue() {;}
    ~NoteOffQueue() {;}
private:
};


class Sequencer
{
public:

    typedef enum
    {
	ASYNCHRONOUS_MIDI,
	ASYNCHRONOUS_AUDIO,
	RECORD_MIDI,
	RECORD_AUDIO
    } RecordStatus;

    typedef enum
    {
        MIDI_AND_AUDIO_SUBSYS_OK,  // Everything's OK
        MIDI_SUBSYS_OK,            // MIDI's OK
        AUDIO_SUBSYS_OK,           // AUDIO's OK
        NO_SEQUENCE_SUBSYS         // Nothing's OK
    } SequencerStatus;

    Sequencer();
    ~Sequencer();

    // Activate a recording state
    //
    void record(const RecordStatus& recordStatus);

    // Our current recording status.  Asynchronous record states
    // mean that we're just accepting asynchronous events but not
    // recording them - they are forwarded onto the GUI for level/
    // activity display purposes.
    //
    RecordStatus recordStatus() { return m_recordStatus; }
    
    // Get the difference in TimeStamps - Arts::TimeStamp
    // doesn't currently have any operators
    //
    //
    Arts::TimeStamp deltaTime(const Arts::TimeStamp &ts1,
                              const Arts::TimeStamp &ts2);

    // Aggregate TimeStamps
    //
    Arts::TimeStamp aggregateTime(const Arts::TimeStamp &ts1,
                                  const Arts::TimeStamp &ts2);

    // Get the time from the beginning of playback
    //
    inline Arts::TimeStamp playTime(Arts::TimeStamp const &ts)
               { return (deltaTime(ts, m_playStartTime)); }

    // Create a recording TimeStamp - take the current time from the
    // record start time and then add on any playStartPosition value.
    //
    //
    inline Arts::TimeStamp recordTime(Arts::TimeStamp const &ts)
               { return (aggregateTime(deltaTime(ts, m_recordStartTime),
                            Arts::TimeStamp(m_playStartPosition.sec,
                                            m_playStartPosition.usec))); }

    // Wrap any recorded MIDI into a MappedComposition and return it
    //
    //
    MappedComposition getMappedComposition();

    // Process MappedComposition into MIDI events and send to aRTS
    //
    void processMidiOut(Rosegarden::MappedComposition mappedComp,
                        const Rosegarden::RealTime &playLatency);

    // Initialise internal states ready for new playback to commence
    //
    void initialisePlayback(const Rosegarden::RealTime &position);

    // Reset internal states while playing (like when looping
    // and jumping to a new time)
    //
    void resetPlayback(const Rosegarden::RealTime &position,
                       const Rosegarden::RealTime &playLatency);

    // Get the sequencer time
    //
    Rosegarden::RealTime getSequencerTime();

    bool isPlaying() { return m_playing; }

    void stopPlayback();

    // Control of Notes off.  At the moment aRTS doesn't support
    // the killing of Notes already pending to play.
    //
    void processNotesOff(const Rosegarden::RealTime &midiTime);
    void allNotesOff();

    Rosegarden::RealTime getStartPosition() { return m_playStartPosition; }

    // Temporary cheat to enable quick sending of midi events from a 
    // thin aRTS client - not to be used in anger!
    //
    Arts::MidiPort* playMidiPort() { return &m_midiPlayPort; }

    // Return the status of the sound systems
    //
    SequencerStatus getStatus() const { return m_sequencerStatus; }

    void setPlayStartPosition(const Rosegarden::RealTime &pos)
        { m_playStartPosition = pos; }

private:
    // start MIDI and Audio subsystems
    //
    void initialiseMidi();
    void initialiseAudio();

    // get a vector of recorded events from aRTS
    // (only for internal use)
    //
    inline std::vector<Arts::MidiEvent>* getMidiQueue()
        { return m_midiRecordPort.getQueue(); }

    // process a raw aRTS MIDI event into internal representation
    // (only for internal use)
    //
    void processMidiIn(const Arts::MidiCommand &midiCommand,
                       const Arts::TimeStamp &timeStamp);

    // aRTS sound server reference
    //
    Arts::SoundServerV2      m_soundServer;

    // aRTS MIDI devices
    //
    Arts::MidiManager        m_midiManager;
    Arts::Dispatcher         m_dispatcher;
    Arts::MidiClient         m_midiPlayClient;
    Arts::MidiClient         m_midiRecordClient;
    RosegardenMidiRecord     m_midiRecordPort;
    Arts::MidiPort           m_midiPlayPort;

    // aRTS Audio devices
    //
    Arts::Synth_AMAN_PLAY    m_amanPlay;
    Arts::Synth_AMAN_RECORD  m_amanRecord;
    Arts::Synth_CAPTURE_WAV  m_captureWav;
    Arts::Synth_PLAY_WAV     m_playWav;

    // TimeStamps mark the real world (aRts) times of the start
    // of play or record.  These are for internal use when
    // calculating how to queue up playback.
    //
    Arts::TimeStamp        m_playStartTime;
    Arts::TimeStamp        m_recordStartTime;
    NoteOffQueue           m_noteOffQueue;

    // Absolute time playback start position
    Rosegarden::RealTime   m_playStartPosition;

    // Current recording status
    RecordStatus           m_recordStatus;

    // Playback flags
    bool                   m_startPlayback;
    bool                   m_playing;

    MappedComposition      m_recordComposition;

    std::map<unsigned int, MappedEvent*> m_noteOnMap;

    // State of the sequencer
    //
    SequencerStatus        m_sequencerStatus;

};


}
 

#endif // _ROSEGARDEN_SEQUENCER_H_
