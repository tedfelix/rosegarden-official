/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2021 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/


#ifndef RG_MIDIEVENT_H
#define RG_MIDIEVENT_H

#include "Midi.h"
#include "base/Event.h"

namespace Rosegarden
{


/// For standard MIDI file I/O.
/**
 * MidiEvent is a representation of MIDI which we use
 * for the import/export of MIDI files.  It uses std::string for
 * meta event messages which makes them nice and easy to handle.
 *
 * We don't use this class at all for playback or recording of MIDI -
 * for that look at MappedEvent and MappedEventList.
 *
 * Rosegarden doesn't have any internal concept of MIDI events, only
 * the Event class which offers a superset of MIDI functionality.
 */
class MidiEvent
{

public:
    MidiEvent();

    /// An event with one data byte.  E.g. Program Change.
    MidiEvent(timeT time,
              MidiByte eventCode,
              MidiByte data1);

    /// An event with two data bytes.  E.g. Note-On.
    MidiEvent(timeT time,
              MidiByte eventCode,
              MidiByte data1,
              MidiByte data2);

    /// Meta event
    MidiEvent(timeT time,
              MidiByte eventCode,
              MidiByte metaEventCode,
              const std::string &metaMessage);

    /// Sysex event
    MidiEvent(timeT time,
              MidiByte eventCode,
              const std::string &sysEx);

    void setTime(const timeT &time)  { m_time = time; }
    timeT getTime() const  { return m_time; }

    void setDuration(const timeT& duration) { m_duration = duration; }
    timeT getDuration() const { return m_duration; }

    MidiByte getEventCode() const { return m_eventCode; }
    MidiByte getMessageType() const
        { return m_eventCode & MIDI_MESSAGE_TYPE_MASK; }
    MidiByte getChannelNumber() const
        { return m_eventCode & MIDI_CHANNEL_NUM_MASK; }

    MidiByte getData1() const { return m_data1; }
    MidiByte getPitch() const { return m_data1; }

    MidiByte getData2() const { return m_data2; }
    MidiByte getVelocity() const { return m_data2; }

    bool isMeta() const { return (m_eventCode == MIDI_FILE_META_EVENT); }
    MidiByte getMetaEventCode() const { return m_metaEventCode; }
    void setMetaMessage(const std::string &meta) { m_metaMessage = meta; }
    std::string getMetaMessage() const { return m_metaMessage; }

    friend QDebug &operator<<(QDebug &, const MidiEvent &);

private:
    /// Delta or absolute time, depending.
    timeT m_time;
    timeT m_duration;
    MidiByte m_eventCode;
    MidiByte m_data1;
    MidiByte m_data2;

    MidiByte m_metaEventCode;
    std::string m_metaMessage;
};

QDebug &operator<<(QDebug &, const MidiEvent &);


}

#endif // RG_MIDIEVENT_H
