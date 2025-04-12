/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRANSPOSECOMMAND_H
#define RG_TRANSPOSECOMMAND_H

#include "document/BasicCommand.h"

#include <QString>
#include <QCoreApplication>


namespace Rosegarden
{


class EventSelection;


class TransposeCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::TransposeCommand)

public:
    TransposeCommand(int semitones, EventSelection &selection) :
        BasicCommand(getGlobalName(semitones), selection, true),
        m_selection(&selection),
        m_semitones(semitones),
        m_steps(0),
        m_diatonic(false)
    { }

    TransposeCommand(int semitones, int steps, EventSelection &selection) :
        BasicCommand(tr("Transpose by &Interval..."), selection, true),
        m_selection(&selection),
        m_semitones(semitones),
        m_steps(steps),
        m_diatonic(true)
    { }

    static QString getGlobalName(int semitones = 0) {
        switch (semitones) {
        case   1: return tr("&Up a Semitone");
        case  -1: return tr("&Down a Semitone");
        case  12: return tr("Up an &Octave");
        case -12: return tr("Down an Octa&ve");
        default:  return tr("&Transpose by Semitones...");
        }
    }

protected:
    void modifySegment() override;

private:

    // only used on 1st execute (cf bruteForceRedo)
    EventSelection *m_selection;

    int m_semitones;
    int m_steps;
    bool m_diatonic;
};


}

#endif
