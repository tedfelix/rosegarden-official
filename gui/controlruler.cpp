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
    m_currentXOffset = -x;
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

    paint.setPen(RosegardenGUIColours::MatrixElementBorder);

    if (m_velocityColour == 0)
       paint.setBrush(RosegardenGUIColours::MatrixElementBlock);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    //QRect clipRect = paint.clipRegion().boundingRect();
    //paint.drawRect(0, 0, 100, 100);

    Segment::iterator it = m_segment->begin();

    for (; it != m_segment->end(); it++)
    {
        if ((*it)->has(m_propertyName))
        {
            int x = int(m_rulerScale->getXForTime((*it)->getAbsoluteTime()))
                    + m_currentXOffset + int(m_xorigin);

            int width = 
                int(m_rulerScale->getXForTime((*it)->getAbsoluteTime() +
                                              (*it)->getDuration()))
                    + m_currentXOffset + int(m_xorigin) - x;

            int value = (*it)->get<Rosegarden::Int>(m_propertyName);
            int blockHeight = int(double(height()) * (value/127.0));

            if (m_velocityColour)
                paint.setBrush(m_velocityColour->getColour(value));
            
            paint.drawRect(x, height() - blockHeight, width, blockHeight);
        }
    }

    /*
    timeT from = m_rulerScale->getTimeForX
       (clipRect.x() - m_currentXOffset - 100 - m_xorigin);
    timeT   to = m_rulerScale->getTimeForX
       (clipRect.x() + clipRect.width() - m_currentXOffset + 100 - m_xorigin);

    QRect boundsForHeight = m_fontMetrics.boundingRect("^j|lM");
    int fontHeight = boundsForHeight.height();
    int textY = (height() - 6)/2 + fontHeight/2;

    double prevEndX = -1000.0;
    double prevTempo = 0.0;
    long prevBpm = 0;

    typedef std::map<timeT, bool> TimePoints;
    int tempoChangeHere = 1, timeSigChangeHere = 2;
    TimePoints timePoints;

    for (int tempoNo = m_composition->getTempoChangeNumberAt(from) + 1;
        tempoNo <= m_composition->getTempoChangeNumberAt(to); ++tempoNo) {

       timePoints.insert
           (TimePoints::value_type
            (m_composition->getRawTempoChange(tempoNo).first,
             tempoChangeHere));
    }
       
    for (int sigNo = m_composition->getTimeSignatureNumberAt(from) + 1;
        sigNo <= m_composition->getTimeSignatureNumberAt(to); ++sigNo) {

       timeT time(m_composition->getTimeSignatureChange(sigNo).first);
       if (timePoints.find(time) != timePoints.end()) {
           timePoints[time] |= timeSigChangeHere;
       } else {
           timePoints.insert(TimePoints::value_type(time, timeSigChangeHere));
       }
    }

    for (TimePoints::iterator i = timePoints.begin(); ; ++i) {

       timeT t0, t1;

       if (i == timePoints.begin()) {
           t0 = from;
       } else {
           TimePoints::iterator j(i);
           --j;
           t0 = j->first;
       }

       if (i == timePoints.end()) {
           t1 = to;
       } else {
           t1 = i->first;
       }

       QColor colour = TempoColour::getColour(m_composition->getTempoAt(t0));
        paint.setPen(colour);
        paint.setBrush(colour);

       double x0, x1;
       x0 = m_rulerScale->getXForTime(t0) + m_currentXOffset + m_xorigin;
       x1 = m_rulerScale->getXForTime(t1) + m_currentXOffset + m_xorigin;
        paint.drawRect(x0, 0, x1 - x0, height());

       if (i == timePoints.end()) break;
    }

    paint.setPen(Qt::black);
    paint.setBrush(Qt::black);

    for (TimePoints::iterator i = timePoints.begin();
        i != timePoints.end(); ++i) {

       timeT time = i->first;
       double x = m_rulerScale->getXForTime(time) + m_currentXOffset
                   + m_xorigin;
       
       if (i->second & timeSigChangeHere) {

           Rosegarden::TimeSignature sig =
              m_composition->getTimeSignatureAt(time);

           QString numStr = QString("%1").arg(sig.getNumerator());
           QString denStr = QString("%1").arg(sig.getDenominator());

           QRect numBounds = m_fontMetrics.boundingRect(numStr);
           QRect denBounds = m_fontMetrics.boundingRect(denStr);

           paint.setFont(m_boldFont);
           paint.drawText(x - numBounds.width()/2,
                        numBounds.height(), numStr);
           paint.drawText(x - denBounds.width()/2,
                        numBounds.height() + denBounds.height(), denStr);
       }

       if (i->second & tempoChangeHere) { 
       
           double tempo = m_composition->getTempoAt(time);
           long bpm = long(tempo);

           QString tempoString = QString("%1").arg(bpm);
           if (tempo == prevTempo) {
              if (m_small) continue;
              tempoString = "=";
           } else if (bpm == prevBpm) {
              tempoString = (tempo > prevTempo ? "+" : "-");
           } else {
              if (m_small && (bpm != (bpm / 10 * 10))) {
                  if (bpm == prevBpm + 1) tempoString = "+";
                  else if (bpm == prevBpm - 1) tempoString = "-";
              }
           }
           prevTempo = tempo;
           prevBpm = bpm;

           QRect bounds = m_fontMetrics.boundingRect(tempoString);

           paint.setFont(m_font);
           if (x > bounds.width() / 2) x -= bounds.width() / 2;
           if (prevEndX >= x - 3) x = prevEndX + 3;
           paint.drawText(x, textY, tempoString);
           prevEndX = x + bounds.width();
       }
    }
    */
}

ControlBox::ControlBox(QString label,
                       int width,
                       int height,
                       QWidget *parent,
                       const char *name):
        QWidget(parent, name),
        m_label(label + i18n(" control box")),
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

