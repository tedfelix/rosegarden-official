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

#ifndef RG_BARLINE_ITEM_H
#define RG_BARLINE_ITEM_H

#include <QGraphicsItem>

#include "StaffLayout.h"


namespace Rosegarden {

class BarLineItem : public QGraphicsItem
{
public:
    BarLineItem(double layoutX,
		int barLineHeight, int baseBarThickness, int lineSpacing,
		int inset, StaffLayout::BarStyle style) :
        QGraphicsItem(),
        m_layoutX(layoutX),
        m_barLineHeight(barLineHeight),
        m_baseBarThickness(baseBarThickness),
        m_lineSpacing(lineSpacing),
        m_inset(inset),
        m_colour(Qt::black),
        m_style(style) { }

    double getLayoutX() const { return m_layoutX; }

    void setColour(QColor colour) { m_colour = colour; }

    QRectF boundingRect() const;

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget);

protected:
    double m_layoutX;
    int m_barLineHeight;
    int m_baseBarThickness;
    int m_lineSpacing;
    int m_inset;
    QColor m_colour;
    StaffLayout::BarStyle m_style;
};

}

#endif
