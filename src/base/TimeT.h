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

#ifndef RG_TIMET_H
#define RG_TIMET_H

namespace Rosegarden
{
    // Time in internal units.
    typedef long timeT;

    // Time used in the rewriter to represent tupled notes exactly.
    // It doesn't have a fixed conversion to timeT, instead each bar
    // figures out a scaling that allows all tupleting in it to be
    // represented exactly.
    typedef timeT stretchedTimeT;

    // Time in 256th notes; equivalently, 4 times the time in smallest
    // notes.  We need the unit to be 1/4 of the smallest note so we
    // can represent dotted and double-dotted notes exactly.
    typedef int Num256ths;
}

#endif /* ifndef RG_TIMET_H */
