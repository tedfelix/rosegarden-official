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


#ifndef RG_MIDI_EVENT_H
#define RG_MIDI_EVENT_H

#include "Midi.h"
#include "base/Event.h"

// MidiEvent holds MIDI and Event data during MIDI file I/O.
// We don't use this class at all for playback or recording of MIDI -
// for that look at MappedEvent and MappedEventList.
//
// Rosegarden doesn't have any internal concept of MIDI events, only
// Events which are a superset of MIDI functionality.
//
// Check out Event in base/ for more information.
//
//
//

namespace Rosegarden
{
class MidiEvent
{

public:
    MidiEvent();

    // No data event
    //
    MidiEvent(timeT deltaTime,
              MidiByte eventCode);

    // single data byte case
    //
    MidiEvent(timeT deltaTime,
              MidiByte eventCode,
              MidiByte data1);

    // double data byte
    //
    MidiEvent(timeT deltaTime,
              MidiByte eventCode,
              MidiByte data1,
              MidiByte data2);

    // Meta event
    //
    MidiEvent(timeT deltaTime,
              MidiByte eventCode,
              MidiByte metaEventCode,
              const std::string &metaMessage);

    // Sysex style constructor
    //
    MidiEvent(timeT deltaTime,
              MidiByte eventCode,
              const std::string &sysEx);


    ~MidiEvent();

    // View our event as text
    //
    void print();


    void setTime(const timeT &time) { m_deltaTime = time; }
    void setDuration(const timeT& duration) {m_duration = duration;}
    timeT addTime(const timeT &time);

    MidiByte getMessageType() const
        { return ( m_eventCode & MIDI_MESSAGE_TYPE_MASK ); }

    MidiByte getChannelNumber() const
        { return ( m_eventCode & MIDI_CHANNEL_NUM_MASK ); }

    timeT getTime() const { return m_deltaTime; }
    timeT getDuration() const { return m_duration; }

    MidiByte getPitch() const { return m_data1; }
    MidiByte getVelocity() const { return m_data2; }
    MidiByte getData1() const { return m_data1; }
    MidiByte getData2() const { return m_data2; }
    MidiByte getEventCode() const { return m_eventCode; }

    bool isMeta() const { return(m_eventCode == MIDI_FILE_META_EVENT); }

    MidiByte getMetaEventCode() const { return m_metaEventCode; }
    std::string getMetaMessage() const { return m_metaMessage; }
    void setMetaMessage(const std::string &meta) { m_metaMessage = meta; }

    friend bool operator<(const MidiEvent &a, const MidiEvent &b);

private:

    MidiEvent& operator=(const MidiEvent);

    timeT m_deltaTime;
    timeT m_duration;
    MidiByte          m_eventCode;
    MidiByte          m_data1;         // or Note
    MidiByte          m_data2;         // or Velocity

    MidiByte          m_metaEventCode;
    std::string       m_metaMessage;

};

// Comparator for sorting
//
struct MidiEventCmp
{
    bool operator()(const MidiEvent &mE1, const MidiEvent &mE2) const
                    { return mE1.getTime() < mE2.getTime(); }
    bool operator()(const MidiEvent *mE1, const MidiEvent *mE2) const
                    { return mE1->getTime() < mE2->getTime(); }
};

}

#endif // RG_MIDI_EVENT_H
