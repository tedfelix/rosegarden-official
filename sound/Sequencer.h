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
// and MIDI are initialised, playback and recording handled.
//
//

#ifndef _ROSEGARDEN_SEQUENCER_H_
#define _ROSEGARDEN_SEQUENCER_H_

#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include "MidiRecord.h"


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
    long songPositionSeconds() { return _songPosition.sec; }
    long songPositionMicroSeconds() { return _songPosition.usec; }

    // Our current recording status.  Asynchronous record states
    // mean that we're just accepting asynchronous events but not
    // recording them - they are forwarded onto the GUI for level/
    // activity purposes.
    //
    RecordStatus recordStatus() { return _recordStatus; }

    inline bool isPlaying() { return _playing; }

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
    Arts::TimeStamp _songPosition;
    Arts::TimeStamp _playStartPosition;
    Arts::TimeStamp _recordStartPosition;

    Arts::TimeStamp _playbackTime;

    RecordStatus _recordStatus;

    bool _playing;

  };

};

#endif // _ROSEGARDEN_SEQUENCER_H_
