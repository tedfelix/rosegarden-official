/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2013 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ChannelManager.h"
#include "base/AllocateChannels.h"
#include "base/Instrument.h"
#include "base/MidiTypes.h"
#include "misc/Debug.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInserterBase.h"
#include "sound/Midi.h"

#include <cassert>

// #define DEBUG_CHANNEL_MANAGER 1

namespace Rosegarden
{

// ctor for ChannelManager
// @author Tom Breton (Tehom) 
ChannelManager::
ChannelManager(Instrument *instrument) :
    m_usingAllocator(false),
    m_instrument(0),
    m_inittedForOutput(false),
    m_triedToGetChannel(false)
{
    // Safe even for NULL.
    connectInstrument(instrument);
}

// Connect signals from instrument.
// Safe even for NULL.
// @author Tom Breton (Tehom) 
void
ChannelManager::
connectInstrument(Instrument *instrument)
{
    if (!instrument) { return; }

    // Disconnect the old instrument, if any.
    if (m_instrument)
        { disconnect(m_instrument); }

    // Connect to the new instrument
    connect(instrument, SIGNAL(wholeDeviceDestroyed()),
            this, SLOT(slotLosingDevice()));
    connect(instrument, SIGNAL(destroyed()),
            this, SLOT(slotLosingInstrument()));
    connect(instrument, SIGNAL(changedChannelSetup()),
            this, SLOT(slotInstrumentChanged()));
    connect(instrument, SIGNAL(channelBecomesFixed()),
            this, SLOT(slotChannelBecomesFixed()));
    connect(instrument, SIGNAL(channelBecomesUnfixed()),
            this, SLOT(slotChannelBecomesUnfixed()));

    setAllocationMode(instrument);
    m_instrument = instrument;
    slotInstrumentChanged();
}

void
ChannelManager::
insertController(ChannelId channel, const Instrument *instrument,
                 MappedInserterBase &inserter, RealTime insertTime,
                 int trackId, MidiByte controller, MidiByte value)
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

// Set default controllers for instrument on channel.
// Adapted from SequenceManager
void
ChannelManager::
setControllers(ChannelId channel, Instrument *instrument,
               MappedInserterBase &inserter,
               RealTime reftime, RealTime insertTime,
               MapperFunctionality *functionality, int trackId)
{
#if 0
    // This was the old logic, but it's not clear that it is still
    // desirable.
    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);
    bool sendControllers = qStrToBool(settings.value("alwayssendcontrollers", "false")) ;
    settings.endGroup();

    if (instrument->hasFixedChannel() && 
        !sendControllers) { return; }
#endif

    // In case some controllers are on that we don't know about, turn
    // all controllers off.
    try {
        const int controllerAllControllersOff = 121;
        MappedEvent mE(instrument->getId(),
                       MappedEvent::MidiController,
                       controllerAllControllersOff,
                       0);
        mE.setRecordedChannel(channel);
        mE.setEventTime(insertTime);
        mE.setTrackId(trackId);
        inserter.insertCopy(mE);
    } catch (...) { }
        
    // Get the appropriate controllers from the callback our mapper
    // gave us.
    ControllerAndPBList CAndPBlist =
        functionality->getControllers(instrument, reftime);
    StaticControllers& list = CAndPBlist.m_controllers;
    for (StaticControllerConstIterator cIt = list.begin();
         cIt != list.end(); ++cIt) {
        MidiByte controlId    = cIt->first;
        MidiByte controlValue = cIt->second;
#ifdef DEBUG_CHANNEL_MANAGER
    SEQMAN_DEBUG << "ChannelManager::setControllers() : sending controller "
                 << (int)controlId
                 << "value"
                 << (int)controlValue
                 << "on channel"
                 << (int)channel
                 << "for time"
                 << reftime
                 << endl;
#endif
        try {
            insertController
                (channel, instrument, inserter, insertTime, trackId,
                 controlId, controlValue);
        } catch (...) { continue; }
    }

    // We only do one type of pitchbend, though GM2 allows others.
    if (CAndPBlist.m_havePitchbend) {
        int raised = CAndPBlist.m_pitchbend + 8192;
        int d1 = (raised >> 7) & 0x7f;
        int d2 = raised & 0x7f;
            
        try {
            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiPitchBend,
                           d1,
                           d2);
            mE.setRecordedChannel(channel);
            mE.setEventTime(insertTime);
            mE.setTrackId(trackId);
            inserter.insertCopy(mE);
        } catch (...) { }
    }
}

// Send program control for instrument on channel.
// Adapted from SequenceManager
void
ChannelManager::
sendProgramForInstrument(ChannelId channel, Instrument *instrument,
                         MappedInserterBase &inserter,
                         RealTime insertTime, int trackId) 
{
#ifdef DEBUG_CHANNEL_MANAGER
    SEQMAN_DEBUG << "ChannelManager::sendProgramForInstrument() : sending prg change for "
                 << instrument->getPresentationName().c_str()
                 << "on channel"
                 << (int)channel
                 << endl;
#endif

    // Send bank select before program change unless we have a fixed
    // channel and no sendsBankSelect.
    if (!instrument->hasFixedChannel() ||
        instrument->sendsBankSelect()) {
        {
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

    // Send program change unless we have a fixed channel and no
    // sendsProgramChange.
    if (!instrument->hasFixedChannel() ||
        instrument->sendsProgramChange()) {
        MappedEvent mE(instrument->getId(),
                       MappedEvent::MidiProgramChange,
                       instrument->getProgramChange());

        mE.setRecordedChannel(channel);
        mE.setEventTime(insertTime);
        mE.setTrackId(trackId);
        inserter.insertCopy(mE);
    }
}

// Insert evt, possibly pre-inserting other events to configure the
// channel.
// @author Tom Breton (Tehom) 
void
ChannelManager::doInsert(MappedInserterBase &inserter, MappedEvent &evt, 
                         RealTime reftime,
                         MapperFunctionality *functionality,
                         bool firstOutput, int trackId)
{
#ifdef DEBUG_CHANNEL_MANAGER
    SEQUENCER_DEBUG
        << "ChannelManager::doInsert"
        << "playing on"
        << (m_instrument ?
            m_instrument->getPresentationName().c_str() :
            "nothing")
        << "at"
        << reftime
        << (firstOutput
            ? "needs init"
            : "doesn't need init")
        << endl;
#endif
    if (!m_channel.validChannel()) {
        // We already tried to init it and failed; don't keep trying.
        if (m_triedToGetChannel) { return; }
        // Try to get a channel.  This sets m_triedToGetChannel.
        reallocate(false);
        // If we still don't have one, give up.
        if (!m_channel.validChannel()) { return; }
        }
    RealTime insertTime = evt.getEventTime();
        // !!! Should probably just abort if there's no instrument.
    if (m_instrument) {
        if (firstOutput || needsInit()) {
            insertChannelSetup(inserter, reftime, insertTime,
                               functionality, trackId);
            setInitted(true);
        }
        evt.setInstrument(m_instrument->getId());
        evt.setRecordedChannel(m_channel.getChannelId());
    }
    evt.setTrackId(trackId);
    inserter.insertCopy(evt);
}

// Insert appropriate MIDI channel-setup
// @author Tom Breton (Tehom) 
void
ChannelManager::insertChannelSetup(MappedInserterBase &inserter,
                                   RealTime reftime, RealTime insertTime,
                                   MapperFunctionality *functionality,
                                   int trackId)
{
#ifdef DEBUG_CHANNEL_MANAGER
    SEQUENCER_DEBUG
        << (m_instrument
            ? "Got instrument"
            : "No instrument")
        << endl;
    if (m_instrument) {
        SEQUENCER_DEBUG << "Instrument type is "
                        << (int)m_instrument->getType()
                        << endl;
    }
#endif

    if (!m_channel.validChannel()) { return; }
    // We don't do this for SoftSynth instruments.
    if (m_instrument &&
        (m_instrument->getType() == Instrument::Midi)) {
        ChannelId channel = m_channel.getChannelId();
        sendProgramForInstrument(channel, m_instrument, inserter,
                                 insertTime, trackId );
        setControllers(channel, m_instrument, inserter, reftime,
                       insertTime, functionality, trackId);
    }
}

// Set a fixed channel when we're not using an allocator.
// @author Tom Breton (Tehom) 
void
ChannelManager::
setChannelIdDirectly(void)
{
    assert(!m_usingAllocator);
    ChannelId channel = m_instrument->getNaturalChannel();
    if (m_instrument->getType() == Instrument::Midi) {
        // !!! Stopgap measure.  If we ever share allocators between
        // MIDI devices, this will have to become smarter.
        if (m_instrument->isPercussion()) { channel = 9; }
    }
    m_channel.setChannelId(channel);
}


// Get the allocator
// @author Tom Breton (Tehom) 
AllocateChannels *
ChannelManager::
getAllocator(void)
{
    assert(m_usingAllocator);
    if (!m_instrument) { return 0; }
    Device *device = m_instrument->getDevice();
    return device->getAllocator();
}

// Connect signals to the allocator
// @author Tom Breton (Tehom) 
void
ChannelManager::
connectAllocator(void)
{
    assert(m_usingAllocator);
    if (!m_channel.validChannel()) { return; }
    connect(getAllocator(), SIGNAL(sigVacateChannel(ChannelId)),
            this, SLOT(slotVacateChannel(ChannelId)),
            Qt::UniqueConnection);
}

// Disconnect from the allocator's signals
// @author Tom Breton (Tehom) 
void
ChannelManager::
disconnectAllocator(void)
{
    if (m_instrument && m_usingAllocator) {
        disconnect(getAllocator(), 0, this, 0);
    }
}

// Set m_usingAllocator appropriately for instrument.  It is safe to
// pass NULL here.
// @author Tom Breton (Tehom) 
void
ChannelManager::
setAllocationMode(Instrument *instrument)
{
    if (!instrument)
        { m_usingAllocator = false; }
    else
        {
            bool wasUsingAllocator = m_usingAllocator;
            switch (instrument->getType()) {
            case Instrument::Midi :
                m_usingAllocator = !instrument->hasFixedChannel();
                break;
            case Instrument::SoftSynth:
                m_usingAllocator = false;
                break;
            case Instrument::Audio:
            default:
#ifdef DEBUG_CHANNEL_MANAGER
                SEQMAN_DEBUG << "ChannelManager::connectInstrument() : Got an "
                    "audio or unrecognizable instrument type."
                             << endl;
#endif
                break;
            }

            // Clear m_channel, otherwise its old value will appear valid.
            if (m_usingAllocator != wasUsingAllocator) {
                m_channel.clearChannelId();
            }
        }
}    

// Allocate a sufficient channel interval in the current allocation mode.
// @author Tom Breton (Tehom) 
void
ChannelManager::reallocate(bool changedInstrument)
{
#ifdef DEBUG_CHANNEL_MANAGER
    SEQUENCER_DEBUG << "ChannelManager::reallocate "
                    << (m_usingAllocator ? "using allocator" :
                        "not using allocator")
                    << "for"
                    << (void *)m_instrument
                    << endl;
#endif
    if (m_instrument) {
        if (m_usingAllocator) {
            // Only Midi instruments should have m_usingAllocator set.
            assert(m_instrument->getType() == Instrument::Midi);
            getAllocator()->
                reallocateToFit(*m_instrument, m_channel,
                                m_start, m_end,
                                m_startMargin, m_endMargin,
                                changedInstrument);
            connectAllocator();
        } else {
            setChannelIdDirectly();
        }
    }

#ifdef DEBUG_CHANNEL_MANAGER
    if (m_channel.validChannel()) {
        SEQUENCER_DEBUG << "  Channel is valid";
    } else {
        SEQUENCER_DEBUG << "  ??? Channel is invalid!  (end of reallocate())";
    }
#endif

    m_triedToGetChannel = true;
}

// Free the channel interval it owned.  Safe even when
// m_usingAllocator is false.
// @author Tom Breton (Tehom) 
void ChannelManager::freeChannelInterval(void)
{
    if (m_instrument && m_usingAllocator) {
        AllocateChannels *allocater = getAllocator();
        if (allocater) {
            allocater->freeChannelInterval(m_channel);
            disconnectAllocator();
        }
        m_triedToGetChannel = false;
    }
}

// Set the instrument we are playing on, releasing any old one.
// @author Tom Breton (Tehom)
void
ChannelManager::
setInstrument(Instrument *instrument)
{
#ifdef DEBUG_CHANNEL_MANAGER
    SEQUENCER_DEBUG << "ChannelManager::setInstrument: Setting instrument to" 
                    << (void *)instrument
                    << "It was"
                    << (void *)m_instrument
                    << endl;
#endif
    if (instrument != m_instrument) {
        if (m_instrument) {
            Device *oldDevice = m_instrument->getDevice();
            Device *newDevice = instrument ? instrument->getDevice() : 0;
            // Don't hold onto a channel on a device we're no longer
            // playing thru.  Even if newDevice == 0, we free oldDevice's
            // channel.
            if (oldDevice != newDevice) {
                freeChannelInterval();
            }
        }
        reallocate(true);
        connectInstrument(instrument);
        setDirty();
    }
}

// Print our status, for tracing.
// @author Tom Breton (Tehom) 
void
ChannelManager::
debugPrintStatus(void)
{
#ifdef DEBUG_CHANNEL_MANAGER
    SEQUENCER_DEBUG
        << "ChannelManager "
        << (m_inittedForOutput ? "doesn't need" : "needs")
        << "initting"
        << endl;
#endif    
}

// Something is kicking everything off "channel" in our device.  It is
// the signaller's responsibility to put AllocateChannels right (in
// fact this signal only sent by AllocateChannels)
// @author Tom Breton (Tehom) 
void
ChannelManager::
slotVacateChannel(ChannelId channel)
{
    if (m_channel.getChannelId() == channel) {
        m_channel.clearChannelId();
        disconnectAllocator();
    }
}


// Our instrument and its entire device are being destroyed.  We can
// skip setting the device's allocator right since it's going away.
// @author Tom Breton (Tehom) 
void
ChannelManager::
slotLosingDevice(void)
{
    m_instrument = 0;
    m_channel.clearChannelId();
}

// Our instrument is being destroyed.  We may or may not have
// received slotLosingDevice first.
// @author Tom Breton (Tehom) 
void
ChannelManager::
slotLosingInstrument(void)
{
    freeChannelInterval();
    m_instrument = 0;
}

// Our instrument's channel is now fixed.
// @author Tom Breton (Tehom) 
void
ChannelManager::
slotChannelBecomesFixed(void)
{
#ifdef DEBUG_CHANNEL_MANAGER
    SEQUENCER_DEBUG << "ChannelManager::slotChannelBecomesFixed" 
                    << (m_usingAllocator ? "using allocator" :
                        "not using allocator")
                    << "for"
                    << (void *)m_instrument
                    << endl;
#endif
    ChannelId channel = m_instrument->getNaturalChannel();
    if (!m_usingAllocator && (channel == m_channel.getChannelId()))
        { return; }

    // Free the channel that we had (safe even if already fixed)
    freeChannelInterval();
    m_usingAllocator = false;

    // Set the new channel.
    setChannelIdDirectly();
    setDirty();
}

// Our instrument's channel is now unfixed.
// @author Tom Breton (Tehom) 
void
ChannelManager::
slotChannelBecomesUnfixed(void)
{
#ifdef DEBUG_CHANNEL_MANAGER
    SEQUENCER_DEBUG << "ChannelManager::slotChannelBecomesUnfixed" 
                    << (m_usingAllocator ? "using allocator" :
                        "not using allocator")
                    << "for"
                    << (void *)m_instrument
                    << endl;
#endif
    // If we were already unfixed, do nothing.
    if (m_usingAllocator) { return; }

    m_usingAllocator = true;
    // We no longer have a channel interval.
    m_channel.clearChannelId();
    // Get a new one.
    reallocate(false);
    setDirty();
}

// Our instrument has changed how to set up the channel.
// @author Tom Breton (Tehom) 
void
ChannelManager::
slotInstrumentChanged(void)
{
    // Reset to the fixedness of the instrument.  This is safe even
    // when fixedness hasn't really changed.
    if(m_instrument) {
        if(m_instrument->hasFixedChannel() ||
           (m_instrument->getType() != Instrument::Midi))
            { slotChannelBecomesFixed(); }
        else
            { slotChannelBecomesUnfixed(); }
    }

    // The above code won't always set dirty flag, so set it now.
    setDirty();
}

/***  MetronomeMapper::Callback ***/

ControllerAndPBList
ChannelManager::MapperFunctionalitySimple::
getControllers(Instrument *instrument, RealTime /*start*/)
{
    return ControllerAndPBList(instrument->getStaticControllers());
}

}

#include "ChannelManager.moc"
