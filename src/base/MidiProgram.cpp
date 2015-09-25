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
#if 1
    // Incomplete
    return m_percussion == b.m_percussion && m_msb == b.m_msb && m_lsb == b.m_lsb;
#else
    // Complete
    return (m_percussion == b.m_percussion  &&
            m_msb == b.m_msb  &&
            m_lsb == b.m_lsb  &&
            m_name == b.m_name);
#endif
}

bool
MidiBank::fullCompare(const MidiBank &rhs) const
{
    return (m_percussion == rhs.m_percussion  &&
            m_msb == rhs.m_msb  &&
            m_lsb == rhs.m_lsb  &&
            m_name == rhs.m_name);
}

bool
MidiBank::partialCompare(const MidiBank &rhs) const
{
    return (m_percussion == rhs.m_percussion  &&
            m_msb == rhs.m_msb  &&
            m_lsb == rhs.m_lsb);
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

MidiProgram::MidiProgram(const MidiBank &bank, MidiByte program, std::string name, std::string keyMapping) :
    m_bank(bank), m_program(program), m_name(name), m_keyMapping(keyMapping)
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

const std::string &
MidiProgram::getName() const
{
    return m_name;
}

void
MidiProgram::setName(const std::string &name)
{
    m_name = name;
}

const std::string &
MidiProgram::getKeyMapping() const
{
    return m_keyMapping;
}

void
MidiProgram::setKeyMapping(const std::string &keyMapping)
{
    m_keyMapping = keyMapping;
}

MidiKeyMapping::MidiKeyMapping() :
    m_name("")
{
}

MidiKeyMapping::MidiKeyMapping(const std::string &name) :
    m_name(name)
{
    // nothing else
}

MidiKeyMapping::MidiKeyMapping(const std::string &name, const KeyNameMap &map) :
    m_name(name),
    m_map(map)
{
    // nothing else
}

bool
MidiKeyMapping::operator==(const MidiKeyMapping &m) const
{
    return (m_map == m.m_map);
}

std::string
MidiKeyMapping::getMapForKeyName(MidiByte pitch) const
{
    KeyNameMap::const_iterator i = m_map.find(pitch);
    if (i != m_map.end()) {
	return i->second;
    } else {
	return "";
    }
}

#if 0
// unused
int
MidiKeyMapping::getOffset(MidiByte pitch) const
{
    int c = 0;
    for (KeyNameMap::const_iterator i = m_map.begin(); i != m_map.end(); ++i) {
        if (i->first == pitch) return c;
        ++c;
    }
    return -1;
}
#endif

int
MidiKeyMapping::getPitchForOffset(int offset) const
{
    KeyNameMap::const_iterator i = m_map.begin();
    while (i != m_map.end() && offset > 0) {
	++i; --offset;
    }
    if (i == m_map.end()) return -1;
    else return i->first;
}

int
MidiKeyMapping::getPitchExtent() const
{
    int minPitch = 0, maxPitch = 0;
    KeyNameMap::const_iterator mi = m_map.begin();
    if (mi != m_map.end()) {
	minPitch = mi->first;
	mi = m_map.end();
	--mi;
	maxPitch = mi->first;
	return maxPitch - minPitch + 1;
    }
    return maxPitch - minPitch;
}

void
MidiKeyMapping::extend(int minPitch, int maxpitch)
{
    int minKMPitch = getPitchForOffset(0);
    int maxKMPitch = getPitchForOffset(0) + getPitchExtent() - 1;
    if (minPitch < minKMPitch) getMap()[minPitch] = std::string("");
    if (maxpitch > maxKMPitch) getMap()[maxpitch] = std::string("");
}
	

}

