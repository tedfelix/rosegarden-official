/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "BarLineItem.h"

#include <QPainter>

namespace Rosegarden {
        
QRectF
BarLineItem::boundingRect() const
{
    int bx = int(x());
    int by = int(y());
    int x0 = bx, y0 = by, x1, y1 = by + m_barLineHeight;

    x1 = 0;     // Avoid a "may be used uninitialized" compilation warning

    switch (m_style) {

    case StaffLayout::PlainBar:
        x1 = x0 + m_baseBarThickness;
        break;

    case StaffLayout::DoubleBar:
        x1 = x0 + m_baseBarThickness * 4;
        break;

    case StaffLayout::HeavyDoubleBar:
        x0 -= m_baseBarThickness * 6;
        x1 = bx;
        break;

    case StaffLayout::RepeatEndBar:
        x0 -= m_baseBarThickness * 6 + m_lineSpacing * 2 / 3;
        x1 = bx;
        break;

    case StaffLayout::RepeatStartBar:
        x1 = x0 + m_baseBarThickness * 6 + m_lineSpacing * 2 / 3 + m_inset;
        break;

    case StaffLayout::RepeatBothBar:
        x0 -= m_baseBarThickness * 4 + m_lineSpacing * 2 / 3;
        x1 = x0 + m_baseBarThickness * 9 + m_lineSpacing * 2 / 3 + m_inset;
        break;

    case StaffLayout::NoVisibleBar:
        x1 = x0 + 1;
        break;
    }

    QRectF rectf(x0 - x(),
                 y0 - y(),
                 x1 - x0 + 1,
                 y1 - y0 + 1);
    return rectf;
}

void
BarLineItem::paint(QPainter *painter,
                   const QStyleOptionGraphicsItem */* option */,
                   QWidget */* widget */)
{
    int bx = 0;
    int by = 0;

    painter->save();
//    painter->setPen(QPen(m_colour, 0));
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_colour);

    switch (m_style) {

    case StaffLayout::PlainBar:
        painter->drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        break;

    case StaffLayout::DoubleBar:
        painter->drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        painter->drawRect(bx + m_baseBarThickness * 3, by,
                          m_baseBarThickness, m_barLineHeight);
        break;

    case StaffLayout::HeavyDoubleBar:
        bx -= m_baseBarThickness * 5;
        painter->drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        painter->drawRect(bx + m_baseBarThickness * 3, by,
                          m_baseBarThickness * 3, m_barLineHeight);
        break;

    case StaffLayout::RepeatEndBar:
        bx -= m_baseBarThickness * 5 + m_lineSpacing * 2 / 3;
        painter->drawEllipse(bx, by + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                             m_lineSpacing / 3, m_lineSpacing / 3);
        painter->drawEllipse(bx, by + m_barLineHeight / 2 + (m_lineSpacing / 3),
                             m_lineSpacing / 3, m_lineSpacing / 3);
        bx += m_lineSpacing * 2 / 3;
        painter->drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        painter->drawRect(bx + m_baseBarThickness * 3, by,
                          m_baseBarThickness * 3, m_barLineHeight);
        break;

    case StaffLayout::RepeatStartBar:

        if (m_inset > 0) {
            painter->drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
            bx += m_inset;
        }

        painter->drawRect(bx, by, m_baseBarThickness * 3, m_barLineHeight);
        painter->drawRect(bx + m_baseBarThickness * 5, by,
                          m_baseBarThickness, m_barLineHeight);
        bx += m_baseBarThickness * 6 + (m_lineSpacing / 3);
        painter->drawEllipse(bx, by + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                             m_lineSpacing / 3, m_lineSpacing / 3);
        painter->drawEllipse(bx, by + m_barLineHeight / 2 + (m_lineSpacing / 3),
                             m_lineSpacing / 3, m_lineSpacing / 3);
        break;

    case StaffLayout::RepeatBothBar:

        if (m_inset > 0) {
            painter->drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
            bx += m_inset;
        }

        bx -= m_baseBarThickness * 4 + m_lineSpacing * 2 / 3;
        painter->drawEllipse(bx, by + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                             m_lineSpacing / 3, m_lineSpacing / 3);
        painter->drawEllipse(bx, by + m_barLineHeight / 2 + (m_lineSpacing / 3),
                             m_lineSpacing / 3, m_lineSpacing / 3);
        bx += m_lineSpacing * 2 / 3;
        painter->drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        painter->drawRect(bx + m_baseBarThickness * 3, by,
                          m_baseBarThickness * 3, m_barLineHeight);
        painter->drawRect(bx + m_baseBarThickness * 8, by,
                          m_baseBarThickness, m_barLineHeight);
        bx += m_baseBarThickness * 9 + (m_lineSpacing / 3);
        painter->drawEllipse(bx, by + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                             m_lineSpacing / 3, m_lineSpacing / 3);
        painter->drawEllipse(bx, by + m_barLineHeight / 2 + (m_lineSpacing / 3),
                             m_lineSpacing / 3, m_lineSpacing / 3);
        
        break;

    case StaffLayout::NoVisibleBar:
        break;
    }

    painter->restore();
}


}
