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

class NotationProperties
{
public:
    NotationProperties(const std::string &prefix);

    // These are only of interest to notation views, but are the
    // same across all notation views

    static const Rosegarden::PropertyName NOTE_STYLE;
    static const Rosegarden::PropertyName HEIGHT_ON_STAFF;
    static const Rosegarden::PropertyName STEM_UP;
    static const Rosegarden::PropertyName BEAMED;
    static const Rosegarden::PropertyName SLASHES;

    // The rest are, or may be, view-local

    const Rosegarden::PropertyName SELECTED;
    const Rosegarden::PropertyName MIN_WIDTH;
    const Rosegarden::PropertyName CALCULATED_ACCIDENTAL;
    const Rosegarden::PropertyName DISPLAY_ACCIDENTAL;
    const Rosegarden::PropertyName UNBEAMED_STEM_LENGTH;
    const Rosegarden::PropertyName DRAW_FLAG;
    const Rosegarden::PropertyName NOTE_HEAD_SHIFTED;
    const Rosegarden::PropertyName NEEDS_EXTRA_SHIFT_SPACE;
    const Rosegarden::PropertyName CHORD_PRIMARY_NOTE;
    const Rosegarden::PropertyName TIE_LENGTH;
    const Rosegarden::PropertyName SLUR_ABOVE;
    const Rosegarden::PropertyName SLUR_Y_DELTA;
    const Rosegarden::PropertyName SLUR_LENGTH;
    const Rosegarden::PropertyName GRACE_NOTE_OFFSET;
    const Rosegarden::PropertyName NOTE_TYPE;
    const Rosegarden::PropertyName NOTE_DOTS;

    // Set in applyBeam in notationsets.cpp:

    const Rosegarden::PropertyName BEAM_GRADIENT;
    const Rosegarden::PropertyName BEAM_SECTION_WIDTH;
    const Rosegarden::PropertyName BEAM_NEXT_BEAM_COUNT;
    const Rosegarden::PropertyName BEAM_NEXT_PART_BEAMS;
    const Rosegarden::PropertyName BEAM_THIS_PART_BEAMS;
    const Rosegarden::PropertyName BEAM_MY_Y;
    const Rosegarden::PropertyName TUPLING_LINE_MY_Y;
    const Rosegarden::PropertyName TUPLING_LINE_WIDTH;
    const Rosegarden::PropertyName TUPLING_LINE_GRADIENT;

};

#endif

