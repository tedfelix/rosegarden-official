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

#include "MappedEvent.h"
#include "BaseProperties.h"
 
namespace Rosegarden
{

MappedEvent::MappedEvent(const Event &e,
                         const Rosegarden::RealTime &absoluteTime,
                         const Rosegarden::RealTime &duration,
                         const Rosegarden::InstrumentId &instrument):
       m_pitch(e.get<Int>(BaseProperties::PITCH)),
       m_absoluteTime(absoluteTime),
       m_duration(duration),
       m_audioStartMarker(0, 0),
       m_type(Internal),
       m_instrument(instrument)
{
    // Attempt to get a velocity - if it fails then
    // set the velocity to default maximum (127)
    //
    try
    {
        m_velocity = e.get<Int>(BaseProperties::VELOCITY);
    }
    catch(...)
    {
        m_velocity = 127;
    }
}

MappedEvent::MappedEvent(const Rosegarden::RealTime &absTime,
                         const Rosegarden::RealTime &duration,
                         const Rosegarden::RealTime &audioStartMarker,
                         const Rosegarden::InstrumentId &instrument,
                         const MappedEventType type,
                         const int &id):
      m_pitch(id),
      m_absoluteTime(absTime),
      m_duration(duration),
      m_audioStartMarker(audioStartMarker),
      m_type(type),
      m_instrument(instrument)
{
}


bool
operator<(const MappedEvent &a, const MappedEvent &b)
{
    return a.getAbsoluteTime() < b.getAbsoluteTime();
}


}

