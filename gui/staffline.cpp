
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

#include "staffline.h"

#include "rosedebug.h"

StaffLine::StaffLine(QCanvas *c, QCanvasItemGroup *g, int height) :
    QCanvasLineGroupable(c, g),
    m_height(height)
{
    // empty
}

void
StaffLine::setHighlighted(bool highlighted)
{
//     kdDebug(KDEBUG_AREA) << "StaffLine::setHighlighted("
//                          << highlighted << ")\n";

    if (highlighted) {

        m_normalPen = pen();
        QPen newPen = m_normalPen;
        newPen.setColor(red);
        setPen(newPen);

    } else {

        setPen(m_normalPen);

    }
}

