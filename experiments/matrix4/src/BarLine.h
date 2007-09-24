
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

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
#include <QPen>
#include <QBrush>
#include <QGraphicsScene>
#include <QGraphicsItem>

namespace Rosegarden {

class BarLine : public QGraphicsItem
{
public:
    BarLine(double layoutX,
            int barLineHeight, int baseBarThickness, int lineSpacing,
            int inset, LinedStaff::BarStyle style, QGraphicsItem* parent = 0) :
        QGraphicsItem(parent),
        m_layoutX(layoutX),
        m_barLineHeight(barLineHeight),
        m_baseBarThickness(baseBarThickness),
        m_lineSpacing(lineSpacing),
        m_inset(inset),
        m_style(style) { setFlag(QGraphicsItem::ItemIgnoresTransformations); }

    double getLayoutX() const { return m_layoutX; }
    
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);

    void setPen(QPen pen) { m_pen = pen; }
    void setBrush(QBrush brush) { m_brush = brush; }
    
protected:
    QPen m_pen;
    QBrush m_brush;
    
    double m_layoutX;
    int m_barLineHeight;
    int m_baseBarThickness;
    int m_lineSpacing;
    int m_inset;
    LinedStaff::BarStyle m_style;
};

}

#endif /*BARLINE_H_*/
