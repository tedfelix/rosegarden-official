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

#ifndef _MAPPEDEVENT_H_
#define _MAPPEDEVENT_H_

// Used as a transformation stage between Composition, Events and output
// at the Sequencer this class and MidiComposition eliminate the notion
// of the Segment and Track for ease of Event access.  The MappedEvents
// are ready for playing or routing through an Instrument or Effects
// boxes.
//
// MappedEvents can also represent instructions for playback of audio
// samples - if the m_type is Audio then the sequencer will attempt to
// map the Pitch (m_data1) to the audio id.
// 
// The MappedEvent/Instrument relationship is interesting - we don't
// want to duplicate the entire Instrument at the Sequencer level as
// it'd be messy and unnecessary.  Instead we use a MappedInstrument
// which is just a very cut down Sequencer-side version of an Instrument.
//
//

#include "Composition.h" // for Rosegarden::RealTime
#include "Event.h"
#include <multiset.h>
#include <qdatastream.h>

namespace Rosegarden
{

class MappedEvent
{
public:
    typedef enum
    {
        MidiNote,
        MidiProgramChange,
        MidiKeyPressure,
        MidiChannelPressure,
        MidiPitchWheel,
        MidiController,
        Audio
    } MappedEventType;

    MappedEvent(): m_instrument(0),
                   m_type(MidiNote),
                   m_data1(0),
                   m_data2(0),
                   m_eventTime(0, 0),
                   m_duration(0, 0),
                   m_audioStartMarker(0, 0) {;}

    // Construct from Events to Internal (MIDI) type MappedEvent
    //
    MappedEvent(const Event &e);

    // Another Internal constructor from Events
    MappedEvent(InstrumentId id,
                const Event &e,
                const Rosegarden::RealTime &eventTime,
                const Rosegarden::RealTime &duration);

    // A shortcut for creating MIDI/Internal MappedEvents
    // from base properties
    //
    MappedEvent(InstrumentId id,
                MidiByte pitch,
                MidiByte velocity,
                const Rosegarden::RealTime &absTime,
                const Rosegarden::RealTime &duration):
        m_instrument(id),
        m_type(MidiNote),
        m_data1(pitch),
        m_data2(velocity),
        m_eventTime(absTime),
        m_duration(duration),
        m_audioStartMarker(Rosegarden::RealTime(0,0)) {;}

    // A general MappedEvent constructor for any MappedEvent type
    //
    MappedEvent(InstrumentId id,
                MappedEventType type,
                MidiByte pitch,
                MidiByte velocity,
                const RealTime &absTime,
                const RealTime &duration,
                const RealTime &audioStartMarker):
        m_instrument(id),
        m_type(type),
        m_data1(pitch),
        m_data2(velocity),
        m_eventTime(absTime),
        m_duration(duration),
        m_audioStartMarker(audioStartMarker) {;}

    // Audio MappedEvent shortcut constructor
    //
    MappedEvent(InstrumentId id,
                MidiByte audioID,
                const Rosegarden::RealTime &eventTime,
                const Rosegarden::RealTime &duration,
                const Rosegarden::RealTime &audioStartMarker):
         m_instrument(id),
         m_type(Audio),
         m_data1(audioID),
         m_data2(0),
         m_eventTime(eventTime),
         m_duration(duration),
         m_audioStartMarker(audioStartMarker) {;}

    // More generalised MIDI event containers for
    // large and small events (one param, two param)
    //
    MappedEvent(InstrumentId id,
                MappedEventType type,
                MidiByte data1,
                MidiByte data2):
         m_instrument(id),
         m_type(type),
         m_data1(data1),
         m_data2(data2),
         m_eventTime(Rosegarden::RealTime(0, 0)),
         m_duration(Rosegarden::RealTime(0, 0)),
         m_audioStartMarker(Rosegarden::RealTime(0, 0)) {;}

    MappedEvent(InstrumentId id,
                MappedEventType type,
                MidiByte data1):
        m_instrument(id),
        m_type(type),
        m_data1(data1),
        m_data2(0),
        m_eventTime(Rosegarden::RealTime(0, 0)),
        m_duration(Rosegarden::RealTime(0, 0)),
        m_audioStartMarker(Rosegarden::RealTime(0, 0)) {;}
                

    // Copy constructor
    //
    MappedEvent(const MappedEvent &mE):
        m_instrument(getInstrument()),
        m_type(getType()),
        m_data1(mE.getData1()),
        m_data2(getData2()),
        m_eventTime(mE.getEventTime()),
        m_duration(getDuration()),
        m_audioStartMarker(getAudioStartMarker()) {;}
                
    ~MappedEvent() {;}

    // Event time
    //
    void setEventTime(const Rosegarden::RealTime &a) { m_eventTime = a; }
    Rosegarden::RealTime getEventTime() const { return m_eventTime; }

    // Duration
    //
    void setDuration(const Rosegarden::RealTime &d) { m_duration = d; }
    Rosegarden::RealTime getDuration() const { return m_duration; }

    // Instrument
    void setInstrument(InstrumentId id) { m_instrument = id; }
    InstrumentId getInstrument() const { return m_instrument; }

    MidiByte getPitch() const { return m_data1; }

    // Keep pitch within MIDI limits
    //
    void setPitch(MidiByte p)
    {
        m_data1 = p;
        if (m_data1 > MidiMaxValue) m_data1 = MidiMaxValue;
    }

    void setVelocity(MidiByte v) { m_data2 = v; }
    MidiByte getVelocity() const { return m_data2; }

    // And the trendy names for them
    //
    MidiByte getData1() const { return m_data1; }
    MidiByte getData2() const { return m_data2; }

    // Also use the pitch as the Audio file ID
    //
    void setAudioID(MidiByte id) { m_data1 = id; }
    int getAudioID() const { return m_data1; }

    // A sample doesn't have to be played from the beginning.  When
    // passing an Audio event this value may be set to indicate from
    // where in the sample it should be played.  Duration is measured
    // against total sounding length (not absolute position).
    //
    //
    void setAudioStartMarker(const Rosegarden::RealTime &aS)
        { m_audioStartMarker = aS; }
    Rosegarden::RealTime getAudioStartMarker() const
        { return m_audioStartMarker; }

    MappedEventType getType() const { return m_type; }
    void setType(const MappedEventType &value) { m_type = value; }
    
    // How MappedEvents are ordered in the MappedComposition
    //
    struct MappedEventCmp
    {
        bool operator()(const MappedEvent *mE1, const MappedEvent *mE2) const
        {
            return *mE1 < *mE2;
        }
    };

    friend bool operator<(const MappedEvent &a, const MappedEvent &b);

private:
    InstrumentId     m_instrument;
    MappedEventType  m_type;
    MidiByte         m_data1;
    MidiByte         m_data2;
    RealTime         m_eventTime;
    RealTime         m_duration;
    RealTime         m_audioStartMarker;

};

}

#endif
