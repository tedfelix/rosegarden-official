/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_MIDI_H
#define RG_MIDI_H

#include "base/Instrument.h" // for MidiByte
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

const std::string MIDI_FILE_HEADER              = "MThd";
const std::string MIDI_TRACK_HEADER             = "MTrk";

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
const MidiByte MIDI_SYSTEM_EXCLUSIVE       = 0xF0;
const MidiByte MIDI_TC_QUARTER_FRAME       = 0xF1;
const MidiByte MIDI_SONG_POSITION_PTR      = 0xF2;
const MidiByte MIDI_SONG_SELECT            = 0xF3;
const MidiByte MIDI_TUNE_REQUEST           = 0xF6;
const MidiByte MIDI_END_OF_EXCLUSIVE       = 0xF7;

const MidiByte MIDI_TIMING_CLOCK           = 0xF8;
const MidiByte MIDI_START                  = 0xFA;
const MidiByte MIDI_CONTINUE               = 0xFB;
const MidiByte MIDI_STOP                   = 0xFC;
const MidiByte MIDI_ACTIVE_SENSING         = 0xFE;
const MidiByte MIDI_SYSTEM_RESET           = 0xFF;

// System Exclusive Extensions
//

// Non-commercial use
//
const MidiByte MIDI_SYSEX_NONCOMMERCIAL    = 0x7D;

// Universal non-real time use
// Format:
//
//   0xF0 0x7E <device id> <sub id #1> <sub id #2> <data> 0xF7
//
const MidiByte MIDI_SYSEX_NON_RT           = 0x7E;

// RealTime e.g Midi Machine Control (MMC)
//
//   0xF0 0x7F <device id> <sub id #1> <sub id #2> <data> 0xF7
//
const MidiByte MIDI_SYSEX_RT               = 0x7F;

// Sub IDs for RealTime SysExs
//
const MidiByte MIDI_SYSEX_RT_COMMAND       = 0x06;
const MidiByte MIDI_SYSEX_RT_RESPONSE      = 0x07;

// MMC commands
//
const MidiByte MIDI_MMC_STOP               = 0x01;
const MidiByte MIDI_MMC_PLAY               = 0x02;
const MidiByte MIDI_MMC_DEFERRED_PLAY      = 0x03;
const MidiByte MIDI_MMC_FAST_FORWARD       = 0x04;
const MidiByte MIDI_MMC_REWIND             = 0x05;
const MidiByte MIDI_MMC_RECORD_STROBE      = 0x06; // punch in
const MidiByte MIDI_MMC_RECORD_EXIT        = 0x07; // punch out
const MidiByte MIDI_MMC_RECORD_PAUSE       = 0x08;
const MidiByte MIDI_MMC_PAUSE              = 0x08;
const MidiByte MIDI_MMC_EJECT              = 0x0A;
const MidiByte MIDI_MMC_LOCATE             = 0x44; // jump to


// Midi Event Code for META Event
//
const MidiByte MIDI_FILE_META_EVENT        = 0xFF;

// META Event Codes
//
const MidiByte MIDI_SEQUENCE_NUMBER        = 0x00;
const MidiByte MIDI_TEXT_EVENT             = 0x01;
const MidiByte MIDI_COPYRIGHT_NOTICE       = 0x02;
const MidiByte MIDI_TRACK_NAME             = 0x03;
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

// Some controllers
//
const MidiByte MIDI_CONTROLLER_BANK_MSB      = 0x00;
const MidiByte MIDI_CONTROLLER_VOLUME        = 0x07;
const MidiByte MIDI_CONTROLLER_BANK_LSB      = 0x20;
const MidiByte MIDI_CONTROLLER_MODULATION    = 0x01;
const MidiByte MIDI_CONTROLLER_PAN           = 0x0A;
const MidiByte MIDI_CONTROLLER_SUSTAIN       = 0x40;
const MidiByte MIDI_CONTROLLER_RESONANCE     = 0x47;
const MidiByte MIDI_CONTROLLER_RELEASE       = 0x48;
const MidiByte MIDI_CONTROLLER_ATTACK        = 0x49;
const MidiByte MIDI_CONTROLLER_FILTER        = 0x4A;
const MidiByte MIDI_CONTROLLER_REVERB        = 0x5B;
const MidiByte MIDI_CONTROLLER_CHORUS        = 0x5D;

// Registered and Non-Registered Parameter Controllers
//
const MidiByte MIDI_CONTROLLER_NRPN_1        = 0x62;
const MidiByte MIDI_CONTROLLER_NRPN_2        = 0x63;
const MidiByte MIDI_CONTROLLER_RPN_1         = 0x64;
const MidiByte MIDI_CONTROLLER_RPN_2         = 0x65;

const MidiByte MIDI_CONTROLLER_SOUNDS_OFF    = 0x78;
const MidiByte MIDI_CONTROLLER_RESET         = 0x79; // reset all controllers
const MidiByte MIDI_CONTROLLER_LOCAL         = 0x7A; // 0 = off, 127 = on
const MidiByte MIDI_CONTROLLER_ALL_NOTES_OFF = 0x7B;


// MIDI percussion channel
const MidiByte MIDI_PERCUSSION_CHANNEL     = 9;



}


#endif // RG_MIDI_H
