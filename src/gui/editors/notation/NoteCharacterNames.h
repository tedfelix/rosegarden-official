/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTE_CHAR_NAME_H
#define RG_NOTE_CHAR_NAME_H

#include <QString>

namespace Rosegarden {

typedef QString CharName;

/// A selection of Unicode character names for symbols in a note font

namespace NoteCharacterNames
{
extern const CharName SHARP;
extern const CharName FLAT;
extern const CharName NATURAL;
extern const CharName DOUBLE_SHARP;
extern const CharName DOUBLE_FLAT;

extern const CharName BREVE;
extern const CharName WHOLE_NOTE;
extern const CharName VOID_NOTEHEAD;
extern const CharName NOTEHEAD_BLACK;

extern const CharName X_NOTEHEAD;
extern const CharName CIRCLE_X_NOTEHEAD;
extern const CharName SEMIBREVIS_WHITE;
extern const CharName SEMIBREVIS_BLACK;
extern const CharName TRIANGLE_NOTEHEAD_UP_WHITE;
extern const CharName TRIANGLE_NOTEHEAD_UP_BLACK;
extern const CharName SQUARE_NOTEHEAD_WHITE;
extern const CharName SQUARE_NOTEHEAD_BLACK;

extern const CharName FLAG_PARTIAL;
extern const CharName FLAG_PARTIAL_FINAL;

extern const CharName FLAG_1;
extern const CharName FLAG_2;
extern const CharName FLAG_3;
extern const CharName FLAG_4;

extern const CharName MULTI_REST;
extern const CharName MULTI_REST_ON_STAFF;
extern const CharName WHOLE_REST;
extern const CharName WHOLE_REST_ON_STAFF;
extern const CharName HALF_REST;
extern const CharName HALF_REST_ON_STAFF;
extern const CharName QUARTER_REST;
extern const CharName EIGHTH_REST;
extern const CharName SIXTEENTH_REST;
extern const CharName THIRTY_SECOND_REST;
extern const CharName SIXTY_FOURTH_REST;

extern const CharName DOT;

extern const CharName ACCENT;
extern const CharName TENUTO;
extern const CharName STACCATO;
extern const CharName STACCATISSIMO;
extern const CharName MARCATO;
extern const CharName OPEN;
extern const CharName STOPPED;
extern const CharName FERMATA;
extern const CharName TRILL;
extern const CharName TRILL_LINE;
extern const CharName TURN;
extern const CharName UP_BOW;
extern const CharName DOWN_BOW;
extern const CharName HARMONIC;

extern const CharName MORDENT;
extern const CharName MORDENT_INVERTED;
extern const CharName MORDENT_LONG;
extern const CharName MORDENT_LONG_INVERTED;

extern const CharName PEDAL_MARK;
extern const CharName PEDAL_UP_MARK;

extern const CharName C_CLEF;
extern const CharName G_CLEF;
extern const CharName F_CLEF;
extern const CharName TWO_BAR_CLEF;

extern const CharName COMMON_TIME;
extern const CharName CUT_TIME;
extern const CharName DIGIT_ZERO;
extern const CharName DIGIT_ONE;
extern const CharName DIGIT_TWO;
extern const CharName DIGIT_THREE;
extern const CharName DIGIT_FOUR;
extern const CharName DIGIT_FIVE;
extern const CharName DIGIT_SIX;
extern const CharName DIGIT_SEVEN;
extern const CharName DIGIT_EIGHT;
extern const CharName DIGIT_NINE;

extern const CharName SEGNO;
extern const CharName CODA;
extern const CharName BREATH_MARK;

extern const CharName UNKNOWN;
}

}

#endif

