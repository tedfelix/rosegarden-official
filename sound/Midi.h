// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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


#ifndef _ROSEGARDEN_MIDI_H_
#define _ROSEGARDEN_MIDI_H_

#include <string>

// Yes we use the STL here.  Don't worry, it's fine.
//
//

namespace Rosegarden
{
// Within the namespace we define our static MIDI messages 
// that'll help us create and understand MIDI files.
//
// CreateMessageByte(MSG, CHANNEL)  = (MSG) | (CHANNEL)
//
//
typedef unsigned char MidiByte;

const std::string MIDI_FILE_HEADER              = "MThd";
const std::string MIDI_SEGMENT_HEADER             = "MTrk";

const MidiByte MIDI_STATUS_BYTE_MASK       = 0x80;
const MidiByte MIDI_MESSAGE_TYPE_MASK      = 0xF0;
const MidiByte MIDI_CHANNEL_NUM_MASK       = 0x0F;

// our basic MIDI messages
//
const MidiByte MIDI_NOTE_OFF               = 0x80;
const MidiByte MIDI_NOTE_ON                = 0x90;
const MidiByte MIDI_POLY_AFTERTOUCH        = 0xA0;
const MidiByte MIDI_CTRL_CHANGE            = 0xB0;
const MidiByte MIDI_PROG_CHANGE            = 0xC0;
const MidiByte MIDI_CHNL_AFTERTOUCH        = 0xD0;
const MidiByte MIDI_PITCH_BEND             = 0xE0;

// channel mode
//
const MidiByte MIDI_SELECT_CHNL_MODE       = 0xB0;

// system messages
const MidiByte MIDI_SYSTEM_MSG             = 0xF0;
const MidiByte MIDI_SYSTEM_EXCLUSIVE       = 0xF0;
const MidiByte MIDI_SONG_POSITION_PTR      = 0xF2;
const MidiByte MIDI_SONG_SELECT            = 0xF3;
const MidiByte MIDI_TUNE_REQUEST           = 0xF6;
const MidiByte MIDI_EOX                    = 0xF7;

const MidiByte MIDI_TIMING_CLOCK           = 0xF8;
const MidiByte MIDI_START                  = 0xFA;
const MidiByte MIDI_CONTINUE               = 0xFB;
const MidiByte MIDI_STOP                   = 0xFC;
const MidiByte MIDI_ACTIVE_SENSING         = 0xFE;
const MidiByte MIDI_SYSTEM_RESET           = 0xFF;

  
// Midi Event Code for META Event
const MidiByte MIDI_FILE_META_EVENT        = 0xFF;

// META Event Codes
//
const MidiByte MIDI_SEQUENCE_NUMBER        = 0x00;
const MidiByte MIDI_TEXT_EVENT             = 0x01;
const MidiByte MIDI_COPYRIGHT_NOTICE       = 0x02;
const MidiByte MIDI_SEGMENT_NAME             = 0x03;
const MidiByte MIDI_INSTRUMENT_NAME        = 0x04;
const MidiByte MIDI_LYRIC                  = 0x05;
const MidiByte MIDI_TEXT_MARKER            = 0x06;
const MidiByte MIDI_CUE_POINT              = 0x07;
const MidiByte MIDI_CHANNEL_PREFIX         = 0x20;

// There is contention over what 0x21 really means.
// It's either a miswritten CHANNEL PREFIX or it's
// a non-standard PORT MAPPING used by a sequencer.
// Either way we include it (and generally ignore it)
// as it's a part of many MIDI files that already 
// exist.
const MidiByte MIDI_CHANNEL_PREFIX_OR_PORT = 0x21;

const MidiByte MIDI_END_OF_TRACK           = 0x2F;
const MidiByte MIDI_SET_TEMPO              = 0x51;
const MidiByte MIDI_SMPTE_OFFSET           = 0x54;
const MidiByte MIDI_TIME_SIGNATURE         = 0x58;
const MidiByte MIDI_KEY_SIGNATURE          = 0x59;
const MidiByte MIDI_SEQUENCER_SPECIFIC     = 0x7F;


}


#endif // _ROSEGARDEN_MIDI_H_
