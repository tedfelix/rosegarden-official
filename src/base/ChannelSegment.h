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

#ifndef _CHANNELSEGMENT_H_
#define _CHANNELSEGMENT_H_

#include <base/Event.h>

namespace Rosegarden
{

class FreeChannels;

// @type ChannelIdRaw The index of a channel on some
// device.  A negative value indicates no channel.
// @author Tom Breton (Tehom)
typedef int ChannelIdRaw;

 
// @class ChannelId Indication of a channel on some
// device.  Exists largely to encapsulate ChannelIdRaw inside
// Segment.
// @author Tom Breton (Tehom)
class ChannelId
{
    friend class FreeChannels;
 public:
 ChannelId(ChannelIdRaw id = -1) :
    m_id(id)
    {};
 private:
    ChannelIdRaw m_id;
};

// @class ChannelSegment  A timewise section of a channel on some
// device
// @author Tom Breton (Tehom)
class ChannelSegment
{
 public:
 ChannelSegment(void) :
    m_channel(-1)
        {}
    
 ChannelSegment(ChannelIdRaw channel, timeT start, timeT end) :
  m_channel(channel),
    m_start(start),
    m_end(end)
    {}

  // Comparison operation.
  struct Cmp
  {
    bool operator()(const ChannelSegment &cs1, const ChannelSegment &cs2) const {
      return cs1.m_start < cs2.m_start;
    }
    bool operator()(const ChannelSegment *cs1, const ChannelSegment *cs2) const {
      return operator()(*cs1, *cs2);
    }
  };

  ChannelIdRaw m_channel;
  timeT     m_start;
  timeT     m_end;
};
}

#endif /* ifndef _CHANNELSEGMENT_H_ */
