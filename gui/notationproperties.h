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

#ifndef _NOTATION_PROPERTIES_H_
#define _NOTATION_PROPERTIES_H_

#include "Event.h"

/**
 * Property names for properties that are computed and cached within
 * the notation module, but that need not necessarily be saved with
 * the file.
 *
 * If you add something here, remember to add the definition to
 * notationproperties.cpp as well...
 */

namespace NotationProperties
{

extern const Rosegarden::PropertyName HEIGHT_ON_STAFF;
extern const Rosegarden::PropertyName MIN_WIDTH;
extern const Rosegarden::PropertyName CALCULATED_ACCIDENTAL;
extern const Rosegarden::PropertyName DISPLAY_ACCIDENTAL;
extern const Rosegarden::PropertyName STEM_UP;
extern const Rosegarden::PropertyName UNBEAMED_STEM_LENGTH;
extern const Rosegarden::PropertyName DRAW_FLAG;
extern const Rosegarden::PropertyName NOTE_HEAD_SHIFTED;
extern const Rosegarden::PropertyName NEEDS_EXTRA_SHIFT_SPACE;
extern const Rosegarden::PropertyName CHORD_PRIMARY_NOTE;
//!!!extern const Rosegarden::PropertyName NOTE_NAME;
extern const Rosegarden::PropertyName TIE_LENGTH;
extern const Rosegarden::PropertyName SLUR_ABOVE;
extern const Rosegarden::PropertyName SLUR_Y_DELTA;
extern const Rosegarden::PropertyName SLUR_LENGTH;
extern const Rosegarden::PropertyName SELECTED;

// Set in applyBeam in notationsets.cpp:

extern const Rosegarden::PropertyName BEAMED;
extern const Rosegarden::PropertyName BEAM_GRADIENT;
extern const Rosegarden::PropertyName BEAM_SECTION_WIDTH;
extern const Rosegarden::PropertyName BEAM_NEXT_BEAM_COUNT;
extern const Rosegarden::PropertyName BEAM_NEXT_PART_BEAMS;
extern const Rosegarden::PropertyName BEAM_THIS_PART_BEAMS;
extern const Rosegarden::PropertyName BEAM_MY_Y;
extern const Rosegarden::PropertyName TUPLING_LINE_MY_Y;
extern const Rosegarden::PropertyName TUPLING_LINE_WIDTH;
extern const Rosegarden::PropertyName TUPLING_LINE_GRADIENT;

}

#endif

