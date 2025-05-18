/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NotationTypes.h"

#include "BaseProperties.h"
#include "NotationRules.h"
#include "Pitch.h"

#include <iostream>
#include <cstdlib> // for atoi
#include <limits.h> // for SHRT_MIN
#include <sstream>
#include <cstdio> // needed for sprintf()


namespace Rosegarden
{

using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;

// This is the fundamental definition of the resolution used throughout.
// It must be a multiple of 16, and should ideally be a multiple of 96.
// ??? See "timebase" in TimeT.h.  Probably should switch to that everywhere.
static const timeT basePPQ = 960;

const int MIN_SUBORDERING = SHRT_MIN;

namespace Accidentals
{
    /**
     * NoAccidental means the accidental will be inferred
     * based on the performance pitch and current key at the
     * location of the note.
     */
    const Accidental NoAccidental = "no-accidental";

    const Accidental Sharp = "sharp";
    const Accidental Flat = "flat";
    const Accidental Natural = "natural";
    const Accidental DoubleSharp = "double-sharp";
    const Accidental DoubleFlat = "double-flat";

    // Additional Accidentals for demi- and sesqui- sharps and flats
    const Accidental QuarterFlat = "demiflat";
    const Accidental ThreeQuarterFlat =  "sesqiflat";
    const Accidental QuarterSharp = "demisharp";
    const Accidental ThreeQuarterSharp = "sesquisharp";

    /* unused
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
    */

    ROSEGARDENPRIVATE_EXPORT int getPitchOffset(const Accidental &accidental) {
        if (accidental == DoubleSharp) return 2;
        else if (accidental == Sharp) return 1;
        else if (accidental == Flat) return -1;
        else if (accidental == DoubleFlat) return -2;
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
    const Mark NoMark = "no-mark";
    const Mark Accent = "accent";
    const Mark Tenuto = "tenuto";
    const Mark Staccato = "staccato";
    const Mark Staccatissimo = "staccatissimo";
    const Mark Marcato = "marcato";
    const Mark Open = "open";
    const Mark Stopped = "stopped";
    const Mark Harmonic = "harmonic";
    const Mark Sforzando = getTextMark("sf");
    const Mark Rinforzando = getTextMark("rf");
    const Mark Trill = "trill";
    const Mark LongTrill = "long-trill";
    const Mark TrillLine = "trill-line";
    const Mark Turn = "turn";
    const Mark Pause = "pause";
    const Mark UpBow = "up-bow";
    const Mark DownBow = "down-bow";

    const Mark Mordent = "mordent";
    const Mark MordentInverted = "mordent-inverted";
    const Mark MordentLong = "mordent-long";
    const Mark MordentLongInverted = "mordent-long-inverted";

    ROSEGARDENPRIVATE_EXPORT string getTextMark(const string& text) {
        return string("text_") + text;
    }

    ROSEGARDENPRIVATE_EXPORT bool isTextMark(Mark mark) {
        return string(mark).substr(0, 5) == "text_";
    }

    ROSEGARDENPRIVATE_EXPORT string getTextFromMark(Mark mark) {
        if (!isTextMark(mark)) return string();
        else return string(mark).substr(5);
    }

    ROSEGARDENPRIVATE_EXPORT string getFingeringMark(const string& fingering) {
        return string("finger_") + fingering;
    }

    ROSEGARDENPRIVATE_EXPORT bool isFingeringMark(Mark mark) {
        return string(mark).substr(0, 7) == "finger_";
    }

    ROSEGARDENPRIVATE_EXPORT bool isApplicableToRests(const Mark& mark) {
        if (mark == Marks::Pause) return true;
        if (isTextMark(mark)) return true;
        return false;
    }

    ROSEGARDENPRIVATE_EXPORT string getFingeringFromMark(Mark mark) {
        if (!isFingeringMark(mark)) return string();
        else return string(mark).substr(7);
    }

    /* unused
    ROSEGARDENPRIVATE_EXPORT int getMarkCount(const Event &e) {
        long markCount = 0;
        e.get<Int>(BaseProperties::MARK_COUNT, markCount);
        return markCount;
    }
    */

    // Marks are stored as properties named "mark1", "mark2", etc....
    // This makes working with them a bit convoluted.
    //
    // This begs the question...  Why weren't marks implemented as
    // individual properties?  E.g.:
    //
    //   // Add staccato
    //   event->set<Int>(BaseProperties::STACCATO, 1);
    //   // Remove staccato
    //   event->unset<Int>(BaseProperties::STACCATO);
    //
    // That would be so much easier.  Getting all the marks could be
    // facilitated by prefixes, e.g. "mark_staccato", "mark_tenuto"...

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

    ROSEGARDENPRIVATE_EXPORT bool removeMark(
            Event &e, const Mark &markToRemove) {

        long markCount = 0;
        e.get<Int>(BaseProperties::MARK_COUNT, markCount);

        // For each mark (mark1, mark2, etc...)...
        for (long j = 0; j < markCount; ++j) {
            PropertyName propertyName(BaseProperties::getMarkPropertyName(j));
            std::string mark;
            // If we found it...
            if (e.get<String>(propertyName, mark) && mark == markToRemove) {
                // Move all the following marks down one.
                // E.g. mark3->mark2, mark4->mark3, etc...
                while (j < markCount - 1) {
                    PropertyName nextPropertyName(
                            BaseProperties::getMarkPropertyName(j+1));
                    if (e.get<String>(nextPropertyName, mark)) {
                        e.set<String>(propertyName, mark);
                    }
                    propertyName = nextPropertyName;
                    ++j;
                }

                // Be sure to remove the last one so we don't keep duplicating
                // it.  MARK_COUNT will prevent us from seeing it, but it
                // will still be there.
                e.unset(BaseProperties::getMarkPropertyName(markCount - 1));

                // Update MARK_COUNT.
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

        static std::vector<Mark> v;
        if (v.size() == 0) {
            static Mark a[] = {
                NoMark, Accent, Tenuto, Staccato, Staccatissimo, Marcato, Open,
                Stopped, Harmonic, Sforzando, Rinforzando, Trill, LongTrill,
                TrillLine, Turn, Pause, UpBow, DownBow, Mordent,
                MordentInverted, MordentLong, MordentLongInverted
            };

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
const PropertyName Clef::ClefPropertyName("clef");
const PropertyName Clef::OctaveOffsetPropertyName("octaveoffset");
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
const PropertyName Key::KeyPropertyName("key");
const Key Key::DefaultKey = Key("C major");
const Key Key::UndefinedKey = Key("undefined");

Key::Key() :
    m_name(DefaultKey.m_name),
    m_accidentalHeights(nullptr)
{
    checkMap();
}


Key::Key(const Event &e) :
    m_name(""),
    m_accidentalHeights(nullptr)
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
    m_accidentalHeights(nullptr)
{
    if (name == "undefined") return;
    checkMap();
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
        throw BadKeyName("No such key as \"" + m_name + "\"");
    }
}

Key::Key(int accidentalCount, bool isSharp, bool isMinor) :
    m_accidentalHeights(nullptr)
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
    m_accidentalHeights(nullptr)
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
    m_accidentalHeights(nullptr)
{
}

Key& Key::operator=(const Key &kc)
{
    m_name = kc.m_name;
    m_accidentalHeights = nullptr;
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

Key Key::transpose(int pitchDelta, int heightDelta) const
{
    Pitch tonic(getTonicPitch(), getAccidentalForStep(0));
    Pitch newTonic = tonic.transpose(*this, pitchDelta, heightDelta);

    return newTonic.getAsKey();
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

Accidental Key::getAccidentalForStep(int steps) const
{
    if (isMinor()) {
        steps = (steps + 5) % 7;
    }

    int accidentalCount = getAccidentalCount();

    if (accidentalCount == 0) {
        return NoAccidental;
    }

    bool sharp = isSharp();

    int currentAccidentalPosition = sharp ? 6 : 3;

    for (int i = 1; i <= accidentalCount; i++) {
        if (steps == currentAccidentalPosition) {
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
                            const std::string& equivalence,
                            const std::string& rg2name,
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
const PropertyName Indication::IndicationTypePropertyName("indicationtype");
//const PropertyName Indication::IndicationDurationPropertyName = "indicationduration";
static const PropertyName IndicationDurationPropertyName("indicationduration"); // !!!

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
Indication::isValid(const std::string &s)
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

// Type of text, e.g. lyric, dynamic, tempo...  See Text::Lyric, etc... below
// and TextPropertyName.
const PropertyName Text::TextTypePropertyName("type");
// Text types for TextTypePropertyName.
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

// The text.
const PropertyName Text::TextPropertyName("text");

// Verse number.
const PropertyName Text::LyricVersePropertyName("verse");

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

Text::Text(const std::string &text, const std::string &textType) :
    m_text(text),
    m_type(textType),
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
Text::isTextOfType(Event *e, const std::string& type)
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

/* unused
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
*/

Event *
Text::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(TextPropertyName, m_text);
    e->set<String>(TextTypePropertyName, m_type);
    if (m_type == Lyric) e->set<Int>(LyricVersePropertyName, m_verse);
    return e;
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

/* unused
Event *Note::getAsRestEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventRestType, absoluteTime, getDuration());
    return e;
}
*/

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
AccidentalTable::processDisplayAccidental(const Accidental &displayAcc,
                                          int heightOnStaff,
                                          bool &cautionary)
{
    Accidental acc = displayAcc;

    int canonicalHeight = Key::canonicalHeight(heightOnStaff);
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
        AccidentalMap::iterator i = m_accidentals.find(heightOnStaff);
        if (i != m_accidentals.end() && !i->second.previousBar) {
            normalAcc = i->second.accidental;
        }
    }

    if (m_barReset != BarResetNone) {
        AccidentalMap::iterator i = m_accidentals.find(heightOnStaff);
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
        m_newAccidentals[heightOnStaff] = AccidentalRec(acc, false);
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
const PropertyName Symbol::SymbolTypePropertyName("type");

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

Symbol::Symbol(const std::string &symbolType) :
    m_type(symbolType)
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

/* unused
bool
Symbol::isSymbolOfType(Event *e, const std::string& type)
{
    return (e->isa(EventType) &&
            e->has(SymbolTypePropertyName) &&
            e->get<String>(SymbolTypePropertyName) == type);
}
*/

Event *
Symbol::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(SymbolTypePropertyName, m_type);
    return e;
}

} // end namespace Rosegarden
