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

#include "Midi.h"
#include "MidiEvent.h"
#include <iostream>

namespace Rosegarden
{

using std::string;
using std::cout;
using std::endl;

MidiEvent::MidiEvent()
{
}

MidiEvent::MidiEvent(const unsigned int &deltaTime,
                     const MidiByte &eventCode,
                     const MidiByte &data1):
                       _deltaTime(deltaTime),
                       _eventCode(eventCode),
                       _data1(data1),
                       _data2(0x00),
                       _metaEventCode(0x00),
                       _metaMessage("")
{
}

MidiEvent::MidiEvent(const unsigned int &deltaTime,
                     const MidiByte &eventCode,
                     const MidiByte &data1,
                     const MidiByte &data2):
                       _deltaTime(deltaTime),
                       _eventCode(eventCode),
                       _data1(data1),
                       _data2(data2),
                       _metaEventCode(0x00),
                       _metaMessage("")

{
}

MidiEvent::MidiEvent(const unsigned int &deltaTime,
                     const MidiByte &eventCode,
                     const MidiByte &metaEventCode,
                     const string &metaMessage):
                       _deltaTime(deltaTime),
                       _eventCode(eventCode),
                       _data1(0x00),
                       _data2(0x00),
                       _metaEventCode(metaEventCode),
                       _metaMessage(metaMessage)

{
}

MidiEvent::~MidiEvent()
{
}

void
MidiEvent::print()
{
  unsigned int tempo;
  int tonality;
  string sharpflat;

  if (_metaEventCode)
  {
    switch(_metaEventCode)
    {
      case MIDI_SEQUENCE_NUMBER:
        cout << "MIDI SEQUENCE NUMBER" << endl;
        break;

      case MIDI_TEXT_EVENT:
        cout << "MIDI TEXT:\t\"" << _metaMessage << "\"" << endl;
        break;

      case MIDI_COPYRIGHT_NOTICE:
        cout << "COPYRIGHT:\t\"" << _metaMessage << "\"" << endl;

      case MIDI_TRACK_NAME:
        cout << "TRACK NAME:\t\"" << _metaMessage << "\"" << endl;
        break;	

      case MIDI_INSTRUMENT_NAME:
        cout << "INSTRUMENT NAME:\t\"" << _metaMessage << "\"" << endl;
        break;

      case MIDI_LYRIC:
        cout << "LYRIC:\t\"" << _metaMessage << "\"" << endl;
        break;

      case MIDI_TEXT_MARKER:
        cout << "MARKER:\t\"" << _metaMessage << "\"" << endl;
        break;

      case MIDI_CUE_POINT:
        cout << "CUE POINT:\t\"" << _metaMessage << "\"" << endl;
        break;

      // Sets a Channel number for a TRACK before it starts
      case MIDI_CHANNEL_PREFIX:
        cout << "CHANNEL PREFIX:\t" << (unsigned int)_metaMessage[0] << endl;
        break;

      // These are actually the same case but this is not an
      // official META event - it just crops up a lot.  We
      // assume it's a MIDI_CHANNEL_PREFIX though
      //
      case MIDI_CHANNEL_PREFIX_OR_PORT:
        cout << "FIXED CHANNEL PREFIX:\t"
             << (unsigned int)_metaMessage[0] << endl;
        break;

      case MIDI_END_OF_TRACK:
        cout << "END OF TRACK" << endl;
        break;

      case MIDI_SET_TEMPO:
        tempo = ((unsigned int)(((MidiByte)_metaMessage[0]) << 16)) + 
                ((unsigned int)(((MidiByte)_metaMessage[1]) << 8)) +
                (short)(MidiByte)_metaMessage[2]; 
        tempo = 60000000/tempo;
        cout << "SET TEMPO:\t" << tempo << endl;
        break;

      case MIDI_SMPTE_OFFSET:
        cout << "SMPTE TIME CODE:\t" << (unsigned int)_metaMessage[0] << ":"
                                     << (unsigned int)_metaMessage[1] << ":"
                                     << (unsigned int)_metaMessage[2] <<
                 "  -  fps = " << (unsigned int)_metaMessage[3] <<
                 "  - subdivsperframe = " << (unsigned int)_metaMessage[4] <<
                 endl;
        break;

      case MIDI_TIME_SIGNATURE:
        cout << "TIME SIGNATURE:\t" << (unsigned int)_metaMessage[0] << "/" <<
                                 (1 << (unsigned int)_metaMessage[1]) << endl;
        break;

      case MIDI_KEY_SIGNATURE:
        tonality = (int)_metaMessage[0];
        if (tonality < 0)
        {
          sharpflat = -tonality + " flat";
        }
        else
        {
          sharpflat = tonality;
          sharpflat +=  " sharp";
        }
      
        cout << "KEY SIGNATURE:\t" << sharpflat << " " <<
              (((int)_metaMessage[1]) == 0 ? "major" : "minor") << endl;

        break;

      case MIDI_SEQUENCER_SPECIFIC:
        cout << "SEQUENCER SPECIFIC:\t\"" << _metaMessage << endl;
        break;


      default:
        cout << "Undefined MIDI META event - "
             << (unsigned int)_metaEventCode << endl;
        break;
    }
  }
  else
  {
    switch(_eventCode & MIDI_MESSAGE_TYPE_MASK)
    {
      case MIDI_NOTE_ON:
        cout << "NOTE ON:\t" << (int)_data1 << " - " << (int)_data2 << endl;
        break;

      case MIDI_NOTE_OFF:
        cout << "NOTE OFF:\t" << (int)_data1 << " - " << (int)_data2 << endl;
        break;

      case MIDI_POLY_AFTERTOUCH:
        cout << "POLY AFTERTOUCH:\t" << (int)_data1 << " - " << (int)_data2
             << endl;
        break;

      case MIDI_CTRL_CHANGE:
        cout << "CTRL CHANGE:\t" << (int)_data1 << " - " << (int)_data2 << endl;
        break;

      case MIDI_PITCH_BEND:
        cout << "PITCH BEND:\t" << (int)_data1 << " - " << (int)_data2 << endl;
        break;

      case MIDI_PROG_CHANGE:
        cout << "PROG CHANGE:\t" << (int)_data1 << endl;
        break;

      case MIDI_CHNL_AFTERTOUCH:
        cout << "CHNL AFTERTOUCH\t" << (int)_data1 << endl;
        break;

      default:
        cout << "Undefined MIDI event" << endl;
        break;
    }
  }

  
  return;
}

// Adds the argument to _deltaTime and returns the result
// thus aggregating the times as we go aint
unsigned int
MidiEvent::addTime(const unsigned int &time)
{
  _deltaTime += time;
  return _deltaTime;
}


}
