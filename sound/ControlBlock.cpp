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

#include "ControlBlock.h"

#include <cstring>

namespace Rosegarden
{

ControlBlock::ControlBlock(unsigned int nbTracks)
    : m_nbTracks(nbTracks)
{
    memset(m_trackInstruments, 0, CONTROLBLOCK_MAX_NB_TRACKS * sizeof(InstrumentId));
}

ControlBlock::ControlBlock()
{
    // DO NOT initialize anything - this ctor is meant to be used by
    // the sequencer, through a placement new over an mmapped file.
}

void ControlBlock::setInstrumentForTrack(unsigned int trackNb, InstrumentId id)
{
    if (trackNb < CONTROLBLOCK_MAX_NB_TRACKS) m_trackInstruments[trackNb] = id;
}

InstrumentId ControlBlock::getInstrumentForTrack(unsigned int trackNb)
{
    if (trackNb < CONTROLBLOCK_MAX_NB_TRACKS) return m_trackInstruments[trackNb];
    return 0;
}

size_t ControlBlock::getSize()
{
    return CONTROLBLOCK_MAX_NB_TRACKS * sizeof(InstrumentId) + 20;
}

}
