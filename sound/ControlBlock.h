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

    static size_t getSize(); // update this when adding new members

protected:
    //--------------- Data members ---------------------------------
    int m_nbTracks;
    InstrumentId m_trackInstruments[CONTROLBLOCK_MAX_NB_TRACKS]; // should be high enough for the moment
};

}

#endif
