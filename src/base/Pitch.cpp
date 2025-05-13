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

#define RG_MODULE_STRING "[Pitch]"
#define RG_NO_DEBUG_PRINT

#include "Pitch.h"

#include "BaseProperties.h"
#include "Event.h"
#include "NotationRules.h"
#include "NotationTypes.h"

#include "misc/Debug.h"
#include "misc/Preferences.h"

#include <vector>

//dmm This will make everything excruciatingly slow if defined:
//#define DEBUG_PITCH


namespace Rosegarden
{


namespace
{

    // this was refactored to take advantage of these translations being
    // available in other contexts, and to avoid extra work for translators
    QString a_notes[] = {
        QObject::tr("C",  "note name"), QObject::tr("C#", "note name"),
        QObject::tr("D",  "note name"), QObject::tr("Eb", "note name"),
        QObject::tr("E",  "note name"), QObject::tr("F",  "note name"),
        QObject::tr("F#", "note name"), QObject::tr("G",  "note name"),
        QObject::tr("Ab", "note name"), QObject::tr("A",  "note name"),
        QObject::tr("Bb", "note name"), QObject::tr("B",  "note name")
    };

    std::vector<QString> initPitchTable()
    {
        std::vector<QString> pitchTable{128};

        const int baseOctave = Preferences::getMIDIPitchOctave();

        for (int pitch = 0; pitch < 128; ++pitch) {
            const int octave = int(pitch / 12.0) + baseOctave;
            pitchTable[pitch] =
                    QString("%1 %2").arg(a_notes[pitch % 12]).arg(octave);
        }

        return pitchTable;
    }

}

QString Pitch::toStringOctave(int pitch)
{
    static std::vector<QString> pitchTable = initPitchTable();

    if (pitch < 0  ||  pitch > 127)
        return QString("*%1*").arg(pitch);

    return pitchTable[pitch];
}

QString Pitch::toString(int pitch)
{
    if (pitch < 0  ||  pitch > 127)
        return QString("*%1*").arg(pitch);

    return a_notes[pitch % 12];
}

static bool
pitchInKey(int pitch, const Key& key)
{
    int pitchOffset = (pitch - key.getTonicPitch() + 12) % 12;

    if (key.isMinor()) {
        static int pitchInMinor[] =
            { true, false, true, true, false, true, false, true, true, false, true, false };
        return pitchInMinor[pitchOffset];
    }
    else {
        static int pitchInMajor[] =
            { true, false, true, false, true, true, false, true, false, true, false, true };
        return pitchInMajor[pitchOffset];
    }
}

/**
 * @param pitch in the range 0..11 (C..B)
 *
 * @author Arnout Engelen
 */
static Accidental
resolveNoAccidental(int pitch,
                    const Key &key,
                    Accidentals::NoAccidentalStrategy noAccidentalStrategy)
{
    Accidental outputAccidental;

    // Find out the accidental to use, based on the strategy specified
    switch (noAccidentalStrategy) {
        case Accidentals::UseKeySharpness:
            noAccidentalStrategy =
                key.isSharp() ? Accidentals::UseSharps : Accidentals::UseFlats;
            // fall through
        case Accidentals::UseFlats:
            // shares code with UseSharps
        case Accidentals::UseSharps:
            if (pitchInKey(pitch, key)) {
                outputAccidental = Accidentals::NoAccidental;
            }
            else {
                if (noAccidentalStrategy == Accidentals::UseSharps) {
                    outputAccidental = Accidentals::Sharp;
                }
                else {
                    outputAccidental = Accidentals::Flat;
                }
            }
            break;
        case Accidentals::UseKey:
            // the distance of the pitch from the tonic of the current
            //  key
            int pitchOffset = (pitch - key.getTonicPitch() + 12) % 12;
            // 0: major, 1: minor
            int minor = key.isMinor();
            static const int pitchToHeight[2][12] =
                {
                    { 0, 0, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6 },
                    // a ., b, c, ., d, ., e, f, ., g, .
                    { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 }
                };

            // map pitchOffset to the extra correction, on top of any
            // accidentals in the key. Example: in F major, with a pitchOffset
            // of 6, the resulting height would be 3 (Bb) and the correction
            // would be +1, so the resulting note would be B-natural
            static const int pitchToCorrection[2][12] =
                {
                    { 0, +1, 0, -1, 0, 0, +1, 0, -1, 0, -1, 0 },
                    { 0, -1, 0, 0, +1, 0, -1, 0, 0, +1, 0, +1 }
                };

            int correction = pitchToCorrection[minor][pitchOffset];

            // Get the accidental normally associated with this height in this
            //  key.
            Accidental normalAccidental = key.getAccidentalForStep(pitchToHeight[minor][pitchOffset]);

            // Apply the pitchCorrection and get the outputAccidental
            outputAccidental = Accidentals::getAccidental(
                    Accidentals::getPitchOffset(normalAccidental) + correction);

    }

    return outputAccidental;
}

/**
 * @param pitch in the range 0..11 (C..B)
 *
 * @author Michael McIntyre
 */
static void
resolveSpecifiedAccidental(int pitch,
                              const Clef &/* clef */,
                              const Key &key,
                              int &height,
                              int &octave,
                              const Accidental &inputAccidental,
                              Accidental &outputAccidental)
{
        // 4.  Get info from the Key
    long accidentalCount = key.getAccidentalCount();
    bool keyIsSharp = key.isSharp(), keyIsFlat = !keyIsSharp;

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
            case 7: keyHasSharpB = true; // fall-through
            case 6: keyHasSharpE = true; // fall-through
            case 5: keyHasSharpA = true; // fall-through
            case 4: keyHasSharpD = true; // fall-through
            case 3: keyHasSharpG = true; // fall-through
            case 2: keyHasSharpC = true; // fall-through
            case 1: keyHasSharpF = true; // fall-through
        }
    } else {
        switch (accidentalCount) {
            case 7: keyHasFlatF = true; // fall-through
            case 6: keyHasFlatC = true; // fall-through
            case 5: keyHasFlatG = true; // fall-through
            case 4: keyHasFlatD = true; // fall-through
            case 3: keyHasFlatA = true; // fall-through
            case 2: keyHasFlatE = true; // fall-through
            case 1: keyHasFlatB = true; // fall-through
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
    const int C = -2, D = -1, E = 0, F = 1, G = 2, A = 3, B = 4;

    // Here we do the actual work of making all the decisions explained above.
    switch (pitch) {
        case 0 :
                 if (inputAccidental == Accidentals::Sharp ||                         // B#
                    (inputAccidental == Accidentals::NoAccidental && keyHasSharpB)) {
                     height = B;
                     octave--;
                     outputAccidental = (keyHasSharpB) ? Accidentals::NoAccidental : Accidentals::Sharp;
                 } else if (inputAccidental == Accidentals::DoubleFlat) {             // Dbb
                     height = D;
                     outputAccidental = Accidentals::DoubleFlat;
                 } else {
                     height = C;                                        // C or C-Natural
                     outputAccidental = (keyHasFlatC || keyHasSharpC ||
                                   (keyHasSharpB &&
                                  inputAccidental == Accidentals::Natural)) ? Accidentals::Natural : Accidentals::NoAccidental;
                 }
                 break;
        case 1 :
                 if (inputAccidental == Accidentals::Sharp ||                       // C#
                    (inputAccidental == Accidentals::NoAccidental &&  keyIsSharp)) {
                     height = C;
                     outputAccidental = (keyHasSharpC) ?  Accidentals::NoAccidental : Accidentals::Sharp;
                 } else if (inputAccidental == Accidentals::Flat ||                 // Db
                           (inputAccidental == Accidentals::NoAccidental && keyIsFlat)) {
                     height = D;
                     outputAccidental = (keyHasFlatD) ? Accidentals::NoAccidental : Accidentals::Flat;
                 } else if (inputAccidental == Accidentals::DoubleSharp) {          // Bx
                    height = B;
                    octave--;
                    outputAccidental = Accidentals::DoubleSharp;
                 }
                 break;
        case 2 :
                 if (inputAccidental == Accidentals::DoubleSharp) {                  // Cx
                     height = C;
                     outputAccidental = Accidentals::DoubleSharp;
                 } else if (inputAccidental == Accidentals::DoubleFlat) {            // Ebb
                     height = E;
                     outputAccidental = Accidentals::DoubleFlat;
                 } else {                                              // D or D-Natural
                     height = D;
                     outputAccidental = (keyHasSharpD || keyHasFlatD) ? Accidentals::Natural : Accidentals::NoAccidental;
                 }
                 break;
        case 3 :
                 if (inputAccidental == Accidentals::Sharp ||                        // D#
                    (inputAccidental == Accidentals::NoAccidental &&  keyIsSharp)) {
                     height = D;
                     outputAccidental = (keyHasSharpD) ? Accidentals::NoAccidental : Accidentals::Sharp;
                 } else if (inputAccidental == Accidentals::Flat ||                  // Eb
                           (inputAccidental == Accidentals::NoAccidental &&  keyIsFlat)) {
                     height = E;
                     outputAccidental = (keyHasFlatE) ? Accidentals::NoAccidental : Accidentals::Flat;
                 } else if (inputAccidental == Accidentals::DoubleFlat) {            // Fbb
                     height = F;
                     outputAccidental = Accidentals::DoubleFlat;
                 }
                 break;
        case 4 :
                 if (inputAccidental == Accidentals::Flat ||                         // Fb
                    (inputAccidental == Accidentals::NoAccidental && keyHasFlatF)) {
                     height = F;
                     outputAccidental = (keyHasFlatF) ? Accidentals::NoAccidental : Accidentals::Flat;
                 } else if (inputAccidental == Accidentals::DoubleSharp) {           // Dx
                     height = D;
                     outputAccidental = Accidentals::DoubleSharp;
                 } else {                                              // E or E-Natural
                     height = E;
                     outputAccidental = (keyHasSharpE || keyHasFlatE ||
                                   (keyHasFlatF && inputAccidental==Accidentals::Natural)) ?
                                           Accidentals::Natural : Accidentals::NoAccidental;
                 }
                 break;
        case 5 :
                 if (inputAccidental == Accidentals::Sharp ||                        // E#
                    (inputAccidental == Accidentals::NoAccidental && keyHasSharpE)) {
                     height = E;
                     outputAccidental = (keyHasSharpE) ? Accidentals::NoAccidental : Accidentals::Sharp;
                 } else if (inputAccidental == Accidentals::DoubleFlat) {            // Gbb
                     height = G;
                     outputAccidental = Accidentals::DoubleFlat;
                 } else {                                              // F or F-Natural
                     height = F;
                     outputAccidental = (keyHasSharpF || keyHasFlatF ||
                                   (keyHasSharpE && inputAccidental==Accidentals::Natural))?
                                           Accidentals::Natural : Accidentals::NoAccidental;
                 }
                 break;
        case 6 :
                 if (inputAccidental == Accidentals::Sharp ||
                    (inputAccidental == Accidentals::NoAccidental && keyIsSharp)) {  // F#
                     height = F;
                     outputAccidental = (keyHasSharpF) ? Accidentals::NoAccidental : Accidentals::Sharp;
                 } else if (inputAccidental == Accidentals::Flat ||                  // Gb
                           (inputAccidental == Accidentals::NoAccidental && keyIsFlat)) {
                     height = G;
                     outputAccidental = (keyHasFlatG) ? Accidentals::NoAccidental : Accidentals::Flat;
                 } else if (inputAccidental == Accidentals::DoubleSharp) {           // Ex
                     height = E;
                     outputAccidental = Accidentals::DoubleSharp;
                 }
                 break;
        case 7 :
                 if (inputAccidental == Accidentals::DoubleSharp) {                  // Fx
                     height = F;
                     outputAccidental = Accidentals::DoubleSharp;
                 } else if (inputAccidental == Accidentals::DoubleFlat) {            // Abb
                     height = A;
                     outputAccidental = Accidentals::DoubleFlat;
                 } else {                                              // G or G-Natural
                     height = G;
                     outputAccidental = (keyHasSharpG || keyHasFlatG) ? Accidentals::Natural : Accidentals::NoAccidental;
                 }
                 break;
        case 8 :
                 if (inputAccidental == Accidentals::Sharp ||
                    (inputAccidental == Accidentals::NoAccidental && keyIsSharp)) {  // G#
                     height = G;
                     outputAccidental = (keyHasSharpG) ? Accidentals::NoAccidental : Accidentals::Sharp;
                 } else if (inputAccidental == Accidentals::Flat ||                  // Ab
                           (inputAccidental == Accidentals::NoAccidental && keyIsFlat)) {
                     height = A;
                     outputAccidental = (keyHasFlatA) ? Accidentals::NoAccidental : Accidentals::Flat;
                 }
                 break;
        case 9 :
                 if (inputAccidental == Accidentals::DoubleSharp) {                  // Gx
                     height = G;
                     outputAccidental = Accidentals::DoubleSharp;
                 } else if (inputAccidental == Accidentals::DoubleFlat) {            // Bbb
                     height = B;
                     outputAccidental = Accidentals::DoubleFlat;
                 } else {                                              // A or A-Natural
                     height = A;
                     outputAccidental = (keyHasSharpA || keyHasFlatA) ? Accidentals::Natural : Accidentals::NoAccidental;
                 }
                 break;
        case 10:
                 if (inputAccidental == Accidentals::DoubleFlat) {                   // Cbb
                     height = C;
                     octave++;  // tweak B/C divide
                     outputAccidental = Accidentals::DoubleFlat;
                 } else if (inputAccidental == Accidentals::Sharp ||                 // A#
                           (inputAccidental == Accidentals::NoAccidental && keyIsSharp)) {
                     height = A;
                     outputAccidental = (keyHasSharpA) ? Accidentals::NoAccidental : Accidentals::Sharp;
                 } else if (inputAccidental == Accidentals::Flat ||                  // Bb
                           (inputAccidental == Accidentals::NoAccidental && keyIsFlat)) {
                     height = B;
                     outputAccidental = (keyHasFlatB) ? Accidentals::NoAccidental : Accidentals::Flat;
                 }
                 break;
        case 11:
                 if (inputAccidental == Accidentals::DoubleSharp) {                  // Ax
                     height = A;
                     outputAccidental = Accidentals::DoubleSharp;
                 } else if (inputAccidental == Accidentals::Flat ||                  // Cb
                           (inputAccidental == Accidentals::NoAccidental && keyHasFlatC)) {
                     height = C;
                     octave++;  // tweak B/C divide
                     outputAccidental = (keyHasFlatC) ? Accidentals::NoAccidental : Accidentals::Flat;
                 } else {                                             // B or B-Natural
                     height = B;
                     outputAccidental = (keyHasSharpB || keyHasFlatB ||
                                   (keyHasFlatC && inputAccidental==Accidentals::Natural)) ?
                                           Accidentals::Natural : Accidentals::NoAccidental;
                 }
    }

    if (outputAccidental == Accidentals::NoAccidental && inputAccidental == Accidentals::Natural) {
        outputAccidental = Accidentals::Natural;
    }

}

bool
Pitch::validAccidental() const
{
        //RG_DEBUG << "Checking whether accidental is valid";
        if (m_accidental == Accidentals::NoAccidental)
        {
                return true;
        }
        int naturalPitch = (m_pitch -
                Accidentals::getPitchOffset(m_accidental) + 12) % 12;
        switch(naturalPitch)
        {
                case 0: //C
                        return true;
                case 1:
                        return false;
                case 2: //D
                        return true;
                case 3:
                        return false;
                case 4: //E
                        return true;
                case 5: //F
                        return true;
                case 6:
                        return false;
                case 7: //G
                        return true;
                case 8:
                        return false;
                case 9: //A
                        return true;
                case 10:
                        return false;
                case 11: //B
                        return true;
        };
        RG_WARNING << "validAccidental(): Internal error";
        return false;
}

Event *
Pitch::getAsNoteEvent(timeT absoluteTime, timeT duration) const
{
    Event *e = new Event(Note::EventType, absoluteTime, duration);
    e->set<Int>(BaseProperties::PITCH, m_pitch);
    e->set<String>(BaseProperties::ACCIDENTAL, m_accidental);
    return e;
}

Key
Pitch::getAsKey() const {
    Key cmaj("C major");

    // Cycle through the circle of fifths, from 7 flats (Cb) to 7 sharps (C#),
    // to find the number of accidentals for the key for this tonic
    Pitch p(-1, 0, Accidentals::Flat);
    int accidentalCount = -7;
    while ((p.getPitchInOctave() != this->getPitchInOctave()  ||
            p.getAccidental(cmaj) != this->getAccidental(cmaj))
        && accidentalCount < 8) {
        accidentalCount++;
        p = p.transpose(cmaj, 7, 4);
    }

    if (p.getPitchInOctave() == this->getPitchInOctave()  &&
        p.getAccidental(cmaj) == this->getAccidental(cmaj)) {
        return Key(abs(accidentalCount), accidentalCount >= 0, false);
    } else {
        // Not any 'regular' key, so the ambiguous ctor is fine
        return Key(this->getPitchInOctave(), false);
    }

}

#if 0
Key
Pitch::getAsKey(bool isMinor) const {
    if (isMinor)
        return getAsKey().getEquivalent();
    else
        return getAsKey();
}
#endif

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
 * This function originally written by Chris Cannam for X11 Rosegarden
 * Entirely rewritten by Chris Cannam for Rosegarden 4
 * Entirely rewritten by Hans Kieserman
 * Entirely rewritten by Michael McIntyre
 * This version by Michael McIntyre <dmmcintyr@users.sourceforge.net>
 * Resolving the accidental was refactored out by Arnout Engelen
 */
void
Pitch::rawPitchToDisplayPitch(
        int rawpitch,
        const Clef &clef,
        const Key &key,
        int &height,
        Accidental &accidental,
        Accidentals::NoAccidentalStrategy noAccidentalStrategy)
{

    // 1. Calculate the octave (for later):
    int octave = rawpitch / 12;

    // 2. Set initial height to 0
    height = 0;

    // 3.  Calculate raw semitone number, yielding a value between 0 (C) and
    // 11 (B)
    int pitch  = rawpitch % 12;

    // clear the in-coming accidental so we can trap any failure to re-set
    // it on the way out:
    Accidental userAccidental = accidental;
    accidental = "";

    if (userAccidental == Accidentals::NoAccidental  ||
        !Pitch(rawpitch, userAccidental).validAccidental())
    {
        userAccidental = resolveNoAccidental(pitch, key, noAccidentalStrategy);
        //RG_DEBUG << "Chose accidental " << userAccidental << " for pitch " << pitch << " in key " << key.getName();
    }
    //else
    //{
    //  RG_DEBUG << "Accidental was specified, as " << userAccidental;
    //}

    resolveSpecifiedAccidental(pitch, clef, key, height, octave, userAccidental, accidental);

    // Failsafe...  If this ever executes, there's trouble to fix...
// WIP - DMM - munged up to explore #937389, which is temporarily deferred,
// owing to its non-critical nature, having been hacked around in the LilyPond
// code

// This is a workaround to prevent lupdate complaining about
// "Parenthesis/brace mismatch between #if and #else branches"
#ifdef DEBUG_PITCH
#define DP true
#else
#define DP false
#endif

    if (DP || (accidental == "")) {
#ifndef DEBUG_PITCH
        RG_DEBUG << "rawPitchToDisplayPitch(): error! returning null accidental for:";
#else
        RG_DEBUG << "rawPitchToDisplayPitch(): calculating: ";
#endif
        RG_DEBUG << "pitch: " << rawpitch << " (" << pitch << " in oct "
                 << octave << ")  userAcc: " << userAccidental
                 << "  clef: " << clef.getClefType() << "  key: " << key.getName();
    }


    // 6.  "Recenter" height in case it's been changed:
    height = ((height + 2) % 7) - 2;

    height += (octave - 5) * 7;
    height += clef.getPitchOffset();


    // 7. Transpose up or down for the clef:
    height -= 7 * clef.getOctave();
}

void
Pitch::displayPitchToRawPitch(int height,
                              Accidental accidental,
                              const Clef &clef,
                              const Key &key,
                              int &pitch,
                              bool ignoreOffset)
{
    int octave = 5;

    // 1. Ask Key for accidental if necessary
    if (accidental == Accidentals::NoAccidental) {
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

    if (accidental != Accidentals::NoAccidental &&
        accidental != Accidentals::Natural) {
        if (accidental == Accidentals::Sharp) { pitch++; }
        else if (accidental == Accidentals::Flat) { pitch--; }
        else if (accidental == Accidentals::DoubleSharp) { pitch += 2; }
        else if (accidental == Accidentals::DoubleFlat) { pitch -= 2; }
    }

    // 4. Adjust for clef
    octave += clef.getOctave();

    pitch += 12 * octave;
}

Pitch::Pitch(const Event &e) :
    // throw (Event::NoData)
    m_accidental(Accidentals::NoAccidental)
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

Pitch::Pitch(int pitchInOctave, int octave,
             const Accidental &explicitAccidental, int octaveBase) :
    m_pitch((octave - octaveBase) * 12 + pitchInOctave),
    m_accidental(explicitAccidental)
{
    // nothing else
}

Pitch::Pitch(int noteInScale, int octave, const Key &key,
             const Accidental &explicitAccidental, int octaveBase) :
    m_pitch(0),
    m_accidental(explicitAccidental)
{
    m_pitch = (key.getTonicPitch());
    m_pitch = (octave - octaveBase) * 12 + m_pitch % 12;

    if (key.isMinor()) m_pitch += scale_Cminor_harmonic[noteInScale];
    else m_pitch += scale_Cmajor[noteInScale];

    m_pitch += Accidentals::getPitchOffset(m_accidental);
}

Pitch::Pitch(int noteInCMajor, int octave, int pitch,
             int octaveBase) :
    m_pitch(pitch)
{
    int natural = (octave - octaveBase) * 12 + scale_Cmajor[noteInCMajor];
    // cppcheck-suppress useInitializationList
    m_accidental = Accidentals::getAccidental(pitch - natural);
}


Pitch::Pitch(char noteName, int octave, const Key &key,
             const Accidental &explicitAccidental, int octaveBase) :
    m_pitch(0),
    m_accidental(explicitAccidental)
{
    int height = getIndexForNote(noteName) - 2;
    displayPitchToRawPitch(height, explicitAccidental,
                           Clef(), key, m_pitch);

    // we now have the pitch within octave 5 (C == 60) -- though it
    // might have spilled over at either end
    if (m_pitch < 60) --octave;
    if (m_pitch > 71) ++octave;
    m_pitch = (octave - octaveBase) * 12 + m_pitch % 12;
}

Pitch::Pitch(int heightOnStaff, const Clef &clef, const Key &key,
             const Accidental &explicitAccidental) :
    m_pitch(0),
    m_accidental(explicitAccidental)
{
    displayPitchToRawPitch
        (heightOnStaff, explicitAccidental, clef, key, m_pitch);
    if (m_pitch < 0) m_pitch = 0;
    if (m_pitch > 127) m_pitch = 127;
}

int
Pitch::getPerformancePitch() const
{
    return m_pitch;
}

Accidental
Pitch::getAccidental(bool useSharps) const
{
    return getDisplayAccidental(Key("C major"),
                useSharps ? Accidentals::UseSharps : Accidentals::UseFlats);
}

Accidental
Pitch::getAccidental(const Key &key) const
{
    if (m_accidental == Accidentals::NoAccidental || !validAccidental())
    {
        const Accidental retval =
                resolveNoAccidental(m_pitch, key, Accidentals::UseKey);
        //RG_DEBUG << "Resolved No/invalid accidental: chose " << retval;
        return retval;
    }
    else
    {
        //RG_DEBUG << "Returning specified accidental";
        return m_accidental;
    }
}

Accidental
Pitch::getDisplayAccidental(const Key &key) const
{
    return getDisplayAccidental(key, Accidentals::UseKey);
}

Accidental
Pitch::getDisplayAccidental(
        const Key &key,
        Accidentals::NoAccidentalStrategy noAccidentalStrategy) const
{
    int heightOnStaff;
    Accidental accidental(m_accidental);
    rawPitchToDisplayPitch(m_pitch, Clef(), key, heightOnStaff, accidental,
            noAccidentalStrategy);
    return accidental;
}

int
Pitch::getNoteInScale(const Key &key) const
{
    int p = m_pitch;
    p -= key.getTonicPitch();
    p -= Accidentals::getPitchOffset(getDisplayAccidental(key));
    p += 24; // in case these calculations made it -ve
    p %= 12;

    if (key.isMinor()) return steps_Cminor_harmonic[p];
    else return steps_Cmajor[p];
}

char
Pitch::getNoteName(const Key &key) const
{
    int index = (getHeightOnStaff(Clef(Clef::Treble), key) + 72) % 7;
    return getNoteForIndex(index);
}

int
Pitch::getHeightOnStaff(const Clef &clef, const Key &key) const
{
    int heightOnStaff;
    Accidental accidental(m_accidental);
    rawPitchToDisplayPitch(m_pitch, clef, key, heightOnStaff, accidental,
            Accidentals::UseKey);
    return heightOnStaff;
}

int
Pitch::getHeightOnStaff(const Clef &clef, bool useSharps) const
{
    int heightOnStaff;
    Accidental accidental(m_accidental);
    rawPitchToDisplayPitch(
            m_pitch,
            clef,
            Key("C major"),
            heightOnStaff,
            accidental,
            useSharps ? Accidentals::UseSharps : Accidentals::UseFlats);

    return heightOnStaff;
}

int
Pitch::getOctave(int octaveBase) const
{
    return m_pitch / 12 + octaveBase;
}

int
Pitch::getOctaveAccidental(int octaveBase, const Accidental& acc) const
{
    int t_pitch = m_pitch;
    if (acc == Accidentals::DoubleFlat) {
        t_pitch += 2;
    } else if (acc == Accidentals::Flat) {
        t_pitch += 1;
    } else if (acc == Accidentals::Sharp) {
        t_pitch -= 1;
    } else if (acc == Accidentals::DoubleSharp) {
        t_pitch -= 2;
    }
    return t_pitch / 12 + octaveBase;
}

int
Pitch::getPitchInOctave() const
{
    return m_pitch % 12;
}

bool
Pitch::isDiatonicInKey(const Key &key) const
{
    if (getDisplayAccidental(key) == Accidentals::NoAccidental) return true;

    // ### as used in the chord identifiers, this calls chords built on
    //     the raised sixth step diatonic -- may be correct, but it's
    //     misleading, as we're really looking for whether chords are
    //     often built on given tone

    if (key.isMinor()) {
        int stepsFromTonic = ((m_pitch - key.getTonicPitch() + 12) % 12);
        if (stepsFromTonic == 9 || stepsFromTonic == 11) return true;
    }

    return false;
}

#if 0
std::string
Pitch::getAsString(bool inclOctave, int octaveBase) const
{
    std::string s;
    s += getNoteName(Key("C major"));

    Accidental acc = getAccidental(Key("C major"));

    if (acc == Accidentals::Sharp) s += "#";
    else if (acc == Accidentals::Flat) s += "b";

    if (!inclOctave) return s;

    char tmp[10];
    sprintf(tmp, "%s%d", s.c_str(), getOctave(octaveBase));
    return std::string(tmp);
}
#endif

int
Pitch::getIndexForNote(char noteName)
{
    if (islower(noteName)) noteName = toupper(noteName);
    if (noteName < 'C') {
        if (noteName < 'A') return 0; // error, really
        else return noteName - 'A' + 5;
    } else {
        if (noteName > 'G') return 0; // error, really
        else return noteName - 'C';
    }
}

char
Pitch::getNoteForIndex(int index)
{
    if (index < 0 || index > 6) return 'C'; // error, really
    return "CDEFGAB"[index];
}

int
Pitch::getPerformancePitchFromRG21Pitch(int heightOnStaff,
                                        const Accidental &accidental,
                                        const Clef &clef,
                                        const Key &)
{
    // X11 Rosegarden pitches are a bit weird; see
    // docs/data_struct/units.txt

    // We pass the accidental and clef, a faked key of C major, and a
    // flag telling displayPitchToRawPitch to ignore the clef offset
    // and take only its octave into account

    int p = 0;
    displayPitchToRawPitch(heightOnStaff, accidental, clef, Key(), p, true);
    return p;
}

Pitch Pitch::transpose(const Key &key, int pitchDelta, int heightDelta) const
{
    // get old accidental
    Accidental oldAccidental = getAccidental(key);

    // get old step
    // TODO: maybe we should write an oldPitchObj.getOctave(0, key) that takes
    //  into account accidentals properly (e.g. yielding '0' instead of '1' for
    //  B#0). For now workaround here.
    Pitch oldPitchWithoutAccidental(
            getPerformancePitch() - Accidentals::getPitchOffset(oldAccidental),
            Accidentals::Natural);
    const Key cmaj = Key();
    const int oldStep = oldPitchWithoutAccidental.getNoteInScale(cmaj) +
                        oldPitchWithoutAccidental.getOctave(0) * 7;

    // calculate new pitch and step
    int newPitch = getPerformancePitch() + pitchDelta;
    int newStep  = oldStep  + heightDelta;

    // could happen for example when transposing the tonic of a key downwards
    if (newStep < 0 || newPitch < 0) {
        newStep += 7;
        newPitch += 12;
    }

    // should not happen
    if (newStep < 0 || newPitch < 0) {
        RG_WARNING << "transpose(): Internal error in NotationTypes";
        if (newStep < 0) newStep = 0;
        if (newPitch < 0) newPitch = 0;
    }

    // calculate new accidental for step
    const int pitchWithoutAccidental =
            ((newStep / 7) * 12 + scale_Cmajor[newStep % 7]);
    const int newAccidentalOffset = newPitch - pitchWithoutAccidental;

    // construct pitch-object to return
    Pitch newPitchObj(newPitch, Accidentals::getAccidental(newAccidentalOffset));

    return newPitchObj;
}


}
