
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "layoutengine.h"

// I woke up on the last day of the year
// with the sudden realisation
// that people have brought terrible ills upon themselves by
// trying to cover the earth with fields in shapes such as squares
// which don't tesselate when mapped onto curved surfaces.
// War and famine would cease, if only we could all
// move at once onto a system of triangles.

LayoutEngine::LayoutEngine() :
    m_status(0)
{
    // empty
}

LayoutEngine::~LayoutEngine()
{
    // empty
}


HorizontalLayoutEngine::HorizontalLayoutEngine() :
    LayoutEngine()
{
    // empty
}

HorizontalLayoutEngine::~HorizontalLayoutEngine()
{
    // empty
}


VerticalLayoutEngine::VerticalLayoutEngine() :
    LayoutEngine()
{
    // empty
}

VerticalLayoutEngine::~VerticalLayoutEngine()
{
    // empty
}

