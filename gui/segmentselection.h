// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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


#ifndef _SEGMENTSELECTION_H_
#define _SEGMENTSELECTION_H_

// Works in a similar fashion to EventSelection but this
// is of course for Segments against Compositions.
//
//

#include "Composition.h"

namespace Rosegarden
{


class SegmentSelection  // Sounds like a box of chocolates
{
public:
    SegmentSelection(Rosegarden::Composition &comp);
    ~SegmentSelection();

    void cut();
    void copy();

    void pasteToComposition(Rosegarden::Composition &comp);

private:

   Composition &m_originalComposition;

};

}

#endif // _SEGMENTSELECTION_H_
