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

#ifndef RG_MIDI_METRONOME_H
#define RG_MIDI_METRONOME_H

#include "Instrument.h"
#include "MidiProgram.h"

namespace Rosegarden {

// A mapped MIDI instrument - a drum track click for example
//
class MidiMetronome
{
public:
    MidiMetronome(InstrumentId instrument,
                  MidiByte barPitch = 37,
                  MidiByte beatPitch = 37,
                  MidiByte subBeatPitch = 37,
                  int depth = 2,
                  MidiByte barVely = 120,
                  MidiByte beatVely = 100,
                  MidiByte subBeatVely = 80) :
	m_instrument(instrument),
	m_barPitch(barPitch),
	m_beatPitch(beatPitch),
	m_subBeatPitch(subBeatPitch),
	m_depth(depth),
	m_barVelocity(barVely),
	m_beatVelocity(beatVely),
	m_subBeatVelocity(subBeatVely)
    {
	// nothing else
    }

    InstrumentId        getInstrument() const { return m_instrument; }
    MidiByte            getBarPitch() const { return m_barPitch; }
    MidiByte            getBeatPitch() const { return m_beatPitch; }
    MidiByte            getSubBeatPitch() const { return m_subBeatPitch; }
    int                 getDepth() const { return m_depth; }
    MidiByte            getBarVelocity() const { return m_barVelocity; }
    MidiByte            getBeatVelocity() const { return m_beatVelocity; }
    MidiByte            getSubBeatVelocity() const { return m_subBeatVelocity; }

    void setInstrument(InstrumentId id) { m_instrument = id; }
    void setBarPitch(MidiByte pitch) { m_barPitch = pitch; }
    void setBeatPitch(MidiByte pitch) { m_beatPitch = pitch; }
    void setSubBeatPitch(MidiByte pitch) { m_subBeatPitch = pitch; }
    void setDepth(int depth) { m_depth = depth; }
    void setBarVelocity(MidiByte barVely) { m_barVelocity = barVely; }
    void setBeatVelocity(MidiByte beatVely) { m_beatVelocity = beatVely; }
    void setSubBeatVelocity(MidiByte subBeatVely) { m_subBeatVelocity = subBeatVely; }

private:
    InstrumentId    m_instrument;
    MidiByte        m_barPitch;
    MidiByte        m_beatPitch;
    MidiByte        m_subBeatPitch;
    int             m_depth;
    MidiByte        m_barVelocity;
    MidiByte        m_beatVelocity;
    MidiByte        m_subBeatVelocity;
};

}

#endif

