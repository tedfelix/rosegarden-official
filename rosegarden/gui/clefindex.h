// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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


#ifndef _CLEF_INDEX_H_
#define _CLEF_INDEX_H_

// used variously by TPB, SPB, PresetHandler to correlate combo box indices to
// clef types
enum { TrebleClef, BassClef, AltoClef, TenorClef, GuitarClef, XylophoneClef, ContrabassClef, 
       CelestaClef, OldCelestaClef, SopranoClef, TwoBarClef };

#endif // _CLEF_INDEX_H_
