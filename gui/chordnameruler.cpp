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

#include "chordnameruler.h"
#include "colours.h"
#include "rosedebug.h"
#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "AnalysisTypes.h"
#include "CompositionTimeSliceAdapter.h"
#include "RulerScale.h"

using Rosegarden::timeT;
using Rosegarden::String;
using Rosegarden::RulerScale;
using Rosegarden::Segment;
using Rosegarden::Composition;
using Rosegarden::CompositionTimeSliceAdapter;
using Rosegarden::AnalysisHelper;
using Rosegarden::Segment;
using Rosegarden::Text;


ChordNameRuler::ChordNameRuler(RulerScale *rulerScale,
			       Composition *composition,
			       Segment *segment,
			       int height,
			       QWidget *parent,
			       const char *name) :
    QWidget(parent, name),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_rulerScale(rulerScale),
    m_composition(composition),
    m_segment(segment),
    m_font("helvetica", 12),
    m_boldFont("helvetica", 12, QFont::Bold),
    m_fontMetrics(m_boldFont)
{
    setBackgroundColor(RosegardenGUIColours::ChordNameRulerBackground);
}

ChordNameRuler::~ChordNameRuler()
{
}

void
ChordNameRuler::slotScrollHoriz(int x)
{
    m_currentXOffset = -x;
    repaint();
}

QSize
ChordNameRuler::sizeHint() const
{
    //!!! could be improved upon

    double width =
	m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
ChordNameRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
ChordNameRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);
    paint.setPen(RosegardenGUIColours::ChordNameRulerForeground);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    kdDebug(KDEBUG_AREA)
	<< "Range (pixels): " << (clipRect.x() - m_currentXOffset) 
	<< " -> " << (clipRect.x() + clipRect.width() - m_currentXOffset)
	<< endl;

    timeT from = m_rulerScale->getTimeForX(clipRect.x() - m_currentXOffset);
    timeT   to = m_rulerScale->getTimeForX(clipRect.x() + clipRect.width() -
					   m_currentXOffset);

    kdDebug(KDEBUG_AREA)
	<< "Range (times): " << from << " -> " << to << endl;

    CompositionTimeSliceAdapter adapter(m_composition, from, to + 1);
    Segment segment;
    AnalysisHelper helper;
    helper.labelChords(adapter, segment);

    Segment::iterator prevSi;
    if (m_segment) prevSi = m_segment->end();

    for (Segment::iterator i = segment.begin(); i != segment.end(); ++i) {
	
	if (!(*i)->isa(Text::EventType)) continue;

	std::string text;
	if (!(*i)->get<String>(Text::TextPropertyName, text)) {
	    kdDebug(KDEBUG_AREA)
		<< "Warning: ChordNameRuler::paintEvent: No text in text event"
		<< endl;
	    continue;
	}

	QString label(text.c_str());
	QRect labelBounds = m_fontMetrics.boundingRect(label);

	double x = m_rulerScale->getXForTime((*i)->getAbsoluteTime()) +
	    m_currentXOffset /* - labelBounds.width()/2 */;
	int y = height()/2 + labelBounds.height()/2;

//!!! uh -- stupid.  I wanted to use the segment at this point to 
// select a good x-coord for the label based on what events are
// actually visible at this time, but (of course) Segment doesn't
// have coords, I'd need a NotationElement.  Maybe it's not worth
// the bother at the moment -- remove all Segment stuff from this
// class?

	QString noteName(label.left(1));
	QString remainder(label.right(label.length() - 1));

	paint.setFont(m_boldFont);
	paint.drawText(x, y, noteName);
	
	QRect noteBounds = m_fontMetrics.boundingRect(noteName);

	paint.setFont(m_font);
	paint.drawText(x + noteBounds.width() + 1, y, remainder);
    }
}

