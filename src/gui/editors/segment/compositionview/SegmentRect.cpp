/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentRect.h"
#include "base/ColourMap.h"

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>


namespace Rosegarden
{


const QColor SegmentRect::DefaultPenColor = QColor(Qt::black);
const QColor SegmentRect::DefaultBrushColor = QColor(COLOUR_DEF_R, COLOUR_DEF_G, COLOUR_DEF_B);

SegmentRect SegmentRect::intersected(const SegmentRect &other) const
{
    SegmentRect intersected;
    intersected.rect = rect.intersected(other.rect);

    // Mix m_brush colors
    const QColor &thisColor = brush.color();
    const QColor &otherColor = other.brush.color();

    QColor color((thisColor.red()   + otherColor.red())   / 2,
                 (thisColor.green() + otherColor.green()) / 2,
                 (thisColor.blue()  + otherColor.blue())  / 2);

    intersected.brush = color;

    // Combine selected
    intersected.selected = (selected  ||  other.selected);

    return intersected;
}


}
