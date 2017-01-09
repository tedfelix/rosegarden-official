/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTATION_TYPES_H
#define RG_NOTATION_TYPES_H

#include <list>
#include <map>

#include <rosegardenprivate_export.h>

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
 *   Pitch
 *   Note
 *   TimeSignature
 *   AccidentalTable
 *   Symbol
 *   GeneratedRegion
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
 * everything in Pitch).
 * 
 * This file also defines the event types and standard property names
 * for the basic events.
 */

namespace Rosegarden 
{

extern ROSEGARDENPRIVATE_EXPORT const int MIN_SUBORDERING;

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
    extern ROSEGARDENPRIVATE_EXPORT const Accidental NoAccidental;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental Sharp;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental Flat;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental Natural;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental DoubleSharp;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental DoubleFlat;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental QuarterFlat;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental ThreeQuarterFlat;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental QuarterSharp;
    extern ROSEGARDENPRIVATE_EXPORT const Accidental ThreeQuarterSharp;

    typedef std::vector<Accidental> AccidentalList;

	/** 
	 * When no accidental is specified for a pitch, there are several
	 * strategies to determine what accidental to display for an 
	 * out-of-key pitch
	 */
	enum NoAccidentalStrategy {
		/** always use sharps */
		UseSharps,
		/** always use flats */
		UseFlats,
		/** always use sharps or always use flats depending on of what
		 * type of accidentals the current key is made up */ 
		UseKeySharpness,
		/** use the most likely accidental for this key */
		UseKey
	};

    /**
     * Get the predefined accidentals (i.e. the ones listed above)
     * in their defined order.
     */
    extern ROSEGARDENPRIVATE_EXPORT AccidentalList getStandardAccidentals();

    /**
     * Get the change in pitch resulting from an accidental: -1 for
     * flat, 2 for double-sharp, 0 for natural or NoAccidental etc.
     * This is not as useful as it may seem, as in reality the
     * effect of an accidental depends on the key as well -- see
     * the Key and Pitch classes.
     */
    extern ROSEGARDENPRIVATE_EXPORT int getPitchOffset(const Accidental &accidental);


    /**
     * Get the Accidental corresponding to a change in pitch: flat
     * for -1, double-sharp for 2, natural for 0 etc.
     *
     * Useful for tying to code that represents accidentals by 
     * their pitch change.
     */
    extern ROSEGARDENPRIVATE_EXPORT Accidental getAccidental(int pitchChange);
}


/**
 * Marks, like Accidentals, are stored in the event as string properties.
 */

typedef std::string Mark;
  
namespace Marks //!!! This would be better as a class, these days
{
    extern ROSEGARDENPRIVATE_EXPORT const Mark NoMark;         // " "

    extern ROSEGARDENPRIVATE_EXPORT const Mark Accent;         // ">"
    extern ROSEGARDENPRIVATE_EXPORT const Mark Tenuto;         // "-"  ("legato" in RG2.1)
    extern ROSEGARDENPRIVATE_EXPORT const Mark Staccato;       // "."
    extern ROSEGARDENPRIVATE_EXPORT const Mark Staccatissimo;  // "'"
    extern ROSEGARDENPRIVATE_EXPORT const Mark Marcato;        // "^"
    extern ROSEGARDENPRIVATE_EXPORT const Mark Open;           // "o"
    extern ROSEGARDENPRIVATE_EXPORT const Mark Stopped;        // "+"
    extern ROSEGARDENPRIVATE_EXPORT const Mark Harmonic;       // "°"

    extern ROSEGARDENPRIVATE_EXPORT const Mark Sforzando;      // "sf"
    extern ROSEGARDENPRIVATE_EXPORT const Mark Rinforzando;    // "rf"

    extern ROSEGARDENPRIVATE_EXPORT const Mark Trill;          // "tr"
    extern ROSEGARDENPRIVATE_EXPORT const Mark LongTrill;      // with wiggly line
    extern ROSEGARDENPRIVATE_EXPORT const Mark TrillLine;      // line on its own
    extern ROSEGARDENPRIVATE_EXPORT const Mark Turn;           // "~"

    extern ROSEGARDENPRIVATE_EXPORT const Mark Pause;          // aka "fermata"

    extern ROSEGARDENPRIVATE_EXPORT const Mark UpBow;          // "v"
    extern ROSEGARDENPRIVATE_EXPORT const Mark DownBow;        // a square with the bottom side missing

    extern ROSEGARDENPRIVATE_EXPORT const Mark Mordent;
    extern ROSEGARDENPRIVATE_EXPORT const Mark MordentInverted;
    extern ROSEGARDENPRIVATE_EXPORT const Mark MordentLong;
    extern ROSEGARDENPRIVATE_EXPORT const Mark MordentLongInverted;

    /**
     * Given a string, return a mark that will be recognised as a
     * text mark containing that string.  For example, the Sforzando
     * mark is actually defined as getTextMark("sf").
     */
    extern ROSEGARDENPRIVATE_EXPORT Mark getTextMark(std::string text);

    /**
     * Return true if the given mark is a text mark.
     */
    extern ROSEGARDENPRIVATE_EXPORT bool isTextMark(Mark mark);

    /**
     * Extract the string from a text mark.
     */
    extern ROSEGARDENPRIVATE_EXPORT std::string getTextFromMark(Mark mark);

    /**
     * Given a string, return a mark that will be recognised as a
     * fingering mark containing that string.  (We use a string
     * instead of a number to permit "fingering" marks containing
     * labels like "+".)
     */
    extern ROSEGARDENPRIVATE_EXPORT Mark getFingeringMark(std::string fingering);

    /**
     * Return true if the given mark is a fingering mark.
     */
    extern ROSEGARDENPRIVATE_EXPORT bool isFingeringMark(Mark mark);

    /**
     * Extract the string from a fingering mark.
     */
    extern ROSEGARDENPRIVATE_EXPORT std::string getFingeringFromMark(Mark mark);

    /**
     * Return true if the given mark makes sense when applied to a rest.
     */
    extern ROSEGARDENPRIVATE_EXPORT bool isApplicableToRests(Mark mark);

    /**
     * Extract the number of marks from an event.
     */
    extern ROSEGARDENPRIVATE_EXPORT int getMarkCount(const Event &e);

    /**
     * Extract the marks from an event.
     */
    extern ROSEGARDENPRIVATE_EXPORT std::vector<Mark> getMarks(const Event &e);

    /**
     * Return the first fingering mark on an event (or NoMark, if none).
     */
    extern ROSEGARDENPRIVATE_EXPORT Mark getFingeringMark(const Event &e);

    /**
     * Add a mark to an event.  If unique is true, add the mark only
     * if the event does not already have it (otherwise permit
     * multiple identical marks).
     */
    extern ROSEGARDENPRIVATE_EXPORT void addMark(Event &e, const Mark &mark, bool unique);

    /**
     * Remove a mark from an event.  Returns true if the mark was
     * there to remove.  If the mark was not unique, removes only
     * the first instance of it.
     */
    extern ROSEGARDENPRIVATE_EXPORT bool removeMark(Event &e, const Mark &mark);

    /**
     * Returns true if the event has the given mark.
     */
    extern ROSEGARDENPRIVATE_EXPORT bool hasMark(const Event &e, const Mark &mark);

    /**
     * Get the predefined marks (i.e. the ones listed above) in their
     * defined order.
     */
    extern ROSEGARDENPRIVATE_EXPORT std::vector<Mark> getStandardMarks();
}


/**
 * Clefs are represented as one of a set of standard strings, stored
 * within a clef Event.  The Clef class defines those standards and
 * provides a few bits of information about the clefs.
 */

class ROSEGARDENPRIVATE_EXPORT Clef
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName ClefPropertyName;
    static const PropertyName OctaveOffsetPropertyName;
    static const Clef DefaultClef;
    static const Clef UndefinedClef;
    typedef Exception BadClefName;

    static const std::string Treble;
    static const std::string French;
    static const std::string Soprano;
    static const std::string Mezzosoprano;
    static const std::string Alto;
    static const std::string Tenor;
    static const std::string Baritone;
    static const std::string Varbaritone;
    static const std::string Bass;
    static const std::string Subbass;
    static const std::string TwoBar;

    /**
     * Construct the default clef (treble).
     */
    Clef() : m_clef(DefaultClef.m_clef), m_octaveOffset(0) { }

    /**
     * Construct a Clef from the clef data in the given event.  If the
     * event is not of clef type or contains insufficient data, this
     * returns the default clef (with a warning).  You should normally
     * test Clef::isValid() to catch that before construction.
     */
    Clef(const Event &e);

    /**
     * Construct a Clef from the given data.  Throws a BadClefName
     * exception if the given string does not match one of the above
     * clef name constants.
     */
    Clef(const std::string &s, int octaveOffset = 0);

    Clef(const Clef &c) : m_clef(c.m_clef), m_octaveOffset(c.m_octaveOffset) {
    }

    Clef &operator=(const Clef &c);

    bool operator==(const Clef &c) const {
        return c.m_clef == m_clef && c.m_octaveOffset == m_octaveOffset;
    }

    bool operator!=(const Clef &c) const {
        return !(c == *this);
    }

    ~Clef() { }

    /**
     * Test whether the given event is a valid Clef event.
     */
    static bool isValid(const Event &e);

    /**
     * Return the basic clef type (Treble, French, Soprano, Mezzosoprano, Alto, Tenor, Baritone, Varbaritone, Bass, Subbass)
     */
    std::string getClefType() const { return m_clef; }

    /**
     * Return any additional octave offset, that is, return 1 for
     * a clef shifted an 8ve up, etc
     */
    int getOctaveOffset() const { return m_octaveOffset; }

    /** 
     * Return the number of semitones a pitch in the treble clef would
     * have to be lowered by in order to be drawn with the same height
     * and accidental in this clef
     */
    int getTranspose() const;

    /**
     * Return the octave component of getTranspose(), i.e. the number
     * of octaves difference in pitch between this clef and the treble
     */
    int getOctave() const;

    /**
     * Return the intra-octave component of getTranspose(), i.e. the
     * number of semitones this clef is distinct in pitch from the treble
     * besides the difference in octaves
     */
    int getPitchOffset() const;

    /**
     * Return the height-on-staff (in Pitch terminology)
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
    int m_octaveOffset;
};

/**
 * All we store in a key Event is the name of the key.  A Key object
 * can be constructed from such an Event or just from its name, and
 * will return all the properties of the key.  The Key class also
 * provides some useful mechanisms for getting information about and
 * transposing between keys.
 */

class ROSEGARDENPRIVATE_EXPORT Key
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName KeyPropertyName;
    static const Key DefaultKey;
    static const Key UndefinedKey;
    typedef Exception BadKeyName;
    typedef Exception BadKeySpec;

    /**
     * Construct the default key (C major).
     */
    Key();

    /**
     * Construct a Key from the key data in the given event.  If the
     * event is not of key type or contains insufficient data, this
     * returns the default key (with a warning).  You should normally
     * test Key::isValid() to catch that before construction.
     */
    Key(const Event &e);

    /**
     * Construct the named key.  Throws a BadKeyName exception if the
     * given string does not match one of the known key names.
     */
    Key(const std::string &name);

    /**
     * Construct a key from signature and mode.  May throw a
     * BadKeySpec exception.
     */
    Key(int accidentalCount, bool isSharp, bool isMinor);

    /**
     * Construct the key with the given tonic and mode. (Ambiguous.)
     * May throw a BadKeySpec exception.
     */
    Key(int tonicPitch, bool isMinor);

    Key(const Key &kc);

    ~Key() {
        delete m_accidentalHeights;
    }

    Key &operator=(const Key &kc);

    bool operator==(const Key &k) const {
        return k.m_name == m_name;
    }

    bool operator!=(const Key &k) const {
        return !(k == *this);
    }

    // We only use this for map, which doesn't need an intelligent
    // ordering.
    bool operator<(const Key &b) const
    { return this->getName() < b.getName(); }

    /**
     * Test whether the given event is a valid Key event.
     */
    static bool isValid(const Event &e);

    /**
     * Return true if this is a minor key.  Unlike in RG2.1,
     * we distinguish between major and minor keys with the
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
     * Return the name of the key, in the form used by X11 RG2.1.
     */
    std::string getRosegarden2Name() const {
        return m_keyDetailMap[m_name].m_rg2name;
    }

    /**
     * Return the accidental at the given height-on-staff
     * (in Pitch terminology) in the given clef.
     */
    Accidental getAccidentalAtHeight(int height, const Clef &clef) const;

    /**
     * Return the accidental for the the given number of steps
     * from the tonic. For example: for F major, step '3' is the
     * Bb, so getAccidentalForStep(3) will yield a Flat.
     */
    Accidental getAccidentalForStep(int steps) const;

    /**
     * Return the heights-on-staff (in Pitch
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

    typedef std::vector<Key> KeyList;

    /**
     * Return all the keys in the given major/minor mode, in
     * no particular order.
     */
    static KeyList getKeys(bool minor = false);


    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

    /**
     * Transpose this key by the specified interval given in pitch and steps
     * 
     * For example: transposing F major by a major triad (4,2) yields
     *  A major.
     */
    Key transpose(int pitchDelta, int heightDelta);

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

        KeyDetails(); // ctor needed in order to live in a map

        KeyDetails(bool sharps, bool minor, int sharpCount,
                   std::string equivalence, std::string rg2name,
                                   int m_tonicPitch);

        KeyDetails(const KeyDetails &d);

        KeyDetails &operator=(const KeyDetails &d);
    };


    typedef std::map<std::string, KeyDetails> KeyDetailMap;
    static KeyDetailMap m_keyDetailMap;
    static void checkMap();
    void checkAccidentalHeights() const;

};


/**
 * Indication is a collective name for graphical marks that span a
 * series of events, such as slurs, dynamic marks etc.  These are
 * stored in indication Events with a type and duration.  The
 * Indication class gives a basic set of indication types.
 */

class Indication
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName IndicationTypePropertyName;
    typedef Exception BadIndicationName;

    static const std::string Slur;
    static const std::string PhrasingSlur;
    static const std::string Crescendo;
    static const std::string Decrescendo;
    static const std::string Glissando;

    static const std::string QuindicesimaUp;
    static const std::string OttavaUp;
    static const std::string OttavaDown;
    static const std::string QuindicesimaDown;

    static const std::string TrillLine;

    static const std::string FigParameterChord;
    static const std::string Figuration;

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

    bool isOttavaType() const {
        return
            m_indicationType == QuindicesimaUp ||
            m_indicationType == OttavaUp ||
            m_indicationType == OttavaDown ||
            m_indicationType == QuindicesimaDown;
    }

    int getOttavaShift() const {
        return (m_indicationType == QuindicesimaUp ? 2 :
                m_indicationType == OttavaUp ? 1 :
                m_indicationType == OttavaDown ? -1 :
                m_indicationType == QuindicesimaDown ? -2 : 0);
    }

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    bool isValid(const std::string &s) const;

    std::string m_indicationType;
    timeT m_duration;
};

/**
 * Definitions for use in the Text event type
 */

class Text
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName TextPropertyName;
    static const PropertyName TextTypePropertyName;
    static const PropertyName LyricVersePropertyName;

    /**
     * Text styles
     */
    static const std::string UnspecifiedType;
    static const std::string StaffName;
    static const std::string ChordName;
    static const std::string KeyName;
    static const std::string Lyric;
    static const std::string Chord;
    static const std::string Dynamic;
    static const std::string Direction;
    static const std::string LocalDirection;
    static const std::string Tempo;
    static const std::string LocalTempo;
    static const std::string Annotation;
    static const std::string LilyPondDirective;

    /**
     * Special LilyPond directives
     */
    static const std::string FakeSegno;   // print segno here
    static const std::string FakeCoda;    // print coda sign here
    static const std::string Alternate1;  // first alternative ending
    static const std::string Alternate2;  // second alternative ending
    static const std::string BarDouble;   // next barline is double
    static const std::string BarEnd;      // next barline is final double
    static const std::string BarDot;      // next barline is dotted
    static const std::string Gliss;       // \glissando on this note (to next note)
    static const std::string Arpeggio;    // \arpeggio on this chord
//    static const std::string ArpeggioUp;  // \ArpeggioUp on this chord
//    static const std::string ArpeggioDn;  // \ArpeggioDown on this chord
    static const std::string Tiny;        // begin \tiny font section
    static const std::string Small;       // begin \small font section
    static const std::string NormalSize;  // begin \normalsize font section
    
    Text(const Event &e)
        /* throw (Event::NoData, Event::BadType) */;
    Text(const std::string &text,
         const std::string &textType = UnspecifiedType);
    Text(const Text &);
    Text &operator=(const Text &);
    ~Text();

    std::string getText() const { return m_text; }
    std::string getTextType() const { return m_type; }

    // Relevant for lyrics, and borrowed for figuration IDs.
    int getVerse() const { return m_verse; } 
    void setVerse(int verse) { m_verse = verse; }

    static bool isTextOfType(Event *, std::string type);

    /**
     * Return those text types that the user should be allowed to
     * specify directly and visually
     */
    static std::vector<std::string> getUserStyles();

    /**
     * Return a list of available special LilyPond directives
     */
    static std::vector<std::string> getLilyPondDirectives();

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_text;
    std::string m_type;
    long m_verse;
};



/**
 * Pitch stores a note's pitch and provides information about it in
 * various different ways, notably in terms of the position of the
 * note on the staff and its associated accidental.
 *
 * (See docs/discussion/units.txt for explanation of pitch units.)
 *
 * This completely replaces the older NotationDisplayPitch class. 
 */

class ROSEGARDENPRIVATE_EXPORT Pitch
{
public:
    /**
     * Construct a Pitch object based on the given Event, which must
     * have a BaseProperties::PITCH property.  If the property is
     * absent, NoData is thrown.  The BaseProperties::ACCIDENTAL
     * property will also be used if present.
     */
    Pitch(const Event &e)
        /* throw Event::NoData */;

    /**
     * Construct a Pitch object based on the given performance (MIDI) pitch.
     */
    Pitch(int performancePitch, 
          const Accidental &explicitAccidental = Accidentals::NoAccidental);

    /**
     * Construct a Pitch based on octave and pitch in octave.  The
     * lowest permissible octave number is octaveBase, and middle C is
     * in octave octaveBase + 5.  pitchInOctave must be in the range
     * 0-11 where 0 is C, 1 is C sharp, etc.
     */
    Pitch(int pitchInOctave, int octave,
          const Accidental &explicitAccidental = Accidentals::NoAccidental,
          int octaveBase = -2);

    /**
     * Construct a Pitch based on octave and note in scale.  The
     * lowest permissible octave number is octaveBase, and middle C is
     * in octave octaveBase + 5.  The octave supplied should be that
     * of the root note in the given key, which may be in a different
     * MIDI octave from the resulting pitch (as MIDI octaves always
     * begin at C).  noteInScale must be in the range 0-6 where 0 is
     * the root of the key and so on.  The accidental is relative to
     * noteInScale: if there is an accidental in the key for this note
     * already, explicitAccidental will be "added" to it.
     *
     * For minor keys, the harmonic scale is used.
     */
    Pitch(int noteInScale, int octave, const Key &key,
          const Accidental &explicitAccidental = Accidentals::NoAccidental,
          int octaveBase = -2);

    /**
     * Construct a Pitch based on (MIDI) octave, note in the C major scale and 
     * performance pitch. The accidental is calculated based on these
     * properties.
     */
    Pitch(int noteInCMajor, int octave, int pitch,
          int octaveBase = -2);

    /**
     * Construct a Pitch based on octave and note name.  The lowest
     * permissible octave number is octaveBase, and middle C is in
     * octave octaveBase + 5.  noteName must be a character in the
     * range [CDEFGAB] or lower-case equivalents.  The key is supplied
     * so that we know how to interpret the NoAccidental case.
     */
    Pitch(char noteName, int octave, const Key &key,
          const Accidental &explicitAccidental = Accidentals::NoAccidental,
          int octaveBase = -2);
    
    /**
     * Construct a Pitch corresponding a staff line or space on a
     * classical 5-line staff.  The bottom staff line has height 0,
     * the top has height 8, and both positive and negative values are
     * permissible.
     */
    Pitch(int heightOnStaff, const Clef &clef, const Key &key,
          const Accidental &explicitAccidental = Accidentals::NoAccidental);

    Pitch(const Pitch &);
    Pitch &operator=(const Pitch &);

    /**
     * Return the MIDI pitch for this Pitch object.
     */
    int getPerformancePitch() const;

    /**
     * Return the accidental for this pitch using a bool to prefer sharps over
     * flats if there is any doubt.  This is the accidental
     * that would be used to display this pitch outside of the context
     * of any key; that is, it may duplicate an accidental actually in
     * the current key.  This should not be used if you need to get an
     * explicit accidental returned for E#, Fb, B# or Cb.
     *
     * This version of the function exists to avoid breaking old code.
     */
    Accidental getAccidental(bool useSharps) const;

    /**
     * Return the accidental for this pitch, using a key.  This should be used
     * if you need an explicit accidental returned for E#, Fb, B# or Cb, which
     * can't be resolved correctly without knowing that their key requires
     * them to take an accidental.  The provided key will also be used to
     * determine whether to prefer sharps over flats.
     */
    Accidental getAccidental(const Key &key) const;

    /**
     * Return the accidental that should be used to display this pitch
     * in a given key.  For example, if the pitch is F-sharp in a key
     * in which F has a sharp, NoAccidental will be returned.  (This
     * is in contrast to getAccidental, which would return Sharp.)
     * This obviously can't take into account things like which
     * accidentals have already been displayed in the bar, etc.
     */
    Accidental getDisplayAccidental(const Key &key) const;

    /**
     * Return the accidental that should be used to display this pitch
     * in a given key, using the given strategy to resolve pitches where
     * an accidental is needed but not specified.
     */
    Accidental getDisplayAccidental(const Key &key, Accidentals::NoAccidentalStrategy) const;

    /**
     * Return the position in the scale for this pitch, as a number in
     * the range 0 to 6 where 0 is the root of the key.
     */
    int getNoteInScale(const Key &key) const;

    /**
     * Return the note name for this pitch, as a single character in
     * the range A to G.  (This is a reference value that should not
     * normally be shown directly to the user, for i18n reasons.)
     */
    char getNoteName(const Key &key) const;

    /**
     * Return the height at which this pitch should display on a
     * conventional 5-line staff.  0 is the bottom line, 1 the first
     * space, etc., so for example middle-C in the treble clef would
     * return -2.
     *
     * Chooses the most likely accidental for this pitch in this key.
     */
    int getHeightOnStaff(const Clef &clef, const Key &key) const;

    /**
     * Return the height at which this pitch should display on a
     * conventional 5-line staff.  0 is the bottom line, 1 the first
     * space, etc., so for example middle-C in the treble clef would
     * return -2.
     *
     * Chooses the accidental specified by the 'useSharps' parameter
     */
    int getHeightOnStaff(const Clef &clef, bool useSharps) const;

    /**
     * Return the octave containing this pitch.  The octaveBase argument
     * specifies the octave containing MIDI pitch 0; middle-C is in octave
     * octaveBase + 5.
     */
    int getOctave(int octaveBase = -2) const;

    /**
     * Return the octave containing this pitch, including the accidentals. 
     * The octaveBase argument specifies the octave containing MIDI pitch 0;
     * middle-C is in octave octaveBase + 5.
     */
    int getOctaveAccidental(int octaveBase = -2, Accidental acc = Accidentals::NoAccidental) const;

    /**
     * Return the pitch within the octave, in the range 0 to 11.
     */
    int getPitchInOctave() const;

    /**
     * Return whether this pitch is diatonic in the given key.
     */
    bool isDiatonicInKey(const Key &key) const;
    
    /**
     * Return a reference name for this pitch.  (C 4, Bb 2, etc...)
     * using the key of C major explicitly, which should allow the accidentals
     * to take their natural forms of C# Eb F# G# Ab Bb from the key, rather
     * than being forced sharp explicilty.
     *
     * This replaces an earlier version of this function that took a "use
     * sharps" argument to return either sharps or flats, which broke after
     * Arnout Engelen did some really nice accidental spelling improvements to
     * make everything more human.
     */
    std::string getAsString(bool inclOctave = true,
                            int octaveBase = -2) const;

    /**
     * Return a number 0-6 corresponding to the given note name, which
     * must be in the range [CDEFGAB] or lower-case equivalents.  The
     * return value is in the range 0-6 with 0 for C, 1 for D etc.
     */
    static int getIndexForNote(char noteName);

    /**
     * Return a note name corresponding to the given note index, which
     * must be in the range 0-6 with 0 for C, 1 for D etc.
     */
    static char getNoteForIndex(int index);

    /**
     * Calculate and return the performance (MIDI) pitch corresponding
     * to the stored height and accidental, interpreting them as
     * Rosegarden-2.1-style values (for backward compatibility use),
     * in the given clef and key
     */
    static int getPerformancePitchFromRG21Pitch(int heightOnStaff,
                                                const Accidental &accidental,
                                                const Clef &clef,
                                                const Key &key);

    /**
     * return the result of transposing the given pitch by the
     * specified interval in the given key. The key is left unchanged,
     * only the pitch is transposed.
     */
    Pitch transpose(const Key &key, int pitchDelta, int heightDelta);

    /** 
      * checks whether the accidental specified for this pitch (if any)
      * is valid - for example, a Sharp for pitch 11 is invalid, as
      * it's between A# and B#.
      */  
    bool validAccidental() const;

    /**
     * Returned event is on heap; caller takes responsibility for ownership
     */
    Event *getAsNoteEvent(timeT absoluteTime, timeT duration) const;

private:
    int m_pitch;
    Accidental m_accidental;

    static void rawPitchToDisplayPitch
    (int, const Clef &, const Key &, int &, Accidental &, 
    Accidentals::NoAccidentalStrategy);

    static void displayPitchToRawPitch
    (int, Accidental, const Clef &, const Key &,
     int &, bool ignoreOffset = false);
};



class TimeSignature;


/**
 * The Note class represents note durations only, not pitch or
 * accidental; it's therefore just as relevant to rest events as to
 * note events.  You can construct one of these from either.
 */

class ROSEGARDENPRIVATE_EXPORT Note
{
public:
    static const std::string EventType;
    static const std::string EventRestType;
    static const int EventRestSubOrdering;

    typedef int Type; // not an enum, too much arithmetic at stake

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

    Note(const Note &n) : m_type(n.m_type), m_dots(n.m_dots) { }
    ~Note() { }

    Note &operator=(const Note &n);

    Type getNoteType()  const { return m_type; }
    int  getDots()      const { return m_dots; }

    /**
     * Return the duration of this note type.
     */
    timeT getDuration()  const {
        return m_dots ? getDurationAux() : (m_shortestTime * (1 << m_type));
    }

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

class ROSEGARDENPRIVATE_EXPORT TimeSignature
{
public:
    static const TimeSignature DefaultTimeSignature;
    typedef Exception BadTimeSignature;

    TimeSignature() :
        m_numerator(DefaultTimeSignature.m_numerator),
        m_denominator(DefaultTimeSignature.m_denominator),
        m_common(false), m_hidden(false), m_hiddenBars(false) { }

    /**
     * Construct a TimeSignature object describing a time signature
     * with the given numerator and denominator.  If preferCommon is
     * true and the time signature is a common or cut-common time, the
     * constructed object will return true for isCommon; if hidden is
     * true, the time signature is intended not to be displayed and
     * isHidden will return true; if hiddenBars is true, the bar lines
     * between this time signature and the next will not be shown.
     */
    TimeSignature(int numerator, int denominator,
                  bool preferCommon = false,
                  bool hidden = false,
                  bool hiddenBars = false)
        /* throw (BadTimeSignature) */;
    
    TimeSignature(const TimeSignature &ts) :
        m_numerator(ts.m_numerator),
        m_denominator(ts.m_denominator),
        m_common(ts.m_common),
        m_hidden(ts.m_hidden),
        m_hiddenBars(ts.m_hiddenBars) { }

    ~TimeSignature() { }

    TimeSignature &operator=(const TimeSignature &ts);

    bool operator==(const TimeSignature &ts) const {
        return ts.m_numerator == m_numerator && ts.m_denominator == m_denominator;
    }
    bool operator!=(const TimeSignature &ts) const {
        return !operator==(ts);
    }
    bool operator<(const TimeSignature &ts) const {
        // We don't really need to ordered time signatures, but to be able to
        // create a map keyed with time signatures. We want to distinguish
        // 4/4 from 2/4 as well as 4/4 from 2/2.
        double ratio1 = (double) m_numerator / (double) m_denominator;
        double ratio2 = (double) ts.m_numerator / (double) ts.m_denominator;
        if (ratio1 == ratio2) return m_denominator > ts.m_denominator;
        else return ratio1 < ratio2;
    }

    int getNumerator()     const { return m_numerator; }
    int getDenominator()   const { return m_denominator; }

    bool isCommon()        const { return m_common; }
    bool isHidden()        const { return m_hidden; }
    bool hasHiddenBars()   const { return m_hiddenBars; }

    void setHidden(bool hidden) { m_hidden = hidden; }

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

    /**
     * Return a list of divisions, subdivisions, subsubdivisions
     * etc of a bar in this time, up to the given depth.  For example,
     * if the time signature is 6/8 and the depth is 3, return a list
     * containing 2, 3, and 2 (there are 2 beats to the bar, each of
     * which is best subdivided into 3 subdivisions, each of which
     * divides most neatly into 2).
     */
    void getDivisions(int depth, std::vector<int> &divisions) const;

private:
    friend class Composition;
    friend class TimeTempoSelection;

    TimeSignature(const Event &e)
        /* throw (Event::NoData, Event::BadType, BadTimeSignature) */;

    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName NumeratorPropertyName;
    static const PropertyName DenominatorPropertyName;
    static const PropertyName ShowAsCommonTimePropertyName;
    static const PropertyName IsHiddenPropertyName;
    static const PropertyName HasHiddenBarsPropertyName;

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    int m_numerator;
    int m_denominator;

    bool m_common;
    bool m_hidden;
    bool m_hiddenBars;

    mutable int  m_barDuration;
    mutable int  m_beatDuration;
    mutable int  m_beatDivisionDuration;
    mutable bool m_dotted;
    void setInternalDurations() const;

    // a time & effort saving device
    static const timeT m_crotchetTime;
    static const timeT m_dottedCrotchetTime;
};



/**
 * AccidentalTable represents a set of accidentals in force at a
 * given time.
 *
 * Keep an AccidentalTable variable on-hand as you track through a
 * staff; then when reading a chord, call processDisplayAccidental
 * on the accidentals found in the chord to obtain the actual
 * displayed accidentals and to tell the AccidentalTable to
 * remember the accidentals that have been found in the chord.
 * Then when the chord ends, call update() on the AccidentalTable
 * so that that chord's accidentals are taken into account for the
 * next one.
 *
 * Create a new AccidentalTable whenever a new key is encountered,
 * and call newBar() or newClef() when a new bar happens or a new
 * clef is encountered.
 */
class ROSEGARDENPRIVATE_EXPORT AccidentalTable
{
public:
    enum OctaveType {
        OctavesIndependent, // if c' and c'' sharp, mark them both sharp
        OctavesCautionary,  // if c' and c'' sharp, put the second one in brackets
        OctavesEquivalent   // if c' and c'' sharp, only mark the first one
    };

    enum BarResetType {
        BarResetNone,       // c# | c -> omit natural
        BarResetCautionary, // c# | c -> add natural to c in brackets
        BarResetExplicit    // c# | c -> add natural to c
    };

    AccidentalTable(const Key & = Key(), const Clef & = Clef(),
                    OctaveType = OctavesCautionary,
                    BarResetType = BarResetCautionary);

    AccidentalTable(const AccidentalTable &);
    AccidentalTable &operator=(const AccidentalTable &);

    Accidental processDisplayAccidental(const Accidental &displayAcc,
                                        int heightOnStaff,
                                        bool &cautionary);

    void update();

    void newBar();
    void newClef(const Clef &);
    
private:
    Key m_key;
    Clef m_clef;
    OctaveType m_octaves;
    BarResetType m_barReset;

    struct AccidentalRec {
        AccidentalRec() : accidental(Accidentals::NoAccidental), previousBar(false) { }
        AccidentalRec(Accidental a, bool p) : accidental(a), previousBar(p) { }
        Accidental accidental;
        bool previousBar;
    };

    typedef std::map<int, AccidentalRec> AccidentalMap;

    AccidentalMap m_accidentals;
    AccidentalMap m_canonicalAccidentals;

    AccidentalMap m_newAccidentals;
    AccidentalMap m_newCanonicalAccidentals;
};

 
/** Definitions for use in the Symbol event type
 *
 * A Symbol has no duration, and the things it represents will probably always
 * be no-ops that are never interpreted by the sequencer or MIDI export engines
 *
 * \author D. Michael McIntyre
 */
class ROSEGARDENPRIVATE_EXPORT Symbol
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;
    static const PropertyName SymbolTypePropertyName;

    /**
     * Symbol types
     */
    static const std::string UnspecifiedType;
    static const std::string Segno;
    static const std::string Coda;
    static const std::string Breath;

    Symbol(const Event &e)
        /* throw (Event::NoData, Event::BadType) */;
    Symbol(const std::string &symbolType = UnspecifiedType);
    Symbol(const Symbol &);
    Symbol &operator=(const Symbol &);
    ~Symbol ();

    std::string getSymbolType() const { return m_type; }

    static bool isSymbolOfType(Event *, std::string type);

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_type;
};

}


#endif
