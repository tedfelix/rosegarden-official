/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2018 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include "MidiEvent.h"

#include "Midi.h"
#include "misc/Debug.h"

namespace Rosegarden
{


MidiEvent::MidiEvent() :
    m_time(0),
    m_duration(0),
    m_eventCode(0),
    m_data1(0),
    m_data2(0),
    m_metaEventCode(0),
    m_metaMessage()
{
}

MidiEvent::MidiEvent(timeT time,
                     MidiByte eventCode,
                     MidiByte data1):
        m_time(time),
        m_duration(0),
        m_eventCode(eventCode),
        m_data1(data1),
        m_data2(0),
        m_metaEventCode(0),
        m_metaMessage("")
{
}

MidiEvent::MidiEvent(timeT time,
                     MidiByte eventCode,
                     MidiByte data1,
                     MidiByte data2):
        m_time(time),
        m_duration(0),
        m_eventCode(eventCode),
        m_data1(data1),
        m_data2(data2),
        m_metaEventCode(0),
        m_metaMessage("")
{
}

MidiEvent::MidiEvent(timeT time,
                     MidiByte eventCode,
                     MidiByte metaEventCode,
                     const std::string &metaMessage):
        m_time(time),
        m_duration(0),
        m_eventCode(eventCode),
        m_data1(0),
        m_data2(0),
        m_metaEventCode(metaEventCode),
        m_metaMessage(metaMessage)
{
}

MidiEvent::MidiEvent(timeT time,
                     MidiByte eventCode,
                     const std::string &sysEx):
        m_time(time),
        m_duration(0),
        m_eventCode(eventCode),
        m_data1(0),
        m_data2(0),
        m_metaEventCode(0),
        m_metaMessage(sysEx)
{
}

QDebug &
operator<<(QDebug &dbg, const MidiEvent &midiEvent)
{
    timeT tempo;
    int tonality;
    std::string sharpflat;

    if (midiEvent.m_metaEventCode) {
        switch (midiEvent.m_metaEventCode) {
        case MIDI_SEQUENCE_NUMBER:
            dbg << "MIDI SEQUENCE NUMBER";
            break;

        case MIDI_TEXT_EVENT:
            dbg << "MIDI TEXT:\t\"" << midiEvent.m_metaMessage << '"';
            break;

        case MIDI_COPYRIGHT_NOTICE:
            dbg << "COPYRIGHT:\t\"" << midiEvent.m_metaMessage << '"';
            break;

        case MIDI_TRACK_NAME:
            dbg << "TRACK NAME:\t\"" << midiEvent.m_metaMessage << '"';
            break;

        case MIDI_INSTRUMENT_NAME:
            dbg << "INSTRUMENT NAME:\t\"" << midiEvent.m_metaMessage << '"';
            break;

        case MIDI_LYRIC:
            dbg << "LYRIC:\t\"" << midiEvent.m_metaMessage << '"';
            break;

        case MIDI_TEXT_MARKER:
            dbg << "MARKER:\t\"" << midiEvent.m_metaMessage << '"';
            break;

        case MIDI_CUE_POINT:
            dbg << "CUE POINT:\t\"" << midiEvent.m_metaMessage << '"';
            break;

            // Sets a Channel number for a TRACK before it starts
        case MIDI_CHANNEL_PREFIX:
            dbg << "CHANNEL PREFIX:\t"
                << (timeT)midiEvent.m_metaMessage[0];
            break;

            // These are actually the same case but this is not an
            // official META event - it just crops up a lot.  We
            // assume it's a MIDI_CHANNEL_PREFIX though
            //
        case MIDI_CHANNEL_PREFIX_OR_PORT:
            dbg << "FIXED CHANNEL PREFIX:\t"
                << (timeT)midiEvent.m_metaMessage[0];
            break;

        case MIDI_END_OF_TRACK:
            dbg << "END OF TRACK";
            break;

        case MIDI_SET_TEMPO:
            tempo =
                ((timeT)(((MidiByte)midiEvent.m_metaMessage[0]) << 16)) +
                ((timeT)(((MidiByte)midiEvent.m_metaMessage[1]) << 8)) +
                (short)(MidiByte)midiEvent.m_metaMessage[2];

            tempo = 60000000 / tempo;
            dbg << "SET TEMPO:\t" << tempo;
            break;

        case MIDI_SMPTE_OFFSET:
            dbg << "SMPTE TIME CODE:\t"
                << (timeT)midiEvent.m_metaMessage[0]
                << ":" << (timeT)midiEvent.m_metaMessage[1]
                << ":" << (timeT)midiEvent.m_metaMessage[2]
                << "  -  fps = " << (timeT)midiEvent.m_metaMessage[3]
                << "  - subdivsperframe = "
                << (timeT)midiEvent.m_metaMessage[4];
            break;

        case MIDI_TIME_SIGNATURE:
            dbg << "TIME SIGNATURE:\t"
                << (timeT)midiEvent.m_metaMessage[0]
                << "/"
                << (1 << (timeT)midiEvent.m_metaMessage[1]);
            break;

        case MIDI_KEY_SIGNATURE:
            tonality = (int)midiEvent.m_metaMessage[0];

            if (tonality < 0) {
                sharpflat = -tonality + " flat";
            } else {
                sharpflat = tonality;
                sharpflat += " sharp";
            }

            dbg << "KEY SIGNATURE:\t" << sharpflat << " "
                << (((int)midiEvent.m_metaMessage[1]) == 0 ? "major" : "minor");

            break;

        case MIDI_SEQUENCER_SPECIFIC:
            dbg << "SEQUENCER SPECIFIC:\t\"" << midiEvent.m_metaMessage;
            break;


        default:
            dbg << "Undefined MIDI META event - "
                << (timeT)midiEvent.m_metaEventCode;
            break;
        }
    } else {
        switch (midiEvent.m_eventCode & MIDI_MESSAGE_TYPE_MASK) {
        case MIDI_NOTE_ON:
            dbg << "NOTE ON:\t" << (int)midiEvent.m_data1 << " - "
                << (int)midiEvent.m_data2;
            break;

        case MIDI_NOTE_OFF:
            dbg << "NOTE OFF:\t" << (int)midiEvent.m_data1 << " - "
                << (int)midiEvent.m_data2;
            break;

        case MIDI_POLY_AFTERTOUCH:
            dbg << "POLY AFTERTOUCH:\t" << (int)midiEvent.m_data1
                << " - " << (int)midiEvent.m_data2;
            break;

        case MIDI_CTRL_CHANGE:
            dbg << "CTRL CHANGE:\t" << (int)midiEvent.m_data1
                << " - " << (int)midiEvent.m_data2;
            break;

        case MIDI_PITCH_BEND:
            dbg << "PITCH BEND:\t" << (int)midiEvent.m_data1
                << " - " << (int)midiEvent.m_data2;
            break;

        case MIDI_PROG_CHANGE:
            dbg << "PROG CHANGE:\t" << (int)midiEvent.m_data1;
            break;

        case MIDI_CHNL_AFTERTOUCH:
            dbg << "CHNL AFTERTOUCH\t" << (int)midiEvent.m_data1;
            break;

        default:
            dbg << "Undefined MIDI event";
            break;
        }
    }

    return dbg;
}


}
