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
// map the Pitch (m_pitch) to the audio id.
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

typedef unsigned int velocityT;

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

    MappedEvent(): m_pitch(0),
                   m_eventTime(0, 0),
                   m_duration(0, 0),
                   m_audioStartMarker(0, 0),
                   m_velocity(0),
                   m_type(MidiNote) {;}

    // Construct from Events to Internal (MIDI) type MappedEvent
    //
    MappedEvent(const Event &e);

    // Another Internal constructor from Events
    MappedEvent(const Event &e,
                const Rosegarden::RealTime &eventTime,
                const Rosegarden::RealTime &duration,
                const Rosegarden::InstrumentId &instrument);

    // A shortcut for creating MIDI/Internal MappedEvents
    // from base properties
    //
    MappedEvent(const int &pitch,
                const Rosegarden::RealTime &absTime,
                const Rosegarden::RealTime &duration,
                const velocityT &velocity,
                const Rosegarden::InstrumentId &instrument):
        m_pitch(pitch),
        m_eventTime(absTime),
        m_duration(duration),
        m_audioStartMarker(Rosegarden::RealTime(0,0)),
        m_velocity(velocity),
        m_type(MidiNote),
        m_instrument(instrument) {;}

    // A general MappedEvent constructor for any MappedEvent type
    //
    MappedEvent(const int &pitch,
                const Rosegarden::RealTime &absTime,
                const Rosegarden::RealTime &duration,
                const Rosegarden::RealTime &audioStartMarker,
                const velocityT &velocity,
                const Rosegarden::InstrumentId &instrument,
                const MappedEventType &type):
        m_pitch(pitch),
        m_eventTime(absTime),
        m_duration(duration),
        m_audioStartMarker(audioStartMarker),
        m_velocity(velocity),
        m_type(type),
        m_instrument(instrument) {;}

    // Audio MappedEvent shortcut constructor
    //
    MappedEvent(const Rosegarden::RealTime &eventTime,
                const Rosegarden::RealTime &duration,
                const Rosegarden::RealTime &audioStartMarker,
                const Rosegarden::InstrumentId &instrument,
                const int &id):
         m_pitch(id),
         m_eventTime(eventTime),
         m_duration(duration),
         m_audioStartMarker(audioStartMarker),
         m_type(Audio),
         m_instrument(instrument) {;}

    // More generalised MIDI event containers for
    // large and small events (one param, two param)
    //
    MappedEvent(InstrumentId instrument,
                MappedEventType type,
                MidiByte data1,
                MidiByte data2):
         m_pitch(data1),
         m_eventTime(Rosegarden::RealTime(0, 0)),
         m_duration(Rosegarden::RealTime(0, 0)),
         m_audioStartMarker(Rosegarden::RealTime(0, 0)),
         m_velocity(data2),
         m_type(type),
         m_instrument(instrument) {;}

    MappedEvent(InstrumentId instrument,
                MappedEventType type,
                MidiByte data1):
        m_pitch(data1),
        m_eventTime(Rosegarden::RealTime(0, 0)),
        m_duration(Rosegarden::RealTime(0, 0)),
        m_audioStartMarker(Rosegarden::RealTime(0, 0)),
        m_velocity(0),
        m_type(type),
        m_instrument(instrument) {;}
                

    // Copy constructor
    //
    MappedEvent(const MappedEvent &mE):
        m_pitch(mE.getPitch()),
        m_eventTime(mE.getEventTime()),
        m_duration(getDuration()),
        m_audioStartMarker(getAudioStartMarker()),
        m_velocity(getVelocity()),
        m_type(getType()),
        m_instrument(getInstrument()) {;}
                
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
    void setInstrument(const InstrumentId &id) { m_instrument = id; }
    InstrumentId getInstrument() const { return m_instrument; }

    // Velocity
    //
    void setVelocity(const velocityT &v) { m_velocity = v; }
    velocityT getVelocity() const { return m_velocity; }

    // Pitch - keep with MIDI limits when setting
    //
    void setPitch(const int &p)
    {
        m_pitch = p;
        if (m_pitch < 0) m_pitch = 0;
        if (m_pitch > 127) m_pitch = 127;
    }

    int getPitch() const { return m_pitch; }

    // Also use the pitch as the Audio file ID
    //
    void setAudioID(const int &id) { m_pitch = id; }
    int getAudioID() const { return m_pitch; }

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

    // The type of the MappedEvent
    //
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

    int                      m_pitch;
    Rosegarden::RealTime     m_eventTime;
    Rosegarden::RealTime     m_duration;
    Rosegarden::RealTime     m_audioStartMarker;
    velocityT                m_velocity;
    MappedEventType          m_type;
    Rosegarden::InstrumentId m_instrument;

};

}

#endif
