/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CHANNELMANAGER_H
#define RG_CHANNELMANAGER_H

#include "base/ChannelInterval.h"
#include "base/Instrument.h"
#include "base/RealTime.h"
#include "base/Track.h"  // For TrackId

#include <QObject>

namespace Rosegarden
{

class AllocateChannels;
class Instrument;
class MappedEvent;
class MappedInserterBase;
class Segment;
class RosegardenDocument;

/// Set of controllers and pitchbends
/**
 * @author Tom Breton (Tehom)
 */
struct ControllerAndPBList
{
    ControllerAndPBList(void) :
        m_havePitchbend(false),
        m_pitchbend(0)
    { }

    ControllerAndPBList(StaticControllers &controllers) :
        m_controllers(controllers),
        m_havePitchbend(false),
        m_pitchbend(0)
    { }

    StaticControllers m_controllers;
    bool              m_havePitchbend;
    int               m_pitchbend;
};

/// Owns and services a ChannelInterval for an Instrument.
/**
 * ChannelManager's purpose is to own and service a ChannelInterval
 * (ChannelManager::m_channelInterval), relative to an Instrument that wants to
 * play on it.
 *
 * There is one ChannelManager for each MIDI Segment.  It is owned by the
 * InternalSegmentMapper for that Segment.  See CompositionMapper which
 * holds the InternalSegmentMapper instances for each Segment.
 *
 * ChannelManager objects are also owned by MetronomeMapper and
 * StudioControl (for preview notes, etc.).
 *
 * makeReady() is probably the main function here.  It allocates a channel
 * and inserts a channel setup into a MappedEventList via an inserter.
 *
 * insertEvent() is probably the most called function here.  It is used by
 * InternalSegmentMapper to insert all of the events from a Segment.
 *
 * Special cases it deals with:
 *
 *   - Eternal channels, e.g. for the metronome.  Call setEternalInterval()
 *     to make a channel interval that's guaranteed to be longer than the
 *     composition.
 *
 *   - Fixed channels, for which it doesn't use a channel allocator
 *     (AllocateChannels), but pretends to be doing all the same stuff.
 *
 * ChannelManager adapts to changes to the instrument, so it may become
 * fixed/unfixed and ready/unready as the instrument is changed.
 *
 * One way in which it services channels is by providing setup events (bank &
 * program, etc).  The note-producing source
 * calls it with an inserter, and ChannelManager puts the respective events
 * into the inserter and then (trusting the note-producing source to insert
 * those events) flags itself ready.
 *
 * @author Tom Breton (Tehom)
 */
class ChannelManager : public QObject
{
    Q_OBJECT

public:
    ChannelManager(Instrument *instrument);
    virtual ~ChannelManager()  { freeChannelInterval(); }

    /// Set the instrument we are playing on, releasing any old one.
    void setInstrument(Instrument *instrument);
    /// Get the instrument we are playing on.  Can return NULL.
    Instrument *getInstrument() const  { return m_instrument; }

    /// Set an interval that this ChannelManager must cover.
    /**
     * This does not do allocation.
     *
     * @see setEternalInterval(), allocateChannelInterval(), and makeReady()
     */
    void setRequiredInterval(RealTime start, RealTime end,
                             RealTime startMargin, RealTime endMargin)
    {
        m_start = start;
        m_end = end;
        m_startMargin = startMargin;
        m_endMargin = endMargin;
    }

    /// Set the interval to the maximum range.
    void setEternalInterval()
    {
        m_start = ChannelInterval::m_earliestTime;
        m_end = ChannelInterval::m_latestTime;
        m_startMargin = RealTime::zeroTime;
        m_endMargin = RealTime::zeroTime;
    }

    // *** Channel Interval Allocation

    /// Allocate a sufficient ChannelInterval in the current allocation mode.
    /**
     * If we already have a ChannelInterval, this will allocate a new one.
     *
     * @see freeChannelInterval() and AllocateChannels
     * @author Tom Breton (Tehom)
     */
    void allocateChannelInterval(bool changedInstrument);

    /// Free the owned ChannelInterval (m_channelInterval).
    /**
     * Safe to call even when m_usingAllocator is false.
     *
     * @see allocateChannelInterval() and AllocateChannels
     * @author Tom Breton (Tehom)
     */
    void freeChannelInterval();

    // *** Channel Setup

    // ??? Why are there all of these variations?  Can we somehow simplify
    //     this and get rid of all the variations?

    /// Insert BS, PC, CCs, and Pitch Bend for an Instrument on a channel.
    /**
     * Inserts the following:
     *
     *   - Bank Select from Instrument
     *   - Program Change from Instrument
     *   - Reset All Controllers (optional based on user preference)
     *   - Control Changes from controllerAndPBList
     *   - Pitchbend from controllerAndPBList
     */
    static void insertChannelSetup(
            TrackId trackId,
            const Instrument *instrument,
            ChannelId channel,
            RealTime insertTime,
            const ControllerAndPBList &controllerAndPBList,
            MappedInserterBase &inserter);

    /// Insert a single CC for an Instrument on a channel.
    static void insertController(
            TrackId trackId,
            const Instrument *instrument,
            ChannelId channel,
            RealTime insertTime,
            MidiByte controller,
            MidiByte value,
            MappedInserterBase &inserter);

    /// Insert appropriate channel setup (if needed) followed by an event.
    /**
     * Note: event is modified by this routine.
     *
     * @author Tom Breton (Tehom)
     */
    void insertEvent(
            TrackId trackId,
            const ControllerAndPBList &controllerAndPBList,
            RealTime reftime,
            MappedEvent &event,
            bool firstOutput,
            MappedInserterBase &inserter);

    /// Allocate a ChannelInterval and insert a channel setup.
    /**
     * @author Tom Breton (Tehom)
     */
    bool makeReady(
            TrackId trackId,
            RealTime time,
            const ControllerAndPBList &controllerAndPBList,
            MappedInserterBase &inserter);

    /// Insert a channel setup that is appropriate for the ChannelInterval.
    /**
     * @author Tom Breton (Tehom)
     */
    void insertChannelSetup(
            TrackId trackId,
            RealTime insertTime,
            const ControllerAndPBList &controllerAndPBList,
            MappedInserterBase &inserter);

    /// Indicate that a channel setup needs to go out.
    void setDirty()  { m_inittedForOutput = false; }

    /// Print our status, for tracing.
    void debugPrintStatus();

private slots:
    // *** AllocateChannels Signal Handler

    /// Something is kicking everything off "channel" in our device.
    /**
     * It is the signaller's responsibility to put AllocateChannels right (in
     * fact this signal only sent by AllocateChannels)
     *
     * Connected to AllocateChannels::sigVacateChannel().
     */
    void slotVacateChannel(ChannelId channel);

    // *** Instrument Signal Handlers

    /// Our Instrument and its entire Device are being destroyed.
    /**
     * This exists so we can take a shortcut.  We can skip setting the
     * device's allocator right since it's going away.
     *
     * Connected to Instrument::wholeDeviceDestroyed().
     */
    void slotLosingDevice();

    /// Our Instrument is being destroyed.
    /**
     * We may or may not have received slotLosingDevice first.
     *
     * Connected to Instrument::destroyed().
     */
    void slotLosingInstrument();

    /// Our Instrument now has different settings so we must reinit the channel.
    /**
     * Connected to Instrument::changedChannelSetup().
     */
    void slotInstrumentChanged();

    /// Our instrument now has a fixed channel.
    /**
     * Connected to Instrument::channelBecomesFixed().
     */
    void slotChannelBecomesFixed();

    /// Our instrument now lacks a fixed channel.
    /**
     * Connected to Instrument::channelBecomesUnfixed().
     */
    void slotChannelBecomesUnfixed();

private:
    // Hide copy ctor and op=
    ChannelManager(const ChannelManager &);
    ChannelManager &operator=(const ChannelManager &);

    /// The instrument this plays on.  I don't own this.
    Instrument *m_instrument;
    void connectInstrument(Instrument *instrument);

    /// Required start time.
    /**
     * m_channelInterval may be larger but never smaller than m_start to m_end.
     *
     * @see m_end, m_channelInterval
     */
    RealTime m_start;

    /// Required end time.
    /**
     * @see m_start, m_channelInterval
     */
    RealTime m_end;

    RealTime m_startMargin;

    RealTime m_endMargin;

    // *** Allocation

    /// The channel interval that is allocated for this segment.
    ChannelInterval m_channelInterval;

    /// Whether we are to get a channel interval thru Device's allocator.
    /**
     * The alternative is to get one as a fixed channel.  Can be true
     * even when we don't currently have a valid a channel.
     */
    bool m_usingAllocator;

    /// Whether we have tried to allocate a channel interval.
    /**
     * Does not imply success.  This allows some flexibility without
     * making us search again every time we insert a note.
     */
    bool m_triedToGetChannel;

    /// Get the channel allocator from the device.
    AllocateChannels *getAllocator();

    /// Set a fixed channel.
    /**
     * @see Instrument::getNaturalChannel()
     */
    void setChannelIdDirectly();

    /// Connect to allocator for sigVacateChannel().
    /**
     * ??? This could probably be inlined into its only caller.  But
     *     then disconnectAllocator() would look odd by itself.
     *
     * @see AllocateChannels::sigVacateChannel(), slotVacateChannel()
     */
    void connectAllocator();
    /// Disconnect from the allocator's signals.
    /**
     * We disconnect just when we don't have a valid channel given by
     * the allocator.  Note that this doesn't necessarily correspond
     * to m_usingAllocator's state.
     */
    void disconnectAllocator();

    /// Set m_usingAllocator appropriately for instrument.
    /**
     * It is safe to pass NULL here.
     */
    void setAllocationMode(Instrument *instrument);

    // *** Channel Setup

    /// Whether the output channel has been set up for m_channelInterval.
    /**
     * Whether makeReady() needs to be called.  insertEvent() is the main
     * user of this.
     *
     * Here we only deal with having the right channel.  doInsert()'s
     * firstOutput argument tells us if we need setup for some other
     * reason such as jumping in time.
     *
     * ??? rename: m_ready?
     */
    bool m_inittedForOutput;
};

}

#endif /* ifndef RG_CHANNELMANAGER_H */
