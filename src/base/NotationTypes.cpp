/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/NotationTypes.h"
#include "NotationRules.h"
#include "base/BaseProperties.h"

#include <iostream>
#include <cstdlib> // for atoi
#include <limits.h> // for SHRT_MIN
#include <sstream>
#include <cstdio> // needed for sprintf()

//dmm This will make everything excruciatingly slow if defined:
//#define DEBUG_PITCH

namespace Rosegarden
{

using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;

// This is the fundamental definition of the resolution used throughout.
// It must be a multiple of 16, and should ideally be a multiple of 96.
static const timeT basePPQ = 960;

const int MIN_SUBORDERING = SHRT_MIN;

namespace Accidentals
{
#pragma GCC diagnostic ignored "-Wattributes"
    /**
     * NoAccidental means the accidental will be inferred
     * based on the performance pitch and current key at the
     * location of the note.
     */
    ROSEGARDENPRIVATE_EXPORT const Accidental NoAccidental = "no-accidental";

    ROSEGARDENPRIVATE_EXPORT const Accidental Sharp = "sharp";
    ROSEGARDENPRIVATE_EXPORT const Accidental Flat = "flat";
    ROSEGARDENPRIVATE_EXPORT const Accidental Natural = "natural";
    ROSEGARDENPRIVATE_EXPORT const Accidental DoubleSharp = "double-sharp";
    ROSEGARDENPRIVATE_EXPORT const Accidental DoubleFlat = "double-flat";

    // Additional Accidentals for demi- and sesqui- sharps and flats
    ROSEGARDENPRIVATE_EXPORT const Accidental QuarterFlat = "demiflat";
    ROSEGARDENPRIVATE_EXPORT const Accidental ThreeQuarterFlat =  "sesqiflat";
    ROSEGARDENPRIVATE_EXPORT const Accidental QuarterSharp = "demisharp";
    ROSEGARDENPRIVATE_EXPORT const Accidental ThreeQuarterSharp = "sesquisharp";

    ROSEGARDENPRIVATE_EXPORT AccidentalList getStandardAccidentals() {

        static Accidental a[] = {
            NoAccidental, Sharp, Flat, Natural, DoubleSharp, DoubleFlat
        };

        static AccidentalList v;
        if (v.size() == 0) {
            for (size_t i = 0; i < sizeof(a)/sizeof(a[0]); ++i)
                v.push_back(a[i]);
        }
        return v;
    }

    ROSEGARDENPRIVATE_EXPORT int getPitchOffset(const Accidental &acc) {
        if (acc == DoubleSharp) return 2;
        else if (acc == Sharp) return 1;
        else if (acc == Flat) return -1;
        else if (acc == DoubleFlat) return -2;
        else return 0;
    }

    ROSEGARDENPRIVATE_EXPORT Accidental getAccidental(int pitchChange) {
        if (pitchChange == -2) return DoubleFlat;
        if (pitchChange == -1) return Flat;
        // Yielding 'Natural' will add a natural-sign even if not needed, so for now
        //  just return NoAccidental
        if (pitchChange == 0) return NoAccidental;
        if (pitchChange == 1) return Sharp;
        if (pitchChange == 2) return DoubleSharp;

        // if we're getting into triple flats/sharps, we're probably atonal
        // and don't case if the accidental is simplified
        return NoAccidental;
    }
} // end namespace Accidentals

using namespace Accidentals;


namespace Marks
{
    ROSEGARDENPRIVATE_EXPORT const Mark NoMark = "no-mark";
    ROSEGARDENPRIVATE_EXPORT const Mark Accent = "accent";
    ROSEGARDENPRIVATE_EXPORT const Mark Tenuto = "tenuto";
    ROSEGARDENPRIVATE_EXPORT const Mark Staccato = "staccato";
    ROSEGARDENPRIVATE_EXPORT const Mark Staccatissimo = "staccatissimo";
    ROSEGARDENPRIVATE_EXPORT const Mark Marcato = "marcato";
    ROSEGARDENPRIVATE_EXPORT const Mark Open = "open";
    ROSEGARDENPRIVATE_EXPORT const Mark Stopped = "stopped";
    ROSEGARDENPRIVATE_EXPORT const Mark Harmonic = "harmonic";
    ROSEGARDENPRIVATE_EXPORT const Mark Sforzando = getTextMark("sf");
    ROSEGARDENPRIVATE_EXPORT const Mark Rinforzando = getTextMark("rf");
    ROSEGARDENPRIVATE_EXPORT const Mark Trill = "trill";
    ROSEGARDENPRIVATE_EXPORT const Mark LongTrill = "long-trill";
    ROSEGARDENPRIVATE_EXPORT const Mark TrillLine = "trill-line";
    ROSEGARDENPRIVATE_EXPORT const Mark Turn = "turn";
    ROSEGARDENPRIVATE_EXPORT const Mark Pause = "pause";
    ROSEGARDENPRIVATE_EXPORT const Mark UpBow = "up-bow";
    ROSEGARDENPRIVATE_EXPORT const Mark DownBow = "down-bow";

    ROSEGARDENPRIVATE_EXPORT const Mark Mordent = "mordent";
    ROSEGARDENPRIVATE_EXPORT const Mark MordentInverted = "mordent-inverted";
    ROSEGARDENPRIVATE_EXPORT const Mark MordentLong = "mordent-long";
    ROSEGARDENPRIVATE_EXPORT const Mark MordentLongInverted = "mordent-long-inverted";

    ROSEGARDENPRIVATE_EXPORT string getTextMark(string text) {
        return string("text_") + text;
    }

    ROSEGARDENPRIVATE_EXPORT bool isTextMark(Mark mark) {
        return string(mark).substr(0, 5) == "text_";
    }

    ROSEGARDENPRIVATE_EXPORT string getTextFromMark(Mark mark) {
        if (!isTextMark(mark)) return string();
        else return string(mark).substr(5);
    }

    ROSEGARDENPRIVATE_EXPORT string getFingeringMark(string fingering) {
        return string("finger_") + fingering;
    }

    ROSEGARDENPRIVATE_EXPORT bool isFingeringMark(Mark mark) {
        return string(mark).substr(0, 7) == "finger_";
    }

    ROSEGARDENPRIVATE_EXPORT bool isApplicableToRests(Mark mark) {
        if (mark == Marks::Pause) return true;
        if (isTextMark(mark)) return true;
        return false;
    }

    ROSEGARDENPRIVATE_EXPORT string getFingeringFromMark(Mark mark) {
        if (!isFingeringMark(mark)) return string();
        else return string(mark).substr(7);
    }

    ROSEGARDENPRIVATE_EXPORT int getMarkCount(const Event &e) {
        long markCount = 0;
        e.get<Int>(BaseProperties::MARK_COUNT, markCount);
        return markCount;
    }

    ROSEGARDENPRIVATE_EXPORT std::vector<Mark> getMarks(const Event &e) {

        std::vector<Mark> marks;

        long markCount = 0;
        e.get<Int>(BaseProperties::MARK_COUNT, markCount);
        if (markCount == 0) return marks;

        for (long j = 0; j < markCount; ++j) {

            Mark mark(Marks::NoMark);
            (void)e.get<String>(BaseProperties::getMarkPropertyName(j), mark);

            marks.push_back(mark);
        }

        return marks;
    }

    ROSEGARDENPRIVATE_EXPORT Mark getFingeringMark(const Event &e) {

        long markCount = 0;
        e.get<Int>(BaseProperties::MARK_COUNT, markCount);
        if (markCount == 0) return NoMark;

        for (long j = 0; j < markCount; ++j) {

            Mark mark(Marks::NoMark);
            (void)e.get<String>(BaseProperties::getMarkPropertyName(j), mark);

            if (isFingeringMark(mark)) return mark;
        }

        return NoMark;
    }

    ROSEGARDENPRIVATE_EXPORT void addMark(Event &e, const Mark &mark, bool unique) {
        if (unique && hasMark(e, mark)) return;

        long markCount = 0;
        e.get<Int>(BaseProperties::MARK_COUNT, markCount);
        e.set<Int>(BaseProperties::MARK_COUNT, markCount + 1);

        PropertyName markProperty = BaseProperties::getMarkPropertyName(markCount);
        e.set<String>(markProperty, mark);
    }

    ROSEGARDENPRIVATE_EXPORT bool removeMark(Event &e, const Mark &mark) {

        long markCount = 0;
        e.get<Int>(BaseProperties::MARK_COUNT, markCount);

        for (long j = 0; j < markCount; ++j) {
            PropertyName pn(BaseProperties::getMarkPropertyName(j));
            std::string m;
            if (e.get<String>(pn, m) && m == mark) {
                e.unset(pn);
                while (j < markCount - 1) {
                    PropertyName npn(BaseProperties::getMarkPropertyName(j+1));
                    if (e.get<String>(npn, m)) {
                        e.set<String>( pn, m);
                    }
                    pn = npn;
                    ++j;
                }
                e.set<Int>(BaseProperties::MARK_COUNT, markCount - 1);
                return true;
            }
        }

        return false;
    }

    ROSEGARDENPRIVATE_EXPORT bool hasMark(const Event &e, const Mark &mark) {
        long markCount = 0;
        e.get<Int>(BaseProperties::MARK_COUNT, markCount);

        for (long j = 0; j < markCount; ++j) {
            std::string m;
            if (e.get<String>(BaseProperties::getMarkPropertyName(j), m) && m == mark) {
                return true;
            }
        }

        return false;
    }

    ROSEGARDENPRIVATE_EXPORT std::vector<Mark> getStandardMarks() {

        static Mark a[] = {
            NoMark, Accent, Tenuto, Staccato, Staccatissimo, Marcato, Open,
            Stopped, Harmonic, Sforzando, Rinforzando, Trill, LongTrill,
            TrillLine, Turn, Pause, UpBow, DownBow, Mordent, MordentInverted,
            MordentLong, MordentLongInverted
        };

        static std::vector<Mark> v;
        if (v.size() == 0) {
            for (size_t i = 0; i < sizeof(a)/sizeof(a[0]); ++i)
                v.push_back(a[i]);
        }
        return v;
    }

} // end namespace Marks

using namespace Marks;


//////////////////////////////////////////////////////////////////////
// Clef
//////////////////////////////////////////////////////////////////////

const string Clef::EventType = "clefchange";
const int Clef::EventSubOrdering = -250;
const PropertyName Clef::ClefPropertyName = "clef";
const PropertyName Clef::OctaveOffsetPropertyName = "octaveoffset";
const string Clef::Treble = "treble";
const string Clef::French = "french";
const string Clef::Soprano = "soprano";
const string Clef::Mezzosoprano = "mezzosoprano";
const string Clef::Alto = "alto";
const string Clef::Tenor = "tenor";
const string Clef::Baritone = "baritone";
const string Clef::Varbaritone = "varbaritone";
const string Clef::Bass = "bass";
const string Clef::Subbass = "subbass";
const string Clef::TwoBar = "twobar";

const Clef Clef::DefaultClef = Clef("treble");
const Clef Clef::UndefinedClef = Clef("undefined");

Clef::Clef(const Event &e) :
    m_clef(DefaultClef.m_clef),
    m_octaveOffset(0)
{
    if (e.getType() != EventType) {
        std::cerr << Event::BadType
            ("Clef model event", EventType, e.getType()).getMessage()
                  << std::endl;
        return;
    }

    std::string s;
    e.get<String>(ClefPropertyName, s);

    if (s != Treble && s != Soprano && s != French && s != Mezzosoprano && s != Alto && s != Tenor && s != Baritone && s != Bass && s != Varbaritone && s != Subbass && s != TwoBar) {
        std::cerr << BadClefName("No such clef as \"" + s + "\"").getMessage()
                  << std::endl;
            return;
    }

    long octaveOffset = 0;
    (void)e.get<Int>(OctaveOffsetPropertyName, octaveOffset);

    m_clef = s;
    m_octaveOffset = octaveOffset;
}

Clef::Clef(const std::string &s, int octaveOffset)
    // throw (BadClefName)
{
    if (s != Treble && s != Soprano && s != French && s != Mezzosoprano
        && s != Alto && s != Tenor && s != Baritone && s != Bass
        && s != Varbaritone && s != Subbass && s != TwoBar && s != "undefined") {

        throw BadClefName("No such clef as \"" + s + "\"");
    }
    m_clef = s;
    m_octaveOffset = octaveOffset;
}

Clef &Clef::operator=(const Clef &c)
{
    if (this != &c) {
        m_clef = c.m_clef;
        m_octaveOffset = c.m_octaveOffset;
    }
    return *this;
}

bool Clef::isValid(const Event &e)
{
    if (e.getType() != EventType) return false;

    std::string s;
    e.get<String>(ClefPropertyName, s);
    if (s != Treble && s != Soprano && s != French && s != Mezzosoprano && s != Alto && s != Tenor && s != Baritone && s != Bass && s != Varbaritone && s != Subbass && s != TwoBar) return false;

    return true;
}

int Clef::getTranspose() const
{
//!!! plus or minus?
    return getOctave() * 12 - getPitchOffset();
}

int Clef::getOctave() const
{
    if (m_clef == Treble || m_clef == French) return 0 + m_octaveOffset;
    else if (m_clef == Bass || m_clef == Varbaritone || m_clef == Subbass) return -2 + m_octaveOffset;
    else return -1 + m_octaveOffset;
}

int Clef::getPitchOffset() const
{
    if (m_clef == Treble) return 0;
    else if (m_clef == French) return -2;
    else if (m_clef == Soprano) return -5;
    else if (m_clef == Mezzosoprano) return -3;
    else if (m_clef == Alto) return -1;
    else if (m_clef == Tenor) return 1;
    else if (m_clef == Baritone) return 3;
    else if (m_clef == Varbaritone) return -4;
    else if (m_clef == Bass) return -2;
    else if (m_clef == Subbass) return 0;
    else if (m_clef == TwoBar) return 0;
    else return -2;
}

int Clef::getAxisHeight() const
{
    if (m_clef == Treble) return 2;
    else if (m_clef == French) return 0;
    else if (m_clef == Soprano) return 0;
    else if (m_clef == Mezzosoprano) return 2;
    else if (m_clef == Alto) return 4;
    else if (m_clef == Tenor) return 6;
    else if (m_clef == Baritone) return 8;
    else if (m_clef == Varbaritone) return 4;
    else if (m_clef == Bass) return 6;
    else if (m_clef == Subbass) return 8;
    else if (m_clef == TwoBar) return 4;
    else return 6;
}

Clef::ClefList
Clef::getClefs()
{
    ClefList clefs;
    clefs.push_back(Clef(TwoBar));
    clefs.push_back(Clef(Bass));
    clefs.push_back(Clef(Varbaritone));
    clefs.push_back(Clef(Subbass));
    clefs.push_back(Clef(Baritone));
    clefs.push_back(Clef(Tenor));
    clefs.push_back(Clef(Alto));
    clefs.push_back(Clef(Mezzosoprano));
    clefs.push_back(Clef(Soprano));
    clefs.push_back(Clef(French));
    clefs.push_back(Clef(Treble));
    return clefs;
}

Event *Clef::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(ClefPropertyName, m_clef);
    e->set<Int>(OctaveOffsetPropertyName, m_octaveOffset);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Key
//////////////////////////////////////////////////////////////////////

Key::KeyDetailMap Key::m_keyDetailMap = Key::KeyDetailMap();

const string Key::EventType = "keychange";
const int Key::EventSubOrdering = -200;
const PropertyName Key::KeyPropertyName = "key";
const Key Key::DefaultKey = Key("C major");
const Key Key::UndefinedKey = Key("undefined");

Key::Key() :
    m_name(DefaultKey.m_name),
    m_accidentalHeights(0)
{
    checkMap();
}


Key::Key(const Event &e) :
    m_name(""),
    m_accidentalHeights(0)
{
    checkMap();
    if (e.getType() != EventType) {
        std::cerr << Event::BadType
            ("Key model event", EventType, e.getType()).getMessage()
                  << std::endl;
        return;
    }
    e.get<String>(KeyPropertyName, m_name);
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
        std::cerr << BadKeyName
            ("No such key as \"" + m_name + "\"").getMessage() << std::endl;
        return;
    }
}

Key::Key(const std::string &name) :
    m_name(name),
    m_accidentalHeights(0)
{
    if (name == "undefined") return;
    checkMap();
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
        throw BadKeyName("No such key as \"" + m_name + "\"");
    }
}

Key::Key(int accidentalCount, bool isSharp, bool isMinor) :
    m_accidentalHeights(0)
{
    checkMap();
    for (KeyDetailMap::const_iterator i = m_keyDetailMap.begin();
         i != m_keyDetailMap.end(); ++i) {
        if ((*i).second.m_sharpCount == accidentalCount &&
            (*i).second.m_minor == isMinor &&
            ((*i).second.m_sharps == isSharp ||
             (*i).second.m_sharpCount == 0)) {
            m_name = (*i).first;
            return;
        }
    }

    std::ostringstream os;

    os << "No " << (isMinor ? "minor" : "major") << " key with "
       << accidentalCount << (isSharp ? " sharp(s)" : " flat(s)");

    throw BadKeySpec(os.str());
}

// Unfortunately this is ambiguous -- e.g. B major / Cb major.
// We need an isSharp argument, but we already have a constructor
// with that signature.  Not quite sure what's the best solution.

Key::Key(int tonicPitch, bool isMinor) :
    m_accidentalHeights(0)
{
        checkMap();
    for (KeyDetailMap::const_iterator i = m_keyDetailMap.begin();
         i != m_keyDetailMap.end(); ++i) {
        if ((*i).second.m_tonicPitch == tonicPitch &&
            (*i).second.m_minor == isMinor) {
            m_name = (*i).first;
            return;
        }
    }

    std::ostringstream os;

    os << "No " << (isMinor ? "minor" : "major") << " key with tonic pitch "
       << tonicPitch;

    throw BadKeySpec(os.str());
}


Key::Key(const Key &kc) :
    m_name(kc.m_name),
    m_accidentalHeights(0)
{
}

Key& Key::operator=(const Key &kc)
{
    m_name = kc.m_name;
    m_accidentalHeights = 0;
    return *this;
}

bool Key::isValid(const Event &e)
{
    if (e.getType() != EventType) return false;
    std::string name;
    e.get<String>(KeyPropertyName, name);
    if (m_keyDetailMap.find(name) == m_keyDetailMap.end()) return false;
    return true;
}

Key::KeyList Key::getKeys(bool minor)
{
    checkMap();
    KeyList result;
    for (KeyDetailMap::const_iterator i = m_keyDetailMap.begin();
         i != m_keyDetailMap.end(); ++i) {
        if ((*i).second.m_minor == minor) {
            result.push_back(Key((*i).first));
        }
    }
    return result;
}

Key Key::transpose(int pitchDelta, int heightDelta)
{
    Pitch tonic(getTonicPitch());
    Pitch newTonic = tonic.transpose(*this, pitchDelta, heightDelta);
    int newTonicPitch = (newTonic.getPerformancePitch() % 12 + 12) % 12;
    return Key (newTonicPitch, isMinor());
}

Accidental Key::getAccidentalAtHeight(int height, const Clef &clef) const
{
    checkAccidentalHeights();
    height = canonicalHeight(height);
    for (size_t i = 0; i < m_accidentalHeights->size(); ++i) {
        if (height ==static_cast<int>(canonicalHeight((*m_accidentalHeights)[i] +
                                                     clef.getPitchOffset()))) {
            return isSharp() ? Sharp : Flat;
        }
    }
    return NoAccidental;
}

Accidental Key::getAccidentalForStep(int step) const
{
    if (isMinor()) {
        step = (step + 5) % 7;
    }

    int accidentalCount = getAccidentalCount();

    if (accidentalCount == 0) {
        return NoAccidental;
    }

    bool sharp = isSharp();

    int currentAccidentalPosition = sharp ? 6 : 3;

    for (int i = 1; i <= accidentalCount; i++) {
        if (step == currentAccidentalPosition) {
            return sharp ? Sharp : Flat;
        }

        currentAccidentalPosition =
            (currentAccidentalPosition + (sharp ? 3 : 4)) % 7;
    }

    return NoAccidental;
}

vector<int> Key::getAccidentalHeights(const Clef &clef) const
{
    // staff positions of accidentals
    checkAccidentalHeights();
    vector<int> v(*m_accidentalHeights);
    int offset = clef.getPitchOffset();

    for (unsigned int i = 0; i < v.size(); ++i) {
        v[i] += offset;
        if (offset > 0)
            if (v[i] > 8) v[i] -= 7;
    }
    return v;
}

void Key::checkAccidentalHeights() const
{
    if (m_accidentalHeights) return;
    m_accidentalHeights = new vector<int>;

    bool sharp = isSharp();
    int accidentals = getAccidentalCount();
    int height = sharp ? 8 : 4;

    for (int i = 0; i < accidentals; ++i) {
        m_accidentalHeights->push_back(height);
        if (sharp) { height -= 3; if (height < 3) height += 7; }
        else       { height += 3; if (height > 7) height -= 7; }
    }
}

int Key::convertFrom(int p, const Key &previousKey,
                     const Accidental &explicitAccidental) const
{
    Pitch pitch(p, explicitAccidental);
    int height = pitch.getHeightOnStaff(Clef(), previousKey);
    Pitch newPitch(height, Clef(), *this, explicitAccidental);
    return newPitch.getPerformancePitch();
}

int Key::transposeFrom(int pitch, const Key &previousKey) const
{
    int delta = getTonicPitch() - previousKey.getTonicPitch();
    if (delta >  6) delta -= 12;
    if (delta < -6) delta += 12;
    return pitch + delta;
}

Event *Key::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(KeyPropertyName, m_name);
    return e;
}


void Key::checkMap() {
    if (!m_keyDetailMap.empty()) return;

    m_keyDetailMap["A major" ] = KeyDetails(true,  false, 3, "F# minor", "A  maj / F# min", 9);
    m_keyDetailMap["F# minor"] = KeyDetails(true,  true,  3, "A major",  "A  maj / F# min", 6);
    m_keyDetailMap["Ab major"] = KeyDetails(false, false, 4, "F minor",  "Ab maj / F  min", 8);
    m_keyDetailMap["F minor" ] = KeyDetails(false, true,  4, "Ab major", "Ab maj / F  min", 5);
    m_keyDetailMap["B major" ] = KeyDetails(true,  false, 5, "G# minor", "B  maj / G# min", 11);
    m_keyDetailMap["G# minor"] = KeyDetails(true,  true,  5, "B major",  "B  maj / G# min", 8);
    m_keyDetailMap["Bb major"] = KeyDetails(false, false, 2, "G minor",  "Bb maj / G  min", 10);
    m_keyDetailMap["G minor" ] = KeyDetails(false, true,  2, "Bb major", "Bb maj / G  min", 7);
    m_keyDetailMap["C major" ] = KeyDetails(true,  false, 0, "A minor",  "C  maj / A  min", 0);
    m_keyDetailMap["A minor" ] = KeyDetails(false, true,  0, "C major",  "C  maj / A  min", 9);
    m_keyDetailMap["Cb major"] = KeyDetails(false, false, 7, "Ab minor", "Cb maj / Ab min", 11);
    m_keyDetailMap["Ab minor"] = KeyDetails(false, true,  7, "Cb major", "Cb maj / Ab min", 8);
    m_keyDetailMap["C# major"] = KeyDetails(true,  false, 7, "A# minor", "C# maj / A# min", 1);
    m_keyDetailMap["A# minor"] = KeyDetails(true,  true,  7, "C# major", "C# maj / A# min", 10);
    m_keyDetailMap["D major" ] = KeyDetails(true,  false, 2, "B minor",  "D  maj / B  min", 2);
    m_keyDetailMap["B minor" ] = KeyDetails(true,  true,  2, "D major",  "D  maj / B  min", 11);
    m_keyDetailMap["Db major"] = KeyDetails(false, false, 5, "Bb minor", "Db maj / Bb min", 1);
    m_keyDetailMap["Bb minor"] = KeyDetails(false, true,  5, "Db major", "Db maj / Bb min", 10);
    m_keyDetailMap["E major" ] = KeyDetails(true,  false, 4, "C# minor", "E  maj / C# min", 4);
    m_keyDetailMap["C# minor"] = KeyDetails(true,  true,  4, "E major",  "E  maj / C# min", 1);
    m_keyDetailMap["Eb major"] = KeyDetails(false, false, 3, "C minor",  "Eb maj / C  min", 3);
    m_keyDetailMap["C minor" ] = KeyDetails(false, true,  3, "Eb major", "Eb maj / C  min", 0);
    m_keyDetailMap["F major" ] = KeyDetails(false, false, 1, "D minor",  "F  maj / D  min", 5);
    m_keyDetailMap["D minor" ] = KeyDetails(false, true,  1, "F major",  "F  maj / D  min", 2);
    m_keyDetailMap["F# major"] = KeyDetails(true,  false, 6, "D# minor", "F# maj / D# min", 6);
    m_keyDetailMap["D# minor"] = KeyDetails(true,  true,  6, "F# major", "F# maj / D# min", 3);
    m_keyDetailMap["G major" ] = KeyDetails(true,  false, 1, "E minor",  "G  maj / E  min", 7);
    m_keyDetailMap["E minor" ] = KeyDetails(true,  true,  1, "G major",  "G  maj / E  min", 4);
    m_keyDetailMap["Gb major"] = KeyDetails(false, false, 6, "Eb minor", "Gb maj / Eb min", 6);
    m_keyDetailMap["Eb minor"] = KeyDetails(false, true,  6, "Gb major", "Gb maj / Eb min", 3);
    m_keyDetailMap["undefined"] = KeyDetails(true,  false, 0, "A minor",  "C  maj / A  min", 0); //=default
}


Key::KeyDetails::KeyDetails()
    : m_sharps(false), m_minor(false), m_sharpCount(0),
      m_equivalence(""), m_rg2name(""), m_tonicPitch(0)
{
}

Key::KeyDetails::KeyDetails(bool sharps, bool minor, int sharpCount,
                            std::string equivalence, std::string rg2name,
                            int tonicPitch)
    : m_sharps(sharps), m_minor(minor), m_sharpCount(sharpCount),
      m_equivalence(equivalence), m_rg2name(rg2name), m_tonicPitch(tonicPitch)
{
}

Key::KeyDetails::KeyDetails(const Key::KeyDetails &d)
    : m_sharps(d.m_sharps), m_minor(d.m_minor),
      m_sharpCount(d.m_sharpCount), m_equivalence(d.m_equivalence),
      m_rg2name(d.m_rg2name), m_tonicPitch(d.m_tonicPitch)
{
}

Key::KeyDetails& Key::KeyDetails::operator=(const Key::KeyDetails &d)
{
    if (&d == this) return *this;
    m_sharps = d.m_sharps; m_minor = d.m_minor;
    m_sharpCount = d.m_sharpCount; m_equivalence = d.m_equivalence;
    m_rg2name = d.m_rg2name; m_tonicPitch = d.m_tonicPitch;
    return *this;
}

//////////////////////////////////////////////////////////////////////
// Indication
//////////////////////////////////////////////////////////////////////

const std::string Indication::EventType = "indication";
const int Indication::EventSubOrdering = -50;
const PropertyName Indication::IndicationTypePropertyName = "indicationtype";
//const PropertyName Indication::IndicationDurationPropertyName = "indicationduration";
static const PropertyName IndicationDurationPropertyName = "indicationduration";//!!!

const std::string Indication::Slur = "slur";
const std::string Indication::PhrasingSlur = "phrasingslur";
const std::string Indication::Crescendo = "crescendo";
const std::string Indication::Decrescendo = "decrescendo";
const std::string Indication::Glissando = "glissando";
const std::string Indication::QuindicesimaUp = "ottava2up";
const std::string Indication::OttavaUp = "ottavaup";
const std::string Indication::OttavaDown = "ottavadown";
const std::string Indication::QuindicesimaDown = "ottava2down";
const std::string Indication::TrillLine = "trill-line";
const std::string Indication::FigParameterChord = "parameter-chord";
const std::string Indication::Figuration = "figuration";

Indication::Indication(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("Indication model event", EventType, e.getType());
    }
    std::string s;
    e.get<String>(IndicationTypePropertyName, s);
    if (!isValid(s)) {
        throw BadIndicationName("No such indication as \"" + s + "\"");
    }
    m_indicationType = s;

    m_duration = e.getDuration();
    if (m_duration == 0) {
        e.get<Int>(IndicationDurationPropertyName, m_duration); // obsolete property
    }
}

Indication::Indication(const std::string &s, timeT indicationDuration)
{
    if (!isValid(s)) {
        throw BadIndicationName("No such indication as \"" + s + "\"");
    }
    m_indicationType = s;
    m_duration = indicationDuration;
}

Indication &
Indication::operator=(const Indication &m)
{
    if (&m != this) {
        m_indicationType = m.m_indicationType;
        m_duration = m.m_duration;
    }
    return *this;
}

Event *
Indication::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, m_duration, EventSubOrdering);
    e->set<String>(IndicationTypePropertyName, m_indicationType);

    // Set this obsolete property as well, as otherwise we could actually
    // crash earlier versions of RG by loading files exported from this one!
    e->set<Int>(IndicationDurationPropertyName, m_duration);

    return e;
}

bool
Indication::isValid(const std::string &s) const
{
    return
        (s == Slur || s == PhrasingSlur ||
         s == Crescendo || s == Decrescendo ||
         s == Glissando || // we have a stub for a glissando already?!
         s == TrillLine ||
         s == QuindicesimaUp || s == OttavaUp ||
         s == OttavaDown || s == QuindicesimaDown ||
         s == FigParameterChord ||
         s == Figuration);
}



//////////////////////////////////////////////////////////////////////
// Text
//////////////////////////////////////////////////////////////////////

const std::string Text::EventType = "text";
const int Text::EventSubOrdering = -70;
const PropertyName Text::TextPropertyName = "text";
const PropertyName Text::TextTypePropertyName = "type";
const PropertyName Text::LyricVersePropertyName = "verse";

// text styles
const std::string Text::UnspecifiedType   = "unspecified";
const std::string Text::StaffName         = "staffname";
const std::string Text::ChordName         = "chordname";
const std::string Text::KeyName           = "keyname";
const std::string Text::Dynamic           = "dynamic";
const std::string Text::Lyric             = "lyric";
const std::string Text::Chord             = "chord";
const std::string Text::Direction         = "direction";
const std::string Text::LocalDirection    = "local_direction";
const std::string Text::Tempo             = "tempo";
const std::string Text::LocalTempo        = "local_tempo";
const std::string Text::Annotation        = "annotation";
const std::string Text::LilyPondDirective = "lilypond_directive";

// special LilyPond directives
const std::string Text::FakeSegno   = "Segno"; // DEPRECATED
const std::string Text::FakeCoda    = "Coda";  // DEPRECATED
const std::string Text::Alternate1  = "Alt1 ->";
const std::string Text::Alternate2  = "Alt2 ->";
const std::string Text::BarDouble   = "|| ->";
const std::string Text::BarEnd      = "|. ->";
const std::string Text::BarDot      = ":  ->";
const std::string Text::Gliss       = "Gliss.";
const std::string Text::Arpeggio    = "Arp.";
//const std::string Text::ArpeggioUp  = "Arp.^";
//const std::string Text::ArpeggioDn  = "Arp._";
const std::string Text::Tiny        = "tiny ->";
const std::string Text::Small       = "small ->";
const std::string Text::NormalSize  = "norm. ->";

Text::Text(const Event &e) :
    m_verse(0)
{
    if (e.getType() != EventType) {
        throw Event::BadType("Text model event", EventType, e.getType());
    }

    m_text = "";
    m_type = Text::UnspecifiedType;

    e.get<String>(TextPropertyName, m_text);
    e.get<String>(TextTypePropertyName, m_type);
    e.get<Int>(LyricVersePropertyName, m_verse);
}

Text::Text(const std::string &s, const std::string &type) :
    m_text(s),
    m_type(type),
    m_verse(0)
{
    // nothing else
}

Text::Text(const Text &t) :
    m_text(t.m_text),
    m_type(t.m_type),
    m_verse(t.m_verse)
{
    // nothing else
}

Text &
Text::operator=(const Text &t)
{
    if (&t != this) {
        m_text = t.m_text;
        m_type = t.m_type;
        m_verse = t.m_verse;
    }
    return *this;
}

Text::~Text()
{
    // nothing
}

bool
Text::isTextOfType(Event *e, std::string type)
{
    return (e->isa(EventType) &&
            e->has(TextTypePropertyName) &&
            e->get<String>(TextTypePropertyName) == type);
}

std::vector<std::string>
Text::getUserStyles()
{
    std::vector<std::string> v;

    v.push_back(Dynamic);
    v.push_back(Direction);
    v.push_back(LocalDirection);
    v.push_back(Tempo);
    v.push_back(LocalTempo);
    v.push_back(Chord);
    v.push_back(Lyric);
    v.push_back(Annotation);
    v.push_back(LilyPondDirective);

    return v;
}

std::vector<std::string>
Text::getLilyPondDirectives()
{
    std::vector<std::string> v;

    v.push_back(Alternate1);
    v.push_back(Alternate2);
    v.push_back(FakeSegno);
    v.push_back(FakeCoda);
    v.push_back(BarDouble);
    v.push_back(BarEnd);
    v.push_back(BarDot);
    v.push_back(Gliss);
    v.push_back(Arpeggio);
//    v.push_back(ArpeggioUp);
//    v.push_back(ArpeggioDn);
    v.push_back(Tiny);
    v.push_back(Small);
    v.push_back(NormalSize);

    return v;
}

Event *
Text::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(TextPropertyName, m_text);
    e->set<String>(TextTypePropertyName, m_type);
    if (m_type == Lyric) e->set<Int>(LyricVersePropertyName, m_verse);
    return e;
}

bool
pitchInKey(int pitch, const Key& key)
{
    int pitchOffset = (pitch - key.getTonicPitch() + 12) % 12;

    static int pitchInMajor[] =
        { true, false, true, false, true, true, false, true, false, true, false, true };
    static int pitchInMinor[] =
        { true, false, true, true, false, true, false, true, true, false, true, false };

    if (key.isMinor()) {
        return pitchInMinor[pitchOffset];
    }
    else {
        return pitchInMajor[pitchOffset];
    }
}

/**
 * @param pitch in the range 0..11 (C..B)
 *
 * @author Arnout Engelen
 */
Accidental
resolveNoAccidental(int pitch,
                  const Key &key,
                  NoAccidentalStrategy noAccidentalStrategy)
{
    Accidental outputAccidental = "";

    // Find out the accidental to use, based on the strategy specified
    switch (noAccidentalStrategy) {
        case UseKeySharpness:
            noAccidentalStrategy =
                key.isSharp() ? UseSharps : UseFlats;
            // fall though
        case UseFlats:
            // shares code with UseSharps
        case UseSharps:
            if (pitchInKey(pitch, key)) {
                outputAccidental = NoAccidental;
            }
            else {
                if (noAccidentalStrategy == UseSharps) {
                    outputAccidental = Sharp;
                }
                else {
                    outputAccidental = Flat;
                }
            }
            break;
        case UseKey:
            // the distance of the pitch from the tonic of the current
            //  key
            int pitchOffset = (pitch - key.getTonicPitch() + 12) % 12;
            // 0: major, 1: minor
            int minor = key.isMinor();
            static int pitchToHeight[2][12] =
                {
                    { 0, 0, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6 },
                    // a ., b, c, ., d, ., e, f, ., g, .
                    { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 }
                };

            // map pitchOffset to the extra correction, on top of any
            // accidentals in the key. Example: in F major, with a pitchOffset
            // of 6, the resulting height would be 3 (Bb) and the correction
            // would be +1, so the resulting note would be B-natural
            static int pitchToCorrection[2][12] =
                {
                    { 0, +1, 0, -1, 0, 0, +1, 0, -1, 0, -1, 0 },
                    { 0, -1, 0, 0, +1, 0, -1, 0, 0, +1, 0, +1 }
                };

            int correction = pitchToCorrection[minor][pitchOffset];

            // Get the accidental normally associated with this height in this
            //  key.
            Accidental normalAccidental = key.getAccidentalForStep(pitchToHeight[minor][pitchOffset]);

            // Apply the pitchCorrection and get the outputAccidental
            outputAccidental = Accidentals::getAccidental(
                getPitchOffset(normalAccidental) + correction);

    }

    return outputAccidental;
}

/**
 * @param pitch in the range 0..11 (C..B)
 *
 * @author Michael McIntyre
 */
void
resolveSpecifiedAccidental(int pitch,
                              const Clef &/* clef */,
                              const Key &key,
                              int &height,
                              int &octave,
                              Accidental &inputAccidental,
                              Accidental &outputAccidental)
{
        // 4.  Get info from the Key
    long accidentalCount = key.getAccidentalCount();
    bool keyIsSharp = key.isSharp(), keyIsFlat = !keyIsSharp;

    // Calculate the flags needed for resolving accidentals against the key.
    // First we initialize them false...
    bool keyHasSharpC = false, keyHasSharpD = false, keyHasSharpE = false,
         keyHasSharpF = false, keyHasSharpG = false, keyHasSharpA = false,
         keyHasSharpB = false, keyHasFlatC  = false, keyHasFlatD  = false,
         keyHasFlatE  = false, keyHasFlatF  = false, keyHasFlatG  = false,
         keyHasFlatA  = false, keyHasFlatB  = false;

    // Then we use "trip points" based on the flat/sharp state of the key and
    // its number of accidentals to set the flags:
    if (keyIsSharp) {
        switch (accidentalCount) {
            case 7: keyHasSharpB = true;
            case 6: keyHasSharpE = true;
            case 5: keyHasSharpA = true;
            case 4: keyHasSharpD = true;
            case 3: keyHasSharpG = true;
            case 2: keyHasSharpC = true;
            case 1: keyHasSharpF = true;
        }
    } else {
        switch (accidentalCount) {
            case 7: keyHasFlatF = true;
            case 6: keyHasFlatC = true;
            case 5: keyHasFlatG = true;
            case 4: keyHasFlatD = true;
            case 3: keyHasFlatA = true;
            case 2: keyHasFlatE = true;
            case 1: keyHasFlatB = true;
        }
   }


    // 5. Determine height on staff and accidental note should display with for key...
    //
    // Every position on the staff is one of six accidental states:
    //
    // Natural, Sharp, Flat, DoubleSharp, DoubleFlat, NoAccidental
    //
    // DoubleSharp and DoubleFlat are always user-specified accidentals, so
    // they are always used to decide how to draw the note, and they are
    // always passed along unchanged.
    //
    // The Natural state indicates that a note is or might be going against
    // the key.  Since the Natural state will always be attached to a plain
    // pitch that can never resolve to a "black key" note, it is not necessary
    // to handle this case differently unless the key has "white key" notes
    // that are supposed to take accidentals for the key.  (eg. Cb Gb B C# major)
    // For most keys we treat it the same as a NoAccidental, and use the key
    // to decide where to draw the note, and what accidental to return.
    //
    // The Sharp and Flat states indicate that a user has specified an
    // accidental for the note, and it might be "out of key."  We check to see
    // if that's the case.  If the note is "in key" then the extra accidental
    // property is removed, and we return NoAccidental.  If the note is "out of
    // key" then the Sharp or Flat is used to decide where to draw the note, and
    // the accidental is passed along unchanged.  (Incomplete?  Will a failure
    // to always pass along the accidental cause strange behavior if a user
    // specifies an explicit Bb in key of F and then transposes to G, wishing
    // the Bb to remain an explicit Bb?  If someone complains, I'll know where
    // to look.)
    //
    // The NoAccidental state is a default state.  We have nothing else upon
    // which to base a decision in this case, so we make the best decisions
    // possible using only the pitch and key.  Notes that are "in key" pass on
    // with NoAccidental preserved, otherwise we return an appropriate
    // accidental for the key.

    // We calculate height on a virtual staff, and then make necessary adjustments to
    // translate them onto a particular Clef later on...
    //
    // ---------F--------- Staff Height   Note(semitone) for each of five states:
    //          E
    // ---------D---------               Natural|  Sharp | Flat   |DblSharp| DblFlat
    //          C                               |        |        |        |
    // ---------B--------- height  4      B(11) | B#( 0) | Bb(10) | Bx( 1) | Bbb( 9)
    //          A          height  3      A( 9) | A#(10) | Ab( 8) | Ax(11) | Abb( 7)
    // ---------G--------- height  2      G( 7) | G#( 8) | Gb( 6) | Gx( 9) | Gbb( 5)
    //          F          height  1      F( 5) | F#( 6) | Fb( 4) | Fx( 7) | Fbb( 3)
    // ---------E--------- height  0      E( 4) | E#( 5) | Eb( 3) | Ex( 6) | Ebb( 2)
    //          D          height -1      D( 2) | D#( 3) | Db( 1) | Dx( 4) | Dbb( 0)
    //       ---C----      height -2      C( 0) | C#( 1) | Cb(11) | Cx( 2) | Cbb(10)


    // use these constants instead of numeric literals in order to reduce the
    // chance of making incorrect height assignments...
    const int C = -2, D = -1, E = 0, F = 1, G = 2, A = 3, B = 4;

    // Here we do the actual work of making all the decisions explained above.
    switch (pitch) {
        case 0 :
                 if (inputAccidental == Sharp ||                         // B#
                    (inputAccidental == NoAccidental && keyHasSharpB)) {
                     height = B;
                     octave--;
                     outputAccidental = (keyHasSharpB) ? NoAccidental : Sharp;
                 } else if (inputAccidental == DoubleFlat) {             // Dbb
                     height = D;
                     outputAccidental = DoubleFlat;
                 } else {
                     height = C;                                        // C or C-Natural
                     outputAccidental = (keyHasFlatC || keyHasSharpC ||
                                   (keyHasSharpB &&
                                  inputAccidental == Natural)) ? Natural : NoAccidental;
                 }
                 break;
        case 1 :
                 if (inputAccidental == Sharp ||                       // C#
                    (inputAccidental == NoAccidental &&  keyIsSharp)) {
                     height = C;
                     outputAccidental = (keyHasSharpC) ?  NoAccidental : Sharp;
                 } else if (inputAccidental == Flat ||                 // Db
                           (inputAccidental == NoAccidental && keyIsFlat)) {
                     height = D;
                     outputAccidental = (keyHasFlatD) ? NoAccidental : Flat;
                 } else if (inputAccidental == DoubleSharp) {          // Bx
                    height = B;
                    octave--;
                    outputAccidental = DoubleSharp;
                 }
                 break;
        case 2 :
                 if (inputAccidental == DoubleSharp) {                  // Cx
                     height = C;
                     outputAccidental = DoubleSharp;
                 } else if (inputAccidental == DoubleFlat) {            // Ebb
                     height = E;
                     outputAccidental = DoubleFlat;
                 } else {                                              // D or D-Natural
                     height = D;
                     outputAccidental = (keyHasSharpD || keyHasFlatD) ? Natural : NoAccidental;
                 }
                 break;
        case 3 :
                 if (inputAccidental == Sharp ||                        // D#
                    (inputAccidental == NoAccidental &&  keyIsSharp)) {
                     height = D;
                     outputAccidental = (keyHasSharpD) ? NoAccidental : Sharp;
                 } else if (inputAccidental == Flat ||                  // Eb
                           (inputAccidental == NoAccidental &&  keyIsFlat)) {
                     height = E;
                     outputAccidental = (keyHasFlatE) ? NoAccidental : Flat;
                 } else if (inputAccidental == DoubleFlat) {            // Fbb
                     height = F;
                     outputAccidental = DoubleFlat;
                 }
                 break;
        case 4 :
                 if (inputAccidental == Flat ||                         // Fb
                    (inputAccidental == NoAccidental && keyHasFlatF)) {
                     height = F;
                     outputAccidental = (keyHasFlatF) ? NoAccidental : Flat;
                 } else if (inputAccidental == DoubleSharp) {           // Dx
                     height = D;
                     outputAccidental = DoubleSharp;
                 } else {                                              // E or E-Natural
                     height = E;
                     outputAccidental = (keyHasSharpE || keyHasFlatE ||
                                   (keyHasFlatF && inputAccidental==Natural)) ?
                                    Natural : NoAccidental;
                 }
                 break;
        case 5 :
                 if (inputAccidental == Sharp ||                        // E#
                    (inputAccidental == NoAccidental && keyHasSharpE)) {
                     height = E;
                     outputAccidental = (keyHasSharpE) ? NoAccidental : Sharp;
                 } else if (inputAccidental == DoubleFlat) {            // Gbb
                     height = G;
                     outputAccidental = DoubleFlat;
                 } else {                                              // F or F-Natural
                     height = F;
                     outputAccidental = (keyHasSharpF || keyHasFlatF ||
                                   (keyHasSharpE && inputAccidental==Natural))?
                                    Natural : NoAccidental;
                 }
                 break;
        case 6 :
                 if (inputAccidental == Sharp ||
                    (inputAccidental == NoAccidental && keyIsSharp)) {  // F#
                     height = F;
                     outputAccidental = (keyHasSharpF) ? NoAccidental : Sharp;
                 } else if (inputAccidental == Flat ||                  // Gb
                           (inputAccidental == NoAccidental && keyIsFlat)) {
                     height = G;
                     outputAccidental = (keyHasFlatG) ? NoAccidental : Flat;
                 } else if (inputAccidental == DoubleSharp) {           // Ex
                     height = E;
                     outputAccidental = DoubleSharp;
                 }
                 break;
        case 7 :
                 if (inputAccidental == DoubleSharp) {                  // Fx
                     height = F;
                     outputAccidental = DoubleSharp;
                 } else if (inputAccidental == DoubleFlat) {            // Abb
                     height = A;
                     outputAccidental = DoubleFlat;
                 } else {                                              // G or G-Natural
                     height = G;
                     outputAccidental = (keyHasSharpG || keyHasFlatG) ? Natural : NoAccidental;
                 }
                 break;
        case 8 :
                 if (inputAccidental == Sharp ||
                    (inputAccidental == NoAccidental && keyIsSharp)) {  // G#
                     height = G;
                     outputAccidental = (keyHasSharpG) ? NoAccidental : Sharp;
                 } else if (inputAccidental == Flat ||                  // Ab
                           (inputAccidental == NoAccidental && keyIsFlat)) {
                     height = A;
                     outputAccidental = (keyHasFlatA) ? NoAccidental : Flat;
                 }
                 break;
        case 9 :
                 if (inputAccidental == DoubleSharp) {                  // Gx
                     height = G;
                     outputAccidental = DoubleSharp;
                 } else if (inputAccidental == DoubleFlat) {            // Bbb
                     height = B;
                     outputAccidental = DoubleFlat;
                 } else {                                              // A or A-Natural
                     height = A;
                     outputAccidental = (keyHasSharpA || keyHasFlatA) ? Natural : NoAccidental;
                 }
                 break;
        case 10:
                 if (inputAccidental == DoubleFlat) {                   // Cbb
                     height = C;
                     octave++;  // tweak B/C divide
                     outputAccidental = DoubleFlat;
                 } else if (inputAccidental == Sharp ||                 // A#
                           (inputAccidental == NoAccidental && keyIsSharp)) {
                     height = A;
                     outputAccidental = (keyHasSharpA) ? NoAccidental : Sharp;
                 } else if (inputAccidental == Flat ||                  // Bb
                           (inputAccidental == NoAccidental && keyIsFlat)) {
                     height = B;
                     outputAccidental = (keyHasFlatB) ? NoAccidental : Flat;
                 }
                 break;
        case 11:
                 if (inputAccidental == DoubleSharp) {                  // Ax
                     height = A;
                     outputAccidental = DoubleSharp;
                 } else if (inputAccidental == Flat ||                  // Cb
                           (inputAccidental == NoAccidental && keyHasFlatC)) {
                     height = C;
                     octave++;  // tweak B/C divide
                     outputAccidental = (keyHasFlatC) ? NoAccidental : Flat;
                 } else {                                             // B or B-Natural
                     height = B;
                     outputAccidental = (keyHasSharpB || keyHasFlatB ||
                                   (keyHasFlatC && inputAccidental==Natural)) ?
                                    Natural : NoAccidental;
                 }
    }

    if (outputAccidental == NoAccidental && inputAccidental == Natural) {
        outputAccidental = Natural;
    }

}

bool
Pitch::validAccidental() const
{
//      std::cout << "Checking whether accidental is valid " << std::endl;
        if (m_accidental == NoAccidental)
        {
                return true;
        }
        int naturalPitch = (m_pitch -
                Accidentals::getPitchOffset(m_accidental) + 12) % 12;
        switch(naturalPitch)
        {
                case 0: //C
                        return true;
                case 1:
                        return false;
                case 2: //D
                        return true;
                case 3:
                        return false;
                case 4: //E
                        return true;
                case 5: //F
                        return true;
                case 6:
                        return false;
                case 7: //G
                        return true;
                case 8:
                        return false;
                case 9: //A
                        return true;
                case 10:
                        return false;
                case 11: //B
                        return true;
        };
        std::cout << "Internal error in validAccidental" << std::endl;
        return false;
}

Event *
Pitch::getAsNoteEvent(timeT absoluteTime, timeT duration) const
{
    Event *e = new Event(Note::EventType, absoluteTime, duration);
    e->set<Int>(BaseProperties::PITCH, m_pitch);
    e->set<String>(BaseProperties::ACCIDENTAL, m_accidental);
    return e;
}

/**
 * Converts performance pitch to height on staff + correct accidentals
 * for current key.
 *
 * This method takes a Clef, Key, Accidental and raw performance pitch, then
 * applies this information to return a height on staff value and an
 * accidental state.  The pitch itself contains a lot of information, but we
 * need to use the Key and user-specified Accidental to make an accurate
 * decision just where to put it on the staff, and what accidental it should
 * display for (or against) the key.
 *
 * This function originally written by Chris Cannam for X11 Rosegarden
 * Entirely rewritten by Chris Cannam for Rosegarden 4
 * Entirely rewritten by Hans Kieserman
 * Entirely rewritten by Michael McIntyre
 * This version by Michael McIntyre <dmmcintyr@users.sourceforge.net>
 * Resolving the accidental was refactored out by Arnout Engelen
 */
void
Pitch::rawPitchToDisplayPitch(int rawpitch,
                              const Clef &clef,
                              const Key &key,
                              int &height,
                              Accidental &accidental,
                              NoAccidentalStrategy noAccidentalStrategy)
{

    // 1. Calculate the octave (for later):
    int octave = rawpitch / 12;

    // 2. Set initial height to 0
    height = 0;

    // 3.  Calculate raw semitone number, yielding a value between 0 (C) and
    // 11 (B)
    int pitch  = rawpitch % 12;

    // clear the in-coming accidental so we can trap any failure to re-set
    // it on the way out:
    Accidental userAccidental = accidental;
    accidental = "";

    if (userAccidental == NoAccidental || !Pitch(rawpitch, userAccidental).validAccidental())
    {
        userAccidental = resolveNoAccidental(pitch, key, noAccidentalStrategy);
        //std::cout << "Chose accidental " << userAccidental << " for pitch " << pitch <<
        //      " in key " << key.getName() << std::endl;
    }
    //else
    //{
    //  std::cout << "Accidental was specified, as " << userAccidental << std::endl;
    //}

    resolveSpecifiedAccidental(pitch, clef, key, height, octave, userAccidental, accidental);

    // Failsafe...  If this ever executes, there's trouble to fix...
// WIP - DMM - munged up to explore #937389, which is temporarily deferred,
// owing to its non-critical nature, having been hacked around in the LilyPond
// code
#ifndef DEBUG_PITCH
    if (accidental == "") {
        std::cerr << "Pitch::rawPitchToDisplayPitch(): error! returning null accidental for:"
#else
        std::cerr << "Pitch::rawPitchToDisplayPitch(): calculating: "
#endif
                  << std::endl << "pitch: " << rawpitch << " (" << pitch << " in oct "
                  << octave << ")  userAcc: " << userAccidental
                  << "  clef: " << clef.getClefType() << "  key: " << key.getName() << std::endl;
#ifndef DEBUG_PITCH
    }
#endif


    // 6.  "Recenter" height in case it's been changed:
    height = ((height + 2) % 7) - 2;

    height += (octave - 5) * 7;
    height += clef.getPitchOffset();


    // 7. Transpose up or down for the clef:
    height -= 7 * clef.getOctave();
}

void
Pitch::displayPitchToRawPitch(int height,
                              Accidental accidental,
                              const Clef &clef,
                              const Key &key,
                              int &pitch,
                              bool ignoreOffset)
{
    int octave = 5;

    // 1. Ask Key for accidental if necessary
    if (accidental == NoAccidental) {
        accidental = key.getAccidentalAtHeight(height, clef);
    }

    // 2. Get pitch and correct octave

    if (!ignoreOffset) height -= clef.getPitchOffset();

    while (height < 0) { octave -= 1; height += 7; }
    while (height >= 7) { octave += 1; height -= 7; }

    if (height > 4) ++octave;

    // Height is now relative to treble clef lines
    switch (height) {

    case 0: pitch =  4; break;  /* bottom line, treble clef: E */
    case 1: pitch =  5; break;  /* F */
    case 2: pitch =  7; break;  /* G */
    case 3: pitch =  9; break;  /* A, in next octave */
    case 4: pitch = 11; break;  /* B, likewise*/
    case 5: pitch =  0; break;  /* C, moved up an octave (see above) */
    case 6: pitch =  2; break;  /* D, likewise */
    }
    // Pitch is now "natural"-ized note at given height

    // 3. Adjust pitch for accidental

    if (accidental != NoAccidental &&
        accidental != Natural) {
        if (accidental == Sharp) { pitch++; }
        else if (accidental == Flat) { pitch--; }
        else if (accidental == DoubleSharp) { pitch += 2; }
        else if (accidental == DoubleFlat) { pitch -= 2; }
    }

    // 4. Adjust for clef
    octave += clef.getOctave();

    pitch += 12 * octave;
}



Pitch::Pitch(const Event &e) :
    // throw (Event::NoData)
    m_accidental(NoAccidental)
{
    m_pitch = e.get<Int>(BaseProperties::PITCH);
    e.get<String>(BaseProperties::ACCIDENTAL, m_accidental);
}

Pitch::Pitch(int performancePitch, const Accidental &explicitAccidental) :
    m_pitch(performancePitch),
    m_accidental(explicitAccidental)
{
    // nothing
}

Pitch::Pitch(int pitchInOctave, int octave,
             const Accidental &explicitAccidental, int octaveBase) :
    m_pitch((octave - octaveBase) * 12 + pitchInOctave),
    m_accidental(explicitAccidental)
{
    // nothing else
}

Pitch::Pitch(int noteInScale, int octave, const Key &key,
             const Accidental &explicitAccidental, int octaveBase) :
    m_pitch(0),
    m_accidental(explicitAccidental)
{
    m_pitch = (key.getTonicPitch());
    m_pitch = (octave - octaveBase) * 12 + m_pitch % 12;

    if (key.isMinor()) m_pitch += scale_Cminor_harmonic[noteInScale];
    else m_pitch += scale_Cmajor[noteInScale];

    m_pitch += Accidentals::getPitchOffset(m_accidental);
}

Pitch::Pitch(int noteInCMajor, int octave, int pitch,
             int octaveBase) :
    m_pitch(pitch)
{
    int natural = (octave - octaveBase) * 12 + scale_Cmajor[noteInCMajor];
    m_accidental = Accidentals::getAccidental(pitch - natural);
}


Pitch::Pitch(char noteName, int octave, const Key &key,
             const Accidental &explicitAccidental, int octaveBase) :
    m_pitch(0),
    m_accidental(explicitAccidental)
{
    int height = getIndexForNote(noteName) - 2;
    displayPitchToRawPitch(height, explicitAccidental,
                           Clef(), key, m_pitch);

    // we now have the pitch within octave 5 (C == 60) -- though it
    // might have spilled over at either end
    if (m_pitch < 60) --octave;
    if (m_pitch > 71) ++octave;
    m_pitch = (octave - octaveBase) * 12 + m_pitch % 12;
}

Pitch::Pitch(int heightOnStaff, const Clef &clef, const Key &key,
             const Accidental &explicitAccidental) :
    m_pitch(0),
    m_accidental(explicitAccidental)
{
    displayPitchToRawPitch
        (heightOnStaff, explicitAccidental, clef, key, m_pitch);
}

Pitch::Pitch(const Pitch &p) :
    m_pitch(p.m_pitch),
    m_accidental(p.m_accidental)
{
    // nothing else
}

Pitch &
Pitch::operator=(const Pitch &p)
{
    if (&p != this) {
        m_pitch = p.m_pitch;
        m_accidental = p.m_accidental;
    }
    return *this;
}

int
Pitch::getPerformancePitch() const
{
    return m_pitch;
}

Accidental
Pitch::getAccidental(bool useSharps) const
{
    return getDisplayAccidental(Key("C major"),
                useSharps ? UseSharps : UseFlats);
}

Accidental
Pitch::getAccidental(const Key &key) const
{
    if (m_accidental == NoAccidental || !validAccidental())
    {
        Accidental retval = resolveNoAccidental(m_pitch, key, UseKey);
        //std::cout << "Resolved No/invalid accidental: chose " << retval << std::endl;
        return retval;
    }
    else
    {
        //std::cout << "Returning specified accidental" << std::endl;
        return m_accidental;
    }
}

Accidental
Pitch::getDisplayAccidental(const Key &key) const
{
    return getDisplayAccidental(key, UseKey);
}

Accidental
Pitch::getDisplayAccidental(const Key &key, NoAccidentalStrategy noAccidentalStrategy) const
{
    int heightOnStaff;
    Accidental accidental(m_accidental);
    rawPitchToDisplayPitch(m_pitch, Clef(), key, heightOnStaff, accidental, noAccidentalStrategy);
    return accidental;
}

int
Pitch::getNoteInScale(const Key &key) const
{
    int p = m_pitch;
    p -= key.getTonicPitch();
    p -= Accidentals::getPitchOffset(getDisplayAccidental(key));
    p += 24; // in case these calculations made it -ve
    p %= 12;

    if (key.isMinor()) return steps_Cminor_harmonic[p];
    else return steps_Cmajor[p];
}

char
Pitch::getNoteName(const Key &key) const
{
    int index = (getHeightOnStaff(Clef(Clef::Treble), key) + 72) % 7;
    return getNoteForIndex(index);
}

int
Pitch::getHeightOnStaff(const Clef &clef, const Key &key) const
{
    int heightOnStaff;
    Accidental accidental(m_accidental);
    rawPitchToDisplayPitch(m_pitch, clef, key, heightOnStaff, accidental, UseKey);
    return heightOnStaff;
}

int
Pitch::getHeightOnStaff(const Clef &clef, bool useSharps) const
{
    int heightOnStaff;
    Accidental accidental(m_accidental);
    rawPitchToDisplayPitch(m_pitch, clef, Key("C major"), heightOnStaff, accidental,
        useSharps ? UseSharps : UseFlats);
    return heightOnStaff;
}

int
Pitch::getOctave(int octaveBase) const
{
    return m_pitch / 12 + octaveBase;
}

int
Pitch::getOctaveAccidental(int octaveBase, Accidental acc) const
{
    int t_pitch = m_pitch;
    if (acc == Accidentals::DoubleFlat) {
        t_pitch += 2;
    } else if (acc == Accidentals::Flat) {
        t_pitch += 1;
    } else if (acc == Accidentals::Sharp) {
        t_pitch -= 1;
    } else if (acc == Accidentals::DoubleSharp) {
        t_pitch -= 2;
    }
    return t_pitch / 12 + octaveBase;
}

int
Pitch::getPitchInOctave() const
{
    return m_pitch % 12;
}

bool
Pitch::isDiatonicInKey(const Key &key) const
{
    if (getDisplayAccidental(key) == Accidentals::NoAccidental) return true;

    // ### as used in the chord identifiers, this calls chords built on
    //     the raised sixth step diatonic -- may be correct, but it's
    //     misleading, as we're really looking for whether chords are
    //     often built on given tone

    if (key.isMinor()) {
        int stepsFromTonic = ((m_pitch - key.getTonicPitch() + 12) % 12);
        if (stepsFromTonic == 9 || stepsFromTonic == 11) return true;
    }

    return false;
}

std::string
Pitch::getAsString(bool inclOctave, int octaveBase) const
{
    std::string s;
    s += getNoteName(Key("C major"));

    Accidental acc = getAccidental(Key("C major"));

    if (acc == Accidentals::Sharp) s += "#";
    else if (acc == Accidentals::Flat) s += "b";

    if (!inclOctave) return s;

    char tmp[10];
    sprintf(tmp, "%s%d", s.c_str(), getOctave(octaveBase));
    return std::string(tmp);
}

int
Pitch::getIndexForNote(char noteName)
{
    if (islower(noteName)) noteName = toupper(noteName);
    if (noteName < 'C') {
        if (noteName < 'A') return 0; // error, really
        else return noteName - 'A' + 5;
    } else {
        if (noteName > 'G') return 0; // error, really
        else return noteName - 'C';
    }
}

char
Pitch::getNoteForIndex(int index)
{
    if (index < 0 || index > 6) return 'C'; // error, really
    return "CDEFGAB"[index];
}

int
Pitch::getPerformancePitchFromRG21Pitch(int heightOnStaff,
                                        const Accidental &accidental,
                                        const Clef &clef,
                                        const Key &)
{
    // X11 Rosegarden pitches are a bit weird; see
    // docs/data_struct/units.txt

    // We pass the accidental and clef, a faked key of C major, and a
    // flag telling displayPitchToRawPitch to ignore the clef offset
    // and take only its octave into account

    int p = 0;
    displayPitchToRawPitch(heightOnStaff, accidental, clef, Key(), p, true);
    return p;
}

Pitch Pitch::transpose(const Key &key, int pitchDelta, int heightDelta)
{
    // get old accidental
    Accidental oldAccidental = getAccidental(key);

    // get old step
    // TODO: maybe we should write an oldPitchObj.getOctave(0, key) that takes into account accidentals
    //  properly (e.g. yielding '0' instead of '1' for B#0). For now workaround here.
    Pitch oldPitchWithoutAccidental(getPerformancePitch() - Accidentals::getPitchOffset(oldAccidental), Natural);
    Key cmaj = Key();
    int oldStep = oldPitchWithoutAccidental.getNoteInScale(cmaj) + oldPitchWithoutAccidental.getOctave(0) * 7;

    // calculate new pitch and step
    int newPitch = getPerformancePitch() + pitchDelta;
    int newStep  = oldStep  + heightDelta;

    // could happen for example when transposing the tonic of a key downwards
    if (newStep < 0 || newPitch < 0) {
        newStep += 7;
        newPitch += 12;
    }

    // should not happen
    if (newStep < 0 || newPitch < 0) {
        std::cerr << "Internal error in NotationTypes, Pitch::transpose()"
            << std::endl;
    }

    // calculate new accidental for step
    int pitchWithoutAccidental = ((newStep / 7) * 12 + scale_Cmajor[newStep % 7]);
    int newAccidentalOffset = newPitch - pitchWithoutAccidental;

    // construct pitch-object to return
    Pitch newPitchObj(newPitch, Accidentals::getAccidental(newAccidentalOffset));
    return newPitchObj;
}

//////////////////////////////////////////////////////////////////////
// Note
//////////////////////////////////////////////////////////////////////

const string Note::EventType = "note";
const string Note::EventRestType = "rest";
const int Note::EventRestSubOrdering = 10;

const timeT Note::m_shortestTime = basePPQ / 16;

Note& Note::operator=(const Note &n)
{
    if (&n == this) return *this;
    m_type = n.m_type;
    m_dots = n.m_dots;
    return *this;
}

timeT Note::getDurationAux() const
{
    int duration = m_shortestTime * (1 << m_type);
    int extra = duration / 2;
    for (int dots = m_dots; dots > 0; --dots) {
        duration += extra;
        extra /= 2;
    }
    return duration;
}


Note Note::getNearestNote(timeT duration, int maxDots)
{
    int tag = Shortest - 1;
    timeT d(duration / m_shortestTime);
    while (d > 0) { ++tag; d /= 2; }

//    cout << "Note::getNearestNote: duration " << duration <<
//      " leading to tag " << tag << endl;
    if (tag < Shortest) return Note(Shortest);
    if (tag > Longest)  return Note(Longest, maxDots);

    timeT prospective = Note(tag, 0).getDuration();
    int dots = 0;
    timeT extra = prospective / 2;

    while (dots <= maxDots &&
           dots <= tag) { // avoid TooManyDots exception from Note ctor
        prospective += extra;
        if (prospective > duration) return Note(tag, dots);
        extra /= 2;
        ++dots;
//      cout << "added another dot okay" << endl;
    }

    if (tag < Longest) return Note(tag + 1, 0);
    else return Note(tag, std::max(maxDots, tag));
}

Event *Note::getAsNoteEvent(timeT absoluteTime, int pitch) const
{
    Event *e = new Event(EventType, absoluteTime, getDuration());
    e->set<Int>(BaseProperties::PITCH, pitch);
    return e;
}

Event *Note::getAsRestEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventRestType, absoluteTime, getDuration());
    return e;
}



//////////////////////////////////////////////////////////////////////
// TimeSignature
//////////////////////////////////////////////////////////////////////

const string TimeSignature::EventType = "timesignature";
const int TimeSignature::EventSubOrdering = -150;
const PropertyName TimeSignature::NumeratorPropertyName = "numerator";
const PropertyName TimeSignature::DenominatorPropertyName = "denominator";
const PropertyName TimeSignature::ShowAsCommonTimePropertyName = "common";
const PropertyName TimeSignature::IsHiddenPropertyName = "hidden";
const PropertyName TimeSignature::HasHiddenBarsPropertyName = "hiddenbars";
const TimeSignature TimeSignature::DefaultTimeSignature = TimeSignature(4, 4);

TimeSignature::TimeSignature(int numerator, int denominator,
                             bool preferCommon, bool hidden, bool hiddenBars)
    // throw (BadTimeSignature)
    : m_numerator(numerator), m_denominator(denominator),
      m_common(preferCommon &&
               (m_denominator == m_numerator &&
                (m_numerator == 2 || m_numerator == 4))),
      m_hidden(hidden),
      m_hiddenBars(hiddenBars)
{
    if (numerator < 1 || denominator < 1) {
        throw BadTimeSignature("Numerator and denominator must be positive");
    }
}

TimeSignature::TimeSignature(const Event &e)
    // throw (Event::NoData, Event::BadType, BadTimeSignature)
{
    if (e.getType() != EventType) {
        throw Event::BadType("TimeSignature model event", EventType, e.getType());
    }
    m_numerator = 4;
    m_denominator = 4;

    if (e.has(NumeratorPropertyName)) {
        m_numerator = e.get<Int>(NumeratorPropertyName);
    }

    if (e.has(DenominatorPropertyName)) {
        m_denominator = e.get<Int>(DenominatorPropertyName);
    }

    m_common = false;
    e.get<Bool>(ShowAsCommonTimePropertyName, m_common);

    m_hidden = false;
    e.get<Bool>(IsHiddenPropertyName, m_hidden);

    m_hiddenBars = false;
    e.get<Bool>(HasHiddenBarsPropertyName, m_hiddenBars);

    if (m_numerator < 1 || m_denominator < 1) {
        throw BadTimeSignature("Numerator and denominator must be positive");
    }
}

TimeSignature& TimeSignature::operator=(const TimeSignature &ts)
{
    if (&ts == this) return *this;
    m_numerator = ts.m_numerator;
    m_denominator = ts.m_denominator;
    m_common = ts.m_common;
    m_hidden = ts.m_hidden;
    m_hiddenBars = ts.m_hiddenBars;
    return *this;
}

timeT TimeSignature::getBarDuration() const
{
    setInternalDurations();
    return m_barDuration;
}

timeT TimeSignature::getBeatDuration() const
{
    setInternalDurations();
    return m_beatDuration;
}

timeT TimeSignature::getUnitDuration() const
{
    return m_crotchetTime * 4 / m_denominator;
}

Note::Type TimeSignature::getUnit() const
{
    int c, d;
    for (c = 0, d = m_denominator; d > 1; d /= 2) ++c;
    return Note::Semibreve - c;
}

bool TimeSignature::isDotted() const
{
    setInternalDurations();
    return m_dotted;
}

Event *TimeSignature::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(NumeratorPropertyName, m_numerator);
    e->set<Int>(DenominatorPropertyName, m_denominator);
    e->set<Bool>(ShowAsCommonTimePropertyName, m_common);
    e->set<Bool>(IsHiddenPropertyName, m_hidden);
    e->set<Bool>(HasHiddenBarsPropertyName, m_hiddenBars);
    return e;
}

// This doesn't consider subdivisions of the bar larger than a beat in
// any time other than 4/4, but it should handle the usual time signatures
// correctly (compound time included).

void TimeSignature::getDurationListForInterval(DurationList &dlist,
                                               timeT duration,
                                               timeT startOffset) const
{
    setInternalDurations();

    timeT offset = startOffset;
    timeT durationRemaining = duration;

    while (durationRemaining > 0) {

        // Everything in this loop is of the form, "if we're on a
        // [unit] boundary and there's a [unit] of space left to fill,
        // insert a [unit] of time."

        // See if we can insert a bar of time.

        if (offset % m_barDuration == 0
            && durationRemaining >= m_barDuration) {

            getDurationListForBar(dlist);
            durationRemaining -= m_barDuration,
                offset += m_barDuration;

        }

        // If that fails and we're in 4/4 time, see if we can insert a
        // half-bar of time.

        //_else_ if!
        else if (m_numerator == 4 && m_denominator == 4
                 && offset % (m_barDuration/2) == 0
                 && durationRemaining >= m_barDuration/2) {

            dlist.push_back(m_barDuration/2);
            durationRemaining -= m_barDuration/2;
            offset += m_barDuration/2;

        }

        // If that fails, see if we can insert a beat of time.

        else if (offset % m_beatDuration == 0
                 && durationRemaining >= m_beatDuration) {

            dlist.push_back(m_beatDuration);
            durationRemaining -= m_beatDuration;
            offset += m_beatDuration;

        }

        // If that fails, see if we can insert a beat-division of time
        // (half the beat in simple time, a third of the beat in compound
        // time)

        else if (offset % m_beatDivisionDuration == 0
                 && durationRemaining >= m_beatDivisionDuration) {

            dlist.push_back(m_beatDivisionDuration);
            durationRemaining -= m_beatDivisionDuration;
            offset += m_beatDivisionDuration;

        }

        // cc: In practice, if the time we have remaining is shorter
        // than our shortest note then we should just insert a single
        // unit of the correct time; we won't be able to do anything
        // useful with any shorter units anyway.

        else if (durationRemaining <= Note(Note::Shortest).getDuration()) {

            dlist.push_back(durationRemaining);
            offset += durationRemaining;
            durationRemaining = 0;

        }

        // If that fails, keep halving the beat division until we
        // find something to insert. (This could be part of the beat-division
        // case; it's only in its own place for clarity.)

        else {

            timeT currentDuration = m_beatDivisionDuration;

            while ( !(offset % currentDuration == 0
                      && durationRemaining >= currentDuration) ) {

                if (currentDuration <= Note(Note::Shortest).getDuration()) {

                    // okay, this isn't working.  If our duration takes
                    // us past the next beat boundary, fill with an exact
                    // rest duration to there and then continue  --cc

                    timeT toNextBeat =
                        m_beatDuration - (offset % m_beatDuration);

                    if (durationRemaining > toNextBeat) {
                        currentDuration = toNextBeat;
                    } else {
                        currentDuration  = durationRemaining;
                    }
                    break;
                }

                currentDuration /= 2;
            }

            dlist.push_back(currentDuration);
            durationRemaining -= currentDuration;
            offset += currentDuration;

        }

    }

}

void TimeSignature::getDurationListForBar(DurationList &dlist) const
{

    // If the bar's length can be represented with one long symbol, do it.
    // Otherwise, represent it as individual beats.

    if (m_barDuration == m_crotchetTime ||
        m_barDuration == m_crotchetTime * 2 ||
        m_barDuration == m_crotchetTime * 4 ||
        m_barDuration == m_crotchetTime * 8 ||
        m_barDuration == m_dottedCrotchetTime ||
        m_barDuration == m_dottedCrotchetTime * 2 ||
        m_barDuration == m_dottedCrotchetTime * 4 ||
        m_barDuration == m_dottedCrotchetTime * 8) {

        dlist.push_back(getBarDuration());

    } else {

        for (int i = 0; i < getBeatsPerBar(); ++i) {
            dlist.push_back(getBeatDuration());
        }

    }

}

int TimeSignature::getEmphasisForTime(timeT offset)
{
    setInternalDurations();

    if      (offset % m_barDuration == 0)
        return 4;
    else if (m_numerator == 4 && m_denominator == 4 &&
             offset % (m_barDuration/2) == 0)
        return 3;
    else if (offset % m_beatDuration == 0)
        return 2;
    else if (offset % m_beatDivisionDuration == 0)
        return 1;
    else
        return 0;
}


void TimeSignature::getDivisions(int depth, std::vector<int> &divisions) const
{
    divisions.clear();

    if (depth <= 0) return;
    timeT base = getBarDuration(); // calls setInternalDurations
/*
    if (m_numerator == 4 && m_denominator == 4) {
        divisions.push_back(2);
        base /= 2;
        --depth;
    }
*/
    if (depth <= 0) return;

    divisions.push_back(base / m_beatDuration);
    base = m_beatDuration;
    --depth;

    if (depth <= 0) return;

    if (m_dotted) divisions.push_back(3);
    else divisions.push_back(2);
    --depth;

    while (depth > 0) {
        divisions.push_back(2);
        --depth;
    }

    return;
}


void TimeSignature::setInternalDurations() const
{
    int unitLength = m_crotchetTime * 4 / m_denominator;

    m_barDuration = m_numerator * unitLength;

    // Is 3/8 dotted time?  This will report that it isn't, because of
    // the check for m_numerator > 3 -- but otherwise we'd get a false
    // positive with 3/4

    // [rf] That's an acceptable answer, according to my theory book. In
    // practice, you can say it's dotted time iff it has 6, 9, or 12 on top.

    m_dotted = (m_numerator % 3 == 0 &&
                m_numerator > 3 &&
                m_barDuration >= m_dottedCrotchetTime);

    if (m_dotted) {
        m_beatDuration = unitLength * 3;
        m_beatDivisionDuration = unitLength;
    }
    else {
        m_beatDuration = unitLength;
        m_beatDivisionDuration = unitLength / 2;
    }

}

const timeT TimeSignature::m_crotchetTime       = basePPQ;
const timeT TimeSignature::m_dottedCrotchetTime = basePPQ + basePPQ/2;



//////////////////////////////////////////////////////////////////////
// AccidentalTable
//////////////////////////////////////////////////////////////////////

AccidentalTable::AccidentalTable(const Key &key, const Clef &clef,
                                 OctaveType octaves, BarResetType barReset) :
    m_key(key), m_clef(clef),
    m_octaves(octaves), m_barReset(barReset)
{
    // nothing else
}

AccidentalTable::AccidentalTable(const AccidentalTable &t) :
    m_key(t.m_key), m_clef(t.m_clef),
    m_octaves(t.m_octaves), m_barReset(t.m_barReset),
    m_accidentals(t.m_accidentals),
    m_canonicalAccidentals(t.m_canonicalAccidentals),
    m_newAccidentals(t.m_newAccidentals),
    m_newCanonicalAccidentals(t.m_newCanonicalAccidentals)
{
    // nothing else
}

AccidentalTable &
AccidentalTable::operator=(const AccidentalTable &t)
{
    if (&t != this) {
        m_key = t.m_key;
        m_clef = t.m_clef;
        m_octaves = t.m_octaves;
        m_barReset = t.m_barReset;
        m_accidentals = t.m_accidentals;
        m_canonicalAccidentals = t.m_canonicalAccidentals;
        m_newAccidentals = t.m_newAccidentals;
        m_newCanonicalAccidentals = t.m_newCanonicalAccidentals;
    }
    return *this;
}

Accidental
AccidentalTable::processDisplayAccidental(const Accidental &acc0, int height,
                                          bool &cautionary)
{
    Accidental acc = acc0;

    int canonicalHeight = Key::canonicalHeight(height);
    Accidental keyAcc = m_key.getAccidentalAtHeight(canonicalHeight, m_clef);

    Accidental normalAcc = NoAccidental;
    Accidental canonicalAcc = NoAccidental;
    Accidental prevBarAcc = NoAccidental;

    if (m_octaves == OctavesEquivalent ||
        m_octaves == OctavesCautionary) {

        AccidentalMap::iterator i = m_canonicalAccidentals.find(canonicalHeight);
        if (i != m_canonicalAccidentals.end() && !i->second.previousBar) {
            canonicalAcc = i->second.accidental;
        }
    }

    if (m_octaves == OctavesEquivalent) {
        normalAcc = canonicalAcc;
    } else {
        AccidentalMap::iterator i = m_accidentals.find(height);
        if (i != m_accidentals.end() && !i->second.previousBar) {
            normalAcc = i->second.accidental;
        }
    }

    if (m_barReset != BarResetNone) {
        AccidentalMap::iterator i = m_accidentals.find(height);
        if (i != m_accidentals.end() && i->second.previousBar) {
            prevBarAcc = i->second.accidental;
        }
    }

//    std::cerr << "AccidentalTable::processDisplayAccidental: acc " << acc0 << ", h " << height << ", caut " << cautionary << ", ch " << canonicalHeight << ", keyacc " << keyAcc << " canacc " << canonicalAcc << " noracc " << normalAcc << " oct " << m_octaves << " barReset = " << m_barReset << " pbacc " << prevBarAcc << std::endl;

    if (acc == NoAccidental) acc = keyAcc;

    if (m_octaves == OctavesIndependent ||
        m_octaves == OctavesEquivalent) {

        if (normalAcc == NoAccidental) {
            normalAcc = keyAcc;
        }

        if (acc == normalAcc) {
            if (!cautionary) acc = NoAccidental;
        } else if (acc == NoAccidental) {
            if (normalAcc != Natural) {
                acc = Natural;
            }
        }

    } else {

        if (normalAcc != NoAccidental) {
            if (acc != normalAcc) {
                if (acc == NoAccidental) {
                    if (normalAcc != Natural) {
                        acc = Natural;
                    }
                }
            } else { // normalAcc != NoAccidental, acc == normalAcc
                if (canonicalAcc != NoAccidental && canonicalAcc != normalAcc) {
                    cautionary = true;
                } else { // canonicalAcc == NoAccidental || canonicalAcc == normalAcc
                    if (!cautionary) {
                        acc = NoAccidental;
                    }
                }
            }
        } else { // normalAcc == NoAccidental
            if (acc != keyAcc && keyAcc != Natural) {
                if (acc == NoAccidental) {
                    acc = Natural;
                }
            } else { // normalAcc == NoAccidental, acc == keyAcc
                if (canonicalAcc != NoAccidental && canonicalAcc != keyAcc) {
                    cautionary = true;
                    if (acc == NoAccidental) {
                        acc = Natural;
                    }
                } else { // canonicalAcc == NoAccidental || canonicalAcc == keyAcc
                    if (!cautionary) {
                        acc = NoAccidental;
                    }
                }
            }
        }
    }

    if (m_barReset != BarResetNone) {
        if (acc == NoAccidental) {
            if (prevBarAcc != NoAccidental &&
                prevBarAcc != keyAcc &&
                !(prevBarAcc == Natural && keyAcc == NoAccidental)) {
                cautionary = (m_barReset == BarResetCautionary);
                if (keyAcc == NoAccidental) {
                    acc = Natural;
                } else {
                    acc = keyAcc;
                }
            }
        }
    }

    if (acc != NoAccidental) {
        m_newAccidentals[height] = AccidentalRec(acc, false);
        m_newCanonicalAccidentals[canonicalHeight] = AccidentalRec(acc, false);
    }

    return acc;
}

void
AccidentalTable::update()
{
    m_accidentals = m_newAccidentals;
    m_canonicalAccidentals = m_newCanonicalAccidentals;
}

void
AccidentalTable::newBar()
{
    for (AccidentalMap::iterator i = m_accidentals.begin();
         i != m_accidentals.end(); ) {

        if (i->second.previousBar) {
            AccidentalMap::iterator j = i;
            ++j;
            m_accidentals.erase(i);
            i = j;
        } else {
            i->second.previousBar = true;
            ++i;
        }
    }

    m_canonicalAccidentals.clear();

    m_newAccidentals = m_accidentals;
    m_newCanonicalAccidentals.clear();
}

void
AccidentalTable::newClef(const Clef &clef)
{
    m_clef = clef;
}


//////////////////////////////////////////////////////////////////////
// Symbol
//////////////////////////////////////////////////////////////////////

const std::string Symbol::EventType = "symbol";
const int Symbol::EventSubOrdering = -70;
const PropertyName Symbol::SymbolTypePropertyName = "type";

// symbol styles
const std::string Symbol::UnspecifiedType   = "unspecified";
const std::string Symbol::Segno             = "segno";
const std::string Symbol::Coda              = "coda";
const std::string Symbol::Breath            = "breath";


Symbol::Symbol(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("Symbol model event", EventType, e.getType());
    }

    m_type = Symbol::UnspecifiedType;

    e.get<String>(SymbolTypePropertyName, m_type);
}

Symbol::Symbol(const std::string &type) :
    m_type(type)
{
    // nothing else
}

Symbol::Symbol(const Symbol &t) :
    m_type(t.m_type)
{
    // nothing else
}

Symbol &
Symbol::operator=(const Symbol &t)
{
    if (&t != this) {
        m_type = t.m_type;
    }
    return *this;
}

Symbol::~Symbol()
{
    // nothing
}

bool
Symbol::isSymbolOfType(Event *e, std::string type)
{
    return (e->isa(EventType) &&
            e->has(SymbolTypePropertyName) &&
            e->get<String>(SymbolTypePropertyName) == type);
}

Event *
Symbol::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(SymbolTypePropertyName, m_type);
    return e;
}

} // end namespace Rosegarden
