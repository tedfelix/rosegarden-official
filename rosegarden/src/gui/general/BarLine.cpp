/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
BarLine::drawShape(QPainter &painter)
{
    int bx = int(x());
    int by = int(y());

    switch (m_style) {

    case LinedStaff::PlainBar:
        painter.drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        break;

    case LinedStaff::DoubleBar:
        painter.drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        painter.drawRect(bx + m_baseBarThickness * 3, by,
                         m_baseBarThickness, m_barLineHeight);
        break;

    case LinedStaff::HeavyDoubleBar:
        bx -= m_baseBarThickness * 5;
        painter.drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        painter.drawRect(bx + m_baseBarThickness * 3, by,
                         m_baseBarThickness * 3, m_barLineHeight);
        break;

    case LinedStaff::RepeatEndBar:
        bx -= m_baseBarThickness * 5 + m_lineSpacing * 2 / 3;
        painter.drawEllipse(bx, by + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3);
        painter.drawEllipse(bx, by + m_barLineHeight / 2 + (m_lineSpacing / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3);
        bx += m_lineSpacing * 2 / 3;
        painter.drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        painter.drawRect(bx + m_baseBarThickness * 3, by,
                         m_baseBarThickness * 3, m_barLineHeight);
        break;

    case LinedStaff::RepeatStartBar:

        if (m_inset > 0) {
            painter.drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
            bx += m_inset;
        }

        painter.drawRect(bx, by, m_baseBarThickness * 3, m_barLineHeight);
        painter.drawRect(bx + m_baseBarThickness * 5, by,
                         m_baseBarThickness, m_barLineHeight);
        bx += m_baseBarThickness * 6 + (m_lineSpacing / 3);
        painter.drawEllipse(bx, by + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3);
        painter.drawEllipse(bx, by + m_barLineHeight / 2 + (m_lineSpacing / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3);
        break;

    case LinedStaff::RepeatBothBar:

        if (m_inset > 0) {
            painter.drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
            bx += m_inset;
        }

        bx -= m_baseBarThickness * 4 + m_lineSpacing * 2 / 3;
        painter.drawEllipse(bx, by + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3);
        painter.drawEllipse(bx, by + m_barLineHeight / 2 + (m_lineSpacing / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3);
        bx += m_lineSpacing * 2 / 3;
        painter.drawRect(bx, by, m_baseBarThickness, m_barLineHeight);
        painter.drawRect(bx + m_baseBarThickness * 3, by,
                         m_baseBarThickness * 3, m_barLineHeight);
        painter.drawRect(bx + m_baseBarThickness * 8, by,
                         m_baseBarThickness, m_barLineHeight);
        bx += m_baseBarThickness * 9 + (m_lineSpacing / 3);
        painter.drawEllipse(bx, by + m_barLineHeight / 2 - (m_lineSpacing * 2 / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3);
        painter.drawEllipse(bx, by + m_barLineHeight / 2 + (m_lineSpacing / 3),
                            m_lineSpacing / 3, m_lineSpacing / 3);
        
        break;

    case LinedStaff::NoVisibleBar:
        break;
    }
}

QPointArray
BarLine::areaPoints() const
{
    int bx = int(x());
    int by = int(y());
    int x0 = bx, y0 = by, x1, y1 = by + m_barLineHeight;

    switch (m_style) {

    case LinedStaff::PlainBar:
        x1 = x0 + m_baseBarThickness;
        break;

    case LinedStaff::DoubleBar:
        x1 = x0 + m_baseBarThickness * 4;
        break;

    case LinedStaff::HeavyDoubleBar:
        x0 -= m_baseBarThickness * 6;
        x1 = bx;
        break;

    case LinedStaff::RepeatEndBar:
        x0 -= m_baseBarThickness * 6 + m_lineSpacing * 2 / 3;
        x1 = bx;
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

    QPointArray p(4);
    p[0] = QPoint(x0, y0);
    p[1] = QPoint(x1, y0);
    p[2] = QPoint(x1, y1);
    p[3] = QPoint(x0, y1);
    return p;
}

}
