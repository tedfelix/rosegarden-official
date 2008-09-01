
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TRANSPOSECOMMAND_H_
#define _RG_TRANSPOSECOMMAND_H_

#include "document/BasicSelectionCommand.h"
#include <QString>
#include <klocale.h>




namespace Rosegarden
{

class EventSelection;


class TransposeCommand : public BasicSelectionCommand
{
public:
    TransposeCommand(int semitones, EventSelection &selection) :
        BasicSelectionCommand(getGlobalName(semitones), selection, true),
        m_selection(&selection), m_semitones(semitones), m_diatonic(false) { }

    TransposeCommand(int semitones, int steps, EventSelection &selection) :
        BasicSelectionCommand(getDiatonicGlobalName(semitones, steps), selection, true),
        m_selection(&selection), m_semitones(semitones), m_steps(steps), m_diatonic(true) { }

    static QString getDiatonicGlobalName(int semitones = 0, int step = 0) {
        switch (semitones) {
        default:  return i18n("Transpose by &Interval...");
        }
    }

    static QString getGlobalName(int semitones = 0) {
        switch (semitones) {
        case   1: return i18n("&Up a Semitone");
        case  -1: return i18n("&Down a Semitone");
        case  12: return i18n("Up an &Octave");
        case -12: return i18n("Down an Octa&ve");
        default:  return i18n("&Transpose by Semitones...");
        }
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_semitones;
    int m_steps;
    bool m_diatonic;
};



}

#endif
