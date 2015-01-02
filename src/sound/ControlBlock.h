/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLBLOCK_H
#define RG_CONTROLBLOCK_H

#include "base/MidiProgram.h"
#include "base/Track.h"

namespace Rosegarden 
{

class RosegardenDocument;
class Studio;
 
class InstrumentAndChannel
{
public:
    InstrumentAndChannel(InstrumentId id, int channel) :
        m_id(id),
        m_channel(channel)
    { }

    // Default to an invalid value.
    InstrumentAndChannel(void) :
        m_id(0),
        m_channel(-1)
    { }
    
    InstrumentId m_id;
    int          m_channel;
};

/**
 * ONLY PUT PLAIN DATA HERE - NO POINTERS EVER
 */
struct TrackInfo 
{
    InstrumentAndChannel getChannelAsReady(Studio &studio);
    void conform(Studio &studio);
    void releaseThruChannel(Studio &studio);
    void makeChannelReady(Studio &studio);
    void instrumentChangedFixity(Studio &studio);
private:
    void allocateThruChannel(Studio &studio);

public:
    bool m_deleted;
    bool m_muted;
    bool m_armed;
    char m_channelFilter;
    DeviceId m_deviceFilter;
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
};

#define CONTROLBLOCK_MAX_NB_TRACKS 1024 // can't be a symbol

/// Control data passed from GUI thread to sequencer thread.
/**
 * This class contains data that is being passed from GUI threads to
 * sequencer threads.  It used to be mapped into a shared memory
 * backed file, which had to be of fixed size and layout (with no
 * internal pointers).  The design reflects that history to an extent,
 * though nowadays it is a simple singleton class with no such
 * constraint.
 *
 * @see SequencerDataBlock
 */
class ControlBlock
{
public:
    static ControlBlock *getInstance();

    void setDocument(RosegardenDocument *doc);

    unsigned int getMaxTrackId() const { return m_maxTrackId; }
    void updateTrackData(Track*);

    void setInstrumentForTrack(TrackId trackId, InstrumentId);
    InstrumentId getInstrumentForTrack(TrackId trackId) const;

    void setTrackArmed(TrackId trackId, bool);
    bool isTrackArmed(TrackId trackId) const;

    void setTrackMuted(TrackId trackId, bool);
    bool isTrackMuted(TrackId trackId) const;

    void setTrackDeleted(TrackId trackId, bool);
    bool isTrackDeleted(TrackId trackId) const;
    
    void setTrackChannelFilter(TrackId trackId, char);
    char getTrackChannelFilter(TrackId trackId) const;
    
    void setTrackDeviceFilter(TrackId trackId, DeviceId);
    DeviceId getTrackDeviceFilter(TrackId trackId) const;
    
    bool isInstrumentMuted(InstrumentId instrumentId) const;
    bool isInstrumentUnused(InstrumentId instrumentId) const;

    void setInstrumentForMetronome(InstrumentId instId)
    { m_metronomeInfo.m_instrumentId = instId; }
    InstrumentId getInstrumentForMetronome() const
    { return m_metronomeInfo.m_instrumentId; }

    void setMetronomeMuted(bool mute) { m_metronomeInfo.m_muted = mute; }
    bool isMetronomeMuted() const     { return m_metronomeInfo.m_muted; }

    bool isSolo() const      { return m_solo; }
    void setSolo(bool value) { m_solo = value; }
    TrackId getSelectedTrack() const     { return m_selectedTrack; }
    void setSelectedTrack(TrackId track);

    void setThruFilter(MidiFilter filter) { m_thruFilter = filter; }
    MidiFilter getThruFilter() const { return m_thruFilter; }

    void setRecordFilter(MidiFilter filter) { m_recordFilter = filter; }
    MidiFilter getRecordFilter() const { return m_recordFilter; }
    
    void setMidiRoutingEnabled(bool enabled) { m_routing = enabled; }
    bool isMidiRoutingEnabled() const { return m_routing; } 
    
    /**
     * Gets an instrument id and output channel for the given DeviceId
     * and input Channel. If there is an armed track having a matching
     * device and channel filters, this method returns the instrument
     * assigned to the track, even if there are more tracks matching
     * the same filters. If there is not a single match, it returns
     * the value corresponding to the selected track.
     */
    InstrumentAndChannel
        getInsAndChanForEvent(unsigned int dev, 
                              unsigned int chan);

    InstrumentAndChannel
        getInsAndChanForSelectedTrack(void);

    void vacateThruChannel(int channel);
    void instrumentChangedProgram(InstrumentId instrumentId);
    void instrumentChangedFixity(InstrumentId instrumentId);
    
protected:
    ControlBlock();
    void clearTracks(void);

    RosegardenDocument *m_doc;
    unsigned int m_maxTrackId;
    bool m_solo;
    bool m_routing;
    bool m_isSelectedChannelReady;
    MidiFilter m_thruFilter;
    MidiFilter m_recordFilter;
    TrackId m_selectedTrack;
    TrackInfo m_metronomeInfo;
    TrackInfo m_trackInfo[CONTROLBLOCK_MAX_NB_TRACKS]; // should be high enough for the moment
};

}

#endif
