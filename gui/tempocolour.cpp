// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#include "tempocolour.h"
#include "colours.h"
#include "rosedebug.h"

QColor
TempoColour::getColour(double tempo)
{
    int h, s, v;
    RosegardenGUIColours::SegmentBlock.hsv(&h, &s, &v);
    v += 20;
    if (v > 255) v = 255;

    h = 220 - int(tempo);

    while (h < 0) h += 360;
    while (h >= 360) h -= 360;

    return QColor(h, s, v, QColor::Hsv);
}

