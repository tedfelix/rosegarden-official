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
#include "Composition.h"
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

    // get a vector of recorded events from aRTS
    inline vector<Arts::MidiEvent>* getMidiQueue()
      { return _midiRecordPort.getQueue(); }

    // Our current recording status.  Asynchronous record states
    // mean that we're just accepting asynchronous events but not
    // recording them - they are forwarded onto the GUI for level/
    // activity display purposes.
    //
    RecordStatus recordStatus() { return _recordStatus; }
    
    inline bool isPlaying() { return _playing; }

    // set and get tempo
    const unsigned int tempo() const { return _tempo; }
    void tempo(const unsigned int &tempo) { _tempo = tempo; }

    // resolution - required?
    const unsigned int resolution() const { return _ppq; }

    // update the song position to the current time according
    // to the MIDI Timers
    void updateSongPosition();

    const unsigned int songPosition() const { return _songPosition; }

    // get the difference in TimeStamps
    Arts::TimeStamp deltaTime(const Arts::TimeStamp &ts1,
                              const Arts::TimeStamp &ts2);

    // get the TimeStamp from the beginning of recording
    inline Arts::TimeStamp recordTime(Arts::TimeStamp const &ts)
      { return (deltaTime(ts, _recordStartTime)); }

    // Perform conversion from seconds and microseconds (TimeStamp)
    // to our internal MIDI clock representation.  Make sure you do
    // this from the normalised time (i.e. working from zero).
    //
    inline unsigned int convertToAbsoluteTime(const Arts::TimeStamp &timeStamp)
    {
      return (unsigned int) ( 384.0 * ( (double) timeStamp.sec +
                              ( ( (double) timeStamp.usec ) / 1000000.0 ) ) *
                                  (double) _tempo / 60.0 );
    }

    // process a raw aRTS MIDI event into internal representation
    void processMidiIn(const Arts::MidiCommand &midiCommand,
                      const Arts::TimeStamp &timeStamp);

    // Process a chunk of Rosegarden Composition into MIDI events and send
    // them to aRTS.
    void processMidiOut(const Rosegarden::Composition &composition);

  private:

    void initializeMidi();

    Arts::Dispatcher _dispatcher;
    Arts::SoundServer _soundServer;
    Arts::MidiManager _midiManager;

    Arts::MidiClient _midiPlayClient;
    Arts::MidiClient _midiRecordClient;
    RosegardenMidiRecord _midiRecordPort;
    Arts::MidiPort _midiPlayPort;

    // A distinction - songPositions are in MIDI clocks
    // from the beginning of the composition..
    //
    unsigned int _songPosition;
    unsigned int _songPlayPosition;
    unsigned int _songRecordPosition;

    // TimeStamps mark the actual times of the commencement
    // of play or record.  These are for internal use only.
    Arts::TimeStamp _playStartTime;
    Arts::TimeStamp _recordStartTime;

    RecordStatus _recordStatus;

    bool _playing;

    unsigned int _ppq;   // sequencer resolution
    unsigned int _tempo; // Beats Per Minute

    Rosegarden::Track *_recordTrack;

    map<unsigned int, Event*> _noteOnMap;

  };

}

#endif // _ROSEGARDEN_SEQUENCER_H_
