/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "PropertyViewRuler.h"

#include "base/Event.h"
#include <klocale.h>
#include "misc/Strings.h"
#include "base/PropertyName.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "DefaultVelocityColour.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/HZoomable.h"
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qrect.h>
#include <qsize.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qwidget.h>


namespace Rosegarden
{

PropertyViewRuler::PropertyViewRuler(RulerScale *rulerScale,
                                     Segment *segment,
                                     const PropertyName &property,
                                     double xorigin,
                                     int height,
                                     QWidget *parent,
                                     const char *name) :
        QWidget(parent, name),
        m_propertyName(property),
        m_xorigin(xorigin),
        m_height(height),
        m_currentXOffset(0),
        m_width( -1),
        m_segment(segment),
        m_rulerScale(rulerScale),
        m_fontMetrics(m_boldFont)
{
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);

    setBackgroundColor(GUIPalette::getColour(GUIPalette::SegmentCanvas));

    QString tip = i18n("%1 controller").arg(strtoqstr(property));
    QToolTip::add
        (this, tip);
}

PropertyViewRuler::~PropertyViewRuler()
{
    // nothing
}

void
PropertyViewRuler::slotScrollHoriz(int x)
{
    int w = width(), h = height();
    x = int(double(x) / getHScaleFactor());
    int dx = x - ( -m_currentXOffset);
    m_currentXOffset = -x;

    if (dx > w*3 / 4 || dx < -w*3 / 4) {
        update();
        return ;
    }

    if (dx > 0) { // moving right, so the existing stuff moves left
        bitBlt(this, 0, 0, this, dx, 0, w - dx, h);
        repaint(w - dx, 0, dx, h);
    } else {      // moving left, so the existing stuff moves right
        bitBlt(this, -dx, 0, this, 0, 0, w + dx, h);
        repaint(0, 0, -dx, h);
    }
}

QSize
PropertyViewRuler::sizeHint() const
{
    double width =
        m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
        m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) +
        m_xorigin;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
PropertyViewRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
PropertyViewRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);

    if (getHScaleFactor() != 1.0)
        paint.scale(getHScaleFactor(), 1.0);

    paint.setPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder));

    QRect clipRect = e->rect().normalize();

    timeT from = m_rulerScale->getTimeForX
                 (clipRect.x() - m_currentXOffset - m_xorigin);

    Segment::iterator it = m_segment->findNearestTime(from);

    for (; m_segment->isBeforeEndMarker(it); it++) {
        long value = 0;

        if (!(*it)->get
                <Int>(m_propertyName, value))
            continue;

        int x = int(m_rulerScale->getXForTime((*it)->getAbsoluteTime()))
                + m_currentXOffset + int(m_xorigin);

        int xPos = x * int(getHScaleFactor());

        if (xPos < clipRect.x())
            continue;

        if (xPos > (clipRect.x() + clipRect.width()))
            break;

        // include fiddle factor (+2)
        int width =
            int(m_rulerScale->getXForTime((*it)->getAbsoluteTime() +
                                          (*it)->getDuration()) + 2)
            + m_currentXOffset + int(m_xorigin) - x;

        int blockHeight = int(double(height()) * (value / 127.0));

        paint.setBrush(DefaultVelocityColour::getInstance()->getColour(value));

        paint.drawRect(x, height() - blockHeight, width, blockHeight);
    }
}

}
#include "PropertyViewRuler.moc"
