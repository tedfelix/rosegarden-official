
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

#include "Event.h"
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

  // Our main constructor used to convert from the Track Events
  //
  MappedEvent(const Event &e): _pitch(e.get<Int>("pitch")),
                               _absoluteTime(e.getAbsoluteTime()),
                               _duration(e.getDuration()),
                               _velocity(127) {;}

  MappedEvent(const int &pitch, const timeT &absTime, const timeT &duration,
              const velocityT &velocity, const instrumentT &instrument):
                              _pitch(pitch),
                              _absoluteTime(absTime),
                              _duration(duration),
                              _velocity(velocity),
                              _instrument(instrument) {;}
  ~MappedEvent() {;}

  void setPitch(const int &p) { _pitch = p; }
  void setAbsoluteTime(const timeT &a) { _absoluteTime = a; }
  void setDuration(const timeT &d) { _duration = d; }
  void setInstrument(const instrumentT &i) { _instrument = i; }
  void setVelocity(const velocityT &v) { _velocity = v; }

  int   getPitch() const { return _pitch; }
  timeT getAbsoluteTime() const { return _absoluteTime; }
  timeT getDuration() const { return _duration; }
  velocityT getVelocity() const { return _velocity; }
  instrumentT getInstrument() const { return _instrument; }

  struct MappedEventCmp
  {
    bool operator()(const MappedEvent *mE1, const MappedEvent *mE2) const
    {
      return *mE1 < *mE2;
    }
  };

  friend bool operator<(const MappedEvent &a, const MappedEvent &b);

private:

  int          _pitch;
  timeT        _absoluteTime;
  timeT        _duration;
  velocityT    _velocity;
  instrumentT  _instrument;

};

}

#endif
