/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLBLOCK_H
#define RG_CONTROLBLOCK_H

#include "base/Device.h"  // DeviceId
#include "base/MidiProgram.h"  // InstrumentId, MidiFilter
#include "base/Track.h"  // TrackId

namespace Rosegarden 
{

class RosegardenDocument;
class Studio;

struct InstrumentAndChannel
{
    // Default to an invalid value.
    InstrumentAndChannel() :
        id(0),
        channel(-1)
    { }
    InstrumentAndChannel(InstrumentId idIn, int channelIn) :
        id(idIn),
        channel(channelIn)
    { }

    InstrumentId id;
    int channel;
};

struct TrackInfo 
{
public:
    /// This should be the ctor.
    void clear();

    /// Get instrument and channel, preparing the channel if needed.
    /**
     * Return the instrument id and channel number that this track plays on,
     * preparing the channel if needed.  If impossible, return an invalid
     * instrument and channel.
     *
     * @author Tom Breton (Tehom)
     */
    InstrumentAndChannel getChannelAsReady(Studio &studio);

    /// Make track info conformant to its situation.
    /**
     * In particular, acquire or release a channel for thru events to
     * play on.
     *
     * @author Tom Breton (Tehom)
     */
    void conform(Studio &studio);

    /// Release the channel that thru MIDI events played on.
    /**
     * @author Tom Breton (Tehom)
     */
    void releaseThruChannel(Studio &studio);

    /// Make the channel ready to play on.  Send the program, etc.
    /**
     * @author Tom Breton (Tehom)
     */
    void makeChannelReady(Studio &studio);

    /// Adjust channel based on fixed/auto mode change.
    void instrumentChangedFixity(Studio &studio);

    /*******************************************************
     * !!! ONLY PUT PLAIN DATA HERE - NO POINTERS EVER !!! *
     *******************************************************/

    /// Track is no longer in the Composition.
    bool m_deleted;

    bool m_muted;
    bool m_archived;
    bool m_armed;
    bool m_solo;

    /// Recording filters: Device
    DeviceId m_deviceFilter;
    /// Recording filters: Channel
    char m_channelFilter;
    /// Recording filters: Thru Routing
    Track::ThruRouting m_thruRouting;

    InstrumentId m_instrumentId;

    /// The channel to play thru MIDI events on, if any.
    /**
     * For unarmed unselected tracks, this will be an invalid channel.  For
     * fixed-channel instruments, this is the instrument's fixed channel.
     */
    int m_thruChannel;
    /// Whether the thru channel is ready - the right program has been sent, etc.
    bool m_isThruChannelReady;
    /// Whether we have allocated a thru channel.
    bool m_hasThruChannel;
    /**
     * This duplicates information in ControlBlock.  It exists so that
     * we can check if we need a thru channel using just this class.
     */
    bool m_selected;
    /**
     * This is usually the same as Instrument's fixedness, but can
     * disagree briefly when fixedness changes.  Meaningless if no channel.
     */
    bool m_useFixedChannel;

private:
    void allocateThruChannel(Studio &studio);
};

// should be high enough for the moment
#define CONTROLBLOCK_MAX_NB_TRACKS 1024

/// Control data passed from GUI thread to sequencer thread.
/**
 * This class contains data that is being passed from GUI threads to
 * sequencer threads.  It used to be mapped into a shared memory
 * backed file, which had to be of fixed size and layout (with no
 * internal pointers).  The design reflects that history to an extent,
 * though nowadays it is a simple singleton class with no such
 * constraint.
 *
 * SequenceManager monitors the Composition and updates the data here.
 * RosegardenSequencer and the mappers (e.g. InternalSegmentMapper) use
 * the data found here.
 *
 * ??? It seems strange that this class/object is used for communication
 *     between threads, yet it is lock free.  I suspect this is OK since
 *     we never move more than a word at a time.  The only issue might
 *     be inconsistency across fields.  And in that case, there is little
 *     that can really go wrong.
 *
 * @see SequencerDataBlock
 */
class ControlBlock
{
public:
    static ControlBlock *getInstance();

    void setDocument(RosegardenDocument *doc);

    //unsigned int getMaxTrackId() const { return m_maxTrackId; }

    /// Update m_trackInfo for the track.
    void updateTrackData(Track *);

    void setInstrumentForTrack(TrackId trackId, InstrumentId);
    InstrumentId getInstrumentForTrack(TrackId trackId) const;
    bool isInstrumentUnused(InstrumentId instrumentId) const;

    void setTrackArmed(TrackId trackId, bool armed);
    //bool isTrackArmed(TrackId trackId) const;

    void setTrackMuted(TrackId trackId, bool muted);
    bool isTrackMuted(TrackId trackId) const;
    bool isInstrumentMuted(InstrumentId instrumentId) const;

    void setTrackArchived(TrackId trackId, bool archived);
    bool isTrackArchived(TrackId trackId) const;

    void setSolo(TrackId trackId, bool solo);
    bool isSolo(TrackId trackId) const;
    bool isAnyTrackInSolo() const;

    void setTrackDeleted(TrackId trackId, bool deleted);
    //bool isTrackDeleted(TrackId trackId) const;

    /// Recording filters: Device
    void setTrackDeviceFilter(TrackId trackId, DeviceId);
    //DeviceId getTrackDeviceFilter(TrackId trackId) const;

    /// Recording filters: Channel
    void setTrackChannelFilter(TrackId trackId, char channel);
    //char getTrackChannelFilter(TrackId trackId) const;

    /// Recording filters: Thru Routing
    void setTrackThruRouting(TrackId trackId, Track::ThruRouting thruRouting);

    void setInstrumentForMetronome(InstrumentId instId)
        { m_metronomeInfo.m_instrumentId = instId; }
    //InstrumentId getInstrumentForMetronome() const
    //    { return m_metronomeInfo.m_instrumentId; }

    void setMetronomeMuted(bool mute) { m_metronomeInfo.m_muted = mute; }
    bool isMetronomeMuted() const     { return m_metronomeInfo.m_muted; }

    void setSelectedTrack(TrackId track);
    TrackId getSelectedTrack() const     { return m_selectedTrack; }

    void setThruFilter(MidiFilter filter) { m_thruFilter = filter; }
    MidiFilter getThruFilter() const { return m_thruFilter; }

    void setRecordFilter(MidiFilter filter) { m_recordFilter = filter; }
    MidiFilter getRecordFilter() const { return m_recordFilter; }
    
    /// Get the output instrument and channel for an incoming event.
    InstrumentAndChannel getInstAndChanForEvent(
            bool recording, DeviceId deviceId, char channel);

    void vacateThruChannel(int channel);
    void instrumentChangedProgram(InstrumentId instrumentId);
    void instrumentChangedFixity(InstrumentId instrumentId);
    
private:
    // Singleton.  Use getInstance().
    ControlBlock();

    void clearTracks(void);

    RosegardenDocument *m_doc;

    unsigned int m_maxTrackId;

    bool m_isSelectedChannelReady;
    MidiFilter m_thruFilter;
    MidiFilter m_recordFilter;

    TrackId m_selectedTrack;

    TrackInfo m_metronomeInfo;

    TrackInfo m_trackInfo[CONTROLBLOCK_MAX_NB_TRACKS];
};

}

#endif
