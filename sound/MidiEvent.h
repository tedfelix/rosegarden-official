// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.1
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
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


#ifndef _ROSEGARDEN_MIDI_EVENT_H_
#define _ROSEGARDEN_MIDI_EVENT_H_

#include "Midi.h"

// We use this class to hold our MIDI data before and after file access.
// This is our stepping stone between the Application and the MIDI file.  
//
//

namespace Rosegarden
{
class MidiEvent
{

public:
    MidiEvent();

    // single data byte case
    //
    MidiEvent(const unsigned int &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &data1);

    // double data byte
    //
    MidiEvent(const unsigned int &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &data1,
              const MidiByte &data2);

    // meta event
    //
    MidiEvent(const unsigned int &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &metaEventCode,
              const std::string &metaMessage);

    ~MidiEvent();

    void print();

    unsigned int time() const { return m_deltaTime; }
    unsigned int addTime(const unsigned int &time);
    void setTime(const unsigned int &time) { m_deltaTime = time; }

    inline const MidiByte messageType()
    { return ( m_eventCode & MIDI_MESSAGE_TYPE_MASK ); }

    inline const MidiByte channelNumber()
    { return ( m_eventCode & MIDI_CHANNEL_NUM_MASK ); }

    inline const MidiByte eventCode() { return m_eventCode; }

    inline const MidiByte note() { return m_data1; }
    inline const MidiByte velocity() { return m_data2; }

    // Just so we don't have to call them note and vely
    // for things that they're not
    //
    inline const MidiByte data1() { return m_data1; }
    inline const MidiByte data2() { return m_data2; }

    inline const bool isMeta()
    { return (m_eventCode == MIDI_FILE_META_EVENT ? true : false ); }

    inline std::string metaMessage() const { return m_metaMessage; }
    inline const MidiByte metaEventCode() { return m_metaEventCode; }

    void duration(const unsigned int& duration)
    { m_duration = duration; }

    const unsigned int& duration() { return m_duration; }

    friend bool operator<(const MidiEvent &a, const MidiEvent &b);


private:

    MidiEvent& operator=(const MidiEvent mE) {;}

    unsigned int m_deltaTime;
    unsigned int m_duration;
    MidiByte     m_eventCode;
    MidiByte     m_data1;         // or Note
    MidiByte     m_data2;         // or Velocity

    MidiByte     m_metaEventCode;
    std::string  m_metaMessage;
    

};
}

#endif // _ROSEGARDEN_MIDI_EVENT_H_
