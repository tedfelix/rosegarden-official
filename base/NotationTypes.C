
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

const int MIN_SUBORDERING = -100000;

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

Clef& Clef::operator=(const Clef &c)
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
        v[i] += clef.getPitchOffset(); //!!! changed from += 20010930 cc
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

    m_keyDetailMap["A major" ] = KeyDetails(true,  false, 3, "F# minor", "A  maj / F# min");
    m_keyDetailMap["F# minor"] = KeyDetails(true,  true,  3, "A major",  "A  maj / F# min");
    m_keyDetailMap["Ab major"] = KeyDetails(false, false, 4, "F minor",  "Ab maj / F  min");
    m_keyDetailMap["F minor" ] = KeyDetails(false, true,  4, "Ab major", "Ab maj / F  min");
    m_keyDetailMap["B major" ] = KeyDetails(true,  false, 5, "G# minor", "B  maj / G# min");
    m_keyDetailMap["G# minor"] = KeyDetails(true,  true,  5, "B major",  "B  maj / G# min");
    m_keyDetailMap["Bb major"] = KeyDetails(false, false, 2, "G minor",  "Bb maj / G  min");
    m_keyDetailMap["G minor" ] = KeyDetails(false, true,  2, "Bb major", "Bb maj / G  min");
    m_keyDetailMap["C major" ] = KeyDetails(true,  false, 0, "A minor",  "C  maj / A  min");
    m_keyDetailMap["A minor" ] = KeyDetails(true,  true,  0, "C major",  "C  maj / A  min");
    m_keyDetailMap["Cb major"] = KeyDetails(false, false, 7, "Ab minor", "Cb maj / Ab min");
    m_keyDetailMap["Ab minor"] = KeyDetails(false, true,  7, "Cb major", "Cb maj / Ab min");
    m_keyDetailMap["C# major"] = KeyDetails(true,  false, 7, "A# minor", "C# maj / A# min");
    m_keyDetailMap["A# minor"] = KeyDetails(true,  true,  7, "C# major", "C# maj / A# min");
    m_keyDetailMap["D major" ] = KeyDetails(true,  false, 2, "B minor",  "D  maj / B  min");
    m_keyDetailMap["B minor" ] = KeyDetails(true,  true,  2, "D major",  "D  maj / B  min");
    m_keyDetailMap["Db major"] = KeyDetails(false, false, 5, "Bb minor", "Db maj / Bb min");
    m_keyDetailMap["Bb minor"] = KeyDetails(false, true,  5, "Db major", "Db maj / Bb min");
    m_keyDetailMap["E major" ] = KeyDetails(true,  false, 4, "C# minor", "E  maj / C# min");
    m_keyDetailMap["C# minor"] = KeyDetails(true,  true,  4, "E major",  "E  maj / C# min");
    m_keyDetailMap["Eb major"] = KeyDetails(false, false, 3, "C minor",  "Eb maj / C  min");
    m_keyDetailMap["C minor" ] = KeyDetails(false, true,  3, "Eb major", "Eb maj / C  min");
    m_keyDetailMap["F major" ] = KeyDetails(false, false, 1, "D minor",  "F  maj / D  min");
    m_keyDetailMap["D minor" ] = KeyDetails(false, true,  1, "F major",  "F  maj / D  min");
    m_keyDetailMap["F# major"] = KeyDetails(true,  false, 6, "D# minor", "F# maj / D# min");
    m_keyDetailMap["D# minor"] = KeyDetails(true,  true,  6, "F# major", "F# maj / D# min");
    m_keyDetailMap["G major" ] = KeyDetails(true,  false, 1, "E minor",  "G  maj / E  min");
    m_keyDetailMap["E minor" ] = KeyDetails(true,  true,  1, "G major",  "G  maj / E  min");
    m_keyDetailMap["Gb major"] = KeyDetails(false, false, 6, "Eb minor", "Gb maj / Eb min");
    m_keyDetailMap["Eb minor"] = KeyDetails(false, true,  6, "Gb major", "Gb maj / Eb min");
}


Key::KeyDetails::KeyDetails()
    : m_sharps(false), m_minor(false), m_sharpCount(0),
      m_equivalence(""), m_rg2name("")
{
}

Key::KeyDetails::KeyDetails(bool sharps, bool minor, int sharpCount,
                            std::string equivalence, std::string rg2name)
    : m_sharps(sharps), m_minor(minor), m_sharpCount(sharpCount),
      m_equivalence(equivalence), m_rg2name(rg2name)
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
            if (modified) accidental = NoAccidental;
            else accidental = Natural;
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

    switch (accidental) {
    case  DoubleSharp: pitch += 2; break;
    case        Sharp: pitch += 1; break;
    case         Flat: pitch -= 1; break;
    case   DoubleFlat: pitch -= 2; break;
    case      Natural: break;
    case NoAccidental: break;
    }

    // 3. Adjust for clef
    octave += clef.getOctave();

    pitch += 12 * octave;
}

string
NotationDisplayPitch::getAsString(const Clef &clef, const Key &key) const
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

    int octave = performancePitch / 12;
    int pitch  = performancePitch % 12;

    char tmp[1024];

    if (key.isSharp())
        sprintf(tmp, "%s%d", noteNamesSharps[pitch].c_str(), octave);
    else
        sprintf(tmp, "%s%d", noteNamesFlats[pitch].c_str(), octave);
    
    return string(tmp);
    
}

string NotationDisplayPitch::getAccidentalName(Accidental a)
{
    switch (a) {
    case NoAccidental: return "no-accidental";
    case        Sharp: return "sharp";
    case         Flat: return "flat";
    case  DoubleSharp: return "double-sharp";
    case   DoubleFlat: return "double-flat";
    case      Natural: return "natural";
    default: throw BadAccidental();
    }
}

Accidental NotationDisplayPitch::getAccidentalByName(const string &s)
{
    if      (s == "no-accidental") return NoAccidental;
    else if (s == "sharp")         return Sharp;
    else if (s == "flat")          return Flat;
    else if (s == "double-sharp")  return DoubleSharp;
    else if (s == "double-flat")   return DoubleFlat;
    else if (s == "natural")       return Natural;
    else throw BadAccidental();
}



//////////////////////////////////////////////////////////////////////
// Note
//////////////////////////////////////////////////////////////////////

const string Note::EventType = "note";
const string Note::EventRestType = "rest";

const PropertyName Note::NoteType = "NoteType";
const PropertyName Note::NoteDots = "NoteDots";

const PropertyName Note::TiedBackwardPropertyName = "TiedBackward";
const PropertyName Note::TiedForwardPropertyName  = "TiedForward";

const int Note::m_shortestTime       = 6;
//const int Note::m_dottedShortestTime = 9;
 

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
    e->set<Int>("pitch", pitch);
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
}

TimeSignature& TimeSignature::operator=(const TimeSignature &ts)
{
    if (&ts == this) return *this;
    m_numerator = ts.m_numerator;
    m_denominator = ts.m_denominator;
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

    return (m_numerator % 3 == 0 &&
            m_numerator > 3 &&
            getBarDuration() >= Note(Note::Crotchet, true).getDuration());
}

int TimeSignature::getBeatDuration() const
{
    if (isDotted()) {
        // this is surprisingly difficult to work out, I got it badly
        // wrong first time and I'm still not certain about it
        int u = getUnitDuration();
        if (u * 3 >= getBarDuration()) {
            return (u * 3) / 2;
        } else {
            return (u * 3);
        }
    } else {
        return getUnitDuration();
    }
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


void TimeSignature::getDurationListForInterval(DurationList &dlist,
                                               int duration,
                                               int startOffset) const
{
    // We need to do this in three parts: (1) if the startOffset isn't
    // at the start of a bar, fill up the interval from it to the
    // start of the next bar using the "short interval" algorithm.
    // (2) fill up with "optimal bar-length rests" as far as
    // possible. (3) fill any remainder using the "short interval"
    // algorithm.

    int toNextBar;
    int barDuration = getBarDuration();
    int acc = 0;

//    cerr << "TimeSignature::getDurationListForInterval: Desired duration is " << duration << " with startOffset " << startOffset << " and bar duration " << barDuration << endl;

    toNextBar = barDuration - (startOffset % barDuration);

    if (toNextBar > 0 && toNextBar <= duration && toNextBar < barDuration) {
//        cerr << "TimeSignature::getDurationListForInterval: filling to next bar (duration " << toNextBar << ")" << endl;
        getDurationListForShortInterval(dlist, toNextBar, startOffset);
        acc = toNextBar;
    }

    while (duration - acc >= barDuration) {
//        cerr << "TimeSignature::getDurationListForInterval: acc is " << acc << ", filling a bar" << endl;
        getDurationListForBar(dlist);
        acc += barDuration;
    }
    
    if (duration > acc) {
//        cerr << "TimeSignature::getDurationListForInterval: acc is " << acc << ", filling the remaining " << (duration-acc) << endl;
        getDurationListForShortInterval(dlist, duration - acc, 0);
    }
}


void TimeSignature::getDurationListForBar(DurationList &dlist) const
{
    // mostly just a bunch of special-cases, for now

    if (m_numerator < 3) {
//        cerr << "TimeSignature::getDurationListForBar: adding 2/* whole bar " << getBarDuration() << endl;
        // A single long rest should be okay for all the common 2/x
        // timesigs, probably even tolerable for freaks like 2/1
        dlist.push_back(getBarDuration());
        return;
    }

    if (m_numerator == 4 && m_denominator > 2) {
//        cerr << "TimeSignature::getDurationListForBar: adding whole-bar " << getBarDuration() << endl;
        //        dlist.push_back(getBarDuration() / 2);
        //        dlist.push_back(getBarDuration() / 2);
        dlist.push_back(getBarDuration());
        return;
    }

    for (int i = 0; i < getBeatsPerBar(); ++i) {
//        cerr << "TimeSignature::getDurationListForBar: adding beat " << getBeatDuration() << endl;
        dlist.push_back(getBeatDuration());
    }
}



//!!! It might be nice to have a better algorithm for this, but I'm
// not sure if it's possible to be general enough and of high enough
// quality without simply having lots of special cases.

// Example of limitation of the current code: in a 4/4 bar, we have a
// single semibreve rest.  We insert a crotchet note over the start of
// the bar, expecting the semibreve rest to be split into a crotchet
// and a minim.  Instead, it's split into a minim and a crotchet.

// Here's a potential better algorithm:

// Given an interval and a beat duration, we can divide the interval
// into three parts: the lead-in to the first beat, a section composed
// only of full beats, and the lead-out of the final beat (the first
// and third parts being shorter than a single beat duration, and any
// of the three being potentially of zero duration).

// Given a time signature, we can work out the longest natural
// subdivision of the bar by dividing the bar duration by the smallest
// integer factor of the time signature's numerator.  This is the
// initial beat duration.  The result of dividing our numerator by its
// smallest factor is the effective numerator of the beat.  We can
// then subdivide this into sub-beats, using the effective numerator
// in the same way as we used the original numerator.  If the
// numerator for the bar's whole time signature is prime, we divide by
// that; if an effective numerator becomes 1, we divide by 2 for our
// beats but keep the numerator at 1.

// So, to make an interval: Find the beat for the whole bar.  Divide
// the interval into three as described above (lead-in, beats,
// lead-out).  For the lead-in, find the next shortest sub-beat;
// divide the lead-in interval into two (lead-in, beats) and recurse
// on those with the sub-beat duration.  For the main beats, just fill
// up with main-beat-duration rests.  For the lead-out, find the
// sub-beat, divide the lead-out into two (beats, lead-out) and
// recurse.  I think this could work for multi-bar sections too, if we
// start by dividing into the whole bar duration.

// The remaining problem is what to do if a beat duration is not
// expressible as a single rest (possibly dotted, if in dotted time).
// Probably we should just subdivide the beat immediately.

// Might be too complicated overall, that.



// Derived from RG2's MidiMakeRestList in editor/src/MidiIn.c.

// Create a list of durations, totalling (as close as possible) the
// given duration, such that each is an exact note duration and the
// notes are the proper sort for the time signature.  start is the
// elapsed duration since the beginning of the bar (or of the last
// beat); for use independent of a particular bar, pass zero.

// Currently uses no note-durations longer than a dotted-crotchet; for
// general use in /2 time, this is a defect

void TimeSignature::getDurationListForShortInterval(DurationList &dlist,
                                                    int duration,
                                                    int startOffset) const
{
    int toNextBeat;
    int beatDuration = getBeatDuration();

    toNextBeat = beatDuration - (startOffset % beatDuration);

//    cerr << "TimeSignature::getDurationListForShortInterval: duration is "
//         << duration << ", toNextBeat " << toNextBeat << ", startOffset "
//         << startOffset << ", beatDuration " << beatDuration << endl;
               
    if (toNextBeat == duration) {
        getDurationListAux(dlist, duration, true);
    } else if (toNextBeat > duration) {
        getDurationListAux(dlist, duration, false);
    } else {
        // first fill up to the next crotchet (or, in 6/8 or some
        // other such time, the next dotted crotchet); then fill in
        // crotchet or dotted-crotchet leaps until the end of the
        // section needing filling
        getDurationListAux(dlist, toNextBeat, dlist.size() == 0);
        getDurationListAux(dlist, duration - toNextBeat, false);
    }
}


// Derived from RG2's MidiMakeRestListSub in editor/src/MidiIn.c.

void TimeSignature::getDurationListAux(DurationList &dlist, int t,
                                       bool isLeadIn = false) const
    // (we append to dlist, it's expected to have stuff in it already)
{
//    cerr << "TimeSignature::getDurationListAux: duration is " << t
//         << ", isLeadIn is " << isLeadIn << endl;
    
    if (t <= 0) return;

    // We behave differently if we're trying to fill the space leading
    // up to the first beat boundary of an interval.  In this case, we
    // want to end up on semi-beat boundaries as quickly as possible,
    // and usually the best way to achieve that is to fill up the
    // duration list in reverse order, longest duration at the end
    // (and avoid using dotted notes, but we don't currently use those
    // anyway).  isLeadIn indicates this case.

    // This code could probably be rather simpler, and it would be
    // better if taking into account the possibility of using longer
    // beats in time signatures like 4/4

    int shortestTime = Note(Note::Shortest).getDuration();

    if (t < shortestTime) {
//        cerr << "pushing [1] " << t << endl;
	// we want to divide the time exactly, even if it means we
	// can't represent everything quite right in note durations
	if (isLeadIn) dlist.push_front(t);
        else dlist.push_back(t);
	return;
    }
    int current;

    if ((current = (isDotted() ? m_dottedCrotchetTime : m_crotchetTime)) <= t) {
//        cerr << "pushing [2] " << current << endl;
        if (isLeadIn) dlist.push_front(current);
        else dlist.push_back(current);
        getDurationListAux(dlist, t - current, isLeadIn);
        return;
    }
               
    current = shortestTime;
    for (int tag = Note::Shortest + 1; tag <= Note::Crotchet; ++tag) {
        int next = Note(tag).getDuration();
        if (next > t) {
//            cerr << "pushing [3] " << current << endl;
            if (isLeadIn) dlist.push_front(current);
            else dlist.push_back(current);
            getDurationListAux(dlist, t - current, isLeadIn);
            return;
        }
        current = next;
    }
               
    // should only be reached in dotted time for lengths between
    // crotchet and dotted crotchet:

    current = m_crotchetTime;
//    cerr << "pushing [4] " << current << endl;
    if (isLeadIn) dlist.push_front(current);
    else dlist.push_back(current);
    getDurationListAux(dlist, t - current, isLeadIn);
}

const int TimeSignature::m_crotchetTime       = 96;
const int TimeSignature::m_dottedCrotchetTime = 144;


} // close namespace
