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

#ifndef _NOTE_CHAR_NAME_H_
#define _NOTE_CHAR_NAME_H_

#include "PropertyName.h"

typedef Rosegarden::PropertyName CharName;
typedef Rosegarden::PropertyNameHash CharNameHash;
typedef Rosegarden::PropertyNamesEqual CharNamesEqual;


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

extern const CharName FLAG_1;
extern const CharName FLAG_2;
extern const CharName FLAG_3;
extern const CharName FLAG_4;

extern const CharName MULTI_REST;
extern const CharName WHOLE_REST;
extern const CharName HALF_REST;
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
extern const CharName FERMATA;
extern const CharName TRILL;
extern const CharName TURN;
extern const CharName UP_BOW;
extern const CharName DOWN_BOW;

extern const CharName C_CLEF;
extern const CharName G_CLEF;
extern const CharName F_CLEF;

extern const CharName UNKNOWN;
}

#endif

