// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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
#include <set>
#include <vector>

#include "NotationTypes.h"

namespace Rosegarden
{

using std::list;
using std::string;
using std::vector;
using std::multimap;
using std::set;

class Segment;
class Event;
class CompositionTimeSliceAdapter;

///////////////////////////////////////////////////////////////////////////

typedef std::string ChordType;
class ChordLabel;

namespace ChordTypes
{
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

///////////////////////////////////////////////////////////////////////////

/**
 * ChordLabel names chords and identifies them from their masks. See
 * ChordLabel::checkMap() for details on what the masks are and
 * AnalysisHelper::labelChords() for an example.
 */

class ChordLabel
{
public:
    ChordLabel();
    ChordLabel(Key key, int mask, int bass);
    ChordLabel(ChordType type, int rootPitch, int inversion = 0) :
        m_data(type, rootPitch, inversion) { };
    int rootPitch();
    /**
     * Gives the name of the chord in lead-sheet notation: C, Dm,
     * G#7b5...
     */
    std::string getName(Key key) const;
    /**
     * Gives the name of the chord in roman-numeral notation: I, ii,
     * VMm7...
     */
//  std::string getRomanNumeral(Key key);
    bool isValid() const;
    bool operator<(const ChordLabel& other) const;
    // ### I can't believe this is necessary, but the compiler
    //     is asking for it
    bool operator==(const ChordLabel& other) const;

private:
    // #### are m_* names appropriate for a struct?
    //      shouldn't I find a neater way to keep a ChordMap?
    struct ChordData
    {
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

///////////////////////////////////////////////////////////////////////////

class AnalysisHelper
{
public:
    AnalysisHelper() {};

    /**
     * Returns the key in force during a given event.
     */
    Key getKeyForEvent(Event *e, Segment &s);

    /**
     * Returns true if the given pitch is a scale step of the given key.
     */
    bool pitchIsDiatonic(int pitch, Key key);

    /**
     * Inserts in the given Segment labels for all of the chords found in
     * the timeslice in the given CompositionTimeSliceAdapter.
     */
    void labelChords(CompositionTimeSliceAdapter &c, Segment &s);

    /**
     * Returns a time signature that is probably reasonable for the
     * given timeslice.
     */
    TimeSignature guessTimeSignature(CompositionTimeSliceAdapter &c);

    /**
     * Returns a guess at the starting key of the given timeslice.
     */
    Key guessKey(CompositionTimeSliceAdapter &c);

    /**
     * Like labelChords, but the algorithm is more complicated. This tries
     * to guess the chords that should go under a beat even when all of the
     * chord members aren't played at once.
     */
    void guessHarmonies(CompositionTimeSliceAdapter &c, Segment &s);

protected:
    // ### THESE NAMES ARE AWFUL. MUST GREP THEM OUT OF EXISTENCE.
    typedef std::pair<double, ChordLabel> ChordPossibility;
    typedef std::vector<ChordPossibility> HarmonyGuess;
    typedef std::vector<std::pair<timeT, HarmonyGuess> > HarmonyGuessList;
    struct cp_less : public std::binary_function<ChordPossibility, ChordPossibility, bool>
    {
        bool operator()(ChordPossibility l, ChordPossibility r);
    };

    /// For use by guessHarmonies
    void makeHarmonyGuessList(CompositionTimeSliceAdapter &c,
                              HarmonyGuessList &l);

    /// For use by guessHarmonies
    void refineHarmonyGuessList(CompositionTimeSliceAdapter &c,
                                HarmonyGuessList& l,
				Segment &);

    /// For use by guessHarmonies (makeHarmonyGuessList)
    class PitchProfile
    {
    public:
        PitchProfile();
        double& operator[](int i);
        const double& operator[](int i) const;
        double distance(const PitchProfile &other);
        double dotProduct(const PitchProfile &other);
        double productScorer(const PitchProfile &other);
        PitchProfile normalized();
        PitchProfile& operator*=(double d);
        PitchProfile& operator+=(const PitchProfile &d);
    private:
        double m_data[12];
    };

    /// For use by guessHarmonies (makeHarmonyGuessList)
    typedef std::vector<std::pair<PitchProfile, ChordLabel> > HarmonyTable;
    static HarmonyTable m_harmonyTable;

    /// For use by guessHarmonies (makeHarmonyGuessList)
    void checkHarmonyTable();

    /// For use by guessHarmonies (refineHarmonyGuessList)
    // #### grep ProgressionMap to something else
    struct ChordProgression {
        ChordProgression(ChordLabel first_,
                         ChordLabel second_ = ChordLabel(),
                         Key key_ = Key());
        ChordLabel first;
        ChordLabel second;
        Key homeKey;
        // double commonness...
        bool operator<(const ChordProgression& other) const;
        };
    typedef set<ChordProgression> ProgressionMap;
    static ProgressionMap m_progressionMap;

    /// For use by guessHarmonies (refineHarmonyGuessList)
    void checkProgressionMap();

    /// For use by checkProgressionMap
    void addProgressionToMap(Key k,
                             int firstChordNumber,
                             int secondChordNumber);

};

}

#endif
