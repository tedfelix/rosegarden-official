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


#include "Sequencer.h"
#include <iostream>
#include "MidiArts.h"
#include "MidiFile.h"
#include "Composition.h"
#include "Track.h"
#include "Event.h"

namespace Rosegarden
{

Sequencer::Sequencer():
                       _playing(false),
                       _recordStatus(ASYNCHRONOUS_MIDI),
                       _ppq(960)
{
  _songTime.usec = 0;
  _songTime.sec = 0;

  initializeMidi();
}


Sequencer::~Sequencer()
{
}

void
Sequencer::initializeMidi()
{
  _midiManager = Arts::Reference("global:Arts_MidiManager");
  if (_midiManager.isNull())
  {
    cerr << "Can't get MidiManager" << endl;
    exit(1);
  }

  _soundServer = Arts::Reference("global:Arts_SoundServer");
  if (_soundServer.isNull())
  {
    cerr << "Can't get SoundServer" << endl;
    exit(1);
  }

  _midiRecordPort = Arts::DynamicCast(_soundServer.createObject("RosegardenMidiRecord"));


  if (_midiRecordPort.isNull())
  {
    cerr << "Can't create MidiRecorder" << endl;
    exit(1);
  }

  _midiPlayClient = _midiManager.addClient(Arts::mcdPlay,
                                           Arts::mctApplication,
                                          "Rosegarden (play)","rosegarden");
  if (_midiPlayClient.isNull())
  {
    cerr << "Can't create MidiClient" << endl;
    exit(1);
  }

  _midiPlayPort = _midiPlayClient.addOutputPort();
  if (_midiPlayPort.isNull())
  {
    cerr << "Can't create Midi Output Port" << endl;
    exit(1);
  }

  _midiRecordClient = _midiManager.addClient(Arts::mcdRecord,
                                             Arts::mctApplication,
                                             "Rosegarden (record)",
                                             "rosegarden");
  if (_midiRecordClient.isNull())
  {
    cerr << "Can't create MidiRecordClient" << endl;
    exit(1);
  }

  // Create our recording midi port
  //
  _midiRecordClient.addInputPort(_midiRecordPort);

  // set MIDI thru
  //
  _midiRecordPort.setMidiThru(_midiPlayPort);

}

// Increment the song position in microseconds
// and increment the seconds value as required.
//
void
Sequencer::incrementSongPosition(long inc)
{
  _songTime.usec += inc;
  _songTime.sec += _songTime.usec / 1000000;
  _songTime.usec %= 1000000;
}

void
Sequencer::record(const RecordStatus& recordStatus)
{

  if ( recordStatus == RECORD_MIDI )
  {
    cout << "Recording MIDI" << endl;

    // turn MIDI event recording on 
    _midiRecordPort.record(true);

    // if we're already playing the just toggle recording
    // at this point, if not we jump back by the count in
    if ( !_playing )
    {
      _playing = true;
      _songTime.sec -= 2;  // arbitrary for the moment
    }

    // set status and the record start position
    _recordStatus = RECORD_MIDI;
    _recordStartTime = _midiRecordPort.time();
    _songTime = _recordStartTime;

    cout << "Recording Started at : " << _recordStartTime.sec << " : "
                                      << _recordStartTime.usec << endl;
                                     
  }
  else
  {
    cout << "Currently unsupported recording mode." << endl;
  }
  
}

void
Sequencer::play()
{
   if ( !_playing)
   {
     _playing = true;
     _playStartTime = _midiRecordPort.time();
   }
   else
   {
     // jump back to last start position and carry on playing
     _songTime = _playStartTime; 
   }
}

void
Sequencer::stop()
{
  _recordStatus = ASYNCHRONOUS_MIDI;

  if ( _playing )
  {
    // just stop
    _playing = false;
  }
  else
  {
    // if already stopped then return to zero
    _songTime.usec = 0;
    _songTime.sec = 0;
  }
}

Arts::TimeStamp
Sequencer::recordTime(Arts::TimeStamp ts)
{
  long usec = ts.usec - _recordStartTime.usec;
  long sec = ts.sec -  _recordStartTime.sec;

  if ( usec < 0 )
  {
    sec--;
    usec += 1000000;
  }

  // also should check to see if the clock has
  // cycled into negative or back to zero

  return (Arts::TimeStamp(sec, usec));
}

};
