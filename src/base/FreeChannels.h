/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _FREECHANNELS_H_
#define _FREECHANNELS_H_

#include <base/ChannelSegment.h>
#include <base/Composition.h>

#include <set>

namespace Rosegarden
{

// @class A set of currently free channels on some device.
// @author Tom Breton (Tehom)
class FreeChannels :
  public CompositionObserver,
    public std::multiset<ChannelSegment, ChannelSegment::Cmp>
{
 public:
  typedef std::multiset<ChannelSegment, ChannelSegment::Cmp>::iterator iterator;
  FreeChannels(Composition &comp, int maxChannels);
  ~FreeChannels();

 private:
  // Make another channel available if possible.
  iterator tryAddChannel(void);

  // Allocate a time interval, returning the channel it's on.
  ChannelIdRaw allocateInterval(iterator i, timeT start, timeT end);

  void addExistingSegments(void);

  static ChannelId& getChannelId(Segment *s);

  /* Virtual functions */
  
  virtual void segmentAdded(const Composition *, Segment *);

  virtual void segmentRemoved(const Composition *, Segment *);

  virtual void segmentRepeatChanged(const Composition *, Segment *, bool) { }

  virtual void segmentRepeatEndChanged(const Composition *, Segment *, timeT) { }

  virtual void segmentEventsTimingChanged(const Composition *, Segment *,
                                            timeT /* delay */,
                                            RealTime /* rtDelay */) { }

  virtual void segmentStartChanged(const Composition *, Segment *,
                                   timeT /* newStartTime */);

  virtual void segmentEndMarkerChanged(const Composition *, Segment *,
                                       bool /* shorten */);

  virtual void segmentTrackChanged(const Composition *, Segment *,
                                     TrackId /* id */) { }
  virtual void endMarkerTimeChanged(const Composition *, bool /* shorten */) { }


  
  /* Data members */

  // The composition, just so we can notify it in our dtor.
  Composition&     m_composition;
  
  // The total number of channels (Not Channel segments) this device lets us
  // play at once.
  int m_maxChannels;

  // The number of channels we've used.  If less than m_maxChannels,
  // some channels have never been allocated (in this session).  
  // Channels aren't represented in the free list until needed.
  int m_NbChannels;

  // The earliest time we treat.  This never increases and is at least
  // as early as the composition's beginning and as early as any
  // segment's beginning.
  timeT m_startTime;

  // The latest time we treat.  This never decreases and is at least
  // as late as the composition's end and as late as any segment's end.
  timeT m_endTime;
 };
}

#endif /* ifndef _FREECHANNELS_H_ */
