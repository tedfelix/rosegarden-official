// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

// This file is Copyright 2002 Randall Farmer <rfarme@simons-rock.edu>

#ifndef _ANALYSISTYPES_H_
#define _ANALYSISTYPES_H_

#include <string>
#include <map>

#include "NotationTypes.h"

namespace Rosegarden {

using std::string;
using std::multimap;

class Segment;
class Event;
class CompositionAdapter;

///////////////////////////////////////////////////////////////////////////

class AnalysisHelper {
public:
    AnalysisHelper() {};
    /**
     * Returns the key in force during a given event.
     */
    Key getKeyForEvent(Event *e, Segment &s);
    /**
     * Inserts in the Segment labels for all of the chords found in
     * the timeslice in the CompositionAdapter. See
     * TransformsMenuLabelChordsCommand::modifySegment() in
     * notationcommands.cpp
     */
    void labelChords(CompositionAdapter &c, Segment &s);
};

///////////////////////////////////////////////////////////////////////////

typedef std::string ChordType;

namespace ChordTypes {
const ChordType
    NoChord = "no-chord",
    Major = "",
    Minor = "m",
    Diminished = "dim",
    MajorSeventh = "M7",
    DominantSeventh = "7",
    MinorSeventh = "m7",
    HalfDimSeventh = "7b5",
    DimSeventh = "dim7";
}

/**
 * Chord names chords and identifies from their masks. See
 * Chord::checkMap() for details on what the masks are and
 * AnalysisHelper::labelChords() for an example.
 */

class Chord {
public:
    Chord();
    Chord(Key key, int mask, int bass);
    Chord(ChordType type, int rootPitch, int inversion = 0) :
        m_data(type, rootPitch, inversion) { };
    std::string getName(Key key);
    bool isValid();

private:
    struct ChordData {
        ChordData(ChordType type, int rootPitch, int inversion = 0) :
            m_type(type),
            m_rootPitch(rootPitch),
            m_inversion(inversion) { };

        ChordData() :
            m_type(ChordTypes::NoChord),
            m_rootPitch(0),
            m_inversion(0) { };

        ChordType m_type;
        int m_rootPitch;
        int m_inversion;
    };

    ChordData m_data;
    void checkMap();

    typedef std::multimap<int, ChordData> ChordMap;
    static ChordMap m_chordMap;
};

}

#endif
