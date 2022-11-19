/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TIMET_H
#define RG_TIMET_H

namespace Rosegarden
{
    // Time in MIDI ticks (aka pulses as in ppqn, pulses per quarter note).
    // ??? rename: MidiTicks
    typedef long timeT;

    // Looks like there are various 960's scattered throughout the system.
    // But 960ppq appears to be the standard for Rosegarden notation anyway.
    // The sequencer deals in RealTime (nanosecond resolution) but is limited
    // by the timer resolution.  See AlsaDriver::setCurrentTimer().
    // 480 is used for MIDI file export.  See MidiFile::convertToMidi().
    constexpr timeT timebase = 960;  // PPQ, PPQN, TPQN
}

#endif
