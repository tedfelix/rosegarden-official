
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

namespace Rosegarden
{

class MappedEvent : public Event
{
public:
  MappedEvent() : Event() {;}
  MappedEvent(const Event &e):Event(e) {;}
  ~MappedEvent() {;}

  struct MappedEventCmp
  {
    bool operator()(const MappedEvent *mE1, const MappedEvent *mE2) const
    {
      return *mE1 < *mE2;
    }
  };

  const unsigned int instrument() const { return _instrument; }
  void instrument(const unsigned int &instrument) { _instrument = instrument; }

private:
  unsigned int _instrument;

};

class MappedComposition : public std::multiset<MappedEvent *,
                                      MappedEvent::MappedEventCmp>
{
public:
  MappedComposition(const unsigned int &sT, const unsigned int &eT):
             _startTime(sT), _endTime(eT) {;}
  ~MappedComposition() {;}

  const unsigned int beginTime() const { return _startTime; }
  const unsigned int endTime() const { return _endTime; }

private:
  unsigned int _startTime;
  unsigned int _endTime;
};

}

#endif
