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

#include <cstdio> // needed for sprintf()
#include "NotationTypes.h"
#include "BaseProperties.h"
#include <iostream>
#include <cstring> // for atoi

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

const int MIN_SUBORDERING = -100000;

namespace Accidentals
{
    const Accidental NoAccidental = "no-accidental";
    const Accidental Sharp = "sharp";
    const Accidental Flat = "flat";
    const Accidental Natural = "natural";
    const Accidental DoubleSharp = "double-sharp";
    const Accidental DoubleFlat = "double-flat";
}

using namespace Accidentals;
  

namespace Marks
{
    const Mark NoMark = "no-mark";
    const Mark Accent = "accent";
    const Mark Tenuto = "tenuto";
    const Mark Staccato = "staccato";
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
}

using namespace Marks;


//////////////////////////////////////////////////////////////////////
// Clef
//////////////////////////////////////////////////////////////////////
    
const string Clef::EventType = "clefchange";
const int Clef::EventSubOrdering = -30;
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
        throw Event::BadType();
    }
    std::string s = e.get<String>(ClefPropertyName);
    if (s != Treble && s != Tenor && s != Alto && s != Bass) {
        throw BadClefName();
    }
    m_clef = s;
}        

Clef::Clef(const std::string &s)
    // throw (BadClefName)
{
    if (s != Treble && s != Tenor && s != Alto && s != Bass) {
        throw BadClefName();
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

Event *Clef::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType);
    e->set<String>(ClefPropertyName, m_clef);
    e->setAbsoluteTime(absoluteTime);
    e->setSubOrdering(EventSubOrdering);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Key
//////////////////////////////////////////////////////////////////////


const string Key::EventType = "keychange";
const int Key::EventSubOrdering = -20;
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
        throw Event::BadType();
    }
    m_name = e.get<String>(KeyPropertyName);
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
        throw BadKeyName();
    }
}

Key::Key(const std::string &name)
    // throw (BadKeyName)
    : m_name(name), m_accidentalHeights(0)
{
    checkMap();
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
        throw BadKeyName();
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
    throw BadKeySpec();
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


std::vector<Key> Key::getKeys(bool minor)
{
    checkMap();
    std::vector<Key> result;
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
	if (height ==
            static_cast<int>(canonicalHeight((*m_accidentalHeights)[i] +
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

void Key::checkAccidentalHeights() const {

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

Event *Key::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType);
    e->set<String>(KeyPropertyName, m_name);
    e->setAbsoluteTime(absoluteTime);
    e->setSubOrdering(EventSubOrdering);
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
    m_keyDetailMap["A minor" ] = KeyDetails(true,  true,  0, "C major",  "C  maj / A  min", 9);
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
      m_rg2name(d.m_rg2name)
{
}

Key::KeyDetails& Key::KeyDetails::operator=(const Key::KeyDetails &d)
{
    if (&d == this) return *this;
    m_sharps = d.m_sharps; m_minor = d.m_minor;
    m_sharpCount = d.m_sharpCount; m_equivalence = d.m_equivalence;
    m_rg2name = d.m_rg2name;
    return *this;
}

//////////////////////////////////////////////////////////////////////
// Indication
//////////////////////////////////////////////////////////////////////

const std::string Indication::EventType = "indication";
const int Indication::EventSubOrdering = -8;
const PropertyName Indication::IndicationTypePropertyName = "indicationtype";
const PropertyName Indication::IndicationDurationPropertyName = "indicationduration";

const std::string Indication::Slur = "slur";
const std::string Indication::Crescendo = "crescendo";
const std::string Indication::Decrescendo = "decrescendo";

Indication::Indication(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType();
    }
    std::string s = e.get<String>(IndicationTypePropertyName);
    if (s != Slur && s != Crescendo && s != Decrescendo) {
        throw BadIndicationName();
    }
    m_indicationType = s;
    m_duration = e.get<Int>(IndicationDurationPropertyName);
}

Indication::Indication(const std::string &s, timeT indicationDuration)
{
    if (s != Slur && s != Crescendo && s != Decrescendo) {
        throw BadIndicationName();
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
    Event *e = new Event(EventType);
    e->set<String>(IndicationTypePropertyName, m_indicationType);
    e->set<Int>(IndicationDurationPropertyName, m_duration);
    e->setAbsoluteTime(absoluteTime);
    e->setSubOrdering(EventSubOrdering);
    return e;
}



//////////////////////////////////////////////////////////////////////
// Text
//////////////////////////////////////////////////////////////////////

const std::string Text::EventType = "text";
const int Text::EventSubOrdering = -9;
const PropertyName Text::TextPropertyName = "text";

Text::Text(const std::string &s) :
    m_text(s)
{
    // nothing else
}

Text::~Text()
{ 
    // nothing
}

Event *
Text::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType);
    e->set<String>(TextPropertyName, m_text);
    e->setAbsoluteTime(absoluteTime);
    e->setSubOrdering(EventSubOrdering);
    return e;
}



//////////////////////////////////////////////////////////////////////
// NotationDisplayPitch
//////////////////////////////////////////////////////////////////////

NotationDisplayPitch::NotationDisplayPitch(int heightOnStaff, Accidental accidental)
    : m_heightOnStaff(heightOnStaff),
      m_accidental(accidental)
{
}

NotationDisplayPitch::NotationDisplayPitch(int pitch, const Clef &clef,
                                           const Key &key,
                                           Accidental explicitAccidental) :
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


// Derived from RG2.1's MidiPitchToVoice in editor/src/Methods.c,
// InitialiseAccidentalTable in Format.c, and PutItemListInClef in
// MidiIn.c.  Converts performance pitch to height on staff + correct
// accidentals for current key.

void
NotationDisplayPitch::rawPitchToDisplayPitch(int pitch,
                                             const Clef &clef,
                                             const Key &key,
                                             int &height,
                                             Accidental &accidental) const
{
    int octave;
    bool modified = false;
    height = 0;

    // 1. Calculate with plain pitches, disregarding clef and key

    octave = pitch / 12;
    pitch  = pitch % 12;

    switch (pitch) {
    case  0: height = -2; break;	            // C  
    case  1: height = -2; modified = true; break;   // C# 
    case  2: height = -1; break;                    // D  
    case  3: height = -1; modified = true; break;   // D# 
    case  4: height =  0; break;                    // E  
    case  5: height =  1; break;                    // F  
    case  6: height =  1; modified = true; break;   // F# 
    case  7: height =  2; break;                    // G  
    case  8: height =  2; modified = true; break;   // G# 
    case  9: height =  3; break;                    // A  
    case 10: height =  3; modified = true; break;   // A# 
    case 11: height =  4; break;                    // B  
    }

    height += (octave - 5) * 7;
    height += clef.getPitchOffset();

    // 2. Adjust accidentals for the current key

    bool sharp = key.isSharp();

    if (accidental != NoAccidental) {
        sharp = (accidental == Sharp || accidental == DoubleSharp);
    }

    accidental = modified ? (sharp ? Sharp : Flat) : NoAccidental;
    if (modified && !sharp) ++height; // because the mod has become a flat

    vector<int> ah(key.getAccidentalHeights(clef));
    for (vector<int>::const_iterator i = ah.begin(); i != ah.end(); ++i) {

        if (Key::canonicalHeight(*i) == Key::canonicalHeight(height)) {
            // the key has an accidental at the same height as this note, so
            // undo the note's accidental if there is one, or make explicit
            // if there isn't

            if (modified && (sharp == key.isSharp())) {
		accidental = NoAccidental;
	    } else if (!modified) {
		accidental = Natural;
	    }
            break;
        }
    }

    // 3. Transpose up or down for the clef

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

    // 1. Get pitch and correct octave

    if (!ignoreOffset) height -= clef.getPitchOffset();

    while (height < 0) { octave -= 1; height += 7; }
    while (height > 7) { octave += 1; height -= 7; }

    if (height > 4) ++octave;

    switch (height) {

    case 0: pitch =  4; break;	/* bottom line, treble clef: E */
    case 1: pitch =  5; break;	/* F */
    case 2: pitch =  7; break;	/* G */
    case 3: pitch =  9; break;	/* A, in next octave */
    case 4: pitch = 11; break;	/* B, likewise*/
    case 5: pitch =  0; break;	/* C, moved up an octave (see above) */
    case 6: pitch =  2; break;	/* D, likewise */
    case 7: pitch =  4; break;	/* E, likewise */
    }

    // 2. Make any implicit accidentals from key explicit, and adjust pitch

    bool sharp = key.isSharp();

    vector<int> ah(key.getAccidentalHeights(clef));
    for (vector<int>::const_iterator i = ah.begin(); i != ah.end(); ++i) {

        if (Key::canonicalHeight(*i) == Key::canonicalHeight(height)) {
            // the key has an accidental at the same height as this note
            if (accidental == Natural) accidental = NoAccidental;
            else if (accidental == NoAccidental) accidental = sharp ? Sharp : Flat;
            break;
        }
    }

    if (accidental != NoAccidental) {
	if      (accidental == DoubleSharp) pitch += 2;
	else if (accidental ==       Sharp) pitch += 1;
	else if (accidental ==        Flat) pitch -= 1;
	else if (accidental ==  DoubleFlat) pitch -= 2;
    }

    // 3. Adjust for clef
    octave += clef.getOctave();

    pitch += 12 * octave;
}

string
NotationDisplayPitch::getAsString(const Clef &clef, const Key &key,
								  bool inclOctave) const
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
        sprintf(tmp, "%s%d", noteNamesSharps[pitch].c_str(), octave);
    else
        sprintf(tmp, "%s%d", noteNamesFlats[pitch].c_str(), octave);
    
    return string(tmp);
    

}


//////////////////////////////////////////////////////////////////////
// Note
//////////////////////////////////////////////////////////////////////

const string Note::EventType = "note";
const string Note::EventRestType = "rest";

const timeT Note::m_shortestTime = basePPQ / 16;
 

Note::Note(const string &n)
    // throw (BadType, MalformedNoteName)
    : m_type(-1), m_dots(0)
{
    string name(n);

    unsigned int pos = name.find('-');
    int dots = 1;

    if (pos > 0 && pos < name.length() - 1) {
        dots = atoi(name.substr(0, pos).c_str());
        name = name.substr(pos + 1);
        if (dots < 2)
            throw MalformedNoteName(n, "Non-numeric or invalid dot count");
    }

    if (name.length() > 7 && name.substr(0, 7) == "dotted ") {
        m_dots = dots;
        name = name.substr(7);
    } else {
        if (dots > 1)
            throw MalformedNoteName(n, "Dot count without dotted tag");
    }

    Type t;
    for (t = Shortest; t <= Longest; ++t) {
        if (name == getEnglishName(t) ||
            name == getAmericanName(t) ||
            name == getShortName(t)) {
            m_type = t;
            break;
        }
    }
    if (m_type == -1) throw BadType(name);
}

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


static string addDots(int dots, string s)
{
#if (__GNUC__ < 3)
    std::ostrstream os;
#else
    std::ostringstream os;
#endif

    if (dots > 1) {
        os << dots << "-";
    }
    os << "dotted " << s;
    return os.str();
}

string Note::getEnglishName(Type type, int dots) const {
    static const string names[] = {
        "hemidemisemiquaver", "demisemiquaver", "semiquaver",
        "quaver", "crotchet", "minim", "semibreve", "breve"
    };
    if (type < 0) { type = m_type; dots = m_dots; }
    if (dots) return addDots(dots, names[type]);
    else return names[type];
}

string Note::getAmericanName(Type type, int dots) const {
    static const string names[] = {
        "sixty-fourth note", "thirty-second note", "sixteenth note",
        "eighth note", "quarter note", "half note", "whole note",
        "double whole note"
    };
    if (type < 0) { type = m_type; dots = m_dots; }
    if (dots) return addDots(dots, names[type]);
    else return names[type];
}

string Note::getShortName(Type type, int dots) const {
    static const string names[] = {
        "64th", "32nd", "16th", "8th", "quarter", "half", "whole",
        "double whole"
    };
    if (type < 0) { type = m_type; dots = m_dots; }
    if (dots) return addDots(dots, names[type]);
    else return names[type];
}


Note Note::getNearestNote(int duration, int maxDots)
{
    int tag = Shortest - 1;
    int d(duration / m_shortestTime);
    while (d > 0) { ++tag; d /= 2; }

//    cout << "Note::getNearestNote: duration " << duration <<
//	" leading to tag " << tag << endl;
    if (tag < Shortest) return Note(Shortest);
    if (tag > Longest)  return Note(Longest, maxDots);

    int prospective = Note(tag, 0).getDuration();
    int dots = 0;
    int extra = prospective / 2;

    while (dots <= maxDots &&
           dots <= tag) { // avoid TooManyDots exception from Note ctor
	prospective += extra;
	if (prospective > duration) return Note(tag, dots);
	extra /= 2;
	++dots;
//	cout << "added another dot okay" << endl;
    }

    cout << "doh! ran out of dots" << endl;
    if (tag < Longest) return Note(tag + 1, 0);
    else return Note(tag, std::max(maxDots, tag));
} 

Event *Note::getAsNoteEvent(timeT absoluteTime, int pitch) const
{
    Event *e = new Event(EventType);
    e->set<Int>(BaseProperties::PITCH, pitch);
    e->setAbsoluteTime(absoluteTime);
    e->setDuration(getDuration());
    return e;
}

Event *Note::getAsRestEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventRestType);
    e->setAbsoluteTime(absoluteTime);
    e->setDuration(getDuration());
    return e;
}


///////////////////////////////////////////////////////////////////////

const string TimeSignature::EventType = "timesignature";
const int TimeSignature::EventSubOrdering = -10;
const PropertyName TimeSignature::NumeratorPropertyName = "numerator";
const PropertyName TimeSignature::DenominatorPropertyName = "denominator";
const TimeSignature TimeSignature::DefaultTimeSignature = TimeSignature(4, 4);

TimeSignature::TimeSignature(int numerator, int denominator)
    // throw (BadTimeSignature)
    : m_numerator(numerator), m_denominator(denominator)
{
    if (numerator < 1 || denominator < 1) throw BadTimeSignature();
    setInternalDurations();
}

TimeSignature::TimeSignature(const Event &e)
    // throw (Event::NoData, Event::BadType, BadTimeSignature)
{
    if (e.getType() != EventType) {
        throw Event::BadType();
    }
    m_numerator = e.get<Int>(NumeratorPropertyName);
    m_denominator = e.get<Int>(DenominatorPropertyName);
    if (m_numerator < 1 || m_denominator < 1) throw BadTimeSignature();
    setInternalDurations();
}

TimeSignature& TimeSignature::operator=(const TimeSignature &ts)
{
    if (&ts == this) return *this;
    m_numerator = ts.m_numerator;
    m_denominator = ts.m_denominator;
    setInternalDurations();
    return *this;
}

Note::Type TimeSignature::getUnit() const
{
    int c, d;
    for (c = 0, d = m_denominator; d > 1; d /= 2) ++c;
    return Note::Semibreve - c;
}

bool TimeSignature::isDotted() const
{
    // Is 3/8 dotted time?  This will report that it isn't, because of
    // the check for m_numerator > 3 -- but otherwise we'd get a false
    // positive with 3/4

    // [rf] That's an acceptable answer, according to my theory book. In
    // practice, you can say it's dotted time iff it has 6, 9, or 12 on top.

    return (m_numerator % 3 == 0 &&
            m_numerator > 3 &&
            getBarDuration() >= Note(Note::Crotchet, true).getDuration());
}

Event *TimeSignature::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType);
    e->set<Int>(NumeratorPropertyName, m_numerator);
    e->set<Int>(DenominatorPropertyName, m_denominator);
    e->setAbsoluteTime(absoluteTime);
    e->setSubOrdering(EventSubOrdering);
    return e;
}

// This doesn't consider subdivisions of the bar larger than a beat in
// any time other than 4/4, but it should handle the usual time signatures
// correctly (compound time included).

void TimeSignature::getDurationListForInterval(DurationList &dlist,
                                               int duration,
                                               int startOffset) const
{
    int offset = startOffset;
    int durationRemaining = duration;

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

	    int currentDuration = m_beatDivisionDuration;

	    // One or both of my safeguards against currentDuration being 0
	    // might be useless.

	    while ( !(offset % currentDuration == 0
		      && durationRemaining >= currentDuration)
		    && currentDuration > 1 ) {

		currentDuration /= 2;
		if (currentDuration == 0) currentDuration = 1;

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
               
void TimeSignature::setInternalDurations()
{
    // "unit length," which might be the beat length or the beat-division
    // length:
    int noteLength = m_crotchetTime * 4 / m_denominator;

    m_barDuration = m_numerator * noteLength;

    if (isDotted()) {
	m_beatDuration = noteLength * 3;
	m_beatDivisionDuration = noteLength;
    }
    else {
	m_beatDuration = noteLength;
	m_beatDivisionDuration = noteLength / 2;
    }

}

const timeT TimeSignature::m_crotchetTime       = basePPQ;
const timeT TimeSignature::m_dottedCrotchetTime = basePPQ + basePPQ/2;


} // close namespace
