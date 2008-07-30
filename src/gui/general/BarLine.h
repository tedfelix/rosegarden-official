
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_BARLINE_H_
#define _RG_BARLINE_H_

#include "LinedStaff.h"
#include <qcanvas.h>

namespace Rosegarden {

class BarLine : public QCanvasPolygonalItem
{
public:
    BarLine(QCanvas *canvas, double layoutX,
            int barLineHeight, int baseBarThickness, int lineSpacing,
            int inset, LinedStaff::BarStyle style) :
        QCanvasPolygonalItem(canvas),
        m_layoutX(layoutX),
        m_barLineHeight(barLineHeight),
        m_baseBarThickness(baseBarThickness),
        m_lineSpacing(lineSpacing),
        m_inset(inset),
        m_style(style) { }

    double getLayoutX() const { return m_layoutX; }
    
    virtual QPointArray areaPoints() const;
    virtual void drawShape(QPainter &);

protected:
    double m_layoutX;
    int m_barLineHeight;
    int m_baseBarThickness;
    int m_lineSpacing;
    int m_inset;
    LinedStaff::BarStyle m_style;
};

}

#endif /*BARLINE_H_*/
