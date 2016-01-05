/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NoteCharacterNames.h"

namespace Rosegarden
{

namespace NoteCharacterNames
{

const CharName SHARP = "MUSIC SHARP SIGN";
const CharName FLAT = "MUSIC FLAT SIGN";
const CharName NATURAL = "MUSIC NATURAL SIGN";
const CharName DOUBLE_SHARP = "MUSICAL SYMBOL DOUBLE SHARP";
const CharName DOUBLE_FLAT = "MUSICAL SYMBOL DOUBLE FLAT";

const CharName BREVE = "MUSICAL SYMBOL BREVE";
const CharName WHOLE_NOTE = "MUSICAL SYMBOL WHOLE NOTE";
const CharName VOID_NOTEHEAD = "MUSICAL SYMBOL VOID NOTEHEAD";
const CharName NOTEHEAD_BLACK = "MUSICAL SYMBOL NOTEHEAD BLACK";

const CharName X_NOTEHEAD = "MUSICAL SYMBOL X NOTEHEAD";
const CharName CIRCLE_X_NOTEHEAD = "MUSICAL SYMBOL CIRCLE X NOTEHEAD";
const CharName BREVIS = "MUSICAL SYMBOL BREVIS";
const CharName SEMIBREVIS_WHITE = "MUSICAL SYMBOL SEMIBREVIS WHITE";
const CharName SEMIBREVIS_BLACK = "MUSICAL SYMBOL SEMIBREVIS BLACK";
const CharName TRIANGLE_NOTEHEAD_UP_WHITE = "MUSICAL SYMBOL TRIANGLE NOTEHEAD UP WHITE";
const CharName TRIANGLE_NOTEHEAD_UP_BLACK = "MUSICAL SYMBOL TRIANGLE NOTEHEAD UP BLACK";
const CharName SQUARE_NOTEHEAD_WHITE = "MUSICAL SYMBOL SQUARE NOTEHEAD WHITE";
const CharName SQUARE_NOTEHEAD_BLACK = "MUSICAL SYMBOL SQUARE NOTEHEAD BLACK";

// These two names are not valid Unicode names.  They describe flags
// that should be used to compose multi-flag notes, rather than used
// on their own.  Unicode has no code point for these, but they're
// common in real fonts.  COMBINING PARTIAL FLAG is a flag that may be
// drawn several times to make a multi-flag note; COMBINING PARTIAL
// FLAG FINAL may be used as the flag nearest the note head and may
// have an additional swash.  (In many fonts, the FLAG 1 character may
// also be suitable for use as PARTIAL FLAG FINAL).
const CharName FLAG_PARTIAL = "MUSICAL SYMBOL COMBINING PARTIAL FLAG";
const CharName FLAG_PARTIAL_FINAL = "MUSICAL SYMBOL COMBINING PARTIAL FLAG FINAL";

const CharName FLAG_1 = "MUSICAL SYMBOL COMBINING FLAG-1";
const CharName FLAG_2 = "MUSICAL SYMBOL COMBINING FLAG-2";
const CharName FLAG_3 = "MUSICAL SYMBOL COMBINING FLAG-3";
const CharName FLAG_4 = "MUSICAL SYMBOL COMBINING FLAG-4";

const CharName MULTI_REST = "MUSICAL SYMBOL MULTI REST"; // Unicode-4 glyph 1D13A
const CharName MULTI_REST_ON_STAFF = "MUSICAL SYMBOL MULTI REST ON STAFF";
const CharName WHOLE_REST = "MUSICAL SYMBOL WHOLE REST"; // Unicode-4 glyph 1D13B
const CharName WHOLE_REST_ON_STAFF = "MUSICAL SYMBOL WHOLE REST ON STAFF";
const CharName HALF_REST = "MUSICAL SYMBOL HALF REST";  // Unicode-4 glyph 1D13C
const CharName HALF_REST_ON_STAFF = "MUSICAL SYMBOL HALF REST ON STAFF";
const CharName QUARTER_REST = "MUSICAL SYMBOL QUARTER REST";
const CharName EIGHTH_REST = "MUSICAL SYMBOL EIGHTH REST";
const CharName SIXTEENTH_REST = "MUSICAL SYMBOL SIXTEENTH REST";
const CharName THIRTY_SECOND_REST = "MUSICAL SYMBOL THIRTY-SECOND REST";
const CharName SIXTY_FOURTH_REST = "MUSICAL SYMBOL SIXTY-FOURTH REST";

const CharName DOT = "MUSICAL SYMBOL COMBINING AUGMENTATION DOT";

const CharName ACCENT = "MUSICAL SYMBOL COMBINING ACCENT";
const CharName TENUTO = "MUSICAL SYMBOL COMBINING TENUTO";
const CharName STACCATO = "MUSICAL SYMBOL COMBINING STACCATO";
const CharName STACCATISSIMO = "MUSICAL SYMBOL COMBINING STACCATISSIMO";
const CharName MARCATO = "MUSICAL SYMBOL COMBINING MARCATO";
const CharName OPEN = "MUSICAL SYMBOL COMBINING OPEN";
const CharName STOPPED = "MUSICAL SYMBOL COMBINING STOPPED";
const CharName FERMATA = "MUSICAL SYMBOL FERMATA";
const CharName TRILL = "MUSICAL SYMBOL TR";
const CharName TRILL_LINE = "MUSICAL SYMBOL COMBINING TRILL LINE";
const CharName TURN = "MUSICAL SYMBOL TURN";

const CharName MORDENT = "MUSICAL SYMBOL MORDENT";
const CharName MORDENT_INVERTED = "MUSICAL SYMBOL INVERTED MORDENT";
const CharName MORDENT_LONG = "MUSICAL SYMBOL LONG MORDENT";
const CharName MORDENT_LONG_INVERTED = "MUSICAL SYMBOL LONG INVERTED MORDENT";

const CharName PEDAL_MARK = "MUSICAL SYMBOL PEDAL MARK";
const CharName PEDAL_UP_MARK = "MUSICAL SYMBOL PEDAL UP MARK";

const CharName UP_BOW = "MUSICAL SYMBOL COMBINING UP BOW";
const CharName DOWN_BOW = "MUSICAL SYMBOL COMBINING DOWN BOW";
const CharName HARMONIC = "MUSICAL SYMBOL COMBINING HARMONIC";

const CharName C_CLEF = "MUSICAL SYMBOL C CLEF";
const CharName G_CLEF = "MUSICAL SYMBOL G CLEF";
const CharName F_CLEF = "MUSICAL SYMBOL F CLEF";
const CharName TWO_BAR_CLEF = "MUSICAL SYMBOL DRUM CLEF-1";

const CharName COMMON_TIME = "MUSICAL SYMBOL COMMON TIME";
const CharName CUT_TIME = "MUSICAL SYMBOL CUT TIME";
const CharName DIGIT_ZERO = "DIGIT ZERO";
const CharName DIGIT_ONE = "DIGIT ONE";
const CharName DIGIT_TWO = "DIGIT TWO";
const CharName DIGIT_THREE = "DIGIT THREE";
const CharName DIGIT_FOUR = "DIGIT FOUR";
const CharName DIGIT_FIVE = "DIGIT FIVE";
const CharName DIGIT_SIX = "DIGIT SIX";
const CharName DIGIT_SEVEN = "DIGIT SEVEN";
const CharName DIGIT_EIGHT = "DIGIT EIGHT";
const CharName DIGIT_NINE = "DIGIT NINE";

const CharName SEGNO = "MUSICAL SYMBOL SEGNO";
const CharName CODA = "MUSICAL SYMBOL CODA";
const CharName BREATH_MARK = "MUSICAL SYMBOL BREATH_MARK";


const CharName UNKNOWN = "__UNKNOWN__";

}

}
