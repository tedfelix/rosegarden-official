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

#ifndef RG_ALLOCATECHANNELS_H
#define RG_ALLOCATECHANNELS_H

#include <base/ChannelInterval.h>
#include <base/Composition.h>

#include <QObject>

#include <set>
#include <list>

namespace Rosegarden
{

class Instrument;

/// A set of currently free channel intervals.
/**
 * Does not concern itself with Device or Instrument.
 *
 * Deriving from STL containers can be quite dangerous as STL containers
 * have no virtual dtors.  In this case, there is only a single object
 * of this class in AllocateChannels.  Since there is no dynamic binding,
 * there is no danger.
 *
 * @author Tom Breton (Tehom)
 */
class FreeChannels :
    public std::multiset<ChannelInterval, ChannelInterval::Cmp>
{
public:
    typedef std::multiset<ChannelInterval, ChannelInterval::Cmp> container;
    typedef container::iterator iterator;

    // Unnecessary.
//    FreeChannels(void) {}
//    ~FreeChannels() {}

    // Reallocate a channel interval to fit start and end.
    void reallocateToFit(ChannelInterval &ci, RealTime start, RealTime end,
                         Instrument *instrument,
                         RealTime marginBefore,
                         RealTime marginAfter);
  
    // Free a channel interval
    void freeChannelInterval(ChannelInterval &old);

  
    // Make it so that "channelNb" can be allocated from.
    void addChannel(ChannelId channelNb);

    // Make it so that "channelNb" won't be allocated from.  It is
    // caller's responsibility to deal with objects currently holding
    // channel intervals.
    void removeChannel(ChannelId channelNb);

private:

    // Allocate a channel interval
    ChannelInterval allocateChannelInterval(RealTime start, RealTime end,
                                            Instrument *instrument,
                                            RealTime marginBefore,
                                            RealTime marginAfter);

    // Allocate a time interval from a known free ChannelInterval
    ChannelInterval allocateChannelIntervalFrom(
            iterator i, RealTime start, RealTime end,
            Instrument *instrument,
            RealTime marginBefore,
            RealTime marginAfter);

    void dump();
};

// @class ChannelSetup.  Dummy class.  It tells us how to initialize
// AllocateChannels, but in fact it's only MIDI so there is no
// information to pass.
struct ChannelSetup
{
public:
    static const ChannelSetup MIDI;
};

/**
 * Allocates channel intervals appropriately for a given instrument that
 * plays on a channel-bearing device (ie just MIDI instruments).  Don't
 * call its methods if instrument has a fixed channel.
 *
 * @author Tom Breton (Tehom)
 */
class AllocateChannels : public QObject
{
    Q_OBJECT

public:
    typedef std::set<ChannelId> FixedChannelSet;

    AllocateChannels(ChannelSetup setup);
    ~AllocateChannels(void);

    void reallocateToFit(Instrument& instrument, ChannelInterval &ci,
                         RealTime start, RealTime end,
                         RealTime marginBefore,
                         RealTime marginAfter,
                         bool changedInstrument);

    void freeChannelInterval(ChannelInterval &old);

    void reserveFixedChannel(ChannelId channel);
    void releaseFixedChannel(ChannelId channel)
        { releaseReservedChannel(channel, m_fixedChannels); }

    ChannelId reallocateThruChannel(
            Instrument& instrument, ChannelId channel);
    ChannelId allocateThruChannel(Instrument& instrument);
    void releaseThruChannel(ChannelId channel)
        { releaseReservedChannel(channel, m_thruChannels); }

    static bool isPercussion(ChannelId channel);
    static bool isPercussion(ChannelInterval &ci);
    static ChannelId getPercussionChannel(void) { return 9; }

signals:
    void sigVacateChannel(ChannelId channel);
  
private:
    // Release channel from channelSet, making it available for normal use.
    void releaseReservedChannel(
            ChannelId channel, FixedChannelSet& channelSet);
    // Reserve a channel, with respect only to channelSet and
    // m_freeChannels.
    void reserveChannel(ChannelId channel, FixedChannelSet& channelSet);
      
    // Channel intervals for "normal" instruments: Not percussion, not
    // fixed.  ChannelManagers holding pieces of this are connected to
    // sigVacateChannel.
    FreeChannels    m_freeChannels;
    // Channels that we've reserved for specific instruments.  No two
    // instruments share a fixed channel.  This doesn't record
    // instrument identity.
    FixedChannelSet m_fixedChannels;

    // Channels for "MIDI thru" to play on.  They are arranged to not
    // conflict with fixed channels.
    FixedChannelSet m_thruChannels;
};
}

#endif /* ifndef RG_ALLOCATECHANNELS_H */
