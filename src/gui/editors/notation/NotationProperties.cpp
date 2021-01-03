/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "NotationProperties.h"

#include "base/PropertyName.h"


namespace Rosegarden
{

const PropertyName NotationProperties::NOTE_STYLE         = "NoteStyle";
const PropertyName NotationProperties::HEIGHT_ON_STAFF = "HeightOnStaff";
const PropertyName NotationProperties::BEAMED         = "Beamed";
const PropertyName NotationProperties::BEAM_ABOVE         = "BeamAbove";
const PropertyName NotationProperties::SLASHES        = "Slashes";
const PropertyName NotationProperties::STEM_UP        = "NoteStemUp";
const PropertyName NotationProperties::USE_CAUTIONARY_ACCIDENTAL = "UseCautionaryAccidental";
const PropertyName NotationProperties::OTTAVA_SHIFT       = "OttavaShift";
const PropertyName NotationProperties::SLUR_ABOVE         = "SlurAbove";

NotationProperties::NotationProperties(const std::string &prefix) :

        VIEW_LOCAL_STEM_UP (prefix + "StemUp"),

        MIN_WIDTH	(prefix + "MinWidth"),

        CALCULATED_ACCIDENTAL	(prefix + "NoteCalculatedAccidental"),
        DISPLAY_ACCIDENTAL	(prefix + "NoteDisplayAccidental"),
        DISPLAY_ACCIDENTAL_IS_CAUTIONARY(prefix + "NoteDisplayAccidentalIsCautionary"),
        ACCIDENTAL_SHIFT (prefix + "NoteAccidentalShift"),
        ACCIDENTAL_EXTRA_SHIFT (prefix + "NoteAccidentalExtraShift"),
        UNBEAMED_STEM_LENGTH	(prefix + "UnbeamedStemLength"),
        DRAW_FLAG	(prefix + "NoteDrawFlag"),
        NOTE_HEAD_SHIFTED	(prefix + "NoteHeadShifted"),
        NEEDS_EXTRA_SHIFT_SPACE	(prefix + "NeedsExtraShiftSpace"),
        NOTE_DOT_SHIFTED	(prefix + "NoteDotShifted"),
        CHORD_PRIMARY_NOTE	(prefix + "ChordPrimaryNote"),
        CHORD_MARK_COUNT	(prefix + "ChordMarkCount"),
        TIE_LENGTH	(prefix + "TieLength"),
        SLUR_Y_DELTA	(prefix + "SlurYDelta"),
        SLUR_LENGTH	(prefix + "SlurLength"),
        LYRIC_EXTRA_WIDTH	(prefix + "LyricExtraWidth"),
        REST_TOO_SHORT (prefix + "RestTooShort"),
        REST_OUTSIDE_STAVE (prefix + "RestOutsideStave"),

        BEAM_GRADIENT	(prefix + "BeamGradient"),
        BEAM_SECTION_WIDTH	(prefix + "BeamSectionWidth"),
        BEAM_NEXT_BEAM_COUNT	(prefix + "BeamNextBeamCount"),
        BEAM_NEXT_PART_BEAMS	(prefix + "BeamNextPartBeams"),
        BEAM_THIS_PART_BEAMS	(prefix + "BeamThisPartBeams"),
        BEAM_MY_Y	(prefix + "BeamMyY"),

        TUPLING_LINE_MY_Y	(prefix + "TuplingLineMyY"),
        TUPLING_LINE_WIDTH	(prefix + "TuplingLineWidth"),
        TUPLING_LINE_GRADIENT	(prefix + "TuplingLineGradient"),
        TUPLING_LINE_FOLLOWS_BEAM	(prefix + "TuplingLineFollowsBeam")

{
    // nothing else
}

}
