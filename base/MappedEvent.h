
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

// Used as a transformation stage between Composition Events and MIDI
// events this eliminates the notion of Track.  The Composition is
// converted into a MappedComposition containing ordered Events with
// MIDI friendly information.
//

#include "Event.h"
#include <multiset.h>
#include <qdatastream.h>

namespace Rosegarden
{

typedef unsigned int instrumentT;

class MappedEvent
{
public:
  MappedEvent() {;}
  MappedEvent(const Event &e): _pitch(e.get<Int>("pitch")),
                              _absoluteTime(e.getAbsoluteTime()),
                              _duration(e.getDuration()) {;}

  MappedEvent(const int &pitch, const timeT &absTime, const timeT &duration,
              const instrumentT &instrument):
                              _pitch(pitch),
                              _absoluteTime(absTime),
                              _duration(duration),
                              _instrument(instrument) {;}
  ~MappedEvent() {;}

  void setInstrument(const instrumentT &i) { _instrument = i; }
  void setDuration(const timeT &d) { _duration = d; }
  void setAbsoluteTime(const timeT &a) { _absoluteTime = a; }
  void setPitch(const int &p) { _pitch = p; }

  instrumentT getInstrument() const { return _instrument; }
  timeT getDuration() const { return _duration; }
  timeT getAbsoluteTime() const { return _absoluteTime; }
  int   getPitch() const { return _pitch; }

  struct MappedEventCmp
  {
    bool operator()(const MappedEvent *mE1, const MappedEvent *mE2) const
    {
      return *mE1 < *mE2;
    }
  };

  friend bool operator<(const MappedEvent &a, const MappedEvent &b);

private:

  int _pitch;
  timeT _absoluteTime;
  timeT _duration;
  instrumentT _instrument;

};

}

#endif
