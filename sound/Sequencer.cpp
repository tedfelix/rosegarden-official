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
#include "NotationTypes.h"
#include <MappedComposition.h>

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

Sequencer::Sequencer():
                       _playStartTime(0, 0),
                       _recordStartTime(0, 0),
                       _recordStatus(ASYNCHRONOUS_MIDI),
                       _startPlayback(true),
                       _playing(false),
                       _ppq(Note(Note::Crotchet).getDuration()),
                       _tempo(120),
                       _recordTrack(0)
{
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
    cerr << "RosegardenSequencer - Can't get aRTS MidiManager" << endl;
    exit(1);
  }

  _soundServer = Arts::Reference("global:Arts_SoundServer");
  if (_soundServer.isNull())
  {
    cerr << "RosegardenSequencer - Can't get aRTS SoundServer" << endl;
    exit(1);
  }

  _midiRecordPort = Arts::DynamicCast(_soundServer.createObject("RosegardenMidiRecord"));


  if (_midiRecordPort.isNull())
  {
    cerr << "RosegardenSequencer - Can't create aRTS MidiRecorder" << endl;
    exit(1);
  }

  _midiPlayClient = _midiManager.addClient(Arts::mcdPlay,
                                           Arts::mctApplication,
                                          "Rosegarden (play)","rosegarden");
  if (_midiPlayClient.isNull())
  {
    cerr << "RosegardenSequencer - Can't create aRTS MidiClient" << endl;
    exit(1);
  }

  _midiPlayPort = _midiPlayClient.addOutputPort();
  if (_midiPlayPort.isNull())
  {
    cerr << "RosegardenSequencer - Can't create aRTS Midi Output Port" << endl;
    exit(1);
  }

  _midiRecordClient = _midiManager.addClient(Arts::mcdRecord,
                                             Arts::mctApplication,
                                             "Rosegarden (record)",
                                             "rosegarden");
  if (_midiRecordClient.isNull())
  {
    cerr << "RosegardenSequencer - Can't create aRTS MidiRecordClient" << endl;
    exit(1);
  }

  // Create our recording midi port
  //
  _midiRecordClient.addInputPort(_midiRecordPort);

  // set MIDI thru
  //
  _midiRecordPort.setMidiThru(_midiPlayPort);

}

void
Sequencer::record(const RecordStatus& recordStatus)
{

  if ( recordStatus == RECORD_MIDI )
  {
    cout << "Recording MIDI" << endl;

    // turn MIDI event recording on 
    _midiRecordPort.record(true);

    // if we're already playing then just toggle recording
    // at this point, if not we jump back by the count in
/*
    if ( !_playing )
      play();
*/

    // set status and the record start position
    _recordStatus = RECORD_MIDI;
    _recordStartTime = _midiRecordPort.time();

    cout << "Recording Started at : " << _recordStartTime.sec << " : "
                                      << _recordStartTime.usec << endl;
                                     
    _recordTrack = new Track;
    _recordTrack->setInstrument(1);
    _recordTrack->setStartIndex(0);

  }
  else
  {
    cout << "Currently unsupported recording mode." << endl;
  }
  
}

Arts::TimeStamp
Sequencer::deltaTime(const Arts::TimeStamp &ts1, const Arts::TimeStamp &ts2)
{
  int usec = ts1.usec - ts2.usec;
  int sec = ts1.sec - ts2.sec;

  if ( usec < 0 )
  {
    sec--;
    usec += 1000000;
  }

  assert( sec >= 0 );

  return (Arts::TimeStamp(sec, usec));
}

Arts::TimeStamp
Sequencer::aggregateTime(const Arts::TimeStamp &ts1, const Arts::TimeStamp &ts2)
{
  int usec = ts1.usec + ts2.usec;
  int sec = ( usec / 1000000 ) + ts1.sec + ts2.sec;
  usec %= 1000000;
  return(Arts::TimeStamp(sec, usec));
}


void
Sequencer::processMidiIn(const Arts::MidiCommand &midiCommand,
                         const Arts::TimeStamp &timeStamp)
{
  Rosegarden::MidiByte channel;
  Rosegarden::MidiByte message;
  //Rosegarden::Event *event;

  if (_recordTrack == 0)
  {
    cerr << "RosegardenSequencer - no Track created to processMidi on to - is recording enabled?" << endl;
    exit(1);
  }

  channel = midiCommand.status & MIDI_CHANNEL_NUM_MASK;
  message = midiCommand.status & MIDI_MESSAGE_TYPE_MASK;

  // Check for a hidden NOTE OFF (NOTE ON with zero velocity)
  if ( message == MIDI_NOTE_ON && midiCommand.data2 == 0 )
  {
    message = MIDI_NOTE_OFF;
  }

  // we use a map of Notes and this is the key
  unsigned int chanNoteKey = ( channel << 8 ) + midiCommand.data1;
  //double absoluteSecs;
  int duration;

  // scan for our event
  switch(message)
  {
    case MIDI_NOTE_ON:
      if ( _noteOnMap[chanNoteKey] == 0 )
      {
        _noteOnMap[chanNoteKey] = new Event;

        // set time since recording started in Absolute internal time
        _noteOnMap[chanNoteKey]->
            setAbsoluteTime(convertToMidiTime(timeStamp));

        // set note type and pitch
        _noteOnMap[chanNoteKey]->setType(Note::EventType);
        _noteOnMap[chanNoteKey]->set<Int>("pitch", midiCommand.data1);
      }
      break;

    case MIDI_NOTE_OFF:
      // if we match an open NOTE_ON
      //
      if ( _noteOnMap[chanNoteKey] != 0 )
      {
        duration = convertToMidiTime(timeStamp) -
                   _noteOnMap[chanNoteKey]->getAbsoluteTime();

        // for the moment, ensure we're positive like this
        //
        assert(duration >= 0);

        // set the duration
        _noteOnMap[chanNoteKey]->setDuration(duration);

        // insert the record
        //
        _recordTrack->insert(_noteOnMap[chanNoteKey]);

        // tell us about it
        cout << "INSERTED NOTE at time " 
             << _noteOnMap[chanNoteKey]->getAbsoluteTime()
             << " of duration "
             << _noteOnMap[chanNoteKey]->getDuration() << endl;

        // reset the reference
        _noteOnMap[chanNoteKey] = 0;

      }
      else
        cout << "MIDI_NOTE_OFF with no matching MIDI_NOTE_ON" << endl;
      break;

    default:
      cout << "OTHER EVENT" << endl;
      break;
  }
}


void
Sequencer::processMidiOut(Rosegarden::MappedComposition mappedComp,
                          const timeT &playLatency)
{
  Arts::MidiEvent event;

  // MidiCommandStatus
  //   mcsCommandMask
  //   mcsChannelMask
  //   mcsNoteOff
  //   mcsNoteOn
  //   mcsKeyPressure
  //   mcsParameter
  //   mcsProgram 
  //   mcsChannelPressure
  //   mcsPitchWheel

  // for the moment hardcode the channel
  MidiByte channel = 0;

  unsigned int midiRelativeTime = 0;
  unsigned int midiRelativeStopTime = 0;

  // get current port time at start of playback
  if (_startPlayback)
  {
    _playStartTime = _midiPlayPort.time();
    _startPlayback = false;
    _playing = true;
  }


  for ( MappedComposition::iterator i = mappedComp.begin();
                                    i != mappedComp.end(); ++i )
  {
    // sort out the correct TimeStamp for playback
    assert((*i)->getAbsoluteTime() >= _playStartPosition);

    // add the fiddle factor for timeT to MIDI conversion in here
    midiRelativeTime = convertToMidiTime((*i)->getAbsoluteTime() -
                                               _playStartPosition +
                                               playLatency);


    event.time = aggregateTime(_playStartTime,
                               convertToArtsTimeStamp(midiRelativeTime));

    midiRelativeStopTime = midiRelativeTime +
                           convertToMidiTime((*i)->getDuration());
    
    // load the command structure
    event.command.status = Arts::mcsNoteOn | channel;
    event.command.data1 = (*i)->getPitch();   // pitch
    event.command.data2 = 127;  // hardcode velocity

    // Test our timing
    Arts::TimeStamp now = _midiPlayPort.time();
    int secAhead = event.time.sec - now.sec;
    int uSecAhead = event.time.usec - now.usec;

    if (uSecAhead < 0) 
    {
      secAhead--;
      uSecAhead += 1000000;
    }

    if (secAhead < 0)
    {
      std::cerr << "Failed to process NOTE events in time - lagging by "
                << secAhead << "s and " << uSecAhead << "ms" << endl;
    }

    // if a NOTE ON
    // send the event out
    _midiPlayPort.processEvent(event);

    int secFromStart = event.time.sec - _playStartTime.sec;
    int usecFromStart = event.time.usec - _playStartTime.usec;

    if (usecFromStart < 0)
    {
      secFromStart--;
      usecFromStart += 1000000;
    }

    cout << "Event sent to aRts at " << secFromStart << "s & "
         << usecFromStart << "ms" << endl;

    // and log it on the Note OFF stack
    NoteOffEvent *noteOffEvent =
           new NoteOffEvent(midiRelativeStopTime,
                            (Rosegarden::MidiByte)event.command.data1,
                            (Rosegarden::MidiByte)event.command.status);

    _noteOffQueue.insert(noteOffEvent);

  }

  // If there's no midiRelativeTime set then set one
  //
  if (midiRelativeTime == 0)
  {
    Arts::TimeStamp now = _midiPlayPort.time();
    int sec = now.sec - _playStartTime.sec;
    int usec = now.usec - _playStartTime.usec;
 
    if (usec < 0)
    {
      sec--;
      usec += 1000000;
    }

    Arts::TimeStamp relativeNow(sec, usec);
    midiRelativeTime = convertToMidiTime(relativeNow);
  }

  // Process NOTE OFFs for current time
  processNotesOff(midiRelativeTime);
}


void
Sequencer::processNotesOff(unsigned int midiTime)
{
  Arts::MidiEvent event;
  MidiByte channel = 0;

  // process any pending NOTE OFFs
  for ( NoteOffQueue::iterator i = _noteOffQueue.begin();
                               i != _noteOffQueue.end(); ++i )

  {
    // If there's a pregnant NOTE OFF around then send it
    if ((*i)->getMidiTime() <= midiTime)
    {
      //event.time = _midiPlayPort.time();
      event.time = aggregateTime(_playStartTime,
                              convertToArtsTimeStamp((*i)->getMidiTime()));
      event.command.data1 = (*i)->getPitch();
      event.command.data2 = 127;
      event.command.status = Arts::mcsNoteOff | channel;
      _midiPlayPort.processEvent(event);
    }
  }
}

// Force all pending note offs to stop immediately
//
void
Sequencer::allNotesOff()
{
  Arts::MidiEvent event;
  MidiByte channel = 0;

  for ( NoteOffQueue::iterator i = _noteOffQueue.begin();
                               i != _noteOffQueue.end(); ++i )
  {
      event.time = _midiPlayPort.time();
      event.command.data1 = (*i)->getPitch();
      event.command.data2 = 127;
      event.command.status = Arts::mcsNoteOff | channel;
      _midiPlayPort.processEvent(event);
  }
}

void 
Sequencer::initializePlayback(const timeT &position)
{
  _startPlayback = true;
  _playStartTime.sec = 0;
  _playStartTime.usec = 0;
  _playStartPosition = position;
}

void
Sequencer::stopPlayback()
{
  allNotesOff();
  _playing = false;
}

Rosegarden::timeT
Sequencer::getSequencerTime()
{
  if (_playing)
  {
    Arts::TimeStamp artsTimeNow = _midiPlayPort.time();
    unsigned int midiTimeRelative = convertToMidiTime(artsTimeNow) -
                                    convertToMidiTime(_playStartTime);
    return (_playStartPosition + convertToGuiTime(midiTimeRelative));
  }

  return (0);
}

}
