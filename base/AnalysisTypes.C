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

// This file is Copyright 2002 Randall Farmer <rfarme@simons-rock.edu>

#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <cmath> // fabs, pow

#include "AnalysisTypes.h"
#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "CompositionTimeSliceAdapter.h"
#include "BaseProperties.h"

namespace Rosegarden
{

using std::string;
using std::cerr;
using std::endl;
using std::multimap;
using std::vector;
using std::partial_sort_copy;

///////////////////////////////////////////////////////////////////////////
// Miscellany (doesn't analyze anything)
///////////////////////////////////////////////////////////////////////////

Key
AnalysisHelper::getKeyForEvent(Event *e, Segment &s)
{
    Segment::iterator i =
	e ? s.findNearestTime(e->getAbsoluteTime()) //cc
   	  : s.begin();

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

bool
AnalysisHelper::pitchIsDiatonic(int pitch, Key key)
{
    NotationDisplayPitch dp(pitch, Clef(), key);

    if (dp.getAccidental() == Accidentals::NoAccidental)
        return true;

    // ### as used in the chord identifiers, this calls chords built on
    //     the raised sixth step diatonic -- may be correct, but it's
    //     misleading, as we're really looking for whether chords are
    //     often built on given tone

    if (key.isMinor())
    {
        int stepsFromTonic = ((pitch - key.getTonicPitch() + 12) % 12);

        if (stepsFromTonic == 9 || stepsFromTonic == 11) return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////
// Simple chord identification
///////////////////////////////////////////////////////////////////////////

void
AnalysisHelper::labelChords(CompositionTimeSliceAdapter &c, Segment &s)
{

    Key key;
    if (c.begin() != c.end()) key = getKeyForEvent(*c.begin(), s);
    else key = getKeyForEvent(0, s);

    // no increment (the inner loop does the incrementing)
    for (CompositionTimeSliceAdapter::iterator i = c.begin(); i != c.end();  )
    {
        // Examine all notes starting at the same time as this one.
        //  1. keep the current key in key
        //  2. put the lowest pitch in bass
        //  3. keep track of which pitches are in the chord, ignoring octaves

        int mask = 0;
        int time = (*i)->getAbsoluteTime();
        int bass = 999;
        // no initialization
        for (  ; i != c.end() && (*i)->getAbsoluteTime() == time; ++i)
        {

            if ((*i)->isa(Key::EventType)) {
		key = Key(**i);
		Text text(key.getName(), Text::KeyName);
		s.insert(text.getAsEvent(time));
	    }

            else if ((*i)->isa(Note::EventType))
            {
                int pitch = (*i)->get<Int>(BaseProperties::PITCH);
                if (pitch < bass) bass = pitch;
                mask |= 1 << (pitch % 12);
            }

        }

        if (mask==0) continue;

        ChordLabel ch(key, mask, bass);

        if (ch.isValid())
        {
//            std::cerr << ch.getName(key) << " at time " << time << std::endl;

	    Text text(ch.getName(key), Text::ChordName);
	    s.insert(text.getAsEvent(time));
        }

    }

}


// ChordLabel
/////////////////////////////////////////////////

ChordLabel::ChordMap ChordLabel::m_chordMap;

ChordLabel::ChordLabel()
{
    checkMap();
}

ChordLabel::ChordLabel(Key key, int mask, int /* bass */) :
    m_data()
{
    AnalysisHelper ah;

    checkMap();

    // Look for a chord built on an unaltered scale step of the current key.

    for (ChordMap::iterator i = m_chordMap.find(mask);
         i != m_chordMap.end() && i->first==mask; ++i)
    {

        if (ah.pitchIsDiatonic(i->second.m_rootPitch, key))
        {
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
ChordLabel::getName(Key key) const
{
    return NotationDisplayPitch(m_data.m_rootPitch, Clef(), key)
        .getAsString(Clef(), key, false) + m_data.m_type;
    //			+ (m_data.m_inversion>0 ? " in first inversion" : "");
}

int
ChordLabel::rootPitch()
{
    return m_data.m_rootPitch;
}

bool
ChordLabel::isValid() const
{
    return m_data.m_type != ChordTypes::NoChord;
}

bool
ChordLabel::operator<(const ChordLabel& other) const
{
    if (!isValid()) return true;
    return getName(Key()) < other.getName(Key());
}

bool
ChordLabel::operator==(const ChordLabel& other) const
{
    return getName(Key()) == other.getName(Key());
}

void
ChordLabel::checkMap()
{
    if (!m_chordMap.empty()) return;

    const ChordType basicChordTypes[8] =
        {ChordTypes::Major, ChordTypes::Minor, ChordTypes::Diminished,
         ChordTypes::MajorSeventh, ChordTypes::DominantSeventh,
         ChordTypes::MinorSeventh, ChordTypes::HalfDimSeventh,
         ChordTypes::DimSeventh};

    // What the basicChordMasks mean: each bit set in each one represents
    // a pitch class (pitch%12) in a chord. C major has three pitch
    // classes, C, E, and G natural; if you take the MIDI pitches
    // of those notes modulo 12, you get 0, 4, and 7, so the mask for
    // major triads is (1<<0)+(1<<4)+(1<<7). All the masks are for chords
    // built on C.

    const int basicChordMasks[8] =
    {
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

    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 12; ++j)
        {

            m_chordMap.insert
            (
                std::pair<int, ChordData>
                (
                    (basicChordMasks[i] << j | basicChordMasks[i] >> (12-j))
                    & ((1<<12) - 1),
                    ChordData(basicChordTypes[i], j)
                )
            );

        }
    }

}

///////////////////////////////////////////////////////////////////////////
// Harmony guessing
///////////////////////////////////////////////////////////////////////////

void
AnalysisHelper::guessHarmonies(CompositionTimeSliceAdapter &c, Segment &s)
{
    HarmonyGuessList l;

    // 1. Get the list of possible harmonies
    makeHarmonyGuessList(c, l);

    // 2. Refine the list of possible harmonies by preferring chords in the
    //    current key and looking for familiar progressions and
    //    tonicizations.
    refineHarmonyGuessList(c, l, s);

    // 3. Put labels in the Segment.  For the moment we just do the
    //    really naive thing with the segment arg to refineHarmonyGuessList:
    //    could do much better here
}

// #### explain how this works:
// in terms of other functions (simple chord labelling, key guessing)
// in terms of basic concepts (pitch profile, harmony guess)
// in terms of flow

void
AnalysisHelper::makeHarmonyGuessList(CompositionTimeSliceAdapter &c,
                                     HarmonyGuessList &l)
{
    if (*c.begin() == *c.end()) return;

    checkHarmonyTable();

    // (Fetch the slice's time signature from the timesig guesser)
    TimeSignature timeSig;

    timeT timeSigTime = (*c.begin())->getAbsoluteTime();

    PitchProfile p; // defaults to all zeroes

    // Walk through the piece labelChords style

    // no increment (the first inner loop does the incrementing)
    for (CompositionTimeSliceAdapter::iterator i = c.begin(); i != c.end();  )
    {

        // 2. Update the pitch profile

        timeT time = (*i)->getAbsoluteTime();
        double emphasis =
            double(timeSig.getEmphasisForTime(time - timeSigTime));

        // Scale all the components of the pitch profile down so that
        // 1. notes that are a beat/bar away have less weight than notes
        //    from this beat/bar
        // 2. the difference in weight depends on the metrical importance
        //    of the boundary between the notes: the previous beat should
        //    get less weight if this is the first beat of a new bar

        // ### possibly too much fade here
        //     also, fade should happen w/reference to how many notes played?

        PitchProfile delta;
        int noteCount = 0;

        // no initialization
        for (  ; i != c.end() && (*i)->getAbsoluteTime() == time; ++i)
        {
            if ((*i)->isa(TimeSignature::EventType))
            {
                timeSig = TimeSignature(**i);
                timeSigTime = time;
                emphasis =
                    double(timeSig.getEmphasisForTime(time - timeSigTime));
            }
            else if ((*i)->isa(Note::EventType))
            {
                int pitch = (*i)->get<Int>(BaseProperties::PITCH);
                delta[pitch % 12] += 1 << int(emphasis);
                ++noteCount;
            }
        }

        p *= 1. / (pow(2, emphasis) + 1 + noteCount);
        p += delta;

        // 1. If there could have been a chord change, compare the current
        //    pitch profile with all of the profiles in the table to figure
        //    out which chords we are now nearest.

        // (If these events weren't on a beat boundary, assume there was no
        // chord change and continue -- ### will need this back)
/*        if ((!(i != c.end())) ||
           timeSig.getEmphasisForTime((*i)->getAbsoluteTime() - timeSigTime) < 3)
        {
            continue;
        }*/

        // (If the pitch profile hasn't changed much, continue)

        PitchProfile np = p.normalized();

        HarmonyGuess possibleChords;

        possibleChords.reserve(m_harmonyTable.size());

        for (HarmonyTable::iterator j = m_harmonyTable.begin();
             j != m_harmonyTable.end();
             ++j)
        {
            double score = np.productScorer(j->first);
            possibleChords.push_back(ChordPossibility(score, j->second));
        }

        // 3. Save a short list of the nearest chords in the
        // HarmonyGuessList passed in from guessHarmonies()

        l.push_back(std::pair<timeT, HarmonyGuess>(time, HarmonyGuess()));

        HarmonyGuess& smallerGuess = l.back().second;

        // Have to learn to love this:

        smallerGuess.resize(10);

        partial_sort_copy(possibleChords.begin(),
                          possibleChords.end(),
                          smallerGuess.begin(),
                          smallerGuess.end(),
                          cp_less());

#define GIVE_HARMONYGUESS_DETAILS
#ifdef  GIVE_HARMONYGUESS_DETAILS
        std::cerr << "Time: " << time << std::endl;

        std::cerr << "Profile: ";
        for (int k = 0; k < 12; ++k)
               std::cerr << np[k] << " ";
        std::cerr << std::endl;

        std::cerr << "Best guesses: " << std::endl;
        for (HarmonyGuess::iterator debugi = smallerGuess.begin();
             debugi != smallerGuess.end();
             ++debugi)
        {
            std::cerr << debugi->first << ": " << debugi->second.getName(Key()) << std::endl;
        }
#endif

    }

}

// Comparison function object -- can't declare this in the headers because
// this only works with pair<double, ChordLabel> instantiated,
// pair<double, ChordLabel> can't be instantiated while ChordLabel is an
// incomplete type, and ChordLabel is still an incomplete type at that
// point in the headers.

bool
AnalysisHelper::cp_less::operator()(ChordPossibility l, ChordPossibility r)
{
    // Change name from "less?"
    return l.first > r.first;
}


void
AnalysisHelper::refineHarmonyGuessList(CompositionTimeSliceAdapter &/* c */,
                                       HarmonyGuessList &l, Segment &segment)
{
    // (Fetch the piece's starting key from the key guesser)
    Key key;

    checkProgressionMap();

    if (l.size() < 2)
    {
        l.clear();
        return;
    }

    // Look at the list of harmony guesses two guesses at a time.

    HarmonyGuessList::iterator i = l.begin();
    // j stays ahead of i
    HarmonyGuessList::iterator j = i + 1;

    ChordLabel bestGuessForFirstChord, bestGuessForSecondChord;
    while (j != l.end())
    {

        double highestScore = 0;

        // For each possible pair of chords (i.e., two for loops here)
        for (HarmonyGuess::iterator k = i->second.begin();
             k != i->second.end();
             ++k)
        {
            for (HarmonyGuess::iterator l = j->second.begin();
                 l != j->second.end();
                 ++l)
            {
                // Print the guess being processed:

                //    std::cerr << k->second.getName(Key()) << "->" << l->second.getName(Key()) << std::endl;

                // For a first approximation, let's say the probability that
                // a chord guess is correct is proportional to its score. Then
                // the probability that a pair is correct is the product of
                // its scores.

                double currentScore;
                currentScore = k->first * l->first;

                //    std::cerr << currentScore << std::endl;

                // Is this a familiar progression? Bonus if so.

                bool isFamiliar = false;

                // #### my ways of breaking up long function calls are haphazard
                //      also, does this code belong here?

                ProgressionMap::iterator pmi =
                    m_progressionMap.lower_bound(
                        ChordProgression(k->second, l->second)
                    );

                // no initialization
                for ( ;
                     pmi != m_progressionMap.end()
                     && pmi->first == k->second
                     && pmi->second == l->second;
                     ++pmi)
                {
                    // key doesn't have operator== defined
                    if (key.getName() == pmi->homeKey.getName())
                    {
//                        std::cerr << k->second.getName(Key()) << "->" << l->second.getName(Key()) << " is familiar" << std::endl;
                        isFamiliar = true;
                        break;
                    }
                }

                if (isFamiliar) currentScore *= 5; // #### arbitrary

                // (Are voice-leading rules followed? Penalty if not)

                // Is this better than any pair examined so far? If so, set
                // some variables that should end up holding the best chord
                // progression
                if (currentScore > highestScore)
                {
                    bestGuessForFirstChord  = k->second;
                    bestGuessForSecondChord = l->second;
                    highestScore = currentScore;
                }

            }
        }

        // Since we're not returning any results right now, print them
	std::cerr << "Time: " << j->first << std::endl;
        std::cerr << "Best chords: "
          << bestGuessForFirstChord.getName(Key()) << ", "
          << bestGuessForSecondChord.getName(Key()) << std::endl;
        std::cerr << "Best score: " << highestScore << std::endl;

        // Using the best pair of chords:

        // Is the first chord diatonic?

            // If not, is it a secondary function?
                // If so, change the current key
                // If not, set an "implausible progression" flag

        // (Is the score of the best pair of chords reasonable?
        // If not, set the flag.)

        // Was the progression plausible?

            // If so, replace the ten or so chords in the first guess with the
            // first chord of the best pair, set
            // first-iterator=second-iterator, and ++second-iterator
            // (and possibly do the real key-setting)

            // If not, h.erase(second-iterator++)

	// Temporary hack to get _something_ interesting out:
	Event *e;
	e = Text(bestGuessForFirstChord.getName(Key()), Text::ChordName).
	    getAsEvent(j->first);
	segment.insert(new Event(*e, e->getAbsoluteTime(),
				 e->getDuration(), e->getSubOrdering()-1));
	delete e;

	e = Text(bestGuessForSecondChord.getName(Key()), Text::ChordName).
	    getAsEvent(j->first);
	segment.insert(e);

        // For now, just advance:
        i = j;
        ++j;
    }
}

AnalysisHelper::HarmonyTable AnalysisHelper::m_harmonyTable;

void
AnalysisHelper::checkHarmonyTable()
{
    if (!m_harmonyTable.empty()) return;

    // Identical to basicChordTypes in ChordLabel::checkMap
    const ChordType basicChordTypes[8] =
        {ChordTypes::Major, ChordTypes::Minor, ChordTypes::Diminished,
         ChordTypes::MajorSeventh, ChordTypes::DominantSeventh,
         ChordTypes::MinorSeventh, ChordTypes::HalfDimSeventh,
         ChordTypes::DimSeventh};

    // Like basicChordMasks in ChordLabel::checkMap(), only with
    // ints instead of bits
    const int basicChordProfiles[8][12] =
    {
    //   0  1  2  3  4  5  6  7  8  9 10 11
        {1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0},   // major
        {1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},   // minor
        {1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0},   // diminished
        {1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1},   // major 7th
        {1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0},   // dominant 7th
        {1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0},   // minor 7th
        {1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0},   // half-diminished 7th
        {1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0}    // diminished 7th
    };

    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 12; ++j)
        {
            PitchProfile p;

            for (int k = 0; k < 12; ++k)
                p[(j + k) % 12] = (basicChordProfiles[i][k] == 1)
                                  ? 1.
                                  : -1.;

            PitchProfile np = p.normalized();

            ChordLabel c(basicChordTypes[i], j);

            m_harmonyTable.push_back(std::pair<PitchProfile, ChordLabel>(np, c));
        }
    }

}

AnalysisHelper::ProgressionMap AnalysisHelper::m_progressionMap;

void
AnalysisHelper::checkProgressionMap()
{
    if (!m_progressionMap.empty()) return;
    // majorProgressionFirsts[0] = 5, majorProgressionSeconds[0]=1, so 5->1 is
    // a valid progression. Note that the chord numbers are 1-based, like the
    // Roman numeral symbols
    const int majorProgressionFirsts[9] =
        {5, 2, 6, 3, 7, 4, 4, 3, 5};
    const int majorProgressionSeconds[9] =
        {1, 5, 2, 6, 1, 2, 5, 4, 6};

    // For each major key
    for (int i = 0; i < 12; ++i)
    {
        // Make the key
        Key k(0, false); // tonicPitch = i, isMinor = false
        // Add the common progressions
        for (int j = 0; j < 9; ++j)
        {
            std::cerr << majorProgressionFirsts[j] << ", " << majorProgressionSeconds[j] << std::endl;
            addProgressionToMap(k,
                                majorProgressionFirsts[j],
                                majorProgressionSeconds[j]);
        }
        // Add I->everything
        for (int j = 1; j < 8; ++j)
        {
            addProgressionToMap(k, 1, j);
        }
        // (Add the progressions involving seventh chords)
        // (Add I->seventh chords)
    }
    // (For each minor key)
        // (Do what we just did for major keys)

}

void
AnalysisHelper::addProgressionToMap(Key k,
                                    int firstChordNumber,
                                    int secondChordNumber)
{
    // majorScalePitches[1] should be the pitch of the first step of
    // the scale, so there's padding at the beginning of both these
    // arrays.
    const int majorScalePitches[] = {0, 0, 2, 4, 5, 7, 9, 11};
    const ChordType majorDiationicTriadTypes[] =
        {ChordTypes::NoChord, ChordTypes::Major, ChordTypes::Minor,
         ChordTypes::Minor, ChordTypes::Major, ChordTypes::Major,
         ChordTypes::Minor, ChordTypes::Diminished};

    int offset = k.getTonicPitch();

    if (!k.isMinor())
    {
        ChordLabel firstChord
        (
            majorDiationicTriadTypes[firstChordNumber],
            (majorScalePitches[firstChordNumber] + offset) % 12
        );
        ChordLabel secondChord
        (
            majorDiationicTriadTypes[secondChordNumber],
            (majorScalePitches[secondChordNumber] + offset) % 12
        );
        ChordProgression p(firstChord, secondChord, k);
        m_progressionMap.insert(p);
    }
    // else handle minor-key chords

}

// AnalysisHelper::ChordProgression
/////////////////////////////////////////////////

AnalysisHelper::ChordProgression::ChordProgression(ChordLabel first_,
                                                   ChordLabel second_,
                                                   Key key_) :
    first(first_),
    second(second_),
    homeKey(key_)
{
    // nothing else
}

bool
AnalysisHelper::ChordProgression::operator<(const AnalysisHelper::ChordProgression& other) const
{
    // no need for:
    // if (first == other.first) return second < other.second;
    return first < other.first;
}

// AnalysisHelper::PitchProfile
/////////////////////////////////////////////////

AnalysisHelper::PitchProfile::PitchProfile()
{
    for (int i = 0; i < 12; ++i) m_data[i] = 0;
}

double&
AnalysisHelper::PitchProfile::operator[](int i)
{
    return m_data[i];
}

const double&
AnalysisHelper::PitchProfile::operator[](int i) const
{
    return m_data[i];
}

double
AnalysisHelper::PitchProfile::distance(const PitchProfile &other)
{
    double distance = 0;

    for (int i = 0; i < 12; ++i)
    {
        distance += fabs(other[i] - m_data[i]);
    }

    return distance;
}

double
AnalysisHelper::PitchProfile::dotProduct(const PitchProfile &other)
{
    double product = 0;

    for (int i = 0; i < 12; ++i)
    {
        product += other[i] * m_data[i];
    }

    return product;
}

double
AnalysisHelper::PitchProfile::productScorer(const PitchProfile &other)
{
    double cumulativeProduct = 1;
    double numbersInProduct = 0;

    for (int i = 0; i < 12; ++i)
    {
        if (other[i] > 0)
        {
            cumulativeProduct *= m_data[i];
            ++numbersInProduct;
        }
    }

    if (numbersInProduct > 0)
        return pow(cumulativeProduct, 1/numbersInProduct);

    return 0;
}

AnalysisHelper::PitchProfile
AnalysisHelper::PitchProfile::normalized()
{
    double size = 0;
    PitchProfile normedProfile;

    for (int i = 0; i < 12; ++i)
    {
        size += fabs(m_data[i]);
    }

    if (size == 0) size = 1;

    for (int i = 0; i < 12; ++i)
    {
        normedProfile[i] = m_data[i] / size;
    }

    return normedProfile;
}

AnalysisHelper::PitchProfile&
AnalysisHelper::PitchProfile::operator*=(double d)
{

    for (int i = 0; i < 12; ++i)
    {
        m_data[i] *= d;
    }

    return *this;
}

AnalysisHelper::PitchProfile&
AnalysisHelper::PitchProfile::operator+=(const PitchProfile& d)
{

    for (int i = 0; i < 12; ++i)
    {
        m_data[i] += d[i];
    }

    return *this;
}

///////////////////////////////////////////////////////////////////////////
// Time signature guessing
///////////////////////////////////////////////////////////////////////////

// #### this is too long
// should use constants for basic lengths, not numbers

TimeSignature
AnalysisHelper::guessTimeSignature(CompositionTimeSliceAdapter &c)
{

    // 1. Guess the duration of the beat. The right beat length is going
    //    to be a common note length, and beat boundaries should be likely
    //    to have notes starting on them.

    vector<int> beatScores(4, 0);

    // durations of quaver, dotted quaver, crotchet, dotted crotchet:
    static const int commonBeatDurations[4] = {48, 72, 96, 144};

    int j = 0;
    for (CompositionTimeSliceAdapter::iterator i = c.begin();
         i != c.end() && j < 100;
         ++i, ++j)
    {

        // Skip non-notes
        if (!(*i)->isa(Note::EventType)) continue;

        for (int k = 0; k < 4; ++k)
        {

            // Points for any note of the right length
            if ((*i)->getDuration() == commonBeatDurations[k])
                beatScores[k]++;

            // Score for the probability that a note starts on a beat
            // boundary.

            // Normally, to get the probability that a random beat boundary
            // has a note on it, you'd add a constant for each note on a
            // boundary and divide by the number of beat boundaries.
            // Instead, this multiplies the constant (1/24) by
            // commonBeatDurations[k], which is inversely proportional to
            // the number of beat boundaries.

            if ((*i)->getAbsoluteTime() % commonBeatDurations[k] == 0)
                beatScores[k] += commonBeatDurations[k] / 24;

        }

    }

    int beatDuration = 0,
        bestScore = 0;

    for (int j = 0; j < 4; ++j)
    {
        if (beatScores[j] >= bestScore)
        {
            bestScore = beatScores[j];
            beatDuration = commonBeatDurations[j];
        }
    }

    std::cerr << "AnalysisHelper::guessTimeSignature: beatDuration = " << beatDuration << std::endl;

    // 2. Guess whether the measure has two, three or four beats. The right
    //    measure length should make notes rarely cross barlines and have a
    //    high average length for notes at the start of bars.

    vector<int> measureLengthScores(5, 0);

    for (CompositionTimeSliceAdapter::iterator i = c.begin();
         i != c.end() && j < 100;
         ++i, ++j)
    {

        if (!(*i)->isa(Note::EventType)) continue;

        // k is the guess at the number of beats in a measure
        for (int k = 2; k < 5; ++k)
        {

            // Determine whether this note crosses a barline; points for the
            // measure length if it does NOT.

            int noteOffset = ((*i)->getAbsoluteTime() % (beatDuration * k));
            int noteEnd    = noteOffset + (*i)->getDuration();
            if ( !(noteEnd > (beatDuration * k)) )
                measureLengthScores[k] += 10;


            // Average length of notes at measure starts

            // Instead of dividing by the number of measure starts, this
            // multiplies by the number of beats per measure, which is
            // inversely proportional to the number of measure starts.

            if ((*i)->getAbsoluteTime() % (beatDuration * k) == 0)
                measureLengthScores[k] +=
                    (*i)->getDuration() * k / 24;

        }

    }

    int measureLength = 0;

    bestScore = 0;  // reused from earlier

    for (int j = 2; j < 5; ++j)
    {
        if (measureLengthScores[j] >= bestScore)
        {
            bestScore = measureLengthScores[j];
            measureLength = j;
        }
    }

    std::cerr << "AnalysisHelper::guessTimeSignature: measureLength = " << measureLength << std::endl;

    //
    // 3. Put the result in a TimeSignature object.
    //

    int numerator = 0, denominator = 0;

    if (beatDuration % 72 == 0)
    {

        numerator = 3 * measureLength;

        // 144 means the beat is a dotted crotchet, so the beat division
        // is a quaver, so you want 8 on bottom
        denominator = (144 * 8) / beatDuration;

    }
    else
    {

        numerator = measureLength;

        // 96 means that the beat is a crotchet, so you want 4 on bottom
        denominator = (96 * 4) / beatDuration;

    }

       std::cerr << "AnalysisHelper::guessTimeSignature: numerator = " << numerator << ", denominator = "
         << denominator  << std::endl;

    TimeSignature ts(numerator, denominator);

    return ts;

}

///////////////////////////////////////////////////////////////////////////
// Key guessing
///////////////////////////////////////////////////////////////////////////

Key
AnalysisHelper::guessKey(CompositionTimeSliceAdapter &c)
{

    // 1. Figure out the distribution of emphasis over the twelve
    //    pitch clases in the first few bears. Pitches that occur
    //    more often have greater emphasis, and pitches that occur
    //    at stronger points in the bar have greater emphasis.

    vector<int> weightedNoteCount(12, 0);
    TimeSignature timeSig;
    timeT timeSigTime = 0;

    int j = 0;
    for (CompositionTimeSliceAdapter::iterator i = c.begin();
         i != c.end() && j < 100; ++i, ++j)
    {

        // Set time signature if this is a time-sig event
        if ((*i)->isa(TimeSignature::EventType))
        {
            timeSig = TimeSignature(**i);
            timeSigTime = (*i)->getAbsoluteTime();
            continue;
        }

        // Skip any other non-notes
        if (!(*i)->isa(Note::EventType)) continue;

        // Get pitch, metric strength of this event
        int pitch = (*i)->get<Int>(BaseProperties::PITCH)%12;
        int emphasis =
            1<<timeSig.getEmphasisForTime((*i)->getAbsoluteTime() - timeSigTime);

        // Count notes
        weightedNoteCount[pitch] += emphasis;

    }

    // 2. Figure out what key best fits the distribution of emphasis.
    //    Notes outside a piece's key are rarely heavily emphasized,
    //    and the tonic and dominant of the key are likely to appear.

    // This part is longer than it should be.

    int bestTonic = -1;
    bool bestKeyIsMinor = false;
    int lowestCost = 999999999;

    for (int k = 0; k < 12; ++k)
    {
        int cost =
            // accidentals are costly
              weightedNoteCount[(k + 1 ) % 12]
            + weightedNoteCount[(k + 3 ) % 12]
            + weightedNoteCount[(k + 6 ) % 12]
            + weightedNoteCount[(k + 8 ) % 12]
            + weightedNoteCount[(k + 10) % 12]
            // tonic is very good
            - weightedNoteCount[ k           ] * 5
            // dominant is good
            - weightedNoteCount[(k + 7 ) % 12];
        if (cost < lowestCost)
        {
            bestTonic = k;
            lowestCost = cost;
        }
    }

    for (int k = 0; k < 12; ++k)
    {
        int cost =
            // accidentals are costly
              weightedNoteCount[(k + 1 ) % 12]
            + weightedNoteCount[(k + 4 ) % 12]
            + weightedNoteCount[(k + 6 ) % 12]
            // no cost for raised steps 6/7 (k+9, k+11)
            // tonic is very good
            - weightedNoteCount[ k           ] * 5
            // dominant is good
            - weightedNoteCount[(k + 7 ) % 12];
        if (cost < lowestCost)
        {
            bestTonic = k;
            bestKeyIsMinor = true;
            lowestCost = cost;
        }
    }

	// std::cerr << "AnalysisHelper::guessKey: bestTonic = " << bestTonic << ", bestKeyIsMinor = " << bestKeyIsMinor << std::endl;

    return Key(bestTonic, bestKeyIsMinor);

}

}
