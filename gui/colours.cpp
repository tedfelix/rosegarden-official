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


#include "colours.h"

namespace RosegardenGUIColours
{
    const QColor ActiveRecordTrack = Qt::red;

    const QColor SegmentBorder = Qt::black;
    const QColor SegmentBlock = Qt::blue;
    const QColor SegmentHighlightBlock = Qt::black;
  
    const QColor BarLine = Qt::black;
    const QColor BarLineIncorrect = Qt::red;
    const QColor StaffConnectingLine = QColor(192,192,192);
    const QColor StaffConnectingTerminatingLine = QColor(128,128,128);
    const QColor StaffRulerBackground = QColor(212,212,212);

    const QColor PositionCursor = Qt::magenta;
    const QColor SelectionRectangle = Qt::blue;
    const QColor SelectedElement = Qt::blue;
    const int SelectedElementHue = 240;
}


