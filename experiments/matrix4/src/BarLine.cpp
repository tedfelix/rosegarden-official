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

#include "BarLine.h"

#include <qpainter.h>

namespace Rosegarden {
        
void
BarLine::paint(QPainter *painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    painter->save();
    painter->setPen(m_pen);
    painter->setBrush(m_brush);
    
    switch (m_style) {

    case LinedStaff::PlainBar:
        painter.drawRect(QRectF(x(), y(), m_baseBarThickness, m_barLineHeight));
        break;

    case LinedStaff::DoubleBar:
        painter.drawRect(QRectF(x(), y(), m_baseBarThickness, m_barLineHeight));
        painter.drawRect(QRectF(x() + m_baseBarThickness * 3, y(),
                         m_baseBarThickness, m_barLineHeight));
        break;

    case LinedStaff::HeavyDoubleBar:
        qreal bx = x() - m_baseBarThickness * 5;
        painter.drawRect(QRectF(bx, y(), m_baseBarThickness, m_barLineHeight));
        painter.drawRect(QRectF(bx + m_baseBarThickness * 3, y(),
                         m_baseBarThickness * 3, m_barLineHeight));
        break;

    case LinedStaff::RepeatEndBar:
        qreal bx = x() - m_baseBarThickness * 5 + m_lineSpacing * 2 / 3;
        painter.drawEllipse(QRectF(bx, y() + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3));
        painter.drawEllipse(QRectF(bx, y() + m_barLineHeight / 2 + (m_lineSpacing / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3));
        bx += m_lineSpacing * 2 / 3;
        painter.drawRect(QRectF(bx, y(), m_baseBarThickness, m_barLineHeight));
        painter.drawRect(QRectF(bx + m_baseBarThickness * 3, y(),
                         m_baseBarThickness * 3, m_barLineHeight));
        break;

    case LinedStaff::RepeatStartBar:

        qreal bx = x();
        if (m_inset > 0) {
            painter.drawRect(QRectF(bx, y(), m_baseBarThickness, m_barLineHeight));
            bx += m_inset;
        }

        painter.drawRect(QRectF(bx, y(), m_baseBarThickness * 3, m_barLineHeight));
        painter.drawRect(QRectF(bx + m_baseBarThickness * 5, y(),
                         m_baseBarThickness, m_barLineHeight));
        bx += m_baseBarThickness * 6 + (m_lineSpacing / 3);
        painter.drawEllipse(QRectF(bx, y() + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3));
        painter.drawEllipse(QRectF(bx, y() + m_barLineHeight / 2 + (m_lineSpacing / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3));
        break;

    case LinedStaff::RepeatBothBar:

        qreal bx = x();
        if (m_inset > 0) {
            painter.drawRect(QRectF(bx, y(), m_baseBarThickness, m_barLineHeight));
            bx += m_inset;
        }

        bx -= m_baseBarThickness * 4 + m_lineSpacing * 2 / 3;
        painter.drawEllipse(QRectF(bx, y() + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3));
        painter.drawEllipse(QRectF(bx, y() + m_barLineHeight / 2 + (m_lineSpacing / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3));
        bx += m_lineSpacing * 2 / 3;
        painter.drawRect(QRectF(bx, y(), m_baseBarThickness, m_barLineHeight));
        painter.drawRect(QRectF(bx + m_baseBarThickness * 3, y(),
                         m_baseBarThickness * 3, m_barLineHeight));
        painter.drawRect(QRectF(bx + m_baseBarThickness * 8, y(),
                         m_baseBarThickness, m_barLineHeight));
        bx += m_baseBarThickness * 9 + (m_lineSpacing / 3);
        painter.drawEllipse(QRectF(bx, y() + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3));
        painter.drawEllipse(QRectF(bx, y() + m_barLineHeight / 2 + (m_lineSpacing / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3));
        
        break;

    case LinedStaff::NoVisibleBar:
        break;
    }
}

QRectF
BarLine::boundingRect() const
{
    qreal x0 = x(), y0 = y(), x1, y1 = y() + m_barLineHeight;

    switch (m_style) {

    case LinedStaff::PlainBar:
        x1 = x0 + m_baseBarThickness;
        break;

    case LinedStaff::DoubleBar:
        x1 = x0 + m_baseBarThickness * 4;
        break;

    case LinedStaff::HeavyDoubleBar:
        x0 -= m_baseBarThickness * 6;
        x1 = x();
        break;

    case LinedStaff::RepeatEndBar:
        x0 -= m_baseBarThickness * 6 + m_lineSpacing * 2 / 3;
        x1 = x();
        break;

    case LinedStaff::RepeatStartBar:
        x1 = x0 + m_baseBarThickness * 6 + m_lineSpacing * 2 / 3 + m_inset;
        break;

    case LinedStaff::RepeatBothBar:
        x0 -= m_baseBarThickness * 4 + m_lineSpacing * 2 / 3;
        x1 = x0 + m_baseBarThickness * 9 + m_lineSpacing * 2 / 3 + m_inset;
        break;

    case LinedStaff::NoVisibleBar:
        x1 = x0 + 1;
        break;
    }

    QRectF p(x0, y0, x1, y1);
    return p;
}

}
