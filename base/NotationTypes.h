
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

namespace Rosegarden 
{

typedef std::vector<int> DurationList;

    
enum Accidental {
    NoAccidental, Sharp, Flat, Natural, DoubleSharp, DoubleFlat
};


// somewhat mechanical:

class Clef {
public:
    static const std::string EventType;
    static const std::string ClefPropertyName;
    static const Clef DefaultClef;
    struct BadClefName { };

    static const std::string Treble;
    static const std::string Tenor;
    static const std::string Alto;
    static const std::string Bass;

    Clef();
    Clef(const Event &e); // throw (Event::NoData, Event::BadType, BadClefName);
    Clef(const std::string &s); // throw (BadClefName);
    Clef(const Clef &c);

    Clef &operator=(const Clef &c);

    virtual ~Clef();

    std::string getClefType() const { return m_clef; }

    int getOctave() const;

    int getPitchOffset() const;

private:
    std::string m_clef;
};


/*
  -- Key

     All we store in a key Event is the name of the key.  A Key object
     can be constructed from such an Event or just from its name, and
     will return all the properties of the key.
*/

class Key {
public:
    static const std::string EventType;
    static const std::string KeyPropertyName;
    static const Key DefaultKey;
    struct BadKeyName { };

    Key();
    Key(const Event &e); // throw (Event::NoData, Event::BadType, BadKeyName);
    Key(const std::string &name); // throw (BadKeyName);
    Key(const Key &kc);
    virtual ~Key();

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

    // staff positions of accidentals
    std::vector<int> getAccidentalHeights(const Clef &clef) const;

    // to permit comparison of one height with another irrespective of
    // octave, for key/accidental calculations &c
    static inline unsigned int canonicalHeight(int height) {
	return (height > 0) ? (height % 7) : ((7 - (-height % 7)) % 7);
    }

    Event getAsEvent() const;

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

/*
  -- Pitch

     Events store pitch values using the MIDI pitch scale and as
     simple integers.  These are fixed-frequency pitches, independent
     of clef and key.  Adding 12 to a pitch increments it by one
     octave; pitch 60 is the treble-clef middle C.  (Previous rewrites
     have considered using double the MIDI pitch so as to allow
     quarter-tones; this time let's go for the simpler option as if we
     ever want quarter-tones we can always code them using special
     Event properties.)
     
     For notation purposes we need a display pitch, which is a
     composite of height on the staff plus accidental.  The
     correspondence between display pitch and raw pitch depends on the
     clef and key.  For height on the staff, we use the RG2.1
     convention of 0 = bottom line, 1 = gap between bottom two lines
     and so on up to 8 = top line.  Negative heights are below the
     staff (i.e. on leger lines), heights over 8 are above.

     When we insert a note, we need to query the height of the staff
     line next to which it's being inserted, then translate this back
     to raw pitch according to the clef in force at the x-coordinate
     at which the note is inserted.  For display, we translate from
     raw pitch using both the clef and the key in force.

     Whether an accidental should be displayed or not depends on the
     current key, on whether we've already shown the same accidental
     for that pitch in the same bar, on whether the note event
     explicitly requests an accidental...  All we calculate here is
     whether the pitch "should" have an accidental, not whether it
     really will (e.g. if the accidental has already appeared).

     If we ever see pitch as a plain integer, we assume it's a raw
     internal pitch.
*/

class NotationDisplayPitch
{
public:
    NotationDisplayPitch(int pitch, const Clef &clef, const Key &key);
    NotationDisplayPitch(int heightOnStaff, Accidental accidental);

    int        getHeightOnStaff() const { return m_heightOnStaff; }
    Accidental getAccidental()    const { return m_accidental; }

    int getPerformancePitch(const Clef &clef, const Key &key) const;

    /**
     * Returns the pitch as a string (C4, Bb2, etc...)
     * according to http://www.harmony-central.com/MIDI/Doc/table2.html
     */
    std::string getAsString(const Clef &clef, const Key &key) const;

private:
    int m_heightOnStaff;
    Accidental m_accidental;

    void rawPitchToDisplayPitch(int, const Clef &, const Key &,
                                int &, Accidental &) const;
    void displayPitchToRawPitch(int, Accidental, const Clef &, const Key &,
                                int &) const;
};


/*
  -- Duration

     Events store duration as simple integers where 6 units = one
     hemidemisemiquaver (and 1 unit = 4 MIDI clocks).

*/     

class TimeSignature;

class Note
{
public:
    static const std::string EventType;
    static const std::string NotePropertyName;
    
    typedef int Type; // not an enum, too much arithmetic at stake

    struct BadType {
        std::string type;
        BadType(std::string t = "") : type(t) { }
    };

    struct TooManyDots { };


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


    Note(Type type, int dots = 0); // throw (BadType, TooManyDots);
    Note(const std::string &s); // throw (BadType);
    Note(const Note &);
    virtual ~Note();

    Note &operator=(const Note &n);

    Type getNoteType()  const { return m_type; }

    bool isFilled()     const { return m_type <= Crotchet; }
    bool isStalked()    const { return m_type <= Minim; }
    int  getDots()      const { return m_dots; }
    int  getTailCount() const {
	return (m_type >= Crotchet) ? 0 : (Crotchet - m_type);
    }

    int  getDuration()  const;

    // these default to whatever I am:
    std::string getEnglishName (Type type = -1, int dots = 0) const;
    std::string getAmericanName(Type type = -1, int dots = 0) const;
    std::string getShortName   (Type type = -1, int dots = 0) const;

    static Note getNearestNote(int duration, int maxDots = 2);
  
private:
    Type m_type;
    int m_dots;

    // a time & effort saving device; if changing this, change
    // TimeSignature::m_crotchetTime etc too
    static const int m_shortestTime;
};


class TimeSignature
{
public:
    static const std::string EventType;
    static const std::string NumeratorPropertyName;
    static const std::string DenominatorPropertyName;
    static const TimeSignature DefaultTimeSignature;
    struct BadTimeSignature { };

    TimeSignature();

    TimeSignature(int numerator, int denominator);
//         throw (BadTimeSignature);

    TimeSignature(const Event &e);
//         throw (Event::NoData, Event::BadType, BadTimeSignature);
    
    TimeSignature(const TimeSignature &ts);

    virtual ~TimeSignature();

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

/*
    static DurationList getNoteDurationList(int start, int duration,
                                            const TimeSignature &ts);
*/

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

    void getDurationListAux(DurationList &dlist, int duration) const;

    // a time & effort saving device
    static const int m_crotchetTime;
    static const int m_dottedCrotchetTime;
};
 
}


#endif
