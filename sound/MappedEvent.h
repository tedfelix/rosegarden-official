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

#ifndef _MAPPEDEVENT_H_
#define _MAPPEDEVENT_H_

// Used as a transformation stage between Composition Events and output
// at the Sequencer this class and MidiComposition eliminates the notion
// of Track.  The MappedEvents are ripe for playing.
//
// NOTE: for the moment until the Composition handles it we're hard
// coding the velocity components of all MappedEvents to maximum (127)
//

#include <Event.h>
#include <multiset.h>
#include <qdatastream.h>

namespace Rosegarden
{

typedef unsigned int instrumentT;
typedef unsigned int velocityT;

class MappedEvent
{
public:
    MappedEvent() {;}

    // Our major constructors used to convert from Events -
    // note that we put in place default velocities at this
    // point in case our Composition is missing them.
    //
    //
    MappedEvent(const Event &e);
    MappedEvent(const Event &e, timeT duration);

    MappedEvent(const int &pitch, const timeT &absTime, const timeT &duration,
                const velocityT &velocity, const instrumentT &instrument):
        m_pitch(pitch),
        m_absoluteTime(absTime),
        m_duration(duration),
        m_velocity(velocity),
        m_instrument(instrument) {;}
    ~MappedEvent() {;}

    void setPitch(const int &p) { m_pitch = p; }
    void setAbsoluteTime(const timeT &a) { m_absoluteTime = a; }
    void setDuration(const timeT &d) { m_duration = d; }
    void setInstrument(const instrumentT &i) { m_instrument = i; }
    void setVelocity(const velocityT &v) { m_velocity = v; }

    int   getPitch() const { return m_pitch; }
    timeT getAbsoluteTime() const { return m_absoluteTime; }
    timeT getDuration() const { return m_duration; }
    velocityT getVelocity() const { return m_velocity; }
    instrumentT getInstrument() const { return m_instrument; }

    struct MappedEventCmp
    {
        bool operator()(const MappedEvent *mE1, const MappedEvent *mE2) const
        {
            return *mE1 < *mE2;
        }
    };

    friend bool operator<(const MappedEvent &a, const MappedEvent &b);

private:

    int          m_pitch;
    timeT        m_absoluteTime;
    timeT        m_duration;
    velocityT    m_velocity;
    instrumentT  m_instrument;

};

}

#endif
