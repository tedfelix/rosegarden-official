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

#include <cstring>

#include "rosedebug.h" // remove this

#include "ControlBlock.h"

namespace Rosegarden
{

ControlBlock::ControlBlock(unsigned int nbTracks)
    : m_nbTracks(nbTracks)
{
    m_metronomeInfo.muted = true;
    m_metronomeInfo.instrumentId = 0;
    memset(m_trackInfo, 0, sizeof(m_trackInfo));
}

ControlBlock::ControlBlock()
{
    // DO NOT initialize anything - this ctor is meant to be used by
    // the sequencer, through a placement new over an mmapped file.
}

void ControlBlock::updateTrackData(Track* t)
{
    if (t) {
        setInstrumentForTrack(t->getId(), t->getInstrument());
        setTrackMuted(t->getId(), t->isMuted());
    }
}

void ControlBlock::setInstrumentForMetronome(InstrumentId instId)
{
    m_metronomeInfo.instrumentId = instId;
}

InstrumentId ControlBlock::getInstrumentForMetronome()
{
    return m_metronomeInfo.instrumentId;
}


void ControlBlock::setMetronomeMuted(bool mute)
{
    RG_DEBUG << "ControlBlock::setMetronomeMuted(" << mute << ")\n";
    m_metronomeInfo.muted = mute;
}

bool ControlBlock::isMetronomeMuted()
{
    return m_metronomeInfo.muted;
}

void ControlBlock::setInstrumentForTrack(TrackId trackId, InstrumentId instId)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS) m_trackInfo[trackId].instrumentId = instId;
}

InstrumentId ControlBlock::getInstrumentForTrack(TrackId trackId)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS) return m_trackInfo[trackId].instrumentId;
    return 0;
}

void ControlBlock::setTrackMuted(TrackId trackId, bool mute)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS) m_trackInfo[trackId].muted = mute;
}

bool ControlBlock::isTrackMuted(TrackId trackId)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS) return m_trackInfo[trackId].muted;
    return true;
}

}
