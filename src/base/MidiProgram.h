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

#ifndef RG_MIDIBANK_H
#define RG_MIDIBANK_H

#include <climits>  // UINT_MAX
#include <string>
#include <vector>
#include <map>

namespace Rosegarden
{
typedef unsigned char MidiByte;

/**
 * ??? This data structure doesn't match the concept of a bank.  A bank
 *     is a container of programs.  There is no such containment here.  If
 *     there were, it is possible that much of the code that works with
 *     banks would be simplified.  E.g. there are many places throughout
 *     the system where a program list is searched for all programs in a
 *     bank.  That would be eliminated.
 */
class MidiBank
{
public:
    MidiBank();
    MidiBank(bool percussion, MidiByte msb, MidiByte lsb, std::string name = "");

    bool                isPercussion() const;
    MidiByte            getMSB() const;
    MidiByte            getLSB() const;

    void                setName(std::string name);
    std::string         getName() const;

    /// A full comparison of all fields.
    /**
     * This probably isn't what you want.  See partialCompare().
     */
    bool operator==(const MidiBank &rhs) const;
    bool operator!=(const MidiBank &rhs) const  { return !operator==(rhs); }
    /// Compare all fields except name.
    /**
     * Since MidiProgram stores a partial MidiBank object (without name),
     * a partial comparison such as this is frequently needed.
     */
    bool partialCompare(const MidiBank &rhs) const;

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

    const MidiBank&     getBank() const;
    MidiByte            getProgram() const;
    const std::string  &getName() const;
    const std::string  &getKeyMapping() const;

    void                setName(const std::string &name);
    void                setKeyMapping(const std::string &name);

    // Only compares m_bank and m_program.  Does not compare m_name or
    // m_keyMapping.
    bool partialCompare(const MidiProgram &rhs) const;
    // Only compares m_bank, m_program, and m_name.  Does not compare
    // m_keyMapping.
    bool partialCompareWithName(const MidiProgram &rhs) const;

private:
    MidiBank m_bank;
    MidiByte m_program;
    std::string m_name;
    std::string m_keyMapping;
};

typedef std::vector<MidiProgram> ProgramList;

inline bool
partialCompareWithName(const ProgramList &lhs, const ProgramList &rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    for (unsigned i = 0; i < lhs.size(); ++i) {
        if (!lhs[i].partialCompareWithName(rhs[i])) {
            return false;
        }
    }

    return true;
}

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
    
    /**
     * Return 0 if the supplied argument is the lowest pitch in the
     * mapping, 1 if it is the second-lowest, etc.  Return -1 if it
     * is not in the mapping at all.  Not instant.
     */
//    int                  getOffset(MidiByte pitch) const;

    /**
     * Return the offset'th pitch in the mapping.  Return -1 if there
     * are fewer than offset pitches in the mapping (or offset < 0).
     * Not instant.
     */
    int                  getPitchForOffset(int offset) const;

    /**
     * Return the difference between the top and bottom pitches
     * contained in the map.
     */
    int                  getPitchExtent() const;

    /**
     * Add blank pitches to the key mapping to have it extends from at most
     * minpitch to maxpitch.
     */
    void                 extend(int minPitch = 0, int maxpitch = 127);


private:
    std::string m_name;
    KeyNameMap  m_map;
};

typedef std::vector<MidiKeyMapping> KeyMappingList;


// MidiFilter is a bitmask of MappedEvent::MappedEventType.
// Look in sound/MappedEvent.h
//
typedef unsigned int MidiFilter;


}

#endif

