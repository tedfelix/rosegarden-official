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

#include "AllocateChannels.h"
#include "Instrument.h"
#include "MidiDevice.h"
#include "misc/Debug.h"

#include <algorithm>

// #define DEBUG_CHANNEL_ALLOCATOR 1

namespace Rosegarden
{

// Allocate a channel interval that encompasses startTime and endTime.
// @param startTime is the first instant sound is to be played on the channel.
// @param endTime is the last such instant.
// @author Tom Breton (Tehom)
ChannelInterval
FreeChannels::
allocateChannelInterval(RealTime startTime, RealTime endTime,
                        Instrument *instrument,
                        RealTime marginBefore,
                        RealTime marginAfter)
{
    iterator bestMatch = end();
    // Scoring just tracks wasted space.  Init it to worse than any
    // stored interval can be.
    RealTime leastWastedSpace = ChannelInterval::m_afterLatestTime;
    
    // Scan segments backwards from the last one beginning at time
    // `startTime'
    if (!empty()) {
#ifdef DEBUG_CHANNEL_ALLOCATOR
                    SEQUENCER_DEBUG
                        << "Scanning for existing ChannelInterval"
                        << endl;
#endif

        ChannelInterval dummy(startTime);
        for (reverse_iterator i(upper_bound(dummy));
             i != rend();
             ++i) {
#ifdef DEBUG_CHANNEL_ALLOCATOR
                    SEQUENCER_DEBUG
                        << "Scanning";
#endif
            const ChannelInterval &cs = (*i);
            // Consider each end of the proposed interval.  An end
            // fits if either:
            //
            // * It is big enough to accomodate the respective margin.
            //
            // * It is big enough without the margin and the adjacent
            //   (allocated) channel interval sounds on the same
            //   instrument.

            // Reject complete non-fits early.
            if (cs.m_start > startTime) { continue; }
            if (cs.m_end < endTime)     { continue; }

            // Reject if instrument changed and margin is
            // insufficient.  This considers both the given margins
            // and the adjacent instruments' margins recorded in
            // ChannelInterval.  Its fields m_marginBefore and
            // m_marginAfter refer to our own before/after
            // orientation, not to the reversed orientation that the
            // instruments playing before and after would have.
            if (cs.m_instrumentBefore &&
                (cs.m_instrumentBefore != instrument) &&
                (((cs.m_start +      marginBefore) > startTime) ||
                 ((cs.m_start + cs.m_marginBefore) > startTime)))
                { continue; }

            if (cs.m_instrumentAfter &&
                (cs.m_instrumentAfter != instrument) &&
                (((cs.m_end -      marginAfter) < endTime) ||
                 ((cs.m_end - cs.m_marginAfter) < endTime)))
                { continue; }

            // We found an candidate, but is it the best so far?  Only
            // if it wastes less space than all others we've seen.
            RealTime wastedSpace =
                (cs.m_end - cs.m_start) - (endTime - startTime);
            if (wastedSpace < leastWastedSpace) {
                // Decrement so forward iterator refers to the same
                // element we examined.
                bestMatch = --(i.base());
                leastWastedSpace = wastedSpace;
            }
        }
    }
    if (bestMatch != end()) {
        return allocateChannelIntervalFrom(bestMatch,
                                           startTime, endTime,
                                           instrument,
                                           marginBefore, marginAfter);
    } else {
        // If we found nothing usable, return an unplayable dummy
        // channel
        return ChannelInterval(); 
    }
}

// Free the given channel interval, setting old's channel to unused.
// @author Tom Breton (Tehom)
void
FreeChannels::
freeChannelInterval(ChannelInterval &old)
{
    if (!old.validChannel()) { return; }
#ifdef DEBUG_CHANNEL_ALLOCATOR
    SEQUENCER_DEBUG
        << "Freeing channel interval on"
        << old.getChannelId()
        << endl;
#endif
    // Mutable, to be found.
    iterator before = end(), after = end();

    // Find the free ChannelInterval before it.
    {
        iterator rEnd = --begin();
        ChannelInterval dummy(old.m_start);
        for (iterator i = upper_bound(dummy); i != rEnd; --i) {
            const ChannelInterval cs = (*i);
            if (cs.getChannelId() == old.getChannelId()) {
                before = i;
                break;
            }
        }
    }

    // Figure out whether we will merge with "before"
    bool mergeBefore = (before != end() && (before->m_end   == old.m_start));
    const ChannelInterval &ciBefore = mergeBefore ? *before : old;

    // Find the free ChannelInterval after it.
    {
        ChannelInterval dummy(old.m_end);
        for (iterator i = lower_bound(dummy); i != end(); ++i) {
            const ChannelInterval cs = (*i);
            if (cs.getChannelId() == old.getChannelId()) {
                after = i;
                break;
            }
        }
    }

    // Figure out whether we will merge with "after"
    bool mergeAfter  = (after  != end() && (after ->m_start == old.m_end));
    const ChannelInterval &ciAfter = mergeAfter ? *after : old;
    

    // Add a channelsegment incorporating the whole contiguous time.
    insert(ChannelInterval(old.getChannelId(),
                           ciBefore.m_start,             ciAfter.m_end,
                           ciBefore.m_instrumentBefore,  ciAfter.m_instrumentAfter,
                           ciBefore.m_marginBefore,      ciAfter.m_marginAfter));

    // Remove "before" if we incorporated it.
    if (mergeBefore) { erase(before); }
    // Remove "after" if we incorporated it.
    if (mergeAfter)  { erase(after); }
    old.clearChannelId();
}



// Allocate a time interval
// @param i an iterator indexing a ChannelInterval that includes the
// interval from start to end.
// @param start is the first instant sound is to be played on the channel.
// @param end is the last such instant.
// @returns A ChannelInterval, either a suitable one or non-playing.
// @author Tom Breton (Tehom)
ChannelInterval
FreeChannels::
allocateChannelIntervalFrom(iterator i, RealTime start, RealTime end,
                            Instrument *instrument,
                            RealTime marginBefore,
                            RealTime marginAfter)
{
  const ChannelInterval cs = (*i);

  erase(i);
  if (cs.m_start < start) {
    // There's some length before `start'.  Insert a new piece.  (We
    // can't alter it in place because the base class owns it and
    // makes it const)
      (void)insert(ChannelInterval(cs.getChannelId(),
                                   cs.m_start,            start,
                                   cs.m_instrumentBefore, instrument,
                                   cs.m_marginBefore,     marginBefore));
  } else { }

  if (cs.m_end > end) {
    // There's some length after `end'.  Insert a new piece.
    (void)insert(ChannelInterval(cs.getChannelId(),
                                 end,         cs.m_end,
                                 instrument,  cs.m_instrumentAfter,
                                 marginAfter, cs.m_marginAfter));
  } else {}
 
  return ChannelInterval(cs.getChannelId(),
                         start, end,
                         NULL, NULL,
                         RealTime::zeroTime, RealTime::zeroTime);
}

// Add a channel that may be allocated from.  It is caller's
// responsibility to not duplicate channel numbers.
// @author Tom Breton (Tehom)
void
FreeChannels::
addChannel(ChannelId channelNb)
{
    insert(begin(),
           ChannelInterval(channelNb,
                           ChannelInterval::m_beforeEarliestTime,
                           ChannelInterval::m_afterLatestTime,
                           NULL, NULL,
                           RealTime::zeroTime, RealTime::zeroTime));
}

// Remove channel from being allocated.  It is caller's
// responsibility to deal with objects currently holding channel
// intervals. 
// @author Tom Breton (Tehom)
void
FreeChannels::
removeChannel(ChannelId channelNb)
{
    // Filter the channels.  We build a temporary container, put
    // everything that stays into that container, then swap contents.
    container els;
    for (iterator i = begin(); i != end(); ++i)
        if (i->getChannelId() != channelNb) {
            els.insert(els.end(), *i);
        }
    swap(els);
}


// Re-allocate a ChannelInterval to encompass start and end.
// @author Tom Breton (Tehom)
void
FreeChannels::
reallocateToFit(ChannelInterval &ci, RealTime start, RealTime end,
                Instrument *instrument,
                RealTime marginBefore,
                RealTime marginAfter)
{
    // If it still fits, re-use it.
    if ((ci.validChannel()) &&
        (ci.m_start <= start) &&
        (end <= ci.m_end))
        { return; }

    // Otherwise just replace it.
    if (ci.validChannel())
        { freeChannelInterval(ci); }
    ci = allocateChannelInterval(start, end, instrument,
                                 marginBefore, marginAfter);
}

    /*** ChannelSetup definitions ***/

// This is a stub in case we ever want AllocateChannels to set up
// differently for different devices.
const ChannelSetup ChannelSetup::MIDI = ChannelSetup();
    
    /*** AllocateChannels definitions ***/

// Whether a channelId denotes a percussion channel
// @author Tom Breton (Tehom)
bool
AllocateChannels::
isPercussion(ChannelId channel)
{
    return MidiDevice::isPercussionNumber(channel);
}

// Whether a ChannelInterval denotes a percussion channel
// @author Tom Breton (Tehom)
bool
AllocateChannels::
isPercussion(ChannelInterval &ci)
{
    return isPercussion(ci.getChannelId());
}

// Constructor for AllocateChannels
// @author Tom Breton (Tehom)
AllocateChannels::
AllocateChannels(ChannelSetup /*unused*/) :
    m_freeChannels()
{
    // Quick and dirty: assume ChannelSetup::MIDI.
    for (int i = 0; i < 16; ++i) {
        if (i != 9) {
            (void)m_freeChannels.addChannel(i);
        }
    }
}

AllocateChannels::
~AllocateChannels(void)
{
#ifdef DEBUG_CHANNEL_ALLOCATOR
    SEQUENCER_DEBUG
        << "~AllocateChannels"
        << endl;
#endif    
}

// Re-allocate a ChannelInterval to encompass start and end,
// appropriately for Instrument
// @author Tom Breton (Tehom)
void
AllocateChannels::
reallocateToFit(Instrument& instrument, ChannelInterval &ci,
                RealTime start, RealTime end,
                RealTime marginBefore, RealTime marginAfter,
                bool changedInstrument)
{
#ifdef DEBUG_CHANNEL_ALLOCATOR
    SEQUENCER_DEBUG
        << "reallocateToFit: Reallocating"
        << (instrument.isPercussion() ? "percussion" : "non-percussion")
        << instrument.getName() << instrument.getId()
        << "on bank"
        << (int)instrument.getMSB() << ":" << (int)instrument.getLSB() 
        << "channel "
        << ci.getChannelId()
        << endl;
#endif
    // If we already have a channel but it's the wrong type or it
    // changed instrument, always free it.
    if (ci.validChannel() &&
        ((changedInstrument && (end != ChannelInterval::m_latestTime)) || 
         (instrument.isPercussion() != (isPercussion(ci)))))
        { freeChannelInterval(ci); }

    if (instrument.isPercussion()) {
        // For single channel, this implicitly frees+reallocates
        ci = ChannelInterval(9, start, end,
                             NULL, NULL,
                             RealTime::zeroTime, RealTime::zeroTime);
    } else {
        m_freeChannels.reallocateToFit(ci, start, end,
                                       &instrument,
                                       marginBefore, marginAfter);
    }
    
#ifdef DEBUG_CHANNEL_ALLOCATOR
    SEQUENCER_DEBUG
        << "Now channel "
        << ci.getChannelId()
        << endl;
#endif
}

// Free the given channel interval appropriately.
// @author Tom Breton (Tehom)
void
AllocateChannels::
freeChannelInterval(ChannelInterval &old)
{
    if (isPercussion(old)) {
        old.clearChannelId();
    }
    else {
        m_freeChannels.freeChannelInterval(old);
    }
}

// Reserve a channel for a fixed-channel instrument.
// The signal connections this uses are made by ChannelManager.
// @author Tom Breton (Tehom)
void
AllocateChannels::
reserveFixedChannel(ChannelId channel)
{
    // Remove channel from FreeChannels (if not percussion) so we
    // don't give anything an interval on this channel.
    if (!isPercussion(channel)) {
#ifdef DEBUG_CHANNEL_ALLOCATOR
    SEQUENCER_DEBUG
        << "AllocateChannels: reserveFixedChannel reserving"
        << (int) channel
        << endl;
#endif
        m_freeChannels.removeChannel(channel);
    }

    // Kick everything else off the channel.  They'll get a new
    // channel; that's not a concern here.
    emit sigVacateChannel(channel);

    // Record that this channel is reserved
    m_fixedChannels.insert(channel);

    // Tell any visiting dialogs what's reserved now.
    emit sigChangedReservedChannels(this);
}


// Release a channel that a fixed-channel instrument had.
// @author Tom Breton (Tehom)
void
AllocateChannels::
releaseFixedChannel(ChannelId channel)
{
    FixedChannelSet::iterator i = m_fixedChannels.find(channel);

    if (i == m_fixedChannels.end()) { return; }
#ifdef DEBUG_CHANNEL_ALLOCATOR
    SEQUENCER_DEBUG
        << "AllocateChannels: releaseFixedChannel releasing"
        << (int) channel
        << endl;
#endif
    
    // Remove from reserved channels.
    m_fixedChannels.erase(i);

    // Tell any visiting dialogs what's reserved now.
    emit sigChangedReservedChannels(this);

    // Add channel to FreeChannels (if not percussion)
    if (!isPercussion(channel)) {
        m_freeChannels.addChannel(channel);
    }
}

}

#include "AllocateChannels.moc"
