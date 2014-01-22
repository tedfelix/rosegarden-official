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

#define RG_MODULE_STRING "[AllocateChannels]"
#define RG_NO_DEBUG_PRINT 1

#include "AllocateChannels.h"
#include "Instrument.h"
#include "MidiDevice.h"
#include "misc/Debug.h"
#include "sound/ControlBlock.h"

#include <algorithm>

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
    RG_DEBUG << "allocateChannelInterval";
    iterator bestMatch = end();
    // Scoring just minimizes wasted space by choosing the smallest
    // piece that fits.

    // Initialize (leastOverflow, leastDuration) to longer than any
    // interval can be.
    RealTime leastDuration = ChannelInterval::m_afterLatestTime;
    // leastDuration's overflow bit.  See comments on thisOverflow and
    // thisDuration.
    bool leastOverflow = true;
    
    // Scan segments backwards from the last one beginning at time
    // `startTime'
    if (!empty()) {
                    RG_DEBUG
                        << "Scanning for existing ChannelInterval"
                        << endl;

        ChannelInterval dummy(startTime);
        for (reverse_iterator i(upper_bound(dummy));
             i != rend();
             ++i) {

            const ChannelInterval &cs = (*i);
            RG_DEBUG << "Considering" << cs;
            cs.assertSane();

            // Consider each end of the proposed interval.  An end
            // fits if either:
            //
            // * It is big enough to accomodate the respective margin.
            //
            // * It is big enough without the margin and the adjacent
            //   (allocated) channel interval sounds on the same
            //   instrument.

            // Reject complete non-fits early.
            if (cs.m_start > startTime) {
                RG_DEBUG << "  Rejecting due to free channel's available start time (" << cs.m_start << ") after needed start (" << startTime << ")";
                continue;
            }
            if (cs.m_end < endTime) {
                RG_DEBUG << "  Rejecting due to free channel's available end time (" << cs.m_end << ") before needed end (" << endTime << ")";
                continue;
            }

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
            // if it wastes less space than all others we've seen,
            // which is true if it's smaller than them.

            // Be careful of overflow.  This calculation ranges from 0
            // to twice the maximum RealTime can hold.  Overflow can
            // cause us to see huge waste as negative waste, which
            // results in very inefficient allocation.  To avoid this,
            // we keep an overflow bit (thisOverflow and
            // leastOverflow) and treat it as the most significant
            // bit.
            RealTime thisDuration = cs.m_end - cs.m_start;
            bool thisOverflow = (thisDuration < RealTime::zeroTime);

            RG_DEBUG << "Found a candidate that takes"
                     << (thisOverflow ? "the maximum plus" : "only")
                     << thisDuration;

            if ((thisOverflow < leastOverflow) ||
                ((thisOverflow == leastOverflow) &&
                 (thisDuration < leastDuration))) {

                RG_DEBUG << "Best candidate so far";
                // Decrement so forward iterator refers to the same
                // element we examined.
                bestMatch = --(i.base());
                leastDuration = thisDuration;
                leastOverflow = thisOverflow;
            }
        }
    }
    if (bestMatch != end()) {
        RG_DEBUG << "  FreeChannels::allocateChannelInterval() SUCCESS!!!!";
        return allocateChannelIntervalFrom(bestMatch,
                                           startTime, endTime,
                                           instrument,
                                           marginBefore, marginAfter);
    } else {
        RG_DEBUG << "  FreeChannels::allocateChannelInterval() giving up.  FAIL";
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
    RG_DEBUG << "Freeing" << old;
    // We are sometimes asked to free a zero-length interval.  If so,
    // do nothing.
    if (old.m_start == old.m_end) { return; }
    old.assertSane();

    // The first element which is not considered to go before val
    // (i.e., either it is equivalent or goes after).
    iterator atOrAfter = lower_bound(ChannelInterval(old.m_start));
    iterator prevIterator = end();
    iterator nextIterator = end();

    for (iterator i = begin(); i != atOrAfter; ++i) {
        const ChannelInterval &cs = (*i);
        if (cs.getChannelId() == old.getChannelId() &&
            (cs.m_end == old.m_start)) {
            prevIterator = i;
            break;
        }
    }

    for (iterator i = atOrAfter; i != end(); ++i) {
        const ChannelInterval &cs = (*i);
        if (cs.getChannelId() == old.getChannelId() &&
            (cs.m_start == old.m_end)) {
            nextIterator = i;
            break;
        }
    }

    // Figure out the actual endpoints.
    const ChannelInterval &ciBefore =
        (prevIterator == end()) ? old : *prevIterator;
    
    const ChannelInterval &ciAfter = 
        (nextIterator == end()) ? old : *nextIterator;

    const ChannelInterval
        newChannelInterval(old.getChannelId(),
                           ciBefore.m_start,             ciAfter.m_end,
                           ciBefore.m_instrumentBefore,  ciAfter.m_instrumentAfter,
                           ciBefore.m_marginBefore,      ciAfter.m_marginAfter);
    
    // Physically remove the adjacent intervals that we are merging
    // with.
    if (prevIterator != end()) { erase(prevIterator); }
    if (nextIterator != end()) { erase(nextIterator); }

    newChannelInterval.assertSane();

    // Add a channelsegment incorporating the whole contiguous time.
    insert(newChannelInterval);
    
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

void
FreeChannels::dump()
{
    RG_DEBUG << "FreeChannels::Dump()";
    for (iterator I = begin(); I != end(); ++I) {
        RG_DEBUG << "  Channel:" << I->getChannelId();
        RG_DEBUG << "    Start:" << I->m_start;
        RG_DEBUG << "    End:" << I->m_end;
    }
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
        if (!isPercussion(i)) {
            (void)m_freeChannels.addChannel(i);
        }
    }
}

AllocateChannels::
~AllocateChannels(void)
{
    RG_DEBUG
        << "~AllocateChannels"
        << endl;
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
    RG_DEBUG
        << "reallocateToFit: Reallocating"
        << (instrument.isPercussion() ? "percussion" : "non-percussion")
        << instrument.getName() << instrument.getId()
        << "on bank"
        << (int)instrument.getMSB() << ":" << (int)instrument.getLSB() 
        << "channel "
        << ci.getChannelId()
        << endl;
    // If we already have a channel but it's the wrong type or it
    // changed instrument, always free it.
    if (ci.validChannel() &&
        ((changedInstrument && (end != ChannelInterval::m_latestTime)) || 
         (instrument.isPercussion() != (isPercussion(ci)))))
        { freeChannelInterval(ci); }

    if (instrument.isPercussion()) {
        // For single channel, this implicitly frees+reallocates
        ci = ChannelInterval(getPercussionChannel(), start, end,
                             NULL, NULL,
                             RealTime::zeroTime, RealTime::zeroTime);
    } else {
        m_freeChannels.reallocateToFit(ci, start, end,
                                       &instrument,
                                       marginBefore, marginAfter);
    }
    
    RG_DEBUG
        << "Now channel "
        << ci.getChannelId()
        << endl;
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
    reserveChannel(channel, m_fixedChannels);

    // If m_thruChannels has it, release it from there too.  Fixed
    // channels have priority over thru channels because their channel
    // numbers are chosen by the user.  We do this last to be very
    // sure we're in a nice clean state when ControlBlock looks for
    // another thru channel.
    FixedChannelSet::iterator i = m_thruChannels.find(channel);
    if (i != m_thruChannels.end())
    {
        // This statement is effectively releaseThruChannel, since we
        // already found the channel in m_thruChannels and we know
        // it's not going into m_freeChannels.
        m_thruChannels.erase(i);
        
        // Kick ControlBlock off this channel.  It will allocate
        // another.
        ControlBlock::getInstance()->vacateThruChannel(channel);
    }
}

ChannelId
AllocateChannels::
reallocateThruChannel(Instrument& instrument, ChannelId channel)
{
    // If we already have a valid channel and it has the right
    // percussion-ness, we're done.
    if (channel >= 0) {
        bool isInstrumentPercussion = instrument.isPercussion();
        bool isChannelPercussion    = isPercussion(channel);
        if (isInstrumentPercussion == isChannelPercussion) { return channel; }
    }

    // Otherwise, out with the old, in with the new.
    releaseThruChannel(channel);
    return allocateThruChannel(instrument);
}


// Allocate a channel for MIDI playthru
// The signal connections this uses are made by ChannelManager.
// @author Tom Breton (Tehom)
ChannelId
AllocateChannels::
allocateThruChannel(Instrument& instrument)
{
    if (instrument.isPercussion()) { return getPercussionChannel(); }
    
    // Quick and dirty: assume ChannelSetup::MIDI.  We inspect
    // channels highest-first because they tend to be used
    // lowest-first and we'd prefer not to collide.
    for (int channel = 15; channel >= 0; --channel) {
        // Avoid channels we've already reserved.
        if (m_thruChannels.find(channel) != m_thruChannels.end())
            { continue; }

        // Avoid any channel in m_fixedChannels.
        if (m_fixedChannels.find(channel) != m_fixedChannels.end())
            { continue; }

        // We don't try to find a good channel, we just accept the
        // first candidate.
        reserveChannel(channel, m_thruChannels);
        return channel;
    }

    // Fixed channels has all the channels, so return invalid channel.
    return -1;
}

// Release a reserved channel from channelSet
// @author Tom Breton (Tehom)
void
AllocateChannels::
releaseReservedChannel(ChannelId channel, FixedChannelSet& channelSet)
{
    // Releasing an invalid channel does nothing.
    if (channel < 0) { return; }
    // Releasing the percussion channel does nothing.
    if (isPercussion(channel)) { return; }
    
    FixedChannelSet::iterator i = channelSet.find(channel);

    if (i == channelSet.end()) { return; }
    RG_DEBUG
        << "AllocateChannels: releaseFixedChannel releasing"
        << (int) channel
        << endl;
    
    // Remove from reserved channels.
    channelSet.erase(i);

    // Add channel to FreeChannels (if not percussion)
    if (!isPercussion(channel)) {
        m_freeChannels.addChannel(channel);
    }
}

// Reserve a channel with regard to channelSet.
// @author Tom Breton (Tehom)
void
AllocateChannels::
reserveChannel(ChannelId channel, FixedChannelSet& channelSet)
{
    // Remove channel from FreeChannels (if not percussion) so we
    // don't give anything an interval on this channel.
    if (!isPercussion(channel)) {
        RG_DEBUG
            << "AllocateChannels: reserveFixedChannel reserving"
            << (int) channel
            << endl;
        m_freeChannels.removeChannel(channel);
    }
    // Record that this channel is reserved.  
    channelSet.insert(channel);

    // Kick ChannelManagers off the channel.  They'll get a new
    // channel; that's not a concern here.
    emit sigVacateChannel(channel);
}

}

#include "AllocateChannels.moc"
