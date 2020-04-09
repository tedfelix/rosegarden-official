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

#define RG_MODULE_STRING "[ChannelManager]"
//#define RG_NO_DEBUG_PRINT 1

#include "ChannelManager.h"

#include "base/AllocateChannels.h"
#include "base/Instrument.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInserterBase.h"
#include "sound/Midi.h"

#include <QSettings>

namespace
{
    // Config settings
    // Keeping these at the file level since one of the users is static.
    bool allowReset;
    bool forceChannelSetups;
}

namespace Rosegarden
{


ChannelManager::ChannelManager(Instrument *instrument) :
    m_instrument(nullptr),
    m_start(),
    m_end(),
    m_startMargin(),
    m_endMargin(),
    m_channelInterval(),
    m_usingAllocator(false),
    m_triedToGetChannel(false),
    m_ready(false)
{
    // Safe even for nullptr.
    connectInstrument(instrument);

    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);

    // Allow CC 121 to be sent out to clear out CCs.
    // Edit > Preferences... > MIDI > General > Allow Reset All Controllers
    allowReset =
            settings.value("allowresetallcontrollers", "true").toBool();
    // Write back out so it is easy to find.
    settings.setValue("allowresetallcontrollers", allowReset);

    // Related to Bug #1560
    // When set to true, this causes MIPP channel setups to be sent at the
    // beginning of each Segment for fixed channel Segments.
    forceChannelSetups =
            settings.value("forceChannelSetups", "false").toBool();
    // Write back out so it is easy to find.
    settings.setValue("forceChannelSetups", forceChannelSetups);

}

void
ChannelManager::connectInstrument(Instrument *instrument)
{
    if (!instrument)
        return;

    // Disconnect the old instrument, if any.
    if (m_instrument)
        disconnect(m_instrument);

    // Connect to the new instrument
    connect(instrument, &Instrument::wholeDeviceDestroyed,
            this, &ChannelManager::slotLosingDevice);
    connect(instrument, &QObject::destroyed,
            this, &ChannelManager::slotLosingInstrument);
    connect(instrument, &Instrument::changedChannelSetup,
            this, &ChannelManager::slotInstrumentChanged);
    connect(instrument, &Instrument::channelBecomesFixed,
            this, &ChannelManager::slotChannelBecomesFixed);
    connect(instrument, &Instrument::channelBecomesUnfixed,
            this, &ChannelManager::slotChannelBecomesUnfixed);

    setAllocationMode(instrument);
    m_instrument = instrument;
    slotInstrumentChanged();
}

void ChannelManager::insertController(
        TrackId trackId,
        const Instrument *instrument,
        ChannelId channel,
        RealTime insertTime,
        MidiByte controller,
        MidiByte value,
        MappedInserterBase &inserter)
{
    MappedEvent mE(instrument->getId(),
                   MappedEvent::MidiController,
                   controller,
                   value);
    mE.setRecordedChannel(channel);
    mE.setEventTime(insertTime);
    mE.setTrackId(trackId);
    inserter.insertCopy(mE);
}

void ChannelManager::insertChannelSetup(
        TrackId trackId,
        const Instrument *instrument,
        ChannelId channel,
        RealTime insertTime,
        const ControllerAndPBList &controllerAndPBList,
        MappedInserterBase &inserter)
{
    // Bank Select

    if (!instrument->hasFixedChannel()  ||
        instrument->sendsBankSelect()) {
        {
            // Bank Select MSB
            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiController,
                           MIDI_CONTROLLER_BANK_MSB,
                           instrument->getMSB());
            mE.setRecordedChannel(channel);
            mE.setEventTime(insertTime);
            mE.setTrackId(trackId);
            inserter.insertCopy(mE);
        }
        {
            // Bank Select LSB
            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiController,
                           MIDI_CONTROLLER_BANK_LSB,
                           instrument->getLSB());
            mE.setRecordedChannel(channel);
            mE.setEventTime(insertTime);
            mE.setTrackId(trackId);
            inserter.insertCopy(mE);
        }
    }

    // Program Change

    if (!instrument->hasFixedChannel()  ||
        instrument->sendsProgramChange()) {
        // Program Change
        MappedEvent mE(instrument->getId(),
                       MappedEvent::MidiProgramChange,
                       instrument->getProgramChange());
        mE.setRecordedChannel(channel);
        mE.setEventTime(insertTime);
        mE.setTrackId(trackId);
        inserter.insertCopy(mE);
    }

    // Reset All Controllers

    if (allowReset) {
        // In case some controllers are on that we don't know about, turn
        // all controllers off.  (Reset All Controllers)
        try {
            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiController,
                           MIDI_CONTROLLER_RESET,
                           0);
            mE.setRecordedChannel(channel);
            mE.setEventTime(insertTime);
            mE.setTrackId(trackId);
            inserter.insertCopy(mE);
        } catch (...) {
            // Ignore.
        }
    }

    // Control Changes

    const StaticControllers &list = controllerAndPBList.m_controllers;

    // For each controller
    for (StaticControllers::const_iterator cIt = list.begin();
         cIt != list.end(); ++cIt) {
        const MidiByte controlId = cIt->first;
        const MidiByte controlValue = cIt->second;

        //RG_DEBUG << "insertChannelSetup() : inserting controller " << (int)controlId << "value" << (int)controlValue << "on channel" << (int)channel << "for time" << reftime;

        try {
            // Put it in the inserter.
            insertController(trackId, instrument, channel, insertTime,
                             controlId, controlValue, inserter);
        } catch (...) {
            // Ignore.
        }
    }

    // Pitch Bend

    // If there's a pitch bend, insert it...
    // We only do one type of pitchbend, though GM2 allows others.
    if (controllerAndPBList.m_havePitchbend) {
        const int pitchbend = controllerAndPBList.m_pitchbend;
        const int d1 = (pitchbend >> 7) & 0x7f;
        const int d2 = pitchbend & 0x7f;

        try {
            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiPitchBend,
                           d1,
                           d2);
            mE.setRecordedChannel(channel);
            mE.setEventTime(insertTime);
            mE.setTrackId(trackId);
            inserter.insertCopy(mE);
        } catch (...) {
            // Ignore.
        }
    }
}

void ChannelManager::insertEvent(
        TrackId trackId,
        const ControllerAndPBList &controllerAndPBList,
        RealTime reftime,
        MappedEvent &event,
        bool /*firstOutput*/,
        MappedInserterBase &inserter)
{
    //Profiler profiler("ChannelManager::insertEvent()", true);

    //RG_DEBUG << "insertEvent(): playing on" << (m_instrument ? m_instrument->getPresentationName().c_str() : "nothing") << "at" << reftime << (firstOutput ? "needs init" : "doesn't need init");

    // ??? firstOutput was always ignored.  What would happen if we actually
    //     honored it?  E.g.:
    //       if (firstOutput)
    //           m_ready = false;

    // We got here without being ready.  This might happen briefly
    // if a track becomes unmuted, until the meta-iterator gets around
    // to initting.
    if (!m_ready) {
        if (bug1560Logging())
            RG_DEBUG << "insertEvent(): Calling makeReady()...";
        makeReady(trackId, reftime, controllerAndPBList, inserter);
        // If we're still not ready, we can't do much.
        if (!m_ready)
            return;
    }

    // !!! These checks may not be needed now, could become assertions.
    if (!m_instrument)
        return;
    if (!m_channelInterval.validChannel())
        return;

    event.setInstrument(m_instrument->getId());
    event.setRecordedChannel(m_channelInterval.getChannelId());
    event.setTrackId(trackId);
    inserter.insertCopy(event);
}

bool ChannelManager::makeReady(
        TrackId trackId,
        RealTime time,
        const ControllerAndPBList &controllerAndPBList,
        MappedInserterBase &inserter)
{
    //RG_DEBUG << "makeReady() for" << (m_instrument ? m_instrument->getPresentationName().c_str() : "nothing") << "at" << time;

    // We don't even have an instrument to play on.
    if (!m_instrument)
        return false;

    if (bug1560Logging())
        RG_DEBUG << "makeReady() for" << (m_instrument ? m_instrument->getPresentationName().c_str() : "nothing") << "at" << time;

    // Try to get a valid channel if we lack one.
    if (!m_channelInterval.validChannel()) {
        // We already tried to get one and failed; don't keep trying.
        if (m_triedToGetChannel)
            return false;
        
        // Try to get a channel.  This sets m_triedToGetChannel.
        allocateChannelInterval(false);
        
        // If we still don't have one, give up.
        if (!m_channelInterval.validChannel())
            return false;
    }

    // Figure out whether playback is starting in the middle of a
    // Segment.  Bug #1560.
    // Note: This also causes channel setups on channel 10 due to the
    //       MetronomeMapper.  Not sure if that's an issue, but might be.
    bool startingInMiddle =
            m_instrument->hasFixedChannel()  &&
            (m_start < time  &&  time <= m_end);

    if (bug1560Logging()) {
        RG_DEBUG << "makeReady()";
        RG_DEBUG << "  m_start =" << m_start;
        RG_DEBUG << "  m_end =" << m_end;
        RG_DEBUG << "  time =" << time;
        RG_DEBUG << "  startingInMiddle =" << startingInMiddle;
    }

    // If this instrument is in auto channels mode or we are starting
    // in the middle of a Segment.
    if (!m_instrument->hasFixedChannel()  ||  forceChannelSetups  ||
        startingInMiddle) {

        insertChannelSetup(
                trackId,
                time,
                controllerAndPBList,
                inserter);
    }
    
    m_ready = true;

    return true;
}

void
ChannelManager::insertChannelSetup(
        TrackId trackId,
        RealTime insertTime,
        const ControllerAndPBList &controllerAndPBList,
        MappedInserterBase &inserter)
{
    //RG_DEBUG << "insertChannelSetup() : " << (m_instrument ? "Got instrument" : "No instrument");

    if (!m_instrument)
        return;
    if (!m_channelInterval.validChannel())
        return;

    //RG_DEBUG << "  Instrument type is " << (int)m_instrument->getType();

    // We don't do this for SoftSynth instruments.
    if (m_instrument->getType() == Instrument::Midi) {
        ChannelId channel = m_channelInterval.getChannelId();
        insertChannelSetup(trackId, m_instrument, channel, insertTime,
                           controllerAndPBList, inserter);
    }
}

void
ChannelManager::setChannelIdDirectly()
{
    Q_ASSERT(!m_usingAllocator);

    ChannelId channel = m_instrument->getNaturalChannel();

    if (m_instrument->getType() == Instrument::Midi) {
        // !!! Stopgap measure.  If we ever share allocators between
        // MIDI devices, this will have to become smarter.
        if (m_instrument->isPercussion()) {
            channel = (m_instrument->hasFixedChannel() ?
                       m_instrument->getNaturalChannel() : 9);
        }
    }

    m_channelInterval.setChannelId(channel);
}

AllocateChannels *
ChannelManager::getAllocator()
{
    Q_ASSERT(m_usingAllocator);

    if (!m_instrument)
        return nullptr;

    return m_instrument->getDevice()->getAllocator();
}

void
ChannelManager::connectAllocator()
{
    Q_ASSERT(m_usingAllocator);

    if (!m_channelInterval.validChannel())
        return;

    connect(getAllocator(), &AllocateChannels::sigVacateChannel,
            this, &ChannelManager::slotVacateChannel,
            Qt::UniqueConnection);
}

void
ChannelManager::disconnectAllocator()
{
    if (m_instrument  &&  m_usingAllocator)
        disconnect(getAllocator(), nullptr, this, nullptr);
}

void
ChannelManager::setAllocationMode(Instrument *instrument)
{
    if (!instrument) {
        m_usingAllocator = false;
    } else {
        bool wasUsingAllocator = m_usingAllocator;

        switch (instrument->getType()) {
        case Instrument::Midi :
            m_usingAllocator = !instrument->hasFixedChannel();
            break;

        case Instrument::SoftSynth:
            m_usingAllocator = false;
            break;

        case Instrument::Audio:
        case Instrument::InvalidInstrument:
        default:
            RG_WARNING << "setAllocationMode() : Got an audio or unrecognizable instrument type.";
            break;
        }

        // If the allocation mode has changed, clear m_channelInterval,
        // otherwise its old value will appear valid.
        if (m_usingAllocator != wasUsingAllocator)
            m_channelInterval.clearChannelId();
    }
}    

void
ChannelManager::allocateChannelInterval(bool changedInstrument)
{
    //RG_DEBUG << "allocateChannelInterval(): " << (m_usingAllocator ? "using allocator" : "not using allocator") << "for" << (void *)m_instrument;

    if (m_instrument) {
        if (m_usingAllocator) {
            // Only Midi instruments should have m_usingAllocator set.
            Q_ASSERT(m_instrument->getType() == Instrument::Midi);

            getAllocator()->reallocateToFit(
                    *m_instrument, m_channelInterval,
                    m_start, m_end,
                    m_startMargin, m_endMargin,
                    changedInstrument);

            connectAllocator();
        } else {
            setChannelIdDirectly();
        }
    }

    //RG_DEBUG << "allocateChannelInterval(): Channel is " << (m_channelInterval.validChannel() ? "valid" : "INVALID");

    m_triedToGetChannel = true;
}

void ChannelManager::freeChannelInterval()
{
    if (m_instrument  &&  m_usingAllocator) {
        AllocateChannels *allocator = getAllocator();

        if (allocator) {
            allocator->freeChannelInterval(m_channelInterval);
            disconnectAllocator();
        }

        m_triedToGetChannel = false;
    }
}

void
ChannelManager::setInstrument(Instrument *instrument)
{
    //RG_DEBUG << "setInstrument(): Setting instrument to" << (void *)instrument << "It was" << (void *)m_instrument;

    // No change?  Bail.
    if (instrument == m_instrument)
        return;

    if (m_instrument) {
        Device *oldDevice = m_instrument->getDevice();
        Device *newDevice = instrument ? instrument->getDevice() : nullptr;
        // Don't hold onto a channel on a device we're no longer
        // playing thru.  Even if newDevice == 0, we free oldDevice's
        // channel.
        if (oldDevice != newDevice)
            freeChannelInterval();
    }

    allocateChannelInterval(true);
    connectInstrument(instrument);
    m_ready = false;
}

void
ChannelManager::slotVacateChannel(ChannelId channel)
{
    // Not our channel?  Bail.
    if (m_channelInterval.getChannelId() != channel)
        return;

    m_channelInterval.clearChannelId();
    disconnectAllocator();
}


void
ChannelManager::slotLosingDevice()
{
    m_instrument = nullptr;
    m_channelInterval.clearChannelId();
}

void
ChannelManager::slotLosingInstrument()
{
    freeChannelInterval();
    m_instrument = nullptr;
}

void
ChannelManager::slotChannelBecomesFixed()
{
    //RG_DEBUG << "slotChannelBecomesFixed()" << (m_usingAllocator ? "using allocator" : "not using allocator") << "for" << (void *)m_instrument;

    ChannelId channel = m_instrument->getNaturalChannel();

    // If we're already fixed and set to our natural channel, there's nothing
    // to do.
    if (!m_usingAllocator  &&  (channel == m_channelInterval.getChannelId()))
        return;

    // Free the channel that we had (safe even if already fixed)
    freeChannelInterval();
    m_usingAllocator = false;

    // Set the new channel.
    setChannelIdDirectly();
    m_ready = false;
}

void
ChannelManager::slotChannelBecomesUnfixed()
{
    //RG_DEBUG << "slotChannelBecomesUnfixed" << (m_usingAllocator ? "using allocator" : "not using allocator") << "for" << (void *)m_instrument;

    // If we were already unfixed, do nothing.
    if (m_usingAllocator)
        return;

    m_usingAllocator = true;
    // We no longer have a channel interval.
    m_channelInterval.clearChannelId();
    // Get a new one.
    allocateChannelInterval(false);
    m_ready = false;
}

void
ChannelManager::slotInstrumentChanged()
{
    m_triedToGetChannel = false;

    // Reset to the fixedness of the instrument.  This is safe even
    // when fixedness hasn't really changed.
    if (m_instrument) {
        if (m_instrument->hasFixedChannel()  ||
            m_instrument->getType() != Instrument::Midi) {
            slotChannelBecomesFixed();
        } else {
            slotChannelBecomesUnfixed();
        }
    }

    // The above code won't always set dirty flag, so set it now.
    m_ready = false;
}


}
