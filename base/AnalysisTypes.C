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

#include <iostream>
#include <string>
#include <map>

#include "AnalysisTypes.h"
#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "CompositionAdapter.h"
#include "BaseProperties.h"

namespace Rosegarden {

using std::string;
using std::cerr;
using std::endl;
using std::multimap;

///////////////////////////////////////////////////////////////////////////
// AnalysisHelper
///////////////////////////////////////////////////////////////////////////

Key
AnalysisHelper::getKeyForEvent(Event *e, Segment &s) {

    Segment::iterator i = s.find(e);

    if (i==s.end()) return Key();

    // This is an ugly loop. Is there a better way to iterate backwards
    // through an STL container?

    while (true) {
        if ((*i)->isa(Key::EventType)) {
            return Key(**i);
        }
        if (i != s.begin()) --i;
        else break;
    }

    return Key();
}

void
AnalysisHelper::labelChords(CompositionAdapter &c, Segment &s) {

    Key key = getKeyForEvent(*c.begin(), s);

    // no increment (the inner loop does the incrementing)
    for (CompositionAdapter::iterator i = c.begin(); i != c.end();  ) {

        // Examine all notes starting at the same time as this one.
        //  1. keep the current key in key
        //  2. put the lowest pitch in bass
        //  3. keep track of which pitches are in the chord, ignoring octaves

        int mask = 0;
        int time = (*i)->getAbsoluteTime();
        int bass = 999;
        // no initialization
        for (  ; i != c.end() && (*i)->getAbsoluteTime() == time; ++i) {

            if ((*i)->isa(Key::EventType)) key = Key(**i);

            else if ((*i)->isa(Note::EventType)) {
                int pitch = (*i)->get<Int>(Rosegarden::BaseProperties::PITCH);
                if (pitch < bass) bass = pitch;
                mask |= 1 << (pitch % 12);
            }

        }

        if (mask==0) continue;

        Chord ch(key, mask, bass);

        if (ch.isValid()) {
            // label the chord (but, of course, there aren't text-indication
            // events yet)
            std::cerr << ch.getName(key) << " at time " << time << std::endl;
        }

    }

}

///////////////////////////////////////////////////////////////////////////
// Chord
///////////////////////////////////////////////////////////////////////////


Chord::ChordMap Chord::m_chordMap;

Chord::Chord() {
    checkMap();
}

Chord::Chord(Key key, int mask, int bass) :
    m_data() {

    checkMap();

    // Look for a chord built on an unaltered scale step of the current key.

    for (ChordMap::iterator i = m_chordMap.find(mask);
         i != m_chordMap.end() && i->first==mask; ++i) {

        NotationDisplayPitch dp(i->second.m_rootPitch, Clef(), key);

        if (dp.getAccidental() == Accidentals::NoAccidental) {
            m_data = i->second;
        }

        // Also accept a chord built on the raised leading tone in minor

        if (key.isMinor() &&
            ((i->second.m_rootPitch - key.getTonicPitch() + 12) % 12) == 11 ) {
            m_data = i->second;
        }

    }

    /*
      int rootBassInterval = ((bass - m_data.m_rootPitch + 12) % 12);

      // Pretend nobody cares about second and third inversions
      // (i.e., bass must always be either root or third of chord)
      if      (rootBassInterval > 7) m_data.m_type=ChordTypes::NoChord;
      else if (rootBassInterval > 4) m_data.m_type=ChordTypes::NoChord;
      // Mark first-inversion and root-position chords as such
      else if (rootBassInterval > 0) m_data.m_inversion=1;
      else 						   m_data.m_inversion=0;
    */

}

std::string
Chord::getName(Key key) {
    return NotationDisplayPitch(m_data.m_rootPitch, Clef(), key)
        .getAsString(Clef(), key, false) + m_data.m_type;
    //			+ (m_data.m_inversion>0 ? " in first inversion" : "");
}

bool
Chord::isValid() {
    return m_data.m_type != ChordTypes::NoChord;
}

void
Chord::checkMap() {
    if (!m_chordMap.empty()) return;

    const ChordType basicChordTypes[8] =
        {ChordTypes::Major, ChordTypes::Minor, ChordTypes::Diminished,
         ChordTypes::MajorSeventh, ChordTypes::DominantSeventh,
         ChordTypes::MinorSeventh, ChordTypes::HalfDimSeventh,
         ChordTypes::DimSeventh};

    // What the basicChordMasks mean: each bit set in each one represents
    // a pitch class (pitch%12) in a chord. C major has three pitch
    // classes (C, E, and G natural); if you take the MIDI pitches
    // of those notes modulo 12, you get 0, 4, and 7, so the mask for
    // major triads is (1<<0)+(1<<4)+(1<<7). All the masks are for chords
    // built on C.

    const int basicChordMasks[8] = {
        1 + (1<<4) + (1<<7),			// major
        1 + (1<<3) + (1<<7),			// minor
        1 + (1<<3) + (1<<6),			// diminished
        1 + (1<<4) + (1<<7) + (1<<11),	// major 7th
        1 + (1<<4) + (1<<7) + (1<<10),	// dominant 7th
        1 + (1<<3) + (1<<7) + (1<<10),	// minor 7th
        1 + (1<<3) + (1<<6) + (1<<10),	// half-diminished 7th
        1 + (1<<3) + (1<<6) + (1<<9)	// diminished 7th
    };

    // Each mask is inserted into the map rotated twelve ways; each
    // rotation is a mask you would get by transposing the chord
    // to have a new root (i.e., C, C#, D, D#, E, F...)

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 12; ++j) {

            m_chordMap.insert(
                              pair<int, ChordData>(
                                                   (basicChordMasks[i] << j |
                                                    basicChordMasks[i] >> (12-j)) & ((1<<12) - 1),
                                                   ChordData(basicChordTypes[i], j)
                                                   )
                              );

        }
    }

}

}
