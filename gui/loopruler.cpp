// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <qpainter.h>

#include "loopruler.h"
#include "colours.h"
#include "Event.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "RulerScale.h"

using Rosegarden::RulerScale;

LoopRuler::LoopRuler(RulerScale *rulerScale,
                     int height,
		     double xorigin,
		     bool invert,
                     QWidget *parent,
                     const char *name)
    : QWidget(parent, name),
      m_height(height),
      m_xorigin(xorigin),
      m_invert(invert),
      m_currentXOffset(0),
      m_width(-1),
      m_rulerScale(rulerScale),
      m_grid(rulerScale),
      m_loop(false),
      m_startLoop(0), m_endLoop(0)
{
    setBackgroundColor(RosegardenGUIColours::LoopRulerBackground);

    // Only allow loops to be snapped to Beats on this grid
    //
    m_grid.setSnapTime(Rosegarden::SnapGrid::SnapToBeat);

}

LoopRuler::~LoopRuler()
{
}

void LoopRuler::scrollHoriz(int x)
{
    m_currentXOffset = int(double(-x)/getHScaleFactor());

    repaint();
}

QSize LoopRuler::sizeHint() const
{
    double width =
	m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) +
	m_xorigin;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize LoopRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;

    QSize res = QSize(int(firstBarWidth), m_height);

    return res;
}

void LoopRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);

    if (getHScaleFactor() != 1.0) paint.scale(getHScaleFactor(), 1.0);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.setBrush(colorGroup().foreground());
    drawBarSections(&paint);
    drawLoopMarker(&paint);
}

void LoopRuler::drawBarSections(QPainter* paint)
{
    QRect clipRect = paint->clipRegion().boundingRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
					    m_currentXOffset -
					    m_xorigin);
    int  lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
	firstBar = m_rulerScale->getFirstVisibleBar();
    }

    paint->setPen(RosegardenGUIColours::LoopRulerForeground);

    for (int i = firstBar; i <= lastBar; ++i) {

	double x = m_rulerScale->getBarPosition(i) + m_currentXOffset + m_xorigin;
	if ((x * getHScaleFactor()) > clipRect.x() + clipRect.width()) break;
	    
	double width = m_rulerScale->getBarWidth(i);
	if (width == 0) continue;
	    
	if (x + width < clipRect.x()) continue;

	if (m_invert) {
	    paint->drawLine(int(x), 0, int(x), 5*m_height / 7);
	} else {
	    paint->drawLine(int(x), 2*m_height / 7, int(x), m_height);
	}

	double beatAccumulator = m_rulerScale->getBeatWidth(i);
	double inc = beatAccumulator;
	if (inc == 0) continue;
	
	for (; beatAccumulator < width; beatAccumulator += inc) {
	    if (m_invert) {
		paint->drawLine(int(x + beatAccumulator), 0,
				int(x + beatAccumulator), 2*m_height / 7);
	    } else {
		paint->drawLine(int(x + beatAccumulator), 5*m_height / 7,
				int(x + beatAccumulator), m_height);
	    }
	}
    }
}

void
LoopRuler::drawLoopMarker(QPainter* paint)
{
    double x1 = (int)m_rulerScale->getXForTime(m_startLoop);
    double x2 = (int)m_rulerScale->getXForTime(m_endLoop);

    if (x1 > x2) 
    {
        x2 = x1;
        x1 = (int)m_rulerScale->getXForTime(m_endLoop);
    }

    x1 += m_currentXOffset + m_xorigin;
    x2 += m_currentXOffset + m_xorigin;

    paint->save();
    paint->setBrush(RosegardenGUIColours::LoopHighlight);
    paint->setPen(RosegardenGUIColours::LoopHighlight);
    paint->drawRect(static_cast<int>(x1), 0, static_cast<int>(x2 - x1), m_height);
    paint->restore();

}

void
LoopRuler::mousePressEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
	double x = mE->pos().x() / getHScaleFactor() - m_currentXOffset;
        
        if (m_loop)
            m_endLoop = m_startLoop = m_grid.snapX(x);
        else
	    emit setPointerPosition(m_rulerScale->getTimeForX(x));
    }
}

void
LoopRuler::mouseReleaseEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
        if (m_loop)
        {
            // Cancel the loop if there was no drag
            //
            if (m_endLoop == m_startLoop)
            {
                m_endLoop = m_startLoop = 0;

                // to clear any other loop rulers
                emit setLoop(m_startLoop, m_endLoop);
                update();
            }

            // emit with the args around the right way
            //
            if (m_endLoop < m_startLoop)
                emit setLoop(m_endLoop, m_startLoop);
            else
                emit setLoop(m_startLoop, m_endLoop);
        }
    }
}

void
LoopRuler::mouseDoubleClickEvent(QMouseEvent *mE)
{
    double x = mE->pos().x() / getHScaleFactor() - m_currentXOffset;
    if (x < 0) x = 0;
    
    if (mE->button() == LeftButton && !m_loop)
        emit setPlayPosition(m_rulerScale->getTimeForX(x));
}

void
LoopRuler::mouseMoveEvent(QMouseEvent *mE)
{
    double x = mE->pos().x() / getHScaleFactor() - m_currentXOffset;
    if (x < 0) x = 0;
    
    if (m_loop)
    {
        if (m_grid.snapX(x) != m_endLoop)
        {
           m_endLoop = m_grid.snapX(x);
           update();
        }
    }
    else
        emit setPointerPosition(m_rulerScale->getTimeForX(x));
}

void LoopRuler::slotSetLoopingMode(bool value)
{
    m_loop = value;
}

void LoopRuler::slotSetLoopMarker(Rosegarden::timeT startLoop,
				  Rosegarden::timeT endLoop)
{
    m_startLoop = startLoop;
    m_endLoop = endLoop;

    QPainter paint(this);
    paint.setBrush(colorGroup().foreground());
    drawBarSections(&paint);
    drawLoopMarker(&paint);

    update();
}

