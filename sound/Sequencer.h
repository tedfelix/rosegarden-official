/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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
// retreiving MIDI and audio.
//
//

#ifndef _ROSEGARDEN_SEQUENCER_H_
#define _ROSEGARDEN_SEQUENCER_H_

#include <map>
#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include "Midi.h"
#include "MidiRecord.h"
#include "MidiEvent.h"
#include "Track.h"


namespace Rosegarden
{

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

    Sequencer();
    ~Sequencer();

    // Start playing from the current song position
    //
    void play();

    // Stop playing and/or recording
    //
    void stop();

    // Active a recording state
    //
    void record(const RecordStatus& recordStatus);

    inline vector<Arts::MidiEvent>* getMidiQueue()
      { return _midiRecordPort.getQueue(); }

    inline void sendMidiEvent(const Arts::MidiEvent &midiEvent)
      { _midiPlayPort.processEvent(midiEvent); }
    
    void incrementSongPosition(long inc);
    long songPositionSeconds() { return _songTime.sec; }
    long songPositionMicroSeconds() { return _songTime.usec; }

    // Our current recording status.  Asynchronous record states
    // mean that we're just accepting asynchronous events but not
    // recording them - they are forwarded onto the GUI for level/
    // activity purposes.
    //
    RecordStatus recordStatus() { return _recordStatus; }

    inline bool isPlaying() { return _playing; }

    const unsigned int tempo() const { return _tempo; }
    const unsigned int resolution() const { return _ppq; }

    void tempo(const unsigned int &tempo) { _tempo = tempo; }

    // get the TimeStamp from the beginning of recording
    Arts::TimeStamp recordTime(Arts::TimeStamp ts);



    // processMidi - justification for two methods
    //
    // Below we have two processMidi methods for the Arts and
    // our own representation of MIDI (from files).  Arts returns
    // us a sensible absolute TimeStamp, the file gives us delta
    // times that we have to roll out.  The implementations of
    // these two methods are therefore quite different.
    //
    // Thankyou.

    // process a raw Arts MIDI event into internal representation
    void processMidi(const Arts::MidiCommand &midiCommand,
                     const Arts::TimeStamp &timeStamp);

    // process a raw MIDI file event into internal representation
    void processMidi(const Rosegarden::MidiEvent &midiEvent);


    // Perform conversion from seconds and microseconds (TimeStamp)
    // to our internal clock representation.
    //
    inline unsigned int convertToAbsoluteTime(const Arts::TimeStamp &timeStamp)
    {
      return (unsigned int) ( 384.0 * ( (double) timeStamp.sec +
                              ( ( (double) timeStamp.usec ) / 1000000.0 ) ) *
                                  (double) _tempo / 60.0 );
    }

  private:

    void initializeMidi();

    Arts::Dispatcher _dispatcher;
    Arts::SoundServer _soundServer;
    Arts::MidiManager _midiManager;

    Arts::MidiClient _midiPlayClient;
    Arts::MidiClient _midiRecordClient;
    RosegardenMidiRecord _midiRecordPort;
    Arts::MidiPort _midiPlayPort;

    // some positions in time
    //
    Arts::TimeStamp _songTime;
    Arts::TimeStamp _playStartTime;
    Arts::TimeStamp _recordStartTime;

    RecordStatus _recordStatus;

    bool _playing;

    unsigned int _ppq;   // sequencer resolution
    unsigned int _tempo; // Beats Per Minute

    Rosegarden::Track *_recordTrack;

    map<unsigned int, Event*> _noteOnMap;

  };

};

#endif // _ROSEGARDEN_SEQUENCER_H_
