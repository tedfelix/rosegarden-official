// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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

#include <klocale.h>

#include <qpainter.h>
#include <qtooltip.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qframe.h>

#include "controlruler.h"
#include "colours.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "Segment.h"
#include "RulerScale.h"
#include "velocitycolour.h"

using Rosegarden::RulerScale;
using Rosegarden::Segment;
using Rosegarden::timeT;
using Rosegarden::PropertyName;


ControlRuler::ControlRuler(RulerScale *rulerScale,
                           Segment *segment,
                           const PropertyName &property,
                           VelocityColour *velocityColour,
                           double xorigin,
                           int height,
                           QWidget *parent,
                           const char *name) :
    QWidget(parent, name),
    m_propertyName(property),
    m_xorigin(xorigin),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_segment(segment),
    m_rulerScale(rulerScale),
    m_fontMetrics(m_boldFont),
    m_velocityColour(velocityColour)
{
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);

    setBackgroundColor(RosegardenGUIColours::SegmentCanvas);

    QString tip = strtoqstr(property) + i18n(" controller");
    QToolTip::add(this, tip);
}

ControlRuler::~ControlRuler()
{
    // nothing
}

void
ControlRuler::slotScrollHoriz(int x)
{
    m_currentXOffset = int((double(-x)) / getHScaleFactor());
    repaint();
}

QSize
ControlRuler::sizeHint() const
{
    double width =
       m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
       m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) +
       m_xorigin;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
ControlRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
ControlRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);

    if (getHScaleFactor() != 1.0) paint.scale(getHScaleFactor(), 1.0);

    paint.setPen(RosegardenGUIColours::MatrixElementBorder);

    if (m_velocityColour == 0)
       paint.setBrush(RosegardenGUIColours::MatrixElementBlock);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
       (clipRect.x() - m_currentXOffset - m_xorigin);
//     timeT   to = m_rulerScale->getTimeForX
//        (clipRect.x() + clipRect.width() - m_currentXOffset + 100 - m_xorigin);


    Segment::iterator it = m_segment->findNearestTime(from);
    //Segment::iterator it = m_segment->begin();

    for (; m_segment->isBeforeEndMarker(it); it++)
    {
        if ((*it)->has(m_propertyName))
        {
            int x = int(m_rulerScale->getXForTime((*it)->getAbsoluteTime()))
                    + m_currentXOffset + int(m_xorigin);

            if ((x * getHScaleFactor()) > (clipRect.x() + clipRect.width())) break;

            // include fiddle factor (+2)
            int width = 
                int(m_rulerScale->getXForTime((*it)->getAbsoluteTime() +
                                              (*it)->getDuration()) + 2)
                    + m_currentXOffset + int(m_xorigin) - x;

            int value = (*it)->get<Rosegarden::Int>(m_propertyName);
            int blockHeight = int(double(height()) * (value/127.0));

            if (m_velocityColour)
                paint.setBrush(m_velocityColour->getColour(value));
            
            paint.drawRect(x, height() - blockHeight, width, blockHeight);
        }
    }
}


// ----------------------------- ControlBox -------------------------------
//

ControlBox::ControlBox(QString label,
                       int width,
                       int height,
                       QWidget *parent,
                       const char *name):
        QWidget(parent, name),
        m_label(label),
        m_width(width),
        m_height(height)
{
}

QSize
ControlBox::sizeHint() const
{
    return QSize(m_width, m_height);
}


QSize
ControlBox::minimumSizeHint() const
{
    return QSize(m_width, m_height);
}

void
ControlBox::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);

    paint.setPen(RosegardenGUIColours::MatrixElementBorder);
    //paint.setBrush(RosegardenGUIColours::MatrixElementBlock);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.drawRect(2, 2, m_width - 3, m_height - 3);
    paint.drawText(10, 2 * m_height / 3, m_label);
}

