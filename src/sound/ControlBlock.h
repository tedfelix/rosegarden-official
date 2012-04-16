/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _CONTROLBLOCK_H_
#define _CONTROLBLOCK_H_

#include "base/MidiProgram.h"
#include "base/Track.h"

namespace Rosegarden 
{

class RosegardenDocument;

/**
 * ONLY PUT PLAIN DATA HERE - NO POINTERS EVER
 */
struct TrackInfo 
{
    bool deleted;
    bool muted;
    bool armed;
    char channelFilter;
    DeviceId deviceFilter;
    InstrumentId instrumentId;
};

#define CONTROLBLOCK_MAX_NB_TRACKS 1024 // can't be a symbol

/**
 * This class contains data that is being passed from GUI threads to
 * sequencer threads.  It used to be mapped into a shared memory
 * backed file, which had to be of fixed size and layout (with no
 * internal pointers).  The design reflects that history to an extent,
 * though nowadays it is a simple singleton class with no such
 * constraint.
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
    int getNaturalChannelForInstrument(InstrumentId trackId) const;

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

    void setInstrumentForMetronome(InstrumentId instId) { m_metronomeInfo.instrumentId = instId; }
    InstrumentId getInstrumentForMetronome() const      { return m_metronomeInfo.instrumentId; }

    void setMetronomeMuted(bool mute) { m_metronomeInfo.muted = mute; }
    bool isMetronomeMuted() const     { return m_metronomeInfo.muted; }

    bool isSolo() const      { return m_solo; }
    void setSolo(bool value) { m_solo = value; }
    TrackId getSelectedTrack() const     { return m_selectedTrack; }
    void setSelectedTrack(TrackId track) { m_selectedTrack = track; }

    void setThruFilter(MidiFilter filter) { m_thruFilter = filter; }
    MidiFilter getThruFilter() const { return m_thruFilter; }

    void setRecordFilter(MidiFilter filter) { m_recordFilter = filter; }
    MidiFilter getRecordFilter() const { return m_recordFilter; }
    
    void setMidiRoutingEnabled(bool enabled) { m_routing = enabled; }
    bool isMidiRoutingEnabled() const { return m_routing; } 
    
    /**
     * Gets an InstrumentId for the given DeviceId and Channel. If there
     * is an armed track having a matching device and channel filters, 
     * this method returns the instrument assigned to the track, even if 
     * there are more tracks matching the same filters. If there is not a
     * single match, it returns the instrument assigned to the selected
     * track.
     */
    InstrumentId getInstrumentForEvent(unsigned int dev, 
                                       unsigned int chan);

protected:
    ControlBlock();

    RosegardenDocument *m_doc;
    unsigned int m_maxTrackId;
    bool m_solo;
    bool m_routing;
    MidiFilter m_thruFilter;
    MidiFilter m_recordFilter;
    TrackId m_selectedTrack;
    TrackInfo m_metronomeInfo;
    TrackInfo m_trackInfo[CONTROLBLOCK_MAX_NB_TRACKS]; // should be high enough for the moment
};

}

#endif
