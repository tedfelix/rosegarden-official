/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2026 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ControlBlock]"

#define RG_NO_DEBUG_PRINT 1

#include <cstring>

#include "ControlBlock.h"

#include "base/AllocateChannels.h"
#include "base/Instrument.h"
#include "document/RosegardenDocument.h"
#include "gui/studio/StudioControl.h"
#include "misc/Debug.h"

#include <QtGlobal>
#include <QMutexLocker>

#define LOCKED QMutexLocker rg_ControlBlock_locker(&m_mutex)

#define DEBUG_CONTROL_BLOCK 1


namespace Rosegarden
{

TrackInfo::TrackInfo() :
    m_deleted(true),
    m_muted(true),
    m_archived(false),
    m_armed(false),
    m_solo(false),
    m_deviceFilter(0),
    m_channelFilter(0),
    m_thruRouting(Track::Auto),
    m_instrumentId(0),
    m_thruChannel(0),
    m_isThruChannelReady(false),
    m_hasThruChannel(false),
    m_selected(false),
    m_useFixedChannel(true)
{
}

ControlBlock *
ControlBlock::getInstance()
{
    static ControlBlock *instance = nullptr;
    if (!instance) instance = new ControlBlock();
    return instance;
}

ControlBlock::ControlBlock() :
    m_doc(nullptr),
    m_thruFilter(0),
    m_recordFilter(0),
    m_selectedTrack(0)
{
    m_metronomeInfo.m_muted = true;
    m_metronomeInfo.m_instrumentId = 0;
    setSelectedTrack(0);
}

void
ControlBlock::setDocument(RosegardenDocument *doc)
{
#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "ControlBlock::setDocument()";
#endif
    m_trackInfo.clear();
    m_doc = doc;

    Composition& comp = m_doc->getComposition();

    for (Composition::TrackMap::iterator i = comp.getTracks().begin();
	 i != comp.getTracks().end(); ++i) {
        Track *track = i->second;
        if (!track) continue;
	updateTrackData(track);
    }

    setMetronomeMuted(!comp.usePlayMetronome());

    setThruFilter(m_doc->getStudio().getMIDIThruFilter());
    setRecordFilter(m_doc->getStudio().getMIDIRecordFilter());
    setSelectedTrack(comp.getSelectedTrack());
}

void
ControlBlock::updateTrackData(Track* t)
{
    LOCKED;
    if (t) {
        TrackId trackId = t->getId();
#ifdef DEBUG_CONTROL_BLOCK
        RG_DEBUG << "Updating track"
                 << trackId;
#endif
        auto iter = m_trackInfo.find(trackId);
        if (iter == m_trackInfo.end()) {
            RG_DEBUG << "updateTrackData new trackId" << trackId;
            TrackInfo info;
            m_trackInfo[trackId] = info;
        }

        setInstrumentForTrack(trackId, t->getInstrument());
        setTrackArmed(trackId, t->isArmed());
        setTrackMuted(trackId, t->isMuted());
        setTrackArchived(trackId, t->isArchived());
        setSolo(trackId, t->isSolo());
        setTrackDeleted(trackId, false);
        setTrackDeviceFilter(trackId, t->getMidiInputDevice());
        setTrackChannelFilter(trackId, t->getMidiInputChannel());
        setTrackThruRouting(trackId, t->getThruRouting());
    }
}

void
ControlBlock::setInstrumentForTrack(TrackId trackId, InstrumentId instId)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setInstrumentForTrack unkown trackId" << trackId;
        return;
    }
    TrackInfo &track = (*iter).second;
    track.releaseThruChannel(m_doc->getStudio());
    track.m_instrumentId = instId;
    track.conform(m_doc->getStudio());
}

void
ControlBlock::setTrackMuted(TrackId trackId, bool muted)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setTrackMuted unkown trackId" << trackId;
        return;
    }
    (*iter).second.m_muted = muted;
}

bool ControlBlock::isTrackMuted(TrackId trackId) const
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "isTrackMuted unkown trackId" << trackId;
        return true;
    }
    return (*iter).second.m_muted;
}

void
ControlBlock::setTrackArchived(TrackId trackId, bool archived)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setTrackArchived unkown trackId" << trackId;
        return;
    }
    (*iter).second.m_archived = archived;
}

bool ControlBlock::isTrackArchived(TrackId trackId) const
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "isTrackArchived unkown trackId" << trackId;
        return true;
    }
    return (*iter).second.m_archived;
}

void ControlBlock::setSolo(TrackId trackId, bool solo)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setSolo unkown trackId" << trackId;
        return;
    }
    (*iter).second.m_solo = solo;
}

bool ControlBlock::isSolo(TrackId trackId) const
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "isSolo unkown trackId" << trackId;
        return true;
    }
    return (*iter).second.m_solo;
}

bool ControlBlock::isAnyTrackInSolo() const
{
    // For each track
    for (auto& pair : m_trackInfo) {
        const TrackInfo &track = pair.second;

        // If this track was deleted, try the next.
        if (track.m_deleted)
            continue;

        // Don't include archived tracks.
        if (track.m_archived)
            continue;

        if (track.m_solo)
            return true;
    }

    return false;
}

void
ControlBlock::setTrackArmed(TrackId trackId, bool armed)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setTrackArmed unkown trackId" << trackId;
        return;
    }
    (*iter).second.m_armed = armed;
    (*iter).second.conform(m_doc->getStudio());
}

void
ControlBlock::setTrackDeleted(TrackId trackId, bool deleted)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setTrackDeleted unkown trackId" << trackId;
        return;
    }
    (*iter).second.m_deleted = deleted;
    (*iter).second.conform(m_doc->getStudio());
}

void
ControlBlock::setTrackChannelFilter(TrackId trackId, char channel)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setTrackChannelFilter unkown trackId" << trackId;
        return;
    }
    (*iter).second.m_channelFilter = channel;
}

void
ControlBlock::setTrackDeviceFilter(TrackId trackId, DeviceId device)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setTrackDeviceFilter unkown trackId" << trackId;
        return;
    }
    (*iter).second.m_deviceFilter = device;
}

void ControlBlock::setTrackThruRouting(
        TrackId trackId, Track::ThruRouting thruRouting)
{
    auto iter = m_trackInfo.find(trackId);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setTrackDeviceFilter unkown trackId" << trackId;
        return;
    }
    (*iter).second.m_thruRouting = thruRouting;
}

bool
ControlBlock::isInstrumentMuted(InstrumentId instrumentId) const
{
    // For each track
    for (auto& pair : m_trackInfo) {
        const TrackInfo &track = pair.second;
        if (track.m_instrumentId == instrumentId  &&
            !track.m_deleted  &&
            !track.m_muted  &&
            !track.m_archived)
            return false;
    }
    return true;
}

bool
ControlBlock::isInstrumentUnused(InstrumentId instrumentId) const
{
    LOCKED;
    // For each track
    for (auto& pair : m_trackInfo) {
        const TrackInfo &track = pair.second;
        if (track.m_instrumentId == instrumentId &&
            !track.m_deleted)
            return false;
    }
    return true;
}

void
ControlBlock::
setSelectedTrack(TrackId track)
{
#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "ControlBlock::setSelectedTrack()";
#endif

    auto iter = m_trackInfo.find(track);
    if (iter == m_trackInfo.end()) {
        RG_DEBUG << "setSelectedTrack unkown trackId" << track;
        return;
    }
    // Undo the old selected track.  Safe even if it referred to the
    // same track or to no track.
#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "ControlBlock::setSelectedTrack() deselecting"
             << m_selectedTrack;
#endif
    TrackInfo &oldTrack = m_trackInfo[m_selectedTrack];
    oldTrack.m_selected = false;
    if (m_doc)
        oldTrack.conform(m_doc->getStudio());

    // Set up the new selected track
#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "ControlBlock::setSelectedTrack() selecting"
             << track;
#endif
    TrackInfo &newTrack = m_trackInfo[track];
    newTrack.m_selected = true;
    if (m_doc)
        newTrack.conform(m_doc->getStudio());

    // What's selected is recorded both here and in the trackinfo
    // objects.
    m_selectedTrack = track;
}

InstrumentAndChannel
ControlBlock::
getInstAndChanForEvent(bool recording, DeviceId deviceId, char channel)
{
    // For each track
    for (auto& pair : m_trackInfo) {
        TrackInfo &track = pair.second;

        // Skip archived Tracks.
        if (track.m_archived)
            continue;

        bool deviceMatch =
            (track.m_deviceFilter == ALL_DEVICES  ||
             track.m_deviceFilter == deviceId);
        bool channelMatch =
            (track.m_channelFilter == -1  ||  // all channels
             track.m_channelFilter == static_cast<int>(channel));

        // if the event doesn't match this track's filters, try the next track
        if (!deviceMatch  ||  !channelMatch)
            continue;

        switch(track.m_thruRouting) {
        case Track::Auto:
            // if we are recording
            if (recording) {
                // if this track is armed
                if (track.m_armed) {
                    // route to this track's inst/chan.
                    return track.getChannelAsReady(m_doc->getStudio());
                }
            } else {  // we aren't recording
                // if this track is selected
                if (track.m_selected) {
                    // route to this track's inst/chan.
                    return track.getChannelAsReady(m_doc->getStudio());
                }
            }

            // Try the next track...
            break;

        case Track::On:
            // route to this track's inst/chan.
            return track.getChannelAsReady(m_doc->getStudio());

        case Track::Off:
            // Try the next track...
            break;

        case Track::WhenArmed:
            // If the track is armed
            if (track.m_armed) {
                // route to this track's inst/chan.
                return track.getChannelAsReady(m_doc->getStudio());
            }

            // Try the next track...
            break;
        }
    }

    // Drop the event.
    return InstrumentAndChannel();
}

// Kick all tracks' thru-channels off channel and arrange to find new
// homes for them.  This is called by AllocateChannels when a fixed
// channel has commandeered the channel.
// @author Tom Breton (Tehom)
void
ControlBlock::
vacateThruChannel(int channel)
{
    // For each track
    for (auto& pair : m_trackInfo) {
        TrackInfo &track = pair.second;
        if(track.m_hasThruChannel &&
           (track.m_thruChannel == channel) &&
           !track.m_useFixedChannel) {
            // Setting this flag invalidates the channel as far as
            // track knows.  That's all we need to do; fixed
            // instruments need do nothing and for auto instruments,
            // the relevant AllocateChannels has already removed this
            // channel.
            track.m_hasThruChannel = false;
            track.conform(m_doc->getStudio());
        }
    }
}

// React to an instrument having changed its program.
// @author Tom Breton (Tehom)
void
ControlBlock::
instrumentChangedProgram(InstrumentId instrumentId)
{
    // For each track
    for (auto& pair : m_trackInfo) {
        TrackInfo &track = pair.second;
        if(track.m_hasThruChannel && (track.m_instrumentId == instrumentId)) {
            track.makeChannelReady(m_doc->getStudio());
        }
    }
}

// React to an instrument's channel becoming fixed or unfixed.
// @author Tom Breton (Tehom)
void
ControlBlock::
instrumentChangedFixity(InstrumentId instrumentId)
{
    // For each track
    for (auto& pair : m_trackInfo) {
        TrackInfo &track = pair.second;
        if(track.m_hasThruChannel && (track.m_instrumentId == instrumentId)) {
            track.instrumentChangedFixity(m_doc->getStudio());
        }
    }

}
    /** TrackInfo members **/

void
TrackInfo::
conform(Studio &studio)
{
    bool thruAuto = (m_thruRouting == Track::Auto);
    bool shouldHaveThru = (!thruAuto || m_armed || m_selected) && !m_deleted;
#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "TrackInfo::conform()"
             << (shouldHaveThru ?
                 "should have a thru channel" :
                 "shouldn't have a thru channel")
             << "and"
             << (m_hasThruChannel ? "does" : "doesn't");
#endif

    if (!m_hasThruChannel && shouldHaveThru) {
        allocateThruChannel(studio);
        makeChannelReady(studio);
    }
    else if (m_hasThruChannel && !shouldHaveThru)
        { releaseThruChannel(studio); }
}

InstrumentAndChannel
TrackInfo::getChannelAsReady(Studio &studio)
{
    if (!m_hasThruChannel)
        { return InstrumentAndChannel(); }

    // If our channel might not have the right program, send it now.
    if (!m_isThruChannelReady)
        { makeChannelReady(studio); }
    return InstrumentAndChannel(m_instrumentId, m_thruChannel);
}

void
TrackInfo::makeChannelReady(Studio &studio)
{
#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "TrackInfo::makeChannelReady()";
#endif
    Instrument *instrument =
        studio.getInstrumentById(m_instrumentId);

    // If we have deleted a device, we may get a nullptr instrument.  In
    // that case, we can't do much.
    if (!instrument) { return; }

    // We can get non-Midi instruments here.  There's nothing to do
    // for them.  For fixed, sendChannelSetup is slightly wrong, but
    // could be adapted and parameterized by trackId.
    if ((instrument->getType() == Instrument::Midi)
        && !m_useFixedChannel) {
        // Re-acquire channel.  It may change if instrument's program
        // became percussion or became non-percussion.
        Device* device = instrument->getDevice();
        Q_CHECK_PTR(device);
        AllocateChannels *allocator = device->getAllocator();
        if (allocator) {
            m_thruChannel =
                allocator->reallocateThruChannel(*instrument, m_thruChannel);
            // If somehow we got here without having a channel, we
            // have one now.
            m_hasThruChannel = true;
#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "TrackInfo::makeChannelReady() now has channel"
             << m_hasThruChannel;
#endif
        }
        // This is how Midi instrument readies a fixed channel.
        StudioControl::sendChannelSetup(instrument, m_thruChannel);
    }
    m_isThruChannelReady = true;
}


// Allocate a channel for thru MIDI events to play on.
// @author Tom Breton (Tehom)
void
TrackInfo::allocateThruChannel(Studio &studio)
{
    Instrument *instrument =
        studio.getInstrumentById(m_instrumentId);

    // If we have deleted a device, we may get a nullptr instrument.  In
    // that case, we can't do much.
    if (!instrument) { return; }

    // This value of fixity holds until releaseThruChannel is called.
    m_useFixedChannel = instrument->hasFixedChannel();

    if (m_useFixedChannel) {
        m_thruChannel = instrument->getNaturalMidiChannel();
        m_hasThruChannel = true;
        m_isThruChannelReady = true;
        return;
    }

    Device* device = instrument->getDevice();
    Q_CHECK_PTR(device);
    AllocateChannels *allocator = device->getAllocator();

#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "TrackInfo::allocateThruChannel() "
             << (allocator ?
                 "got an allocator" :
                 "didn't get an allocator");
#endif

    // Device is not a channel-managing device, so instrument's
    // natural channel is correct and requires no further setup.
    if (!allocator)
        {
            m_thruChannel = instrument->getNaturalMidiChannel();
            m_isThruChannelReady = true;
            m_hasThruChannel = true;
            return;
        }

    // Get a suitable channel.
    m_thruChannel = allocator->allocateThruChannel(*instrument);

#ifdef DEBUG_CONTROL_BLOCK
    RG_DEBUG << "TrackInfo::allocateThruChannel() got channel"
             << (int)m_thruChannel;
#endif

    // Right now the channel is probably playing the wrong program.
    m_isThruChannelReady = false;
    m_hasThruChannel = true;
}

void
TrackInfo::releaseThruChannel(Studio &studio)
{
    if (!m_hasThruChannel) { return; }

    Instrument *instrument =
        studio.getInstrumentById(m_instrumentId);

    if (instrument && !m_useFixedChannel) {

        Device* device = instrument->getDevice();
        Q_CHECK_PTR(device);
        AllocateChannels *allocator = device->getAllocator();

        // Device is a channel-managing device (Midi), so release the
        // channel.
        if (allocator)
            { allocator->releaseThruChannel(m_thruChannel); }
    }
    // If we recently deleted a device, we may get a nullptr instrument.
    // In that case, we can't actively release it but we don't need
    // to, we can just mark it released.
    else /* if (!instrument || m_useFixedChannel) */ {}

    m_thruChannel = -1;
    // Channel wants no setup if we somehow encounter it in this
    // state.
    m_isThruChannelReady = true;
    m_hasThruChannel = false;
}

void
TrackInfo::
instrumentChangedFixity(Studio &studio)
{
    // Whether we became fixed or unfixed, release the channel we
    // have (the old way) and get another, which will reflect the
    // current state of the instrument.
    releaseThruChannel(studio);
    allocateThruChannel(studio);
}

}
