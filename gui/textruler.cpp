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

#include "textruler.h"
#include "colours.h"
#include "rosedebug.h"
#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "RulerScale.h"

using Rosegarden::RulerScale;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::Text;
using Rosegarden::String;
using Rosegarden::timeT;


TextRuler::TextRuler(RulerScale *rulerScale,
		     Segment *segment,
                     int height,
                     QWidget *parent,
                     const char *name)
    : QWidget(parent, name),
      m_height(height),
      m_currentXOffset(0),
      m_width(-1),
      m_segment(segment),
      m_rulerScale(rulerScale),
      m_font("helvetica", 12),
      m_fontMetrics(m_font)
{
    m_mySegmentMaybe = (m_segment->getComposition() != 0);
    setBackgroundColor(RosegardenGUIColours::TextRulerBackground);

    m_font.setPixelSize(10);
}

TextRuler::~TextRuler()
{
    if (m_mySegmentMaybe && !m_segment->getComposition()) {
	delete m_segment;
    }
}

void
TextRuler::slotScrollHoriz(int x)
{
    m_currentXOffset = -x;
    repaint();
}

QSize
TextRuler::sizeHint() const
{
    //!!! could be improved upon

    double width =
	m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
TextRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
TextRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX(clipRect.x() - m_currentXOffset);
    timeT   to = m_rulerScale->getTimeForX(clipRect.x() + clipRect.width() -
					   m_currentXOffset);

    paint.setPen(RosegardenGUIColours::TextRulerForeground);

    for (Segment::iterator i = m_segment->findTime(from);
	 i != m_segment->findTime(to) && i != m_segment->end(); ++i) {
	
	if (!(*i)->isa(Text::EventType)) continue;

	std::string text;
	if (!(*i)->get<String>(Text::TextPropertyName, text)) {
	    kdDebug(KDEBUG_AREA)
		<< "Warning: TextRuler::paintEvent: No text in text event"
		<< endl;
	    continue;
	}

	QRect bounds = m_fontMetrics.boundingRect(text.c_str());

	double x = m_rulerScale->getXForTime((*i)->getAbsoluteTime()) +
	    m_currentXOffset - bounds.width()/2;

	int y = height()/2 + bounds.height()/2;

	paint.drawText(x, y, text.c_str());
    }
}

