/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2015 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include "MidiEvent.h"

#include "Midi.h"

#include <iostream>

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

// Show a representation of our MidiEvent purely for information
// purposes (also demos how we decode them)
void
MidiEvent::print()
{
    timeT tempo;
    int tonality;
    std::string sharpflat;

    if (m_metaEventCode) {
        switch (m_metaEventCode) {
        case MIDI_SEQUENCE_NUMBER:
            std::cout << "MIDI SEQUENCE NUMBER\n";
            break;

        case MIDI_TEXT_EVENT:
            std::cout << "MIDI TEXT:\t\"" << m_metaMessage << "\"\n";
            break;

        case MIDI_COPYRIGHT_NOTICE:
            std::cout << "COPYRIGHT:\t\"" << m_metaMessage << "\"\n";
            break;

        case MIDI_TRACK_NAME:
            std::cout << "TRACK NAME:\t\"" << m_metaMessage << "\"\n";
            break;

        case MIDI_INSTRUMENT_NAME:
            std::cout << "INSTRUMENT NAME:\t\"" << m_metaMessage << "\"\n";
            break;

        case MIDI_LYRIC:
            std::cout << "LYRIC:\t\"" << m_metaMessage << "\"\n";
            break;

        case MIDI_TEXT_MARKER:
            std::cout << "MARKER:\t\"" << m_metaMessage << "\"\n";
            break;

        case MIDI_CUE_POINT:
            std::cout << "CUE POINT:\t\"" << m_metaMessage << "\"\n";
            break;

            // Sets a Channel number for a TRACK before it starts
        case MIDI_CHANNEL_PREFIX:
            std::cout << "CHANNEL PREFIX:\t"
                 << (timeT)m_metaMessage[0]
                 << '\n';
            break;

            // These are actually the same case but this is not an
            // official META event - it just crops up a lot.  We
            // assume it's a MIDI_CHANNEL_PREFIX though
            //
        case MIDI_CHANNEL_PREFIX_OR_PORT:
            std::cout << "FIXED CHANNEL PREFIX:\t"
                 << (timeT)m_metaMessage[0] << '\n';
            break;

        case MIDI_END_OF_TRACK:
            std::cout << "END OF TRACK\n";
            break;

        case MIDI_SET_TEMPO:
            tempo =
                ((timeT)(((MidiByte)m_metaMessage[0]) << 16)) +
                ((timeT)(((MidiByte)m_metaMessage[1]) << 8)) +
                (short)(MidiByte)m_metaMessage[2];

            tempo = 60000000 / tempo;
            std::cout << "SET TEMPO:\t" << tempo << '\n';
            break;

        case MIDI_SMPTE_OFFSET:
            std::cout << "SMPTE TIME CODE:\t"
                 << (timeT)m_metaMessage[0]
                 << ":" << (timeT)m_metaMessage[1]
                 << ":" << (timeT)m_metaMessage[2]
                 << "  -  fps = " << (timeT)m_metaMessage[3]
                 << "  - subdivsperframe = "
                 << (timeT)m_metaMessage[4]
                 << '\n';
            break;

        case MIDI_TIME_SIGNATURE:
            std::cout << "TIME SIGNATURE:\t"
                 << (timeT)m_metaMessage[0]
                 << "/"
                 << (1 << (timeT)m_metaMessage[1]) << '\n';
            break;

        case MIDI_KEY_SIGNATURE:
            tonality = (int)m_metaMessage[0];

            if (tonality < 0) {
                sharpflat = -tonality + " flat";
            } else {
                sharpflat = tonality;
                sharpflat += " sharp";
            }

            std::cout << "KEY SIGNATURE:\t" << sharpflat << " "
                 << (((int)m_metaMessage[1]) == 0 ? "major" : "minor")
                 << '\n';

            break;

        case MIDI_SEQUENCER_SPECIFIC:
            std::cout << "SEQUENCER SPECIFIC:\t\"" << m_metaMessage << '\n';
            break;


        default:
            std::cout << "Undefined MIDI META event - "
                 << (timeT)m_metaEventCode << '\n';
            break;
        }
    } else {
        switch (m_eventCode & MIDI_MESSAGE_TYPE_MASK) {
        case MIDI_NOTE_ON:
            std::cout << "NOTE ON:\t" << (int)m_data1 << " - "
                 << (int)m_data2 << '\n';
            break;

        case MIDI_NOTE_OFF:
            std::cout << "NOTE OFF:\t" << (int)m_data1 << " - "
                 << (int)m_data2 << '\n';
            break;

        case MIDI_POLY_AFTERTOUCH:
            std::cout << "POLY AFTERTOUCH:\t" << (int)m_data1
                 << " - " << (int)m_data2 << '\n';
            break;

        case MIDI_CTRL_CHANGE:
            std::cout << "CTRL CHANGE:\t" << (int)m_data1
                 << " - " << (int)m_data2 << '\n';
            break;

        case MIDI_PITCH_BEND:
            std::cout << "PITCH BEND:\t" << (int)m_data1
                 << " - " << (int)m_data2 << '\n';
            break;

        case MIDI_PROG_CHANGE:
            std::cout << "PROG CHANGE:\t" << (int)m_data1 << '\n';
            break;

        case MIDI_CHNL_AFTERTOUCH:
            std::cout << "CHNL AFTERTOUCH\t" << (int)m_data1 << '\n';
            break;

        default:
            std::cout << "Undefined MIDI event\n";
            break;
        }
    }


    return ;
}


}
