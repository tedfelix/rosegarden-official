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

#include "MidiProgram.h"

namespace Rosegarden {

MidiBank::MidiBank()  : 
    m_percussion(false), m_msb(0), m_lsb(0), m_name()
{
    // nothing else
}

MidiBank::MidiBank(bool percussion, MidiByte msb, MidiByte lsb, std::string name) :
    m_percussion(percussion), m_msb(msb), m_lsb(lsb), m_name(name)
{
    // nothing else
}

bool
MidiBank::operator==(const MidiBank &b) const
{
    return m_percussion == b.m_percussion && m_msb == b.m_msb && m_lsb == b.m_lsb;
}
    
bool
MidiBank::isPercussion() const
{
    return m_percussion;
}

MidiByte
MidiBank::getMSB() const
{
    return m_msb;
}

MidiByte
MidiBank::getLSB() const
{
    return m_lsb;
}

std::string
MidiBank::getName() const
{
    return m_name;
}

void
MidiBank::setName(std::string name)
{
    m_name = name; 
}


MidiProgram::MidiProgram() :
    m_bank(), m_program(0), m_name()
{
    // nothing else
}

MidiProgram::MidiProgram(const MidiBank &bank, MidiByte program, std::string name) :
    m_bank(bank), m_program(program), m_name(name)
{
    // nothing else
}

bool
MidiProgram::operator==(const MidiProgram &p) const
{
    return m_bank == p.m_bank && m_program == p.m_program;
}
    
const MidiBank &
MidiProgram::getBank() const
{
    return m_bank;
}

MidiByte
MidiProgram::getProgram() const
{
    return m_program;
}

std::string
MidiProgram::getName() const
{
    return m_name;
}

void
MidiProgram::setName(std::string name)
{
    m_name = name;
}


MidiMetronome::MidiMetronome(InstrumentId instrument,
                             MidiByte pitch,
			     int depth,
                             MidiByte barVely,
                             MidiByte beatVely,
			     MidiByte subBeatVely):
         m_instrument(instrument),
         m_pitch(pitch),
	 m_depth(depth),
         m_barVelocity(barVely),
         m_beatVelocity(beatVely),
	 m_subBeatVelocity(subBeatVely)
{
    // nothing else
}

}

