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
#include "Event.h"

// MidiEvent holds MIDI and Rosegarden::Event data during MIDI file I/O.
// We don't use this class at all for playback or recording of MIDI -
// for that look at MappedEvent and MappedComposition.
//
// Rosegarden doesn't have any internal concept of MIDI events, only
// Rosegarden::Events which are a superset of MIDI functionality.
//
// Check out Rosegarden::Event in base/ for more information.
//
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
    MidiEvent(const Rosegarden::timeT &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &data1);

    // double data byte
    //
    MidiEvent(const Rosegarden::timeT &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &data1,
              const MidiByte &data2);

    // meta event
    //
    MidiEvent(const Rosegarden::timeT &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &metaEventCode,
              const std::string &metaMessage);

    ~MidiEvent();

    void print();

    Rosegarden::timeT getTime() const { return m_deltaTime; }
    Rosegarden::timeT addTime(const Rosegarden::timeT &time);
    void setTime(const Rosegarden::timeT &time) { m_deltaTime = time; }

    inline const MidiByte getMessageType()
        { return ( m_eventCode & MIDI_MESSAGE_TYPE_MASK ); }

    inline const MidiByte getChannelNumber()
        { return ( m_eventCode & MIDI_CHANNEL_NUM_MASK ); }

    inline const MidiByte getEventCode() { return m_eventCode; }

    inline const MidiByte getPitch() { return m_data1; }
    inline const MidiByte getVelocity() { return m_data2; }

    // Just so we don't have to call them note and vely
    // for things that aren't.
    //
    inline const MidiByte getData1() { return m_data1; }
    inline const MidiByte getData2() { return m_data2; }

    inline const bool isMeta() { return(m_eventCode == MIDI_FILE_META_EVENT); }

    inline std::string getMetaMessage() const { return m_metaMessage; }
    inline const MidiByte getMetaEventCode() { return m_metaEventCode; }

    Rosegarden::timeT getDuration() const { return m_duration; }
    void setDuration(const Rosegarden::timeT& duration) {m_duration = duration;}

    friend bool operator<(const MidiEvent &a, const MidiEvent &b);

private:

    MidiEvent& operator=(const MidiEvent);

    Rosegarden::timeT m_deltaTime;
    Rosegarden::timeT m_duration;
    MidiByte          m_eventCode;
    MidiByte          m_data1;         // or Note
    MidiByte          m_data2;         // or Velocity

    MidiByte          m_metaEventCode;
    std::string       m_metaMessage;

};

}

#endif // _ROSEGARDEN_MIDI_EVENT_H_
