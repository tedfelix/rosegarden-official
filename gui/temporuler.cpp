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

#include "temporuler.h"
#include "colours.h"
#include "rosedebug.h"
#include "Composition.h"
#include "RulerScale.h"

using Rosegarden::RulerScale;
using Rosegarden::Composition;
using Rosegarden::timeT;


TempoRuler::TempoRuler(RulerScale *rulerScale,
		       Composition *composition,
		       int height,
		       QWidget *parent,
		       const char *name)
    : QWidget(parent, name),
      m_height(height),
      m_currentXOffset(0),
      m_width(-1),
      m_composition(composition),
      m_rulerScale(rulerScale),
      m_font("helvetica", 12),
      m_fontMetrics(m_font)
{
    setBackgroundColor(RosegardenGUIColours::TextRulerBackground);
    m_font.setPixelSize(10);
}

TempoRuler::~TempoRuler()
{
    // nothing
}

void
TempoRuler::slotScrollHoriz(int x)
{
    m_currentXOffset = -x;
    repaint();
}

QSize
TempoRuler::sizeHint() const
{
    //!!! could be improved upon

    double width =
	m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
TempoRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
TempoRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);
    paint.setPen(RosegardenGUIColours::TextRulerForeground);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
	(clipRect.x() - m_currentXOffset - 100);
    timeT   to = m_rulerScale->getTimeForX
	(clipRect.x() + clipRect.width() - m_currentXOffset + 100);

    QRect boundsForHeight = m_fontMetrics.boundingRect("^j|lM");
    int fontHeight = boundsForHeight.height();
    int textY = (height() - 6)/2 + fontHeight/2;

    for (int tempoNo = m_composition->getTempoChangeNumberAt(from) + 1;
	 tempoNo <= m_composition->getTempoChangeNumberAt(to); ++tempoNo) {

	std::pair<timeT, long> tempoChange =
	    m_composition->getRawTempoChange(tempoNo);

	timeT time = tempoChange.first;
	long tempo = tempoChange.second;
	QString tempoString = QString("%1").arg(tempo / 60);
	QRect bounds = m_fontMetrics.boundingRect(tempoString);

	double x = m_rulerScale->getXForTime(time) + m_currentXOffset;
	paint.drawLine(x, height() - 4, x, height());

	x -= bounds.width() / 2;
	paint.drawText(x, textY, tempoString);
    }
}

