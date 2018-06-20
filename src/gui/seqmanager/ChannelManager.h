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

#include <QObject>

namespace Rosegarden
{

class AllocateChannels;
class Instrument;
class MappedEvent;
class MappedInserterBase;
class Segment;
class RosegardenDocument;
typedef unsigned int TrackId;

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

/// Owns and services a channel interval for an Instrument.
/**
 * ChannelManager's purpose is to own and service a channel interval
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
    virtual ~ChannelManager(void)  { freeChannelInterval(); }

    /// Set the instrument we are playing on, releasing any old one.
    void setInstrument(Instrument *instrument);
    /// Get the instrument we are playing on.  Can return NULL.
    Instrument *getInstrument(void) const  { return m_instrument; }

    // *** MappedEvent Insertion functions

    /// Insert Bank Select and Program Change for an Instrument on a channel.
    /**
     * This routine is essentially a conversion from Instrument to
     * MappedEvents which are inserted into a MappedEventBuffer via
     * an inserter.
     */
    static void insertBSAndPC(
            int trackId,
            const Instrument *instrument,
            ChannelId channel,
            RealTime insertTime,
            MappedInserterBase &inserter);

    /// Insert controllers and pitch bend via inserter.
    /**
     * Set default controllers for instrument on channel.  Inserts the
     * following via the inserter:
     *
     *   - Reset All Controllers
     *   - Control Changes from controllerAndPBList
     *   - Pitchbend from controllerAndPBList
     *
     * Adapted from SequenceManager.
     */
    static void insertControllers(
        ChannelId channel, 
        Instrument *instrument,
        MappedInserterBase &inserter,
        RealTime reftime, 
        RealTime insertTime,
        const ControllerAndPBList &controllerAndPBList,
        int trackId);

    static void insertController(
        ChannelId channel, 
        const Instrument *instrument,
        MappedInserterBase &inserter, 
        RealTime insertTime,
        int trackId, 
        MidiByte controller, 
        MidiByte value);

    /// Free the owned channel interval (m_channelInterval).
    /**
     * Safe even when m_usingAllocator is false.
     */
    void freeChannelInterval(void);

    /// Insert event via inserter, pre-inserting appropriate channel setup.
    void doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                RealTime reftime,
                const ControllerAndPBList &controllerAndPBList,
                bool firstOutput, TrackId trackId);

    bool makeReady(MappedInserterBase &inserter, RealTime time,
            const ControllerAndPBList &controllerAndPBList, TrackId trackId);

    /// Insert appropriate MIDI channel-setup.
    void insertChannelSetup(MappedInserterBase &inserter,
                            RealTime reftime, RealTime insertTime,
                            const ControllerAndPBList &controllerAndPBList,
                            int trackId);

    void setDirty(void)  { m_inittedForOutput = false; }

    /// Set an interval that this ChannelManager must cover.
    /**
     * This does not do allocation.
     */
    void setRequiredInterval(RealTime start, RealTime end,
                             RealTime startMargin, RealTime endMargin)
    {
        m_start       = start;
        m_end         = end;
        m_startMargin = startMargin;
        m_endMargin   = endMargin;
    }

    /// Set the interval to the maximum range.
    void setEternalInterval()
    {
        setRequiredInterval(ChannelInterval::m_earliestTime,
                            ChannelInterval::m_latestTime,
                            RealTime::zeroTime,
                            RealTime::zeroTime);
    }

    /// Allocate a sufficient channel interval in the current allocation mode.
    /*
     * It is safe to call this more than once, ie even if we already have a
     * channel interval.
     */
    void reallocate(bool changedInstrument);

    /// Print our status, for tracing.
    void debugPrintStatus(void);

private slots:
    /// Something is kicking everything off "channel" in our device.
    /**
     * It is the signaller's responsibility to put AllocateChannels right (in
     * fact this signal only sent by AllocateChannels)
     */
    void slotVacateChannel(ChannelId channel);
    /// Our instrument and its entire device are being destroyed.
    /**
     * This exists so we can take a shortcut.  We can skip setting the
     * device's allocator right since it's going away.
     */
    void slotLosingDevice(void);
    /// Our instrument is being destroyed.
    /**
     * We may or may not have received slotLosingDevice first.
     */
    void slotLosingInstrument(void);

    /// Our instrument now has different settings so we must reinit the channel.
    void slotInstrumentChanged(void);

    /// Our instrument now has a fixed channel.
    void slotChannelBecomesFixed(void);
    /// Our instrument now lacks a fixed channel.
    void slotChannelBecomesUnfixed(void);

private:
    // Hide copy ctor and op=
    ChannelManager(const ChannelManager &);
    ChannelManager &operator=(const ChannelManager &);

    /// Connect signals from instrument.  Safe even for NULL.
    void connectInstrument(Instrument *instrument);

    /**************************/
    /*** Internal functions ***/

    /*** Functions about allocating. ***/

    /// Get the channel allocator (AllocateChannels) from the device.
    AllocateChannels *getAllocator(void);
    /// Set a fixed channel.
    /**
     * @see Instrument::getNaturalChannel()
     */
    void setChannelIdDirectly(void);

    /// Connect to allocator for sigVacateChannel().
    /**
     * @see AllocateChannels::sigVacateChannel(), slotVacateChannel()
     */
    void connectAllocator(void);
    /// Disconnect from the allocator's signals.
    /**
     * We disconnect just when we don't have a valid channel given by
     * the allocator.  Note that this doesn't necessarily correspond
     * to m_usingAllocator's state.
     */
    void disconnectAllocator(void);

    /// Set m_usingAllocator appropriately for instrument.
    /**
     * It is safe to pass NULL here.
     */
    void setAllocationMode(Instrument *instrument);

    /*** Functions about setting up the channel ***/

    void setInitted(bool initted)  { m_inittedForOutput = initted; }
    bool needsInit(void)  { return !m_inittedForOutput; }

    /********************/
    /*** Data members ***/

    /// The channel interval that is allocated for this segment.
    /**
     * rename: m_channelInterval
     */
    ChannelInterval m_channelInterval;

    /// Whether we are to get a channel interval thru Device's allocator.
    /**
     * The alternative is to get one as a fixed channel.  Can be true
     * even when we don't currently have a valid a channel.
     */
    bool m_usingAllocator;

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

    /// Margins required if instrument has changed.
    RealTime m_startMargin, m_endMargin;

    /// The instrument this plays on.  I don't own this.
    Instrument *m_instrument;

    /// Whether the output channel has been set up for m_channelInterval.
    /**
     * Here we only deal with having the right channel.  doInsert()'s
     * firstOutput argument tells us if we need setup for some other
     * reason such as jumping in time.
     */
    bool m_inittedForOutput;
    /// Whether we have tried to allocate a channel interval.
    /**
     * Does not imply success.  This allows some flexibility without
     * making us search again every time we insert a note.
     */
    bool m_triedToGetChannel;
};

}

#endif /* ifndef RG_CHANNELMANAGER_H */
