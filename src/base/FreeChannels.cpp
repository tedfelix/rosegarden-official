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

#include "FreeChannels.h"

namespace Rosegarden
{


// @class ChannelIdAccessor.  Access to a Segment's channel all goes
// thru here.
// @author Tom Breton (Tehom)
class ChannelIdAccessor
{
    friend class FreeChannels;
    static void setChannelId(Segment *s, ChannelIdRaw id)
    { s->m_channel.m_channel = id; }
    static ChannelIdRaw &getChannelId(Segment *s)
    { return s->m_channel.m_channel; }
};

// Constructor for FreeChannels, taking a composition and the maximum
// number of channels it may allocate.
// @author Tom Breton (Tehom)
FreeChannels::
FreeChannels(Composition &comp, int maxChannels) :
  m_composition(comp),
  m_maxChannels(maxChannels),
  m_startTime(comp.getStartMarker()),
  m_endTime(comp.getDuration())
{
  comp.addObserver(this);
  // In case composition already has segments in it, which might
  // happen if we add a device during the session.
  addExistingSegments();
}

// Destructor for FreeChannels.  All it does is notify composition.
// @author Tom Breton (Tehom)
FreeChannels::~FreeChannels()
{
  m_composition.removeObserver(this);
}

// Iterate thru composition's segments, allocating channels to
// relevant ones.  
// @author Tom Breton (Tehom)
void
FreeChannels::
addExistingSegments(void)
{
  Composition::segmentcontainer segs = m_composition.getSegments();
  for (Composition::iterator i = segs.begin();
       i != segs.end();
       ++i) {
      if ((*i)->getType() == Segment::Internal) {
          // !!!Punt: Assume all internal segments are relevant.  We
          // ought to also check that it's our device.
          segmentAdded(&m_composition,(*i));
      }
  }
}

// React to adding a segment: find a suitable channelsegment for it.
// @author Tom Breton (Tehom)
void
FreeChannels::
segmentAdded(const Composition * /*unused*/, Segment *s)
{
  const timeT start = s->getStartTime();
  const timeT end = s->getEndTime();
  // Mutable, to be found.
  ChannelIdRaw channel = -1; 
  
  // Scan segments backwards from the last one beginning at time
  // `start'
  iterator rEnd = --begin();
  ChannelSegment dummy(0, start, 0);
  for (iterator i = upper_bound(dummy); i != rEnd; --i) {
      const ChannelSegment cs = (*i);
      if ((cs.m_start <= start) && (cs.m_end >= end)) {
          // We found one that this fits into, so use it.
          channel = allocateInterval(i, start, end);
          break;
      }
  }

  // If that failed, add a new channel.
  if (channel < 0) {
      iterator i = tryAddChannel();
      channel = allocateInterval(i,start,end);
  }
  
  // Tell segment the channel (Even if it's still negative)
  ChannelIdAccessor::setChannelId(s,channel);
}

// React to removing a segment: make its channelsegment available
// again.  
// @author Tom Breton (Tehom)
void
FreeChannels::
segmentRemoved(const Composition * /*unused*/, Segment *s)
{
  const timeT startTime = s->getStartTime();
  const timeT endTime   = s->getEndTime();
  const ChannelIdRaw channel = ChannelIdAccessor::getChannelId(s);
  // Mutable, to be found.
  iterator before = end(), after = end();

  // Find the pieces before and after.
  {
      iterator rEnd = --begin();
      ChannelSegment dummy(0, startTime, 0);
      for (iterator i = upper_bound(dummy); i != rEnd; --i) {
          const ChannelSegment cs = (*i);
          if (cs.m_channel == channel) {
              before = i;
              break;
          }
      }
  }

  {
      ChannelSegment dummy(0, endTime, 0);
      for (iterator i = lower_bound(dummy); i != end(); ++i) {
          const ChannelSegment cs = (*i);
          if (cs.m_channel == channel) {
              after = i;
              break;
          }
      }
  }

  // Figure out whether we will merge, both directions.
  bool mergeBefore = (before != end() && (before->m_end   == startTime));
  bool mergeAfter  = (after  != end() && (after ->m_start == endTime));
  timeT startContiguous = mergeBefore ? before->m_start : startTime;
  timeT endContiguous   = mergeAfter  ? after ->m_end   : endTime;

  // Add a channelsegment incorporating the whole contiguous time.
  insert(ChannelSegment(channel, startContiguous, endContiguous));
  // Remove existing pieces that were incorporated in it.
  if (mergeBefore) { erase(before); }
  if (mergeAfter)  { erase(after); }
}

void
FreeChannels::
segmentStartChanged(const Composition * c,
                    Segment * s,
                    timeT /* newStartTime */)
{
    // !!! For now do it the simple, wasteful way: deallocate,
    // reallocate.

    // !!! Wrong, we need new times for the new one.
    // segmentRemoved(c,s);
    // segmentAdded(c,s);
}

void
FreeChannels::
segmentEndMarkerChanged(const Composition * c,
                        Segment * s,
                        bool /* shorten */)
{
    // !!! For now do it the simple, wasteful way: deallocate, reallocate.
}



// Allocate a time interval
// @param i an iterator indexing a ChannelSegment that includes the
// interval from start to end.
// @returns Id of the channel it's on
// @author Tom Breton (Tehom)
ChannelIdRaw
FreeChannels::
allocateInterval(iterator i, timeT start, timeT end)
{
  const ChannelSegment cs = (*i);

  erase(i);
  if (cs.m_start < start) {
    // There's some length before `start'.  Insert a new piece.  (We
    // can't alter it in place because the base class owns it and
    // makes it const)
    (void)insert(ChannelSegment(cs.m_channel, cs.m_start, start));
  } else { }

  if (cs.m_end > end) {
    // There's some length after `end'.  Insert a new piece.
    (void)insert(ChannelSegment(cs.m_channel, end, cs.m_end));
  } else {}
 
  return cs.m_channel;
}


// Make another channel available if possible.
// @returns On success, return an iterator indexing the new element,
// otherwise return end()
// @author Tom Breton (Tehom)
FreeChannels::iterator
FreeChannels::
tryAddChannel(void)
{
  if (m_NbChannels >= m_maxChannels) { return end(); }
  iterator cs = insert(begin(),
                       ChannelSegment(m_NbChannels, m_startTime, m_endTime));
  ++m_NbChannels;
  return cs;
}

}

