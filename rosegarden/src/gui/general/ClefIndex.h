// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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
enum { TrebleClef,                      // G clef, line 2
       BassClef,                        // F clef, line 4
       CrotalesClef,                    // G clef, line 2, 15 above
       XylophoneClef,                   // G clef, line 2,  8 above
       GuitarClef,                      // G clef, line 2,  8 below
       ContrabassClef,                  // F clef, line 4,  8 below
       CelestaClef,                     // F clef, line 4, 15 above
       OldCelestaClef,                  // F clef, line 4,  8 above
       FrenchClef,                      // G clef, line 1
       SopranoClef,                     // C clef, line 1            
       MezzosopranoClef,                // C clef, line 2            
       AltoClef,                        // C clef, line 3
       TenorClef,                       // C clef, line 4
       BaritoneClef,                    // C clef, line 5
       VarbaritoneClef,                 // F clef, line 3
       SubbassClef,                     // F clef, line 5
       TwoBarClef                       // percussion clef  //!!! doesn't exist yet!
     };

#endif // _CLEF_INDEX_H_
