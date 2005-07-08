// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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
#include <vector>
#include <map>

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

typedef std::vector<MidiBank> BankList;

class MidiProgram
{
public:
    MidiProgram();
    MidiProgram(const MidiBank &bank, MidiByte program, std::string name = "",
		std::string keyMapping = "");

    // comparator disregards name
    bool operator==(const MidiProgram &p) const;
    
    const MidiBank&     getBank() const;
    MidiByte            getProgram() const;
    const std::string  &getName() const;
    const std::string  &getKeyMapping() const;

    void                setName(const std::string &name);
    void                setKeyMapping(const std::string &name);

private:
    MidiBank m_bank;
    MidiByte m_program;
    std::string m_name;
    std::string m_keyMapping;
};

typedef std::vector<MidiProgram> ProgramList;

class MidiKeyMapping
{
public:
    typedef std::map<MidiByte, std::string> KeyNameMap;

    MidiKeyMapping();
    MidiKeyMapping(const std::string &name);
    MidiKeyMapping(const std::string &name, const KeyNameMap &map);

    bool operator==(const MidiKeyMapping &m) const;

    const std::string   &getName() const { return m_name; }
    void                 setName(const std::string &name) { m_name = name; }

    const KeyNameMap    &getMap() const { return m_map; }
    KeyNameMap          &getMap() { return m_map; }
    std::string          getMapForKeyName(MidiByte pitch) const;
    void                 setMap(const KeyNameMap &map) { m_map = map; }
    
    // Return 0 if the supplied argument is the lowest pitch in the
    // mapping, 1 if it is the second-lowest, etc.  Return -1 if it
    // is not in the mapping at all.  Not instant.
    int                  getOffset(MidiByte pitch) const;

    // Return the offset'th pitch in the mapping.  Return -1 if there
    // are fewer than offset pitches in the mapping (or offset < 0).
    // Not instant.
    int                  getPitchForOffset(int offset) const;

    // Return the difference between the top and bottom pitches
    // contained in the map.
    //
    int                  getPitchExtent() const;

private:
    std::string m_name;
    KeyNameMap  m_map;
};

typedef std::vector<MidiKeyMapping> KeyMappingList;

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
		  MidiByte subBeatVely = 80);

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


// MidiFilter is a bitmask of Rosegarden::MappedEvent::MappedEventType.
// Look in sound/MappedEvent.h
//
typedef unsigned int MidiFilter;


}

#endif

