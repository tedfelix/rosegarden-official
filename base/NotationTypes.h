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

#ifndef _NOTATION_TYPES_H_
#define _NOTATION_TYPES_H_

#include <list>

#include "Event.h"
#include "Instrument.h"

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
 * Accidentals are stored in the event as string properties, purely
 * for clarity.  (They aren't manipulated _all_ that often, so this
 * probably isn't a great inefficiency.)  Originally we used an enum
 * for the Accidental type with conversion functions to and from
 * strings, but making Accidental a string seems simpler.
 */

typedef std::string Accidental;

namespace Accidentals
{
    extern const Accidental NoAccidental;
    extern const Accidental Sharp;
    extern const Accidental Flat;
    extern const Accidental Natural;
    extern const Accidental DoubleSharp;
    extern const Accidental DoubleFlat;
}


/**
 * Marks, like Accidentals, are stored in the event as string properties.
 */

typedef std::string Mark;
  
namespace Marks
{
    extern const Mark NoMark;         // " "
    extern const Mark Accent;         // ">"
    extern const Mark Tenuto;         // "-"  ("legato" in RG2.1)
    extern const Mark Staccato;       // "."
    extern const Mark Sforzando;      // "sf"
    extern const Mark Rinforzando;    // "rf"
    extern const Mark Trill;          // "tr"
    extern const Mark Turn;           // "~"
    extern const Mark Pause;          // aka "fermata"
    extern const Mark UpBow;          // "v"
    extern const Mark DownBow;        // a square with the bottom side missing

    /**
     * Given a string, return a mark that will be recognised as a
     * text mark containing that string.  For example, the Sforzando
     * mark is actually defined as getTextMark("sf").
     */
    extern Mark getTextMark(std::string text);

    /**
     * Return true if the given mark is a text mark.
     */
    extern bool isTextMark(Mark mark);

    /**
     * Extract the string from a text mark.
     */
    extern std::string getTextFromMark(Mark mark);
}


/**
 * Clefs are represented as one of a set of standard strings, stored
 * within a clef Event.  The Clef class defines those standards and
 * provides a few bits of information about the clefs.
 */

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

    bool operator==(const Clef &c) const {
	return c.m_clef == m_clef;
    }

    bool operator!=(const Clef &c) const {
	return !(c == *this);
    }

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

    /**
     * Return the height-on-staff (in NotationDisplayPitch terminology)
     * of the clef's axis -- the line around which the clef is drawn.
     */
    int getAxisHeight() const;

    typedef std::vector<Clef> ClefList;

    /**
     * Return all the clefs, in ascending order of pitch
     */
    static ClefList getClefs();

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_clef;
};


struct eqstring {
    bool operator() (const std::string &s1, const std::string &s2) const {
        return s1 == s2;
    }
};

struct hashstring {
    static __HASH_NS::hash<const char *> _H;
    size_t operator() (const std::string &s) const { return _H(s.c_str()); }
};

#if !defined(__GNUC__) || __GNUC__ < 3
__HASH_NS::hash<const char *> hashstring::_H;
#endif

/**
 * All we store in a key Event is the name of the key.  A Key object
 * can be constructed from such an Event or just from its name, and
 * will return all the properties of the key.  The Key class also
 * provides some useful mechanisms for getting information about and
 * transposing between keys.
 */

class Key {
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName KeyPropertyName;
    static const Key DefaultKey;
    struct BadKeyName { };
    struct BadKeySpec { };

    /// Construct the default key (C major).
    Key();

    /// Construct a key from the given Event of type Key::EventType.
    Key(const Event &e)
        /* throw (Event::NoData, Event::BadType, BadKeyName) */;

    /// Construct the named key.
    Key(const std::string &name)
        /* throw (BadKeyName) */;

    /// Construct a key from signature and mode.
    Key(int accidentalCount, bool isSharp, bool isMinor)
        /* throw (BadKeySpec) */;

    /// Construct the key with the given tonic and mode. (Ambiguous.)
    Key(int tonicPitch, bool isMinor)
        /* throw (BadKeySpec) */;

    Key(const Key &kc);

    ~Key() {
	delete m_accidentalHeights;
    }

    Key &operator=(const Key &kc);

    bool operator==(const Key &k) const {
	return k.m_name == m_name;
    }

    /**
     * Return true if this is a minor key.  Unlike in RG2.1,
     * we distinguish betwenn major and minor keys with the
     * same signature.
     */
    bool isMinor() const {
        return m_keyDetailMap[m_name].m_minor;
    }

    /**
     * Return true if this key's signature is made up of
     * sharps, false if flats.
     */
    bool isSharp() const {
        return m_keyDetailMap[m_name].m_sharps;
    }

    /**
     * Return the pitch of the tonic note in this key, as a
     * MIDI (or RG4) pitch modulo 12 (i.e. in the range 0-11).
     * This is the pitch of the note named in the key's name,
     * e.g. 0 for the C in C major.
     */
    int getTonicPitch() const {
        return m_keyDetailMap[m_name].m_tonicPitch;
    }

    /**
     * Return the number of sharps or flats in the key's signature.
     */
    int getAccidentalCount() const {
        return m_keyDetailMap[m_name].m_sharpCount;
    }

    /**
     * Return the key with the same signature but different
     * major/minor mode.  For example if called on C major, 
     * returns A minor.
     */
    Key getEquivalent() const {
        return Key(m_keyDetailMap[m_name].m_equivalence);
    }

    /**
     * Return the name of the key, in a human-readable form
     * also suitable for passing to the Key constructor.
     */
    std::string getName() const {
        return m_name;
    }

    /**
     * Return the name of the key, in the form used by RG2.1.
     */
    std::string getRosegarden2Name() const {
        return m_keyDetailMap[m_name].m_rg2name;
    }

    /**
     * Return the accidental at the given height-on-staff
     * (in NotationDisplayPitch terminology) in the given clef.
     */
    Accidental getAccidentalAtHeight(int height, const Clef &clef) const;

    /**
     * Return the heights-on-staff (in NotationDisplayPitch
     * terminology) of all accidentals in the key's signature,
     * in the given clef.
     */
    std::vector<int> getAccidentalHeights(const Clef &clef) const;

    /**
     * Return the result of applying this key to the given
     * pitch, that is, modifying the pitch so that it has the
     * same status in terms of accidentals as it had when
     * found in the given previous key.
     */
    int convertFrom(int pitch, const Key &previousKey,
		    const Accidental &explicitAccidental =
		    Accidentals::NoAccidental) const;

    /**
     * Return the result of transposing the given pitch into
     * this key, that is, modifying the pitch by the difference
     * between the tonic pitches of this and the given previous
     * key.
     */
    int transposeFrom(int pitch, const Key &previousKey) const;

    /**
     * Reduce a height-on-staff to a single octave, so that it
     * can be compared against the accidental heights returned
     * by the preceding method.
     */
    static inline unsigned int canonicalHeight(int height) {
	return (height > 0) ? (height % 7) : ((7 - (-height % 7)) % 7);
    }

    typedef std::vector<Key> KeySet;

    /**
     * Return all the keys in the given major/minor mode, in
     * no particular order.
     */
    static KeySet getKeys(bool minor = false);


    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_name;
    mutable std::vector<int> *m_accidentalHeights;

    struct KeyDetails {
        bool   m_sharps;
        bool   m_minor;
        int    m_sharpCount;
        std::string m_equivalence;
        std::string m_rg2name;
	int    m_tonicPitch;

        KeyDetails(); // ctor needed in order to live in a hash_map

        KeyDetails(bool sharps, bool minor, int sharpCount,
                   std::string equivalence, std::string rg2name,
				   int m_tonicPitch);

        KeyDetails(const KeyDetails &d);

        KeyDetails &operator=(const KeyDetails &d);
    };


    typedef __HASH_NS::hash_map<std::string, KeyDetails, hashstring, eqstring>
        KeyDetailMap;
    static KeyDetailMap m_keyDetailMap;
    static void checkMap();
    void checkAccidentalHeights() const;

};


/**
 * Indication is a collective name for graphical marks that span a
 * series of events, such as slurs, ties, dynamic marks etc.  These
 * are stored in indication Events with a type and duration.  The
 * Indication class gives a basic set of indication types.
 */

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
 * Definitions for use in the text Event type.
 */

class Text
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName TextPropertyName;
    static const PropertyName TextTypePropertyName;

    static const std::string UnspecifiedType;
    static const std::string StaffName;
    static const std::string ChordName;
    static const std::string KeyName;
    static const std::string Lyric;
    static const std::string Dynamic;
    static const std::string Direction;
    static const std::string LocalDirection;
    static const std::string Tempo;
    static const std::string LocalTempo;

    Text(const Event &e)
	/* throw (Event::NoData, Event::BadType) */;
    Text(const std::string &text,
	 const std::string &textType = UnspecifiedType);
    ~Text();

    std::string getText() const { return m_text; }
    std::string getTextType() const { return m_type; }

    /**
     * Return those text types that the user should be allowed to
     * specify directly and visually
     */
    static std::vector<std::string> getUserStyles();

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_text;
    std::string m_type;
};

class PitchBend
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;

    static const PropertyName MSBPropertyName;
    static const PropertyName LSBPropertyName;

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

    PitchBend(Rosegarden::MidiByte msb, Rosegarden::MidiByte lsb);
    ~PitchBend();

private:
    Rosegarden::MidiByte m_msb;
    Rosegarden::MidiByte m_lsb;
};

class Controller
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;

    static const PropertyName Data1PropertyName;
    static const PropertyName Data2PropertyName;

    static const std::string UnspecifiedType;
    static const std::string Modulation;
    static const std::string Pan;

    Controller(const std::string &t,
               Rosegarden::MidiByte data1,
               Rosegarden::MidiByte data2);
    ~Controller();

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_type;

    Rosegarden::MidiByte m_data1;
    Rosegarden::MidiByte m_data2;

};


/**
 * NotationDisplayPitch stores a note's pitch in terms of the position
 * of the note on the staff and its associated accidental, and
 * converts these values to and from performance (MIDI) pitches.
 *
 * Rationale: When we insert a note, we need to query the height of the
 * staff line next to which it's being inserted, then translate this
 * back to raw pitch according to the clef in force at the x-coordinate
 * at which the note is inserted.  For display, we translate from raw
 * pitch using both the clef and the key in force.
 *
 * Whether an accidental should be displayed or not depends on the
 * current key, on whether we've already shown the same accidental for
 * that pitch in the same bar, on whether the note event explicitly
 * requests an accidental...  All we calculate here is whether the
 * pitch "should" have an accidental, not whether it really will
 * (e.g. if the accidental has already appeared).
 *
 * (See also docs/discussion/units.txt for explanation of pitch units.)
 */

class NotationDisplayPitch
{
public:
    /**
     * Construct a NotationDisplayPitch containing the given staff
     * height and accidental
     */
    NotationDisplayPitch(int heightOnStaff,
			 const Accidental &accidental);

    /**
     * Construct a NotationDisplayPitch containing the height and
     * accidental to which the given performance pitch corresponds
     * in the given clef and key
     */
    NotationDisplayPitch(int pitch, const Clef &clef, const Key &key,
                         const Accidental &explicitAccidental = 
			 Accidentals::NoAccidental);

    int getHeightOnStaff() const { return m_heightOnStaff; }
    Accidental getAccidental() const { return m_accidental; }

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
     *
     * If inclOctave is false, this will return C, Bb, etc.
     */
    std::string getAsString(const Clef &clef, const Key &key,
			    bool inclOctave=true) const;

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


    /**
     * Create a Note object of the given type, representing a
     * particular sort of duration.  Note objects are strictly
     * durational; they don't represent pitch, and may be as
     * relevant to rests as actual notes.
     */
    Note(Type type, int dots = 0) :
	m_type(type < Shortest ? Shortest :
	       type >  Longest ?  Longest :
	       type),
	m_dots(dots) { }

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

    /**
     * Return the duration of this note type.
     */
    timeT getDuration()  const {
        return m_dots ? getDurationAux() : (m_shortestTime * (1 << m_type));
    }

    /**
     * Get the English name of a note (e.g. crotchet, dotted semiquaver).
     * The default arguments are the values of the note on which the
     * method is called; non-default arguments specify another note type.
     */
    std::string getEnglishName(Type type = -1, int dots = 0) const;

    /**
     * Get the US name of a note (e.g. quarter note, dotted sixteenth note).
     * The default arguments are the values of the note on which the
     * method is called; non-default arguments specify another note type.
     */
    std::string getAmericanName(Type type = -1, int dots = 0) const;

    /**
     * Get the short US name of a note (e.g. quarter, dotted 16th).
     * The default arguments are the values of the note on which the
     * method is called; non-default arguments specify another note type.
     */
    std::string getShortName(Type type = -1, int dots = 0) const;

    /**
     * Get the reference name of a note, used to refer to toolbar pixmap
     * files and suchlike.  (e.g. crotchet, dotted-demisemi).
     * The default arguments are the values of the note on which the
     * method is called; non-default arguments specify another note type.
     */
    std::string getReferenceName(bool isRest = false,
				 Type type = -1, int dots = 0) const;

    /**
     * Return the Note whose duration is closest to (but shorter than or
     * equal to) the given duration, permitting at most maxDots dots.
     */
    static Note getNearestNote(timeT duration, int maxDots = 2);

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsNoteEvent(timeT absoluteTime, int pitch) const;

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsRestEvent(timeT absoluteTime) const;

  
private:
    Type m_type;
    int m_dots;

    timeT getDurationAux()  const;

    // a time & effort saving device; if changing this, change
    // TimeSignature::m_crotchetTime etc too
    static const timeT m_shortestTime;
};


/**
 * TimeSignature contains arithmetic methods relevant to time
 * signatures and bar durations, including code for splitting long
 * rest intervals into bite-sized chunks.  Although there is a time
 * signature Event type, these Events don't appear in regular Segments
 * but only in the Composition's reference segment.
 */

class TimeSignature
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName NumeratorPropertyName;
    static const PropertyName DenominatorPropertyName;
    static const PropertyName ShowAsCommonTimePropertyName;
    static const PropertyName IsHiddenPropertyName;
    static const TimeSignature DefaultTimeSignature;
    struct BadTimeSignature { };

    TimeSignature() :
        m_numerator(DefaultTimeSignature.m_numerator),
        m_denominator(DefaultTimeSignature.m_denominator),
	m_common(false), m_hidden(false) { }

    /**
     * Construct a TimeSignature object describing a time signature
     * with the given numerator and denominator.  If preferCommon
     * is true and the time signature is a common or cut-common
     * time, the constructed object will return true for isCommon;
     * if hidden is true, the time signature is intended not to
     * be displayed and isHidden will return true.
     */
    TimeSignature(int numerator, int denominator,
		  bool preferCommon = false, bool hidden = false)
        /* throw (BadTimeSignature) */;

    TimeSignature(const Event &e)
        /* throw (Event::NoData, Event::BadType, BadTimeSignature) */;
    
    TimeSignature(const TimeSignature &ts) :
        m_numerator(ts.m_numerator),
        m_denominator(ts.m_denominator),
	m_common(ts.m_common),
	m_hidden(ts.m_hidden) { }

    ~TimeSignature() { }

    TimeSignature &operator=(const TimeSignature &ts);

    int getNumerator()     const { return m_numerator; }
    int getDenominator()   const { return m_denominator; }
    
    bool isCommon()        const { return m_common; }
    bool isHidden()	   const { return m_hidden; }

    timeT getBarDuration() const;

    /**
     * Return the unit of the time signature.  This is the note
     * implied by the denominator.  For example, the unit of 4/4 time
     * is the crotchet, and that of 6/8 is the quaver.  (The numerator
     * of the time signature gives the number of units per bar.)
     */
    Note::Type getUnit()  const;

    /**
     * Return the duration of the unit of the time signature.
     * See also getUnit().  In most cases getBeatDuration() gives
     * a more meaningful value.
     */
    timeT getUnitDuration() const;

    /**
     * Return true if this time signature indicates dotted time.
     */
    bool isDotted() const;

    /**
     * Return the duration of the beat of the time signature.  For
     * example, the beat of 4/4 time is the crotchet, the same as its
     * unit, but that of 6/8 is the dotted crotchet (there are only
     * two beats in a 6/8 bar).  The beat therefore depends on whether
     * the signature indicates dotted or undotted time.
     */
    timeT getBeatDuration() const;

    /**
     * Return the number of beats in a complete bar.
     */
    int getBeatsPerBar()  const {
        return getBarDuration() / getBeatDuration();
    }

    /**
     * Get the "optimal" list of rest durations to make up a bar in
     * this time signature.
     */
    void getDurationListForBar(DurationList &dlist) const;

    /**
     * Get the "optimal" list of rest durations to make up a time
     * interval of the given total duration, starting at the given
     * offset after the start of a bar, assuming that the interval
     * is entirely in this time signature.
     */
    void getDurationListForInterval(DurationList &dlist,
                                    timeT intervalDuration,
                                    timeT startOffset = 0) const;

    /**
     * Get the level of emphasis for a position in a bar. 4 is lots
     * of emphasis, 0 is none.
     */
    int getEmphasisForTime(timeT offset);

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    int m_numerator;
    int m_denominator;

    bool m_common;
    bool m_hidden;

    mutable int  m_barDuration;
    mutable int  m_beatDuration;
    mutable int  m_beatDivisionDuration;
    mutable bool m_dotted;
    void setInternalDurations() const;

    // a time & effort saving device
    static const timeT m_crotchetTime;
    static const timeT m_dottedCrotchetTime;
};
 
}


#endif
