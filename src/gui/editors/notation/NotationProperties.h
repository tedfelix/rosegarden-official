
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTATIONPROPERTIES_H
#define RG_NOTATIONPROPERTIES_H

#include "base/PropertyName.h"
#include <string>


namespace Rosegarden
{


/**
 * Property names for properties that are computed and cached within
 * the notation module, but that need not necessarily be saved with
 * the file.
 *
 * If you add something here, remember to add the definition to
 * NotationProperties.cpp as well...
 */

class NotationProperties
{
public:
    NotationProperties(const std::string &prefix);

    // These are only of interest to notation views, but are the
    // same across all notation views.

    static const PropertyName HEIGHT_ON_STAFF;
    static const PropertyName NOTE_STYLE;
    static const PropertyName BEAMED;
    static const PropertyName BEAM_ABOVE;
    static const PropertyName SLASHES;
    static const PropertyName STEM_UP;
    static const PropertyName USE_CAUTIONARY_ACCIDENTAL;
    static const PropertyName OTTAVA_SHIFT;
    static const PropertyName SLUR_ABOVE;

    // The rest are, or may be, view-local

    const PropertyName VIEW_LOCAL_STEM_UP;
    const PropertyName MIN_WIDTH;
    const PropertyName CALCULATED_ACCIDENTAL;
    const PropertyName DISPLAY_ACCIDENTAL;
    const PropertyName DISPLAY_ACCIDENTAL_IS_CAUTIONARY;
    const PropertyName ACCIDENTAL_SHIFT;
    const PropertyName ACCIDENTAL_EXTRA_SHIFT;
    const PropertyName UNBEAMED_STEM_LENGTH;
    const PropertyName DRAW_FLAG;
    const PropertyName NOTE_HEAD_SHIFTED;
    const PropertyName NEEDS_EXTRA_SHIFT_SPACE;
    const PropertyName NOTE_DOT_SHIFTED;
    const PropertyName CHORD_PRIMARY_NOTE;
    const PropertyName CHORD_MARK_COUNT;
    const PropertyName TIE_LENGTH;
    const PropertyName SLUR_Y_DELTA;
    const PropertyName SLUR_LENGTH;
    const PropertyName LYRIC_EXTRA_WIDTH;
    const PropertyName REST_TOO_SHORT;
    const PropertyName REST_OUTSIDE_STAVE;

    // Set in applyBeam in NotationSets.cpp:

    const PropertyName BEAM_GRADIENT;
    const PropertyName BEAM_SECTION_WIDTH;
    const PropertyName BEAM_NEXT_BEAM_COUNT;
    const PropertyName BEAM_NEXT_PART_BEAMS;
    const PropertyName BEAM_THIS_PART_BEAMS;
    const PropertyName BEAM_MY_Y;
    const PropertyName TUPLING_LINE_MY_Y;
    const PropertyName TUPLING_LINE_WIDTH;
    const PropertyName TUPLING_LINE_GRADIENT;
    const PropertyName TUPLING_LINE_FOLLOWS_BEAM;

};


}

#endif
