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

#include "notationproperties.h"

namespace NotationProperties
{

const Rosegarden::PropertyName HEIGHT_ON_STAFF      = "HeightOnStaff";
const Rosegarden::PropertyName MIN_WIDTH            = "MinWidth";

const Rosegarden::PropertyName CALCULATED_ACCIDENTAL = "NoteCalculatedAccidental";
const Rosegarden::PropertyName DISPLAY_ACCIDENTAL   = "NoteDisplayAccidental";
const Rosegarden::PropertyName STEM_UP              = "NoteStemUp";
const Rosegarden::PropertyName UNBEAMED_STEM_LENGTH = "UnbeamedStemLength";
const Rosegarden::PropertyName DRAW_FLAG            = "NoteDrawFlag";
const Rosegarden::PropertyName NOTE_HEAD_SHIFTED    = "NoteHeadShifted";
const Rosegarden::PropertyName NEEDS_EXTRA_SHIFT_SPACE = "NeedsExtraShiftSpace";
const Rosegarden::PropertyName CHORD_PRIMARY_NOTE   = "ChordPrimaryNote";
const Rosegarden::PropertyName TIE_LENGTH           = "TieLength";
const Rosegarden::PropertyName SLUR_ABOVE	    = "SlurAbove";
const Rosegarden::PropertyName SLUR_Y_DELTA	    = "SlurYDelta";
const Rosegarden::PropertyName SLUR_LENGTH          = "SlurLength";
const Rosegarden::PropertyName SELECTED		    = "Selected";

const Rosegarden::PropertyName BEAMED               = "Beamed";
const Rosegarden::PropertyName BEAM_GRADIENT        = "BeamGradient";
const Rosegarden::PropertyName BEAM_SECTION_WIDTH   = "BeamSectionWidth";
const Rosegarden::PropertyName BEAM_NEXT_BEAM_COUNT = "BeamNextBeamCount";
const Rosegarden::PropertyName BEAM_NEXT_PART_BEAMS = "BeamNextPartBeams";
const Rosegarden::PropertyName BEAM_THIS_PART_BEAMS = "BeamThisPartBeams";
const Rosegarden::PropertyName BEAM_MY_Y            = "BeamMyY";

const Rosegarden::PropertyName TUPLING_LINE_MY_Y    = "TuplingLineMyY";
const Rosegarden::PropertyName TUPLING_LINE_WIDTH   = "TuplingLineWidth";
const Rosegarden::PropertyName TUPLING_LINE_GRADIENT= "TuplingLineGradient";

}


