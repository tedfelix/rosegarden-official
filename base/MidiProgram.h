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

#ifndef _MIDIBANK_H_
#define _MIDIBANK_H_

#include <string>

namespace Rosegarden
{
typedef unsigned char MidiByte;
typedef unsigned int InstrumentId;

class MidiBank
{
public:
    MidiBank();
    MidiBank(bool percussion, MidiByte msb, MidiByte lsb, std::string name = "");

    // comparator disregards name
    bool operator==(const MidiBank &b) const;
    
    bool                isPercussion() const;
    MidiByte            getMSB() const;
    MidiByte            getLSB() const;
    std::string         getName() const;

    void                setName(std::string name);

private:
    bool m_percussion;
    MidiByte m_msb;
    MidiByte m_lsb;
    std::string m_name;
};

class MidiProgram
{
public:
    MidiProgram();
    MidiProgram(const MidiBank &bank, MidiByte program, std::string name = "");

    // comparator disregards name
    bool operator==(const MidiProgram &p) const;
    
    const MidiBank&     getBank() const;
    MidiByte            getProgram() const;
    std::string         getName() const;

    void                setName(std::string name);

private:
    MidiBank m_bank;
    MidiByte m_program;
    std::string m_name;
};

// A mapped MIDI instrument - a drum track click for example
//
class MidiMetronome
{
public:
    MidiMetronome(InstrumentId instrument,
                  MidiByte pitch = 37,
		  int depth = 2,
                  MidiByte barVely = 120,
                  MidiByte beatVely = 100,
		  MidiByte subBeatVely = 80);

    InstrumentId        getInstrument() const { return m_instrument; }
    MidiByte            getPitch() const { return m_pitch; }
    int                 getDepth() const { return m_depth; }
    MidiByte            getBarVelocity() const { return m_barVelocity; }
    MidiByte            getBeatVelocity() const { return m_beatVelocity; }
    MidiByte            getSubBeatVelocity() const { return m_subBeatVelocity; }

    void setInstrument(InstrumentId id) { m_instrument = id; }
    void setPitch(MidiByte pitch) { m_pitch = pitch; }
    void setDepth(int depth) { m_depth = depth; }
    void setBarVelocity(MidiByte barVely) { m_barVelocity = barVely; }
    void setBeatVelocity(MidiByte beatVely) { m_beatVelocity = beatVely; }
    void setSubBeatVelocity(MidiByte subBeatVely) { m_subBeatVelocity = subBeatVely; }

private:
    InstrumentId    m_instrument;
    MidiByte        m_pitch;
    int             m_depth;
    MidiByte        m_barVelocity;
    MidiByte        m_beatVelocity;
    MidiByte        m_subBeatVelocity;
};

}

#endif

