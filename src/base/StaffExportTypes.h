/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    This file Coypright 2009 D. Michael McIntyre

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_STAFF_EXPORT_H
#define RG_STAFF_EXPORT_H

namespace Rosegarden
{

/**
 * StaffTypes are currently only used for LilyPond export, and amount to named
 * constant indices for the Track Parameters Box. They are used to control the
 * size of notation exported on a given staff, and boil down to a complicated
 * way to insert a \tiny or \small in the data stream ahead of the first clef,
 * etc.
 */

typedef int StaffType;

namespace StaffTypes
{
    const StaffType Normal = 0;
    const StaffType Small  = 1;
    const StaffType Tiny   = 2;
}

/**
 * Brackets are currently only used for LilyPond export, and amount to named
 * constant indices for the Track Parameters Box.  They are used to control how
 * staffs are bracketed together, and it is unfortunately necessary to have a
 * staggering number of them in order to handle all the possible combinations of
 * opening and closing brackets while keeping the interface as simple as
 * possible.
 */

typedef int Bracket;

namespace Brackets
{
    const Bracket None           = 0; //  ----
    const Bracket SquareOn       = 1; //  [
    const Bracket SquareOff      = 2; //     ]
    const Bracket SquareOnOff    = 3; //  [  ]
    const Bracket CurlyOn        = 4; //  {
    const Bracket CurlyOff       = 5; //     }
    const Bracket CurlySquareOn  = 6; //  {[
    const Bracket CurlySquareOff = 7; //    ]}
}

}

#endif
