/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ClefIndex]"

#include "gui/general/ClefIndex.h"
#include "misc/Debug.h"

namespace Rosegarden
{

const Rosegarden::Clef clefIndexToClef(int index)
{
    Rosegarden::Clef clef;

    // insert an initial clef from track parameters
    switch (index) {
    case TrebleClef:       clef = Clef(Clef::Treble);       break;
    case BassClef:         clef = Clef(Clef::Bass);         break;
    case CrotalesClef:     clef = Clef(Clef::Treble, 2);    break;
    case XylophoneClef:    clef = Clef(Clef::Treble, 1);    break;
    case GuitarClef:       clef = Clef(Clef::Treble, -1);   break;
    case ContrabassClef:   clef = Clef(Clef::Bass, -1);     break;
    case CelestaClef:      clef = Clef(Clef::Bass, 2);      break;
    case OldCelestaClef:   clef = Clef(Clef::Bass, 1);      break;
    case FrenchClef:       clef = Clef(Clef::French);       break;
    case SopranoClef:      clef = Clef(Clef::Soprano);      break;
    case MezzosopranoClef: clef = Clef(Clef::Mezzosoprano); break;
    case AltoClef:         clef = Clef(Clef::Alto);         break;
    case TenorClef:        clef = Clef(Clef::Tenor);        break;
    case BaritoneClef:     clef = Clef(Clef::Baritone);     break;
    case VarbaritoneClef:  clef = Clef(Clef::Varbaritone);  break;
    case SubbassClef:      clef = Clef(Clef::Subbass);      break;
    case TwoBarClef:       clef = Clef(Clef::TwoBar);       break;
    default:               clef = Clef(Clef::Treble);       break;
    }
    return clef;
}

int clefNameToClefIndex(QString s)
{
    int m_elClef = 0;
    if (!s.isEmpty()) {
        if (s == "treble")
            m_elClef = TrebleClef;
        else if (s == "bass")
            m_elClef = BassClef;
        else if (s == "crotales")
            m_elClef = CrotalesClef;
        else if (s == "xylophone")
            m_elClef = XylophoneClef;
        else if (s == "guitar")
            m_elClef = GuitarClef;
        else if (s == "contrabass")
            m_elClef = ContrabassClef;
        else if (s == "celesta")
            m_elClef = CelestaClef;
        else if (s == "oldCelesta")
            m_elClef = OldCelestaClef;
        else if (s == "french")
            m_elClef = FrenchClef;
        else if (s == "soprano")
            m_elClef = SopranoClef;
        else if (s == "mezzosoprano")
            m_elClef = MezzosopranoClef;
        else if (s == "alto")
            m_elClef = AltoClef;
        else if (s == "tenor")
            m_elClef = TenorClef;
        else if (s == "baritone")
            m_elClef = BaritoneClef;
        else if (s == "varbaritone")
            m_elClef = VarbaritoneClef;
        else if (s == "subbass")
            m_elClef = SubbassClef;
        else if (s == "two-bar")
            m_elClef = TwoBarClef;
        else {
            RG_DEBUG << "startElement: processed unrecognized clef type: " << s;
        }
    }
    return m_elClef;
}

}
