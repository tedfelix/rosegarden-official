
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

namespace Rosegarden 
{
using std::string;
using std::vector;
using std::cout;
using std::endl;
    
const string Clef::EventType = "clefchange";
const string Clef::ClefPropertyName = "clef";
const string Clef::Treble = "treble";
const string Clef::Tenor = "tenor";
const string Clef::Alto = "alto";
const string Clef::Bass = "bass";

const Clef Clef::DefaultClef = Clef("treble");

Clef::Clef(const Event &e)
    throw (Event::NoData, Event::BadType, BadClefName)
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
    throw (BadClefName)
{
    if (s != Treble && s != Tenor && s != Alto && s != Bass) {
        throw BadClefName();
    }
    m_clef = s;
}

const string Key::EventType = "keychange";
const string Key::KeyPropertyName = "key";
const Key Key::DefaultKey = Key("C major");

Key::KeyDetailMap Key::m_keyDetailMap = Key::KeyDetailMap();

Key::Key(const Event &e)
    throw (Event::NoData, Event::BadType, BadKeyName) :
    m_accidentalHeights(0)
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
    throw (BadKeyName)
    : m_name(name), m_accidentalHeights(0)
{
    checkMap();
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
        throw BadKeyName();
    }
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
	if (height == (int)canonicalHeight((*m_accidentalHeights)[i] +
					   clef.getPitchOffset())) {
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


//////////////////////////////////////////////////////////////////////
// NotationDisplayPitch
//////////////////////////////////////////////////////////////////////

NotationDisplayPitch::NotationDisplayPitch(int pitch, const Clef &clef, const Key &key)
{
    //!!! explicit accidentals in the note event properties?
    rawPitchToDisplayPitch(pitch, clef, key, m_heightOnStaff, m_accidental);
}

NotationDisplayPitch::NotationDisplayPitch(int heightOnStaff, Accidental accidental)
    : m_heightOnStaff(heightOnStaff),
      m_accidental(accidental)
{
}

int
NotationDisplayPitch::getPerformancePitch(const Clef &clef, const Key &key) const
{
    int p = 0;
    displayPitchToRawPitch(m_heightOnStaff, m_accidental, clef, key, p);
    return p;
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
    accidental = NoAccidental;

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

    // 2. Adjust accidentals for the current key

    bool sharp = key.isSharp();

    accidental = modified ? (sharp ? Sharp : Flat) : NoAccidental;
    if (modified && !sharp) ++height; // because the modifier has become a flat

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
                                             int &pitch) const
{
    int octave = 5;

    // 1. Get pitch and correct octave

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


//////////////////////////////////////////////////////////////////////
// Note
//////////////////////////////////////////////////////////////////////

const string Note::EventType = "note";
//!!! worry about this later -- simple solution currently in place ain't bad
const string Note::NotePropertyName = "duration";

const int Note::m_shortestTime       = 6;
//const int Note::m_dottedShortestTime = 9;
const int Note::m_crotchetTime       = 96;
const int Note::m_dottedCrotchetTime = 144;

Note::Note(Type type, int dots) throw (BadType, TooManyDots) :
    m_type(type), m_dots(dots)
{
    //!!! having exceptions here may really bugger up compiler
    // optimisations for simple uses of Note (e.g. "int d =
    // Note(Crotchet, true).getDuration()"):
    if (m_type < Shortest || m_type > Longest) throw BadType();
    
    // We don't permit dotted hemis, double-dotted demis etc
    // because we can't represent notes short enough to make up
    // the rest of the beat (as we have no notes shorter than a
    // hemi).  And if we got to double-dotted hemis, triple-dotted
    // demis etc, we couldn't even represent their durations in
    // our duration units
//!!!    if (m_dots > m_type) throw TooManyDots();
}

Note::Note(const string &n)
    throw (BadType) :
    m_type(-1), m_dots(0)
{
    string name(n);
    if (name.length() > 7 && name.substr(0, 7) == "dotted ") {
        m_dots = 1;
        name = name.substr(7);
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

string Note::getEnglishName(Type type, int dots) const {
    static const string names[] = {
        "hemidemisemiquaver", "demisemiquaver", "semiquaver",
            "quaver", "crotchet", "minim", "semibreve", "breve"
            };
    if (type < 0) { type = m_type; dots = m_dots; }
    //!!! double-dots etc
    return dots ? ("dotted " + names[type]) : names[type];
}

string Note::getAmericanName(Type type, int dots) const {
    static const string names[] = {
        "sixty-fourth note", "thirty-second note", "sixteenth note",
            "eighth note", "quarter note", "half note", "whole note",
            "double whole note"
            };
    if (type < 0) { type = m_type; dots = m_dots; }
    //!!! double-dots etc
    return dots ? ("dotted " + names[type]) : names[type];
}

string Note::getShortName(Type type, int dots) const {
    static const string names[] = {
        "64th", "32nd", "16th", "8th", "quarter", "half", "whole",
            "double whole"
            };
    if (type < 0) { type = m_type; dots = m_dots; }
    //!!! double-dots etc
    return dots ? ("dotted " + names[type]) : names[type];
}


Note Note::getNearestNote(int duration, int maxDots)
{
    int tag = Shortest - 1;
    int d(duration / m_shortestTime);
    while (d > 0) { ++tag; d /= 2; }

    cout << "Note::getNearestNote: duration " << duration <<
	" leading to tag " << tag  << endl;
    if (tag < Shortest) return Note(Shortest);

    int prospective = Note(tag, 0).getDuration();
    int dots = 0;
    int extra = prospective / 2;

    while (dots < maxDots) {
	prospective += extra;
	if (prospective > duration) return Note(tag, dots);
	extra /= 2;
	++dots;
	cout << "added another dot okay" << endl;
    }

    return Note(tag, maxDots); //???

/*
    int d = m_shortestTime;
    Note n(Longest, maxDots);

    //!!! too short -- reconsider?
    if (duration < d) return Note(Shortest);

    for (int tag = Shortest + 1; tag <= Longest + 1; ++tag) {
        if (d + d/2 > duration) {
	    n = Note(tag-1); break;
        }

	//???

        if (d*2 > duration) {
	    n = Note(tag-1, true); break;
        }
        d *= 2;
    }

    //!!! too long -- should subdivide
//    n = Note(Longest, true);
#ifndef NDEBUG
    cout << "Note::getNearestNote(): duration " << duration
	 << ", returning note (" << n.getNoteType() << ", " << n.getDots()
	 << ") (duration is " << n.getDuration() << ")" << endl;
#endif
    return n;
*/
}


// Derived from RG2's MidiMakeRestList in editor/src/MidiIn.c.

// Create a list of durations, totalling (as close as possible) the
// given duration, such that each is an exact note duration and the
// notes are the proper sort for the time signature.  start is the
// elapsed duration since the beginning of the bar (or of the last
// beat); for use independent of a particular bar, pass zero.

// Currently uses no note-durations longer than a dotted-crotchet; for
// general use in /2 time, this is a defect

vector<int> Note::getNoteDurationList(int start, int duration,
                                      const TimeSignature &ts)
{
    int toNextBeat;
    int beatDuration = ts.getBeatDuration();
    vector<int> v;

    toNextBeat = beatDuration - (start % beatDuration);
               
    if (toNextBeat > duration) {
        makeTimeListSub(duration, ts.isDotted(), v);
    } else {
        // first fill up to the next crotchet (or, in 6/8 or some
        // other such time, the next dotted crotchet); then fill in
        // crotchet or dotted-crotchet leaps until the end of the
        // section needing filling
        makeTimeListSub(toNextBeat, ts.isDotted(), v);
        makeTimeListSub(duration - toNextBeat, ts.isDotted(), v);
    }

    return v;
}


// Derived from RG2's MidiMakeRestListSub in editor/src/MidiIn.c.

void Note::makeTimeListSub(int t, bool dotted, vector<int> &v)
    // (we append to v, it's expected to have stuff in it already)
{
    assert(t >= 0);

    if (t < m_shortestTime) return;
    int current;

    if ((current = (dotted ? m_dottedCrotchetTime : m_crotchetTime)) <= t) {
        v.push_back(current);
        makeTimeListSub(t - current, dotted, v);
        return;
    }
               
    current = m_shortestTime;
    for (int tag = Shortest + 1; tag <= Crotchet; ++tag) {
        int next = Note(tag).getDuration();
        if (next > t) {
            v.push_back(current);
            makeTimeListSub(t - current, dotted, v);
            return;
        }
        current = next;
    }
               
    // should only be reached in dotted time for lengths between
    // crotchet and dotted crotchet:

    current = m_crotchetTime;
    v.push_back(current);
    makeTimeListSub(t - current, dotted, v);
}

const string TimeSignature::EventType = "timesignature";
const string TimeSignature::NumeratorPropertyName = "numerator";
const string TimeSignature::DenominatorPropertyName = "denominator";
const TimeSignature TimeSignature::DefaultTimeSignature = TimeSignature(4, 4);

TimeSignature::TimeSignature()
    : m_numerator(DefaultTimeSignature.m_numerator),
      m_denominator(DefaultTimeSignature.m_denominator)
{
}

TimeSignature::TimeSignature(int numerator, int denominator) throw (BadTimeSignature)
    : m_numerator(numerator), m_denominator(denominator)
{
        //!!! check, and throw BadTimeSignature if appropriate
}

TimeSignature::TimeSignature(const Event &e)
    throw (Event::NoData, Event::BadType, BadTimeSignature)
{
    if (e.getType() != EventType) {
        throw Event::BadType();
    }
    m_numerator = e.get<Int>(NumeratorPropertyName);
    m_denominator = e.get<Int>(DenominatorPropertyName);
    //!!! check, and throw BadTimeSignature if appropriate
}

TimeSignature::TimeSignature(const TimeSignature &ts)
    : m_numerator(ts.m_numerator),
      m_denominator(ts.m_denominator)
{
}

TimeSignature::~TimeSignature()
{
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
    return (m_numerator % 3 == 0 &&
            getBarDuration() >= Note(Note::Crotchet, true).getDuration());
}

int TimeSignature::getBeatDuration() const
{
    if (isDotted()) {
        return (getUnitDuration() * 3) / 2; //!!! is this always correct?
    } else {
        return  getUnitDuration();
    }
}

TimeSignature::EventsSet*
TimeSignature::getTimeIntervalAsRests(int startTime,
                                      int duration) const
{
    EventsSet *events = new EventsSet;
    
    int time = startTime,
        unitDuration = getUnitDuration();

    for (int len = 0; len < duration; ++len, time += unitDuration) {
        Event *e = new Event;
        e->setType("rest");
        e->setDuration(unitDuration);
        e->setAbsoluteTime(time);
        events->push_back(e);
    }

    return events;
}

TimeSignature::EventsSet*
TimeSignature::getBarAsRests(int startTime) const
{
    return getTimeIntervalAsRests(startTime, getBarDuration());
}


} // close namespace
