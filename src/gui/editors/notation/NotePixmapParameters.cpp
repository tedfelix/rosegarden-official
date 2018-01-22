/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "NotePixmapParameters.h"

#include "base/NotationTypes.h"


namespace Rosegarden
{

NotePixmapParameters::NotePixmapParameters(Note::Type noteType,
        int dots,
        Accidental accidental) :
        m_noteType(noteType),
        m_dots(dots),
        m_accidental(accidental),
        m_cautionary(false),
        m_shifted(false),
        m_dotShifted(false),
        m_accidentalShift(0),
        m_drawFlag(true),
        m_drawStem(true),
        m_stemGoesUp(true),
        m_stemLength( -1),
        m_legerLines(0),
        m_slashes(0),
        m_selected(false),
        m_highlighted(false),
        m_quantized(false),
        m_trigger(triggerNone),
        m_onLine(false),
        m_safeVertDistance(0),
        m_restOutsideStave(false),
        m_beamed(false),
        m_nextBeamCount(0),
        m_thisPartialBeams(false),
        m_nextPartialBeams(false),
        m_width(1),
        m_gradient(0.0),
        m_tupletCount(0),
        m_tuplingLineY(0),
        m_tuplingLineWidth(0),
        m_tuplingLineGradient(0.0),
        m_tied(false),
        m_tieLength(0),
        m_tiePositionExplicit(false),
        m_tieAbove(false),
        m_inRange(true),
        m_memberOfParallel(false),
        m_forceColor(false),
        m_forcedColor(Qt::black)
{
    // nothing else
}

NotePixmapParameters::~NotePixmapParameters()
{
    // nothing to see here
}

void
NotePixmapParameters::setMarks(const std::vector<Mark> &marks)
{
    m_marks.clear();
    for (unsigned int i = 0; i < marks.size(); ++i)
        m_marks.push_back(marks[i]);
}

void
NotePixmapParameters::removeMarks()
{
    m_marks.clear();
}

std::vector<Rosegarden::Mark>
NotePixmapParameters::getNormalMarks() const
{
    std::vector<Mark> marks;

    for (std::vector<Mark>::const_iterator mi = m_marks.begin();
            mi != m_marks.end(); ++mi) {

        if (*mi == Marks::Pause ||
                *mi == Marks::UpBow ||
                *mi == Marks::DownBow ||
                *mi == Marks::Open ||
                *mi == Marks::Stopped ||
                *mi == Marks::Harmonic ||
                *mi == Marks::Trill ||
                *mi == Marks::LongTrill ||
                *mi == Marks::TrillLine ||
                *mi == Marks::Turn ||
                *mi == Marks::Mordent ||
                *mi == Marks::MordentInverted ||
                *mi == Marks::MordentLong ||
                *mi == Marks::MordentLongInverted ||
                Marks::isFingeringMark(*mi))
            continue;

        marks.push_back(*mi);
    }

    return marks;
}

std::vector<Rosegarden::Mark>
NotePixmapParameters::getAboveMarks() const
{
    std::vector<Mark> marks;

    // fingerings before other marks

    for (std::vector<Mark>::const_iterator mi = m_marks.begin();
            mi != m_marks.end(); ++mi) {

        if (Marks::isFingeringMark(*mi)) {
            marks.push_back(*mi);
        }
    }

    for (std::vector<Mark>::const_iterator mi = m_marks.begin();
            mi != m_marks.end(); ++mi) {

        if (*mi == Marks::Pause ||
                *mi == Marks::UpBow ||
                *mi == Marks::DownBow ||
                *mi == Marks::Open ||
                *mi == Marks::Stopped ||
                *mi == Marks::Harmonic ||
                *mi == Marks::Trill ||
                *mi == Marks::LongTrill ||
                *mi == Marks::TrillLine ||
                *mi == Marks::Mordent ||
                *mi == Marks::MordentInverted ||
                *mi == Marks::MordentLong ||
                *mi == Marks::MordentLongInverted ||
                *mi == Marks::Turn) {
            marks.push_back(*mi);
        }
    }

    return marks;
}

}
