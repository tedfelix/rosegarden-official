// -*- c-basic-offset: 4 -*-


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

#ifndef _NOTATION_TYPES_H_
#define _NOTATION_TYPES_H_

#include <list>

#include "Event.h"

/*
 * NotationTypes.h
 *
 * This file contains definitions of several classes to assist in
 * creating and manipulating certain event types.  The classes are:
 * 
 *   Accidental
 *   Clef
 *   Key
 *   Indication
 *   NotationDisplayPitch
 *   Note
 *   TimeSignature
 *
 * The classes in this file are _not_ actually used for storing
 * events.  Events are always stored in Event objects (see Event.h).
 *
 * These classes are usually constructed on-the-fly when a particular
 * operation specific to a single sort of event is required, and
 * usually destroyed as soon as they go out of scope.  The most common
 * usages are for creating events (create an instance of one of these
 * classes with the data you require, then call getAsEvent on it), for
 * doing notation-related calculations from existing events (such as
 * the bar duration of a time signature), and for doing calculations
 * that are independent of any particular instance of an event (such
 * as the Note methods that calculate duration-related values without
 * reference to any specific pitch or other note-event properties; or
 * everything in NotationDisplayPitch).
 * 
 * This file also defines the event types and standard property names
 * for the basic events.
 */

namespace Rosegarden 
{

extern const int MIN_SUBORDERING;

typedef std::list<int> DurationList;

    
/**
 * Accidental is hardly worth making a class, but there are conversion
 * functions to and from strings available in NotationDisplayPitch.
 */

enum Accidental {
    NoAccidental, Sharp, Flat, Natural, DoubleSharp, DoubleFlat
};


enum Mark {
    NoMark, Dot, Legato, Accent, Sforzando, Rinforzando, Trill, Turn, Pause
};


// somewhat mechanical:

class Clef {
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName ClefPropertyName;
    static const Clef DefaultClef;
    struct BadClefName { };

    static const std::string Treble;
    static const std::string Tenor;
    static const std::string Alto;
    static const std::string Bass;

    Clef() : m_clef(DefaultClef.m_clef) { }

    Clef(const Event &e)
        /* throw (Event::NoData, Event::BadType, BadClefName) */;
    Clef(const std::string &s)
        /* throw (BadClefName) */;

    Clef(const Clef &c) : m_clef(c.m_clef) { }

    Clef &operator=(const Clef &c);

    ~Clef() { }

    std::string getClefType() const { return m_clef; }

    /** 
     * Return the number of semitones a pitch in the treble clef would
     * have to be lowered by in order to be drawn with the same height
     * and accidental in this clef
     */
    int getTransposition() const;

    /**
     * Return the octave component of getTransposition(), i.e. the number
     * of octaves difference in pitch between this clef and the treble
     */
    int getOctave() const;

    /**
     * Return the intra-octave component of getTransposition(), i.e. the
     * number of semitones this clef is distinct in pitch from the treble
     * besides the different in octaves
     */
    int getPitchOffset() const;

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_clef;
};


/**
     All we store in a key Event is the name of the key.  A Key object
     can be constructed from such an Event or just from its name, and
     will return all the properties of the key.
*/

struct eqstring {
    bool operator() (const std::string &s1, const std::string &s2) const {
        return s1 == s2;
    }
};

struct hashstring {
    static std::hash<const char *> _H;
    size_t operator() (const std::string &s) const { return _H(s.c_str()); }
};

std::hash<const char *> hashstring::_H;

class Key {
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName KeyPropertyName;
    static const Key DefaultKey;
    struct BadKeyName { };
    struct BadKeySpec { };

    Key();
    Key(const Event &e)
        /* throw (Event::NoData, Event::BadType, BadKeyName) */;
    Key(const std::string &name)
        /* throw (BadKeyName) */;
    Key(int accidentalCount, bool isSharp, bool isMinor)
        /* throw (BadKeySpec) */;
    Key(const Key &kc);

    ~Key() {
	delete m_accidentalHeights;
    }

    Key &operator=(const Key &kc);

    bool isMinor() const {
        return m_keyDetailMap[m_name].m_minor;
    }

    bool isSharp() const {
        return m_keyDetailMap[m_name].m_sharps;
    }

    int getAccidentalCount() const {
        return m_keyDetailMap[m_name].m_sharpCount;
    }

    Key getEquivalent() const { // e.g. called on C major, return A minor
        return Key(m_keyDetailMap[m_name].m_equivalence);
    }

    std::string getName() const {
        return m_name;
    }

    std::string getRosegarden2Name() const {
        return m_keyDetailMap[m_name].m_rg2name;
    }

    Accidental getAccidentalAtHeight(int height, const Clef &clef) const;

    /// staff positions of accidentals
    std::vector<int> getAccidentalHeights(const Clef &clef) const;

    /** to permit comparison of one height with another irrespective of
        octave, for key/accidental calculations &c */
    static inline unsigned int canonicalHeight(int height) {
	return (height > 0) ? (height % 7) : ((7 - (-height % 7)) % 7);
    }

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

    static std::vector<Key> getKeys(bool minor = false);

private:
    std::string m_name;
    mutable std::vector<int> *m_accidentalHeights;

    struct KeyDetails {
        bool   m_sharps;
        bool   m_minor;
        int    m_sharpCount;
        std::string m_equivalence;
        std::string m_rg2name;

        KeyDetails(); // ctor needed in order to live in a hash_map

        KeyDetails(bool sharps, bool minor, int sharpCount,
                   std::string equivalence, std::string rg2name);

        KeyDetails(const KeyDetails &d);

        KeyDetails &operator=(const KeyDetails &d);
    };

    typedef std::hash_map<std::string, KeyDetails, hashstring, eqstring>
        KeyDetailMap;
    static KeyDetailMap m_keyDetailMap;
    static void checkMap();
    void checkAccidentalHeights() const;
};


class Indication
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName IndicationTypePropertyName;
    static const PropertyName IndicationDurationPropertyName;
    struct BadIndicationName { };

    static const std::string Slur;
    static const std::string Crescendo;
    static const std::string Decrescendo;

    Indication(const Event &e)
        /* throw (Event::NoData, Event::BadType) */;
    Indication(const std::string &s, timeT indicationDuration)
	/* throw (BadIndicationName) */;

    Indication(const Indication &m) : m_indicationType(m.m_indicationType),
			  m_duration(m.m_duration) { }

    Indication &operator=(const Indication &m);

    ~Indication() { }

    std::string getIndicationType() const { return m_indicationType; }
    timeT getIndicationDuration() const { return m_duration; }

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_indicationType;
    timeT m_duration;
};



/**

  NotationDisplayPitch stores a note's pitch in terms of the position
  of the note on the staff and its associated accidental, and
  converts these values to and from performance (MIDI) pitches.

  Rationale: When we insert a note, we need to query the height of the
  staff line next to which it's being inserted, then translate this
  back to raw pitch according to the clef in force at the x-coordinate
  at which the note is inserted.  For display, we translate from raw
  pitch using both the clef and the key in force.

  Whether an accidental should be displayed or not depends on the
  current key, on whether we've already shown the same accidental for
  that pitch in the same bar, on whether the note event explicitly
  requests an accidental...  All we calculate here is whether the
  pitch "should" have an accidental, not whether it really will
  (e.g. if the accidental has already appeared).

  (See also docs/discussion/units.txt for explanation of pitch units.)

*/

class NotationDisplayPitch
{
public:
    /**
     * Construct a NotationDisplayPitch containing the given staff
     * height and accidental
     */
    NotationDisplayPitch(int heightOnStaff, Accidental accidental);

    /**
     * Construct a NotationDisplayPitch containing the height and
     * accidental to which the given performance pitch corresponds
     * in the given clef and key
     */
    NotationDisplayPitch(int pitch, const Clef &clef, const Key &key,
                         Accidental explicitAccidental = NoAccidental);

    int        getHeightOnStaff() const { return m_heightOnStaff; }
    Accidental getAccidental()    const { return m_accidental; }

    /**
     * Calculate and return the performance (MIDI) pitch 
     * corresponding to the stored height and accidental, in the
     * given clef and key
     */
    int getPerformancePitch(const Clef &clef, const Key &key) const;

    /**
     * Calculate and return the performance (MIDI) pitch
     * corresponding to the stored height and accidental,
     * interpreting them as Rosegarden-2.1-style values (for
     * backward compatibility use), in the given clef and key
     */
    int getPerformancePitchFromRG21Pitch(const Clef &clef,
					 const Key &key) const;

    /**
     * Return the stored pitch as a string (C4, Bb2, etc...)
     * according to http://www.harmony-central.com/MIDI/Doc/table2.html
     */
    std::string getAsString(const Clef &clef, const Key &key) const;

    struct BadAccidental { };

    /**
     * Utility functions for accidental names; can throw BadAccidental
     */
    static std::string getAccidentalName  (Accidental accidental);
    static Accidental  getAccidentalByName(const std::string &name);

private:
    int m_heightOnStaff;
    Accidental m_accidental;

    void rawPitchToDisplayPitch(int, const Clef &, const Key &,
                                int &, Accidental &) const;
    void displayPitchToRawPitch(int, Accidental, const Clef &, const Key &,
                                int &, bool ignoreOffset = false) const;
};


class TimeSignature;


/**
 * The Note class represents note durations only, not pitch or
 * accidental; it's therefore just as relevant to rest events as to
 * note events
 */

class Note
{
public:
    static const std::string EventType;
    static const std::string EventRestType;

    typedef int Type; // not an enum, too much arithmetic at stake

    struct BadType {
        std::string type;
        BadType(std::string t = "") : type(t) { }
    };

    struct TooManyDots { };
    
    struct MalformedNoteName {
        std::string name;
        std::string reason;
        MalformedNoteName(std::string n, std::string r) :
            name(n), reason(r) { }
    };


    // define both sorts of names; some people prefer the American
    // names, but I just can't remember which of them is which

    static const Type

        SixtyFourthNote     = 0,
        ThirtySecondNote    = 1,
        SixteenthNote       = 2,
        EighthNote          = 3,
        QuarterNote         = 4,
        HalfNote            = 5,
        WholeNote           = 6,
        DoubleWholeNote     = 7,

        Hemidemisemiquaver  = 0,
        Demisemiquaver      = 1,
        Semiquaver          = 2,
        Quaver              = 3,
        Crotchet            = 4,
        Minim               = 5,
        Semibreve           = 6,
        Breve               = 7,

        Shortest            = 0,
        Longest             = 7;


    Note(Type type, int dots = 0) /* throw (BadType, TooManyDots) */ :
    m_type(type), m_dots(dots) {
        
        // I'm not sure how this'll interact with compiler
        // optimisation -- I'm hoping throw() is implemented as a
        // single out-of-line function call, in which case it'll be
        // fine to inline this constructor in code like
        // "Note(Note::Crotchet, false).getDuration()".  That's quite
        // important; I'd even consider not throwing if it'll screw
        // inlining

        if (m_type < Shortest || m_type > Longest) throw BadType();
    
        // We don't permit dotted hemis, double-dotted demis etc
        // because we can't represent notes short enough to make up
        // the rest of the beat (as we have no notes shorter than a
        // hemi).  And if we got to double-dotted hemis, triple-dotted
        // demis etc, we couldn't even represent their durations in
        // our duration units.  Still, throwing this exception is
        // probably going to cause mayhem -- might be happier just
        // setting m_dots back to m_type if it's found to be larger

        if (m_dots > m_type) throw TooManyDots();
    }

    Note(const std::string &s)
        /* throw (BadType, MalformedNoteName) */;
    Note(const Note &n) : m_type(n.m_type), m_dots(n.m_dots) { }
    ~Note() { }

    Note &operator=(const Note &n);

    Type getNoteType()  const { return m_type; }

    bool isFilled()     const { return m_type <= Crotchet; }
    bool hasStem()      const { return m_type <= Minim; }
    int  getDots()      const { return m_dots; }
    int  getFlagCount() const {
	return (m_type >= Crotchet) ? 0 : (Crotchet - m_type);
    }

    timeT getDuration()  const {
        return m_dots ? getDurationAux() : (m_shortestTime * (1 << m_type));
    }

    // these default to whatever I am:
    std::string getEnglishName (Type type = -1, int dots = 0) const;
    std::string getAmericanName(Type type = -1, int dots = 0) const;
    std::string getShortName   (Type type = -1, int dots = 0) const;

    static Note getNearestNote(int duration, int maxDots = 2);

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsNoteEvent(int pitch, timeT absoluteTime) const;

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsRestEvent(timeT absoluteTime) const;
  
private:
    Type m_type;
    int m_dots;

    timeT getDurationAux()  const;

    // a time & effort saving device; if changing this, change
    // TimeSignature::m_crotchetTime etc too
    static const int m_shortestTime;
};


/**
 * TimeSignature contains arithmetic methods relevant to time
 * signatures and bar durations, including code for splitting long
 * rest intervals into bite-sized chunks.
 */

class TimeSignature
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName NumeratorPropertyName;
    static const PropertyName DenominatorPropertyName;
    static const TimeSignature DefaultTimeSignature;
    struct BadTimeSignature { };

    TimeSignature() :
        m_numerator(DefaultTimeSignature.m_numerator),
        m_denominator(DefaultTimeSignature.m_denominator) { }

    TimeSignature(int numerator, int denominator)
        /* throw (BadTimeSignature) */;

    TimeSignature(const Event &e)
        /* throw (Event::NoData, Event::BadType, BadTimeSignature) */;
    
    TimeSignature(const TimeSignature &ts) :
        m_numerator(ts.m_numerator),
        m_denominator(ts.m_denominator) { }

    ~TimeSignature() { }

    TimeSignature &operator=(const TimeSignature &ts);

    int getNumerator()    const { return m_numerator; }
    int getDenominator()  const { return m_denominator; }
    int getBarDuration()  const { return m_numerator * getUnitDuration(); }

    // We say the "unit" of the time is the duration of the note
    // implied by the denominator.  For example, the unit of 4/4 time
    // is the crotchet, and that of 6/8 is the quaver.  The numerator
    // of the time signature gives the number of units per bar.

    Note::Type getUnit()  const;
    int getUnitDuration() const { return 6 * (64 / m_denominator); }

    // The "beat" of the time depends on whether the signature implies
    // dotted or undotted time.  The beat of 4/4 time is the crotchet,
    // the same as its unit, but that of 6/8 is the dotted crotchet
    // (there are only two beats in a 6/8 bar).  We don't worry
    // ourselves with more complex times (7/16 anyone?) at the moment

    bool isDotted() const;

    int getBeatDuration() const;
    int getBeatsPerBar()  const {
        return getBarDuration() / getBeatDuration();
    }

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

    // get the "optimal" list of rest durations to make up a bar of
    // this time signature

    void getDurationListForBar(DurationList &dlist) const;

    // get the "optimal" list of rest durations to make up a time
    // interval of the given total duration, starting at the given
    // offset after the start of a bar

    void getDurationListForInterval(DurationList &dlist,
                                    int intervalDuration,
                                    int startOffset = 0) const;

private:
    int m_numerator;
    int m_denominator;

    void getDurationListForShortInterval(DurationList &dlist,
                                         int intervalDuration,
                                         int startOffset = 0) const;

    void getDurationListAux(DurationList &dlist, int duration,
                            bool isLeadIn) const;

    // a time & effort saving device
    static const int m_crotchetTime;
    static const int m_dottedCrotchetTime;
};
 
}


#endif
