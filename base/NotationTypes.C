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

#include <cstdio> // needed for sprintf()
#include "NotationTypes.h"
#include "BaseProperties.h"
#include <iostream>
#include <cstdlib> // for atoi
#include <limits.h> // for SHRT_MIN

#if (__GNUC__ < 3)
#include <strstream>
#else
#include <sstream>
#endif

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
    const Accidental NoAccidental = "no-accidental";
    const Accidental Sharp = "sharp";
    const Accidental Flat = "flat";
    const Accidental Natural = "natural";
    const Accidental DoubleSharp = "double-sharp";
    const Accidental DoubleFlat = "double-flat";

    AccidentalList getStandardAccidentals() {

        static Accidental a[] = {
            NoAccidental, Sharp, Flat, Natural, DoubleSharp, DoubleFlat
        };

        static AccidentalList v;
        if (v.size() == 0) {
            for (unsigned int i = 0; i < sizeof(a)/sizeof(a[0]); ++i)
                v.push_back(a[i]);
        }
        return v;
    }
}

using namespace Accidentals;
  

namespace Marks
{
    const Mark NoMark = "no-mark";
    const Mark Accent = "accent";
    const Mark Tenuto = "tenuto";
    const Mark Staccato = "staccato";
    const Mark Staccatissimo = "staccatissimo";
    const Mark Marcato = "marcato";
    const Mark Sforzando = getTextMark("sf");
    const Mark Rinforzando = getTextMark("rf");
    const Mark Trill = "trill";
    const Mark Turn = "turn";
    const Mark Pause = "pause";
    const Mark UpBow = "up-bow";
    const Mark DownBow = "down-bow";

    string getTextMark(string text) {
        return string("text_") + text;
    }

    bool isTextMark(Mark mark) {
        return string(mark).substr(0, 5) == "text_";
    }

    string getTextFromMark(Mark mark) {
        if (!isTextMark(mark)) return string();
        else return string(mark).substr(5);
    }

    std::vector<Mark> getStandardMarks() {

        static Mark a[] = {
            NoMark, Accent, Tenuto, Staccato, Staccatissimo, Marcato,
            Sforzando, Rinforzando, Trill, Turn, Pause, UpBow, DownBow
        };

        static std::vector<Mark> v;
        if (v.size() == 0) {
            for (unsigned int i = 0; i < sizeof(a)/sizeof(a[0]); ++i)
                v.push_back(a[i]);
        }
        return v;
    }

}

using namespace Marks;


//////////////////////////////////////////////////////////////////////
// Clef
//////////////////////////////////////////////////////////////////////
    
const string Clef::EventType = "clefchange";
const int Clef::EventSubOrdering = -250;
const PropertyName Clef::ClefPropertyName = "clef";
const string Clef::Treble = "treble";
const string Clef::Tenor = "tenor";
const string Clef::Alto = "alto";
const string Clef::Bass = "bass";

const Clef Clef::DefaultClef = Clef("treble");

Clef::Clef(const Event &e)
    // throw (Event::NoData, Event::BadType, BadClefName)
{
    if (e.getType() != EventType) {
        throw Event::BadType("Clef model event", EventType, e.getType());
    }
    std::string s = e.get<String>(ClefPropertyName);
    if (s != Treble && s != Tenor && s != Alto && s != Bass) {
        throw BadClefName("No such clef as \"" + s + "\"");
    }
    m_clef = s;
}        

Clef::Clef(const std::string &s)
    // throw (BadClefName)
{
    if (s != Treble && s != Tenor && s != Alto && s != Bass) {
        throw BadClefName("No such clef as \"" + s + "\"");
    }
    m_clef = s;
}

Clef &Clef::operator=(const Clef &c)
{
    if (this != &c) m_clef = c.m_clef;
    return *this;
}

int Clef::getTransposition() const
{
//!!! plus or minus?
    return getOctave() * 12 - getPitchOffset();
}

int Clef::getOctave() const
{
    if (m_clef == Treble) return 0;
    else if (m_clef == Bass) return -2;
    else return -1;
}

int Clef::getPitchOffset() const
{
    if (m_clef == Treble) return 0;
    else if (m_clef == Tenor) return 1;
    else if (m_clef == Alto) return -1;
    else return -2;
}

int Clef::getAxisHeight() const
{
    if (m_clef == Treble) return 2;
    else if (m_clef == Tenor) return 6;
    else if (m_clef == Alto) return 4;
    else return 6;
}

Clef::ClefList
Clef::getClefs()
{
    ClefList clefs;
    clefs.push_back(Clef(Bass));
    clefs.push_back(Clef(Tenor));
    clefs.push_back(Clef(Alto));
    clefs.push_back(Clef(Treble));
    return clefs;
}

Event *Clef::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(ClefPropertyName, m_clef);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Key
//////////////////////////////////////////////////////////////////////

const string Key::EventType = "keychange";
const int Key::EventSubOrdering = -200;
const PropertyName Key::KeyPropertyName = "key";
const Key Key::DefaultKey = Key("C major");

Key::KeyDetailMap Key::m_keyDetailMap = Key::KeyDetailMap();

Key::Key()
    : m_name(DefaultKey.m_name),
      m_accidentalHeights(0)
{
    checkMap();
}


Key::Key(const Event &e)
    // throw (Event::NoData, Event::BadType, BadKeyName)
    : m_accidentalHeights(0)
{
    checkMap();
    if (e.getType() != EventType) {
        throw Event::BadType("Key model event", EventType, e.getType());
    }
    m_name = e.get<String>(KeyPropertyName);
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
        throw BadKeyName("No such key as \"" + m_name + "\"");
    }
}

Key::Key(const std::string &name)
    // throw (BadKeyName)
    : m_name(name), m_accidentalHeights(0)
{
    checkMap();
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
        throw BadKeyName("No such key as \"" + m_name + "\"");
    }
}    

Key::Key(int accidentalCount, bool isSharp, bool isMinor)
    // throw (BadKeySpec)
    : m_accidentalHeights(0)
{
    checkMap();
    for (KeyDetailMap::const_iterator i = m_keyDetailMap.begin();
         i != m_keyDetailMap.end(); ++i) {
        if ((*i).second.m_sharpCount == accidentalCount &&
            (*i).second.m_sharps == isSharp &&
            (*i).second.m_minor == isMinor) {
            m_name = (*i).first;
            return;
        }
    }

#if (__GNUC__ < 3)
    std::ostrstream os;
#else
    std::ostringstream os;
#endif

    os << "No " << (isMinor ? "minor" : "major") << " key with "
       << accidentalCount << (isSharp ? " sharp(s)" : " flat(s)");

#if (__GNUC__ < 3)
    os << std::ends;
#endif

    throw BadKeySpec(os.str());
}

// Unfortunately this is ambiguous -- e.g. B major / Cb major.
// We need an isSharp argument, but we already have a constructor
// with that signature.  Not quite sure what's the best solution.

Key::Key(int tonicPitch, bool isMinor)
    // throw (BadKeySpec)
    : m_accidentalHeights(0)
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

#if (__GNUC__ < 3)
    std::ostrstream os;
#else
    std::ostringstream os;
#endif

    os << "No " << (isMinor ? "minor" : "major") << " key with tonic pitch "
       << tonicPitch;

#if (__GNUC__ < 3)
    os << std::ends;
#endif

    throw BadKeySpec(os.str());
}
    

Key::Key(const Key &kc)
    : m_name(kc.m_name), m_accidentalHeights(0)
{
}

Key& Key::operator=(const Key &kc)
{
    m_name = kc.m_name;
    m_accidentalHeights = 0;
    return *this;
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

Accidental Key::getAccidentalAtHeight(int height, const Clef &clef) const
{
    checkAccidentalHeights();
    height = canonicalHeight(height);
    for (unsigned int i = 0; i < m_accidentalHeights->size(); ++i) {
        if (height ==static_cast<int>(canonicalHeight((*m_accidentalHeights)[i] +
                                                     clef.getPitchOffset()))) {
            return isSharp() ? Sharp : Flat;
        }
    }
    return NoAccidental;
}

vector<int> Key::getAccidentalHeights(const Clef &clef) const
{
    // staff positions of accidentals
    checkAccidentalHeights();
    vector<int> v(*m_accidentalHeights);
    for (unsigned int i = 0; i < v.size(); ++i) {
        v[i] += clef.getPitchOffset();
    }
    return v;
}

void Key::checkAccidentalHeights() const
{
    if (m_accidentalHeights) return;
    m_accidentalHeights = new vector<int>;
  
    bool sharp = isSharp();
    int accidentals = getAccidentalCount();
    int pitch = sharp ? 8 : 4;
  
    for (int i = 0; i < accidentals; ++i) {
        m_accidentalHeights->push_back(pitch);
        if (sharp) { pitch -= 3; if (pitch < 3) pitch += 7; }
        else       { pitch += 3; if (pitch > 7) pitch -= 7; }
    }
}

int Key::convertFrom(int pitch, const Key &previousKey,
                     const Accidental &explicitAccidental) const
{
    NotationDisplayPitch ndp(pitch, Clef(), previousKey, explicitAccidental);
    return ndp.getPerformancePitch(Clef(), *this);
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
const PropertyName Indication::IndicationDurationPropertyName = "indicationduration";

const std::string Indication::Slur = "slur";
const std::string Indication::Crescendo = "crescendo";
const std::string Indication::Decrescendo = "decrescendo";

Indication::Indication(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("Indication model event", EventType, e.getType());
    }
    std::string s = e.get<String>(IndicationTypePropertyName);
    if (s != Slur && s != Crescendo && s != Decrescendo) {
        throw BadIndicationName("No such indication as \"" + s + "\"");
    }
    m_indicationType = s;
    m_duration = e.get<Int>(IndicationDurationPropertyName);
}

Indication::Indication(const std::string &s, timeT indicationDuration)
{
    if (s != Slur && s != Crescendo && s != Decrescendo) {
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
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(IndicationTypePropertyName, m_indicationType);
    e->set<Int>(IndicationDurationPropertyName, m_duration);
    return e;
}



//////////////////////////////////////////////////////////////////////
// Text
//////////////////////////////////////////////////////////////////////

const std::string Text::EventType = "text";
const int Text::EventSubOrdering = -70;
const PropertyName Text::TextPropertyName = "text";
const PropertyName Text::TextTypePropertyName = "type";

const std::string Text::UnspecifiedType = "unspecified";
const std::string Text::StaffName       = "staffname";
const std::string Text::ChordName       = "chordname";
const std::string Text::KeyName         = "keyname";
const std::string Text::Dynamic         = "dynamic";
const std::string Text::Lyric           = "lyric";
const std::string Text::Direction       = "direction";
const std::string Text::LocalDirection  = "local_direction";
const std::string Text::Tempo           = "tempo";
const std::string Text::LocalTempo      = "local_tempo";
const std::string Text::Annotation      = "annotation";

Text::Text(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("Text model event", EventType, e.getType());
    }

    m_text = e.get<String>(TextPropertyName);
    m_type = e.get<String>(TextTypePropertyName);
}

Text::Text(const std::string &s, const std::string &type) :
    m_text(s),
    m_type(type)
{
    // nothing else
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
    v.push_back(Lyric);
    v.push_back(Annotation);

    return v;
}

Event *
Text::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(TextPropertyName, m_text);
    e->set<String>(TextTypePropertyName, m_type);
    return e;
}

//////////////////////////////////////////////////////////////////////
// NotationDisplayPitch
//////////////////////////////////////////////////////////////////////

NotationDisplayPitch::NotationDisplayPitch(int heightOnStaff,
                                           const Accidental &accidental)
    : m_heightOnStaff(heightOnStaff),
      m_accidental(accidental)
{
}

NotationDisplayPitch::NotationDisplayPitch(int pitch, const Clef &clef,
                                           const Key &key,
                                           const Accidental &explicitAccidental) :
    m_accidental(explicitAccidental)
{
    rawPitchToDisplayPitch(pitch, clef, key, m_heightOnStaff, m_accidental);
}

int
NotationDisplayPitch::getPerformancePitch(const Clef &clef, const Key &key) const
{
    int p = 0;
    displayPitchToRawPitch(m_heightOnStaff, m_accidental, clef, key, p);
    return p;
}

int
NotationDisplayPitch::getPerformancePitchFromRG21Pitch(const Clef &clef,
                                                       const Key &) const
{
    // Rosegarden 2.1 pitches are a bit weird; see
    // docs/data_struct/units.txt

    // We pass the accidental and clef, a faked key of C major, and a
    // flag telling displayPitchToRawPitch to ignore the clef offset
    // and take only its octave into account

    int p = 0;
    displayPitchToRawPitch(m_heightOnStaff, m_accidental, clef, Key(), p, true);
    return p;
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
 * This function originally written by Chris Cannam for Rosegarden 2.1
 * Entirely rewritten by Chris Cannam for Rosegarden 4
 * Entirely rewritten by Hans Kieserman
 * Entirely rewritten by Michael McIntyre
 * This version by Michael McIntyre <dmmcintyr@users.sourceforge.net>
 */
void
NotationDisplayPitch::rawPitchToDisplayPitch(int pitch,
                                             const Clef &clef,
                                             const Key &key,
                                             int &height,
                                             Accidental &accidental) const
{

    // 1. Calculate the octave (for later):
    int octave = pitch / 12;

    // 2. Set initial height to 0
    height = 0;

    // 3.  Calculate raw semitone number, yielding a value between 0 (C) and
    // 11 (B)
    pitch  = pitch % 12;

    // 4.  Get info from the Key
    long accidentalCount = key.getAccidentalCount();
    bool keyIsSharp = key.isSharp(), keyIsFlat = !keyIsSharp;
    Accidental userAccidental = accidental;

    // clear the in-coming accidental so we can trap any failure to re-set
    // it on the way out:
    accidental = "";

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
    const char C = -2, D = -1, E = 0, F = 1, G = 2, A = 3, B = 4;
    
    // Here we do the actual work of making all the decisions explained above.
    switch (pitch) {
        case 0 : 
                 if (userAccidental == Sharp ||                         // B#
                    (userAccidental == NoAccidental && keyHasSharpB)) {
                     height = B;
                     octave--;
                     accidental = (keyHasSharpB) ? NoAccidental : Sharp;
                 } else if (userAccidental == DoubleFlat) {             // Dbb
                     height = D;
                     accidental = DoubleFlat;
                 } else {
                     height = C;                                        // C or C-Natural
                     accidental = (keyHasSharpC ||(keyHasSharpB &&
                                  userAccidental == Natural)) ? Natural : NoAccidental;
                 }
                 break;
        case 1 : 
                 if (userAccidental == Sharp ||                       // C#
                    (userAccidental == NoAccidental &&  keyIsSharp)) {
                     height = C;
                     accidental = (keyHasSharpC) ?  NoAccidental : Sharp;
                 } else if (userAccidental == Flat ||                 // Db
                           (userAccidental == NoAccidental && keyIsFlat)) {
                     height = D;
                     accidental = (keyHasFlatD) ? NoAccidental : Flat;
                 } else if (userAccidental == DoubleSharp) {          // Bx
                    height = B;
                    octave--;
                    accidental = DoubleSharp;
                 }
                 break;
        case 2 : 
                 if (userAccidental == DoubleSharp) {                  // Cx
                     height = C;
                     accidental = DoubleSharp;
                 } else if (userAccidental == DoubleFlat) {            // Ebb
                     height = E;
                     accidental = DoubleFlat;
                 } else {                                              // D or D-Natural
                     height = D;
                     accidental = (keyHasSharpD || keyHasFlatD) ? Natural : NoAccidental;
                 }
                 break;
        case 3 : 
                 if (userAccidental == Sharp ||                        // D#
                    (userAccidental == NoAccidental &&  keyIsSharp)) {
                     height = D;
                     accidental = (keyHasSharpD) ? NoAccidental : Sharp;
                 } else if (userAccidental == Flat ||                  // Eb
                           (userAccidental == NoAccidental &&  keyIsFlat)) {
                     height = E;
                     accidental = (keyHasFlatE) ? NoAccidental : Flat;
                 } else if (userAccidental == DoubleFlat) {            // Fbb
                     height = F;
                     accidental = DoubleFlat;
                 }
                 break;
        case 4 : 
                 if (userAccidental == Flat ||                         // Fb
                    (userAccidental == NoAccidental && keyHasFlatF)) {
                     height = F;
                     accidental = (keyHasFlatF) ? NoAccidental : Flat;
                 } else if (userAccidental == DoubleSharp) {           // Dx
                     height = D;
                     accidental = DoubleSharp;
                 } else {                                              // E or E-Natural
                     height = E;
                     accidental = (keyHasSharpE || keyHasFlatE) ? Natural : NoAccidental;
                 }
                 break;
        case 5 : 
                 if (userAccidental == Sharp ||                        // E#
                    (userAccidental == NoAccidental && keyHasSharpE)) {
                     height = E;
                     accidental = (keyHasSharpE) ? NoAccidental : Sharp;
                 } else if (userAccidental == DoubleFlat) {            // Gbb
                     height = G;
                     accidental = DoubleFlat;
                 } else {                                              // F or F-Natural
                     height = F;
                     accidental = (keyHasSharpF || keyHasFlatF) ? Natural : NoAccidental;
                 }
                 break;
        case 6 : 
                 if (userAccidental == Sharp ||
                    (userAccidental == NoAccidental && keyIsSharp)) {  // F#
                     height = F;
                     accidental = (keyHasSharpF) ? NoAccidental : Sharp;
                 } else if (userAccidental == Flat ||                  // Gb
                           (userAccidental == NoAccidental && keyIsFlat)) {
                     height = G;
                     accidental = (keyHasFlatG) ? NoAccidental : Flat;
                 } else if (userAccidental == DoubleSharp) {           // Ex
                     height = E;
                     accidental = DoubleSharp;
                 }
                 break;
        case 7 : 
                 if (userAccidental == DoubleSharp) {                  // Fx
                     height = F;
                     accidental = DoubleSharp;
                 } else if (userAccidental == DoubleFlat) {            // Abb
                     height = A;
                     accidental = DoubleFlat;
                 } else {                                              // G or G-Natural
                     height = G;
                     accidental = (keyHasSharpG || keyHasFlatG) ? Natural : NoAccidental;
                 }
                 break;
        case 8 : 
                 if (userAccidental == Sharp ||
                    (userAccidental == NoAccidental && keyIsSharp)) {  // G#
                     height = G;
                     accidental = (keyHasSharpG) ? NoAccidental : Sharp;
                 } else if (userAccidental == Flat ||                  // Ab
                           (userAccidental == NoAccidental && keyIsFlat)) {
                     height = A;
                     accidental = (keyHasFlatA) ? NoAccidental : Flat;
                 }
                 break;
        case 9 :
                 if (userAccidental == DoubleSharp) {                  // Gx
                     height = G;
                     accidental = DoubleSharp;
                 } else if (userAccidental == DoubleFlat) {            // Bbb
                     height = B;
                     accidental = DoubleFlat;
                 } else {                                              // A or A-Natural
                     height = A;                
                     accidental = (keyHasSharpA || keyHasFlatA) ? Natural : NoAccidental;
                 }
                 break;
        case 10: 
                 if (userAccidental == DoubleFlat) {                   // Cbb
                     height = C;
                     octave++;  // tweak B/C divide
                     accidental = DoubleFlat;
                 } else if (userAccidental == Sharp ||                 // A#
                           (userAccidental == NoAccidental && keyIsSharp)) {
                     height = A;
                     accidental = (keyHasSharpA) ? NoAccidental : Sharp;
                 } else if (userAccidental == Flat ||                  // Bb
                           (userAccidental == NoAccidental && keyIsFlat)) {
                     height = B;
                     accidental = (keyHasFlatB) ? NoAccidental : Flat;
                 }
                 break;
        case 11: 
                 if (userAccidental == DoubleSharp) {                  // Ax
                     height = A;
                     accidental = DoubleSharp;
                 } else if (userAccidental == Flat ||                  // Cb
                           (userAccidental == NoAccidental && keyHasFlatC)) {
                     height = C;
                     octave++;  // tweak B/C divide
                     accidental = (keyHasFlatC) ? NoAccidental : Flat;
                 } else {                                             // B or B-Natural
                     height = B;
                     accidental = (keyHasSharpB || keyHasFlatB) ? Natural : NoAccidental;
                 }
    }

    // Failsafe...  If this ever executes, there's trouble to fix...
    if (accidental == "") {
        std::cerr << "rawPitchToDisplayPitch(): error! returning null accidental for"
                  << std::endl << "pitch: " << pitch << "\tuserAccidental: " << userAccidental
                  << "\tkey: " << accidentalCount << "\t" << (keyIsSharp ? "sharp" : "flat")
                  << std::endl;
    }
    
    // 6.  "Recenter" height in case it's been changed:
    height = ((height + 2) % 7) - 2;

    height += (octave - 5) * 7;
    height += clef.getPitchOffset();


    // 7. Transpose up or down for the clef:
    height -= 7 * clef.getOctave();
}

void
NotationDisplayPitch::displayPitchToRawPitch(int height,
                                             Accidental accidental,
                                             const Clef &clef,
                                             const Key &key,
                                             int &pitch,
                                             bool ignoreOffset) const
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

string
NotationDisplayPitch::getAsString(const Clef &clef, const Key &key,
                                  bool inclOctave, int octaveBase) const
{
    static const string noteNamesSharps[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    static const string noteNamesFlats[]  = {
        "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
    };
    
    int performancePitch = getPerformancePitch(clef, key);

    // highly unlikely, but fatal if it happened:
    if (performancePitch < 0) performancePitch = 0;

    int pitch  = performancePitch % 12;
    int octave = performancePitch / 12;

    if (!inclOctave)
        return key.isSharp() ? noteNamesSharps[pitch] : noteNamesFlats[pitch];

    char tmp[1024];

    if (key.isSharp())
        sprintf(tmp, "%s%d", noteNamesSharps[pitch].c_str(),
                octave + octaveBase);
    else
        sprintf(tmp, "%s%d", noteNamesFlats[pitch].c_str(),
                octave + octaveBase);
    
    return string(tmp);
}

void
NotationDisplayPitch::getInScale(const Clef &clef, const Key &key,
				 int &placeInScale, int &accidentals, int &octave) const
{
    //!!! Maybe we should bring the logic from rawPitchToDisplayPitch down
    // into this method, and make rawPitchToDisplayPitch wrap this

    static int pitches[2][12] = {
	{ 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 },
	{ 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 },
    };
    static int accidentalsForPitches[2][12] = {
	{ 0,  1, 0,  1, 0, 0,  1, 0,  1, 0,  1, 0 },
	{ 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0 },
    };
    
    int performancePitch = getPerformancePitch(clef, key);

    // highly unlikely, but fatal if it happened:
    if (performancePitch < 0) performancePitch = 0;
    if (performancePitch > 127) performancePitch = 127;

    int pitch  = performancePitch % 12;
    octave = performancePitch / 12 - 2;

    if (key.isSharp()) { //!!! need to [optionally?] handle minor keys (similarly in getAsString?)
	placeInScale = pitches[0][pitch];
	accidentals = accidentalsForPitches[0][pitch];
    } else {
	placeInScale = pitches[1][pitch];
	accidentals = accidentalsForPitches[1][pitch];
    }
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

Pitch::Pitch(int heightOnStaff, const Clef &clef, const Key &key,
	     const Accidental &explicitAccidental) :
    m_pitch(0),
    m_accidental(explicitAccidental)
{
    NotationDisplayPitch ndp(heightOnStaff, explicitAccidental);
    m_pitch = ndp.getPerformancePitch(clef, key);
}

Pitch::Pitch(int noteInScale, int octave,
	     const Accidental &explicitAccidental) :
    m_pitch((octave+2) * 12 + noteInScale),
    m_accidental(explicitAccidental)
{
    // nothing else
}

int
Pitch::getPerformancePitch() const
{
    return m_pitch;
}

Accidental
Pitch::getAccidental(bool useSharps) const
{
    return getDisplayAccidental(useSharps ? Key("C major") : Key("A minor"));
}

Accidental
Pitch::getDisplayAccidental(const Key &key) const
{
    NotationDisplayPitch ndp(m_pitch, Clef(), key, m_accidental);
    return ndp.getAccidental();
}

int
Pitch::getNoteInScale(const Key &key) const
{
    return (getHeightOnStaff(Clef(Clef::Treble), key) + 72) % 7;
}

char
Pitch::getNoteName(const Key &key) const
{
    return "CDEFGAB"[getNoteInScale(key)];
}

int
Pitch::getHeightOnStaff(const Clef &clef, const Key &key) const
{
    NotationDisplayPitch ndp(m_pitch, clef, key, m_accidental);
    return ndp.getHeightOnStaff();
}

int
Pitch::getOctave(int octaveBase) const
{
    return m_pitch / 12 + octaveBase;
}

int
Pitch::getPitchInOctave() const
{
    return m_pitch % 12;
}

std::string
Pitch::getAsString(bool useSharps, bool inclOctave, int octaveBase) const
{
    Accidental acc = getAccidental(useSharps);

    std::string s;
    s += getNoteName(useSharps ? Key("C major") : Key("A minor"));

    if (acc == Accidentals::Sharp) s += "#";
    else if (acc == Accidentals::Flat) s += "b";

    if (!inclOctave) return s;

    char tmp[10];
    sprintf(tmp, "%s%d", s.c_str(), getOctave(octaveBase));
    return std::string(tmp);
}


//////////////////////////////////////////////////////////////////////
// Note
//////////////////////////////////////////////////////////////////////

const string Note::EventType = "note";
const string Note::EventRestType = "rest";

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

//!!!    cout << "doh! ran out of dots" << endl;
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



///////////////////////////////////////////////////////////////////////

const string TimeSignature::EventType = "timesignature";
const int TimeSignature::EventSubOrdering = -150;
const PropertyName TimeSignature::NumeratorPropertyName = "numerator";
const PropertyName TimeSignature::DenominatorPropertyName = "denominator";
const PropertyName TimeSignature::ShowAsCommonTimePropertyName = "common";
const PropertyName TimeSignature::IsHiddenPropertyName = "hidden";
const TimeSignature TimeSignature::DefaultTimeSignature = TimeSignature(4, 4);

TimeSignature::TimeSignature(int numerator, int denominator,
                             bool preferCommon, bool hidden)
    // throw (BadTimeSignature)
    : m_numerator(numerator), m_denominator(denominator),
      m_common(preferCommon &&
               (m_denominator == m_numerator &&
                (m_numerator == 2 || m_numerator == 4))),
      m_hidden(hidden)
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
    m_numerator = e.get<Int>(NumeratorPropertyName);
    m_denominator = e.get<Int>(DenominatorPropertyName);

    m_common = false;
    e.get<Bool>(ShowAsCommonTimePropertyName, m_common);

    m_hidden = false;
    e.get<Bool>(IsHiddenPropertyName, m_hidden);

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
            offset += m_barDuration;

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


std::vector<int>
TimeSignature::getDivisions(int depth) const
{
    std::vector<int> divisions;

    if (depth <= 0) return divisions;
    timeT base = getBarDuration(); // calls setInternalDurations

    if (m_numerator == 4 && m_denominator == 4) {
        divisions.push_back(2);
        base /= 2;
        --depth;
    }

    if (depth <= 0) return divisions;

    divisions.push_back(base / m_beatDuration);
    base = m_beatDuration;
    --depth;

    if (depth <= 0) return divisions;

    if (m_dotted) divisions.push_back(3);
    else divisions.push_back(2);
    --depth;

    while (depth > 0) {
        divisions.push_back(2);
        --depth;
    }

    return divisions;
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


} // close namespace
