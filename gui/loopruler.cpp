// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include <qpainter.h>

#include "loopruler.h"
#include "colours.h"
#include "Event.h"
#include "rosedebug.h"
#include "RulerScale.h"

using Rosegarden::RulerScale;

LoopRuler::LoopRuler(RulerScale *rulerScale,
                     int height,
		     bool invert,
                     QWidget *parent,
                     const char *name)
    : QWidget(parent, name),
      m_height(height),
      m_invert(invert),
      m_lastXPaint(0),
      m_currentXOffset(0),
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
    m_currentXOffset = -x;

    repaint();
}

QSize LoopRuler::sizeHint() const
{
    double width = m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) -
	m_rulerScale->getBarPosition(m_rulerScale->getFirstVisibleBar());

    return QSize(int(width), m_height);
}

QSize LoopRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);

    return QSize(int(firstBarWidth), m_height);
}

void LoopRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);
    //paint.setClipRegion(e->region());
    //paint.setClipRect(e->rect().normalize());

    paint.setBrush(colorGroup().foreground());
    drawBarSections(&paint, (e->rect().x() > m_lastXPaint));
    drawLoopMarker(&paint);

    m_lastXPaint = e->rect().x();
}

void LoopRuler::drawBarSections(QPainter* paint, bool rightwards)
{
    int firstBar = m_rulerScale->getFirstVisibleBar(),
        lastBar = m_rulerScale->getLastVisibleBar();

    paint->setPen(RosegardenGUIColours::LoopRulerForeground);

    QRect clipRect = visibleRect(); //paint->clipRegion().boundingRect();

    std::vector<int> bars, beats;

    rightwards = true;//!!!

    for (int i = firstBar; i <= lastBar; ++i) {

	double x = m_rulerScale->getBarPosition(i) + m_currentXOffset;
	if (x > clipRect.x() + clipRect.width()) continue;
	    
	double width = m_rulerScale->getBarWidth(i);
	if (width == 0) continue;
	    
	if (x + width < clipRect.x()) continue;
		
	bars.push_back((int)x);

	double beatAccumulator = m_rulerScale->getBeatWidth(i);
	double inc = beatAccumulator;
	if (inc == 0) continue;
	
	for (; beatAccumulator < width; beatAccumulator += inc) {
	    beats.push_back((int)(x + beatAccumulator));
	}
    }

    for (int i = 0; i < bars.size(); ++i) {
	int n = (rightwards ? i : (bars.size() - i - 1));
	if (m_invert) {
	    paint->drawLine(bars[n], 0, bars[n], 5*m_height / 7);
	} else {
	    paint->drawLine(bars[n], 2*m_height / 7, bars[n], m_height);
	}
    }
    
    for (int i = 0; i < beats.size(); ++i) {
	int n = (rightwards ? i : (beats.size() - i - 1));
	if (m_invert) {
	    paint->drawLine(beats[n], 0, beats[n], 2*m_height / 7);
	} else {
	    paint->drawLine(beats[n], 5*m_height / 7, beats[n], m_height);
	}
    }
}

void
LoopRuler::drawLoopMarker(QPainter* paint)
{
    int x1 = (int)m_rulerScale->getXForTime(m_startLoop);
    int x2 = (int)m_rulerScale->getXForTime(m_endLoop);

    if (x1 > x2) 
    {
        x2 = x1;
        x1 = (int)m_rulerScale->getXForTime(m_endLoop);
    }

    paint->save();
    paint->setBrush(RosegardenGUIColours::LoopHighlight);
    paint->setPen(RosegardenGUIColours::LoopHighlight);
    paint->drawRect(x1, 0, x2 - x1, m_height);
    paint->restore();

}

void
LoopRuler::mousePressEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
        if (m_loop)
            m_endLoop = m_startLoop = m_grid.snapX(mE->pos().x());
        else
            emit setPointerPosition(m_rulerScale->getTimeForX(mE->pos().x()));
    }
}

void
LoopRuler::mouseReleaseEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (mE->button() == LeftButton)
    {
        if (m_loop)
        {
            // Cancel the loop if there was no drag
            //
            if (m_endLoop == m_startLoop)
            {
                m_endLoop = m_startLoop = 0;
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
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (mE->button() == LeftButton && !m_loop)
        emit setPlayPosition(m_rulerScale->getTimeForX(position));
}

void
LoopRuler::mouseMoveEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (m_loop)
    {
        m_endLoop = m_grid.snapX(mE->pos().x());
        update();

    }
    else
        emit setPointerPosition(m_rulerScale->getTimeForX(position));
}

void LoopRuler::setLoopingMode(bool value)
{
    m_loop = value;
}

void LoopRuler::setLoopMarker(Rosegarden::timeT startLoop,
                              Rosegarden::timeT endLoop)
{
    if (startLoop == endLoop) 
        return;

    m_startLoop = startLoop;
    m_endLoop = endLoop;

    QPainter paint(this);
    paint.setBrush(colorGroup().foreground());
    drawBarSections(&paint);
    drawLoopMarker(&paint);

    update();
}

