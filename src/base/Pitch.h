/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

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

#ifndef RG_PITCH_H
#define RG_PITCH_H

#include "NotationTypes.h"

#include <rosegardenprivate_export.h>

#include <QString>

#include <string>


namespace Rosegarden
{


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
    explicit Pitch(const Event &e)
        /* throw Event::NoData */;

    /**
     * Construct a Pitch object based on the given performance (MIDI) pitch.
     */
    explicit Pitch(
            int performancePitch,
            const Accidental &explicitAccidental = Accidentals::NoAccidental);

    /**
     * Construct a Pitch based on octave and pitch in octave.  The
     * lowest permissible octave number is octaveBase, and middle C is
     * in octave octaveBase + 5.  pitchInOctave must be in the range
     * 0-11 where 0 is C, 1 is C sharp, etc.
     */
    Pitch(int pitchInOctave,
          int octave,
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
    Pitch(int noteInScale,
          int octave,
          const Key &key,
          const Accidental &explicitAccidental = Accidentals::NoAccidental,
          int octaveBase = -2);

    /**
     * Construct a Pitch based on (MIDI) octave, note in the C major scale and
     * performance pitch. The accidental is calculated based on these
     * properties.
     */
    Pitch(int noteInCMajor,
          int octave,
          int pitch,
          int octaveBase = -2);

    /**
     * Construct a Pitch based on octave and note name.  The lowest
     * permissible octave number is octaveBase, and middle C is in
     * octave octaveBase + 5.  noteName must be a character in the
     * range [CDEFGAB] or lower-case equivalents.  The key is supplied
     * so that we know how to interpret the NoAccidental case.
     */
    Pitch(char noteName,
          int octave,
          const Key &key,
          const Accidental &explicitAccidental = Accidentals::NoAccidental,
          int octaveBase = -2);

    /**
     * Construct a Pitch corresponding a staff line or space on a
     * classical 5-line staff.  The bottom staff line has height 0,
     * the top has height 8, and both positive and negative values are
     * permissible.
     */
    Pitch(int heightOnStaff,
          const Clef &clef,
          const Key &key,
          const Accidental &explicitAccidental = Accidentals::NoAccidental);

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
     * Return the position in the scale for this pitch, as a number in
     * the range 0 to 6 where 0 is the root of the key.
     */
    int getNoteInScale(const Key &key) const;

    /**
     * Return the note name for this pitch, as a single character in
     * the range A to G.
     *
     * This is a reference value that should not normally be shown directly to
     * the user, for i18n reasons.  Instead pass it through tr() using the
     * "note name" context:
     *
     *   QString noteName = pitch.getNoteName(key);
     *   noteName = QObject::tr(noteName, "note name");
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
    int getOctaveAccidental(
            int octaveBase = -2,
            const Accidental& acc = Accidentals::NoAccidental) const;

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
     * to take their natural forms of C# Eb F# Ab Bb from the key, rather
     * than being forced sharp explicitly.
     *
     * This replaces an earlier version of this function that took a "use
     * sharps" argument to return either sharps or flats, which broke after
     * Arnout Engelen did some really nice accidental spelling improvements to
     * make everything more human.
     */
    //std::string getAsString(bool inclOctave = true,
    //                        int octaveBase = -2) const;

    /// Returns one of C C# D Eb E F F# G Ab A Bb B along with the octave.
    static QString toStringOctave(int pitch);
    /// Returns one of C C# D Eb E F F# G Ab A Bb B.
    static QString toString(int pitch);

    /**
     * Return a number 0-6 corresponding to the given note name, which
     * must be in the range [CDEFGAB] or lower-case equivalents.  The
     * return value is in the range 0-6 with 0 for C, 1 for D etc.
     */
    static int getIndexForNote(char noteName);

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
    Pitch transpose(const Key &key, int pitchDelta, int heightDelta) const;

    /**
     * Returned event is on heap; caller takes responsibility for ownership
     */
    Event *getAsNoteEvent(timeT absoluteTime, timeT duration) const;

    /**
     * Get the major key that has this Pitch as the tonic
     */
    Key getAsKey() const;

#if 0
    /**
     * Get the major or minor key that has this Pitch as the tonic
     */
    Key getAsKey(bool isMinor) const;
#endif

private:

    /// MIDI note number.  E.g. 60 is C3.
    int m_pitch;
    Accidental m_accidental;

    static void rawPitchToDisplayPitch(
            int rawPitch,
            const Clef &,
            const Key &,
            int &height,
            Accidental &,
            Accidentals::NoAccidentalStrategy);

    static void displayPitchToRawPitch(
            int height,
            Accidental,
            const Clef &,
            const Key &,
            int &pitch,
            bool ignoreOffset = false);

    /**
     * Return a note name corresponding to the given note index, which
     * must be in the range 0-6 with 0 for C, 1 for D etc.
     */
    static char getNoteForIndex(int index);

    /**
     * Return the accidental that should be used to display this pitch
     * in a given key, using the given strategy to resolve pitches where
     * an accidental is needed but not specified.
     */
    Accidental getDisplayAccidental(
            const Key &key,
            Accidentals::NoAccidentalStrategy) const;

    /**
      * checks whether the accidental specified for this pitch (if any)
      * is valid - for example, a Sharp for pitch 11 is invalid, as
      * it's between A# and B#.
      */
    bool validAccidental() const;

};


}

#endif
