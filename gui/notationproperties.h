
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

// Property names for properties that are computed and cached within
// the notation module, but that need not necessarily be saved with
// the file.

// If you add something here, remember to add the definition to
// notationproperties.cpp as well...

class Properties
{
public:
    static const Rosegarden::PropertyName HEIGHT_ON_STAFF;
    static const Rosegarden::PropertyName MIN_WIDTH;
    static const Rosegarden::PropertyName ACCIDENTAL;
    static const Rosegarden::PropertyName DISPLAY_ACCIDENTAL;
    static const Rosegarden::PropertyName STEM_UP;
    static const Rosegarden::PropertyName UNBEAMED_STEM_LENGTH;
    static const Rosegarden::PropertyName DRAW_TAIL;
    static const Rosegarden::PropertyName NOTE_HEAD_SHIFTED;
    static const Rosegarden::PropertyName NOTE_NAME;

    // Set in applyBeam in notationsets.cpp:

    static const Rosegarden::PropertyName BEAMED;
    static const Rosegarden::PropertyName BEAM_PRIMARY_NOTE;
    static const Rosegarden::PropertyName BEAM_GRADIENT;
    static const Rosegarden::PropertyName BEAM_SECTION_WIDTH;
    static const Rosegarden::PropertyName BEAM_NEXT_TAIL_COUNT;
    static const Rosegarden::PropertyName BEAM_NEXT_PART_TAILS;
    static const Rosegarden::PropertyName BEAM_THIS_PART_TAILS;
    static const Rosegarden::PropertyName BEAM_MY_Y;
};

#endif

