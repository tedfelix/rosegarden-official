// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _CONTROLBLOCK_H_
#define _CONTROLBLOCK_H_

#include "MidiProgram.h"
#include "Track.h"

namespace Rosegarden 
{

/**
 * ONLY PUT PLAIN DATA HERE - NO POINTERS EVER
 */
struct TrackInfo 
{
    bool muted;
    InstrumentId instrumentId;
};



#define CONTROLBLOCK_MAX_NB_TRACKS 1024 // can't be a symbol

/**
 * Sequencer control block, mmapped by the GUI
 */
class ControlBlock
{
public:
    /// ctor for GUI
    ControlBlock(unsigned int nbTracks);

    /// ctor for sequencer - all data is read from mmapped file
    ControlBlock();

    unsigned int getNbTracks();
    void updateTrackData(Track*);

    void setInstrumentForTrack(TrackId trackId, InstrumentId);
    InstrumentId getInstrumentForTrack(TrackId trackId);

    void setTrackMuted(TrackId trackId, bool);
    bool isTrackMuted(TrackId trackId);

    void setInstrumentForMetronome(InstrumentId);
    InstrumentId getInstrumentForMetronome();

    void setMetronomeMuted(bool);
    bool isMetronomeMuted();

protected:
    //--------------- Data members ---------------------------------
    // PUT ONLY PLAIN DATA HERE - NO POINTERS EVER
    int m_nbTracks;
    TrackInfo m_metronomeInfo;
    TrackInfo m_trackInfo[CONTROLBLOCK_MAX_NB_TRACKS]; // should be high enough for the moment
};

}

#endif
