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
#include "notationproperties.h"
#include "rosestrings.h"
#include "rosedebug.h"

#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "AnalysisTypes.h"
#include "CompositionTimeSliceAdapter.h"
#include "RulerScale.h"

using Rosegarden::timeT;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::RulerScale;
using Rosegarden::Segment;
using Rosegarden::Composition;
using Rosegarden::CompositionTimeSliceAdapter;
using Rosegarden::AnalysisHelper;
using Rosegarden::Text;


ChordNameRuler::ChordNameRuler(RulerScale *rulerScale,
			       Composition *composition,
			       int height,
			       QWidget *parent,
			       const char *name) :
    QWidget(parent, name),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_rulerScale(rulerScale),
    m_composition(composition),
    m_font("helvetica", 12),
    m_boldFont("helvetica", 12, QFont::Bold),
    m_fontMetrics(m_boldFont),
    TEXT_FORMAL_X("TextFormalX"),
    TEXT_ACTUAL_X("TextActualX")
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

    timeT from = m_rulerScale->getTimeForX
	(clipRect.x() - m_currentXOffset - 100);
    timeT   to = m_rulerScale->getTimeForX
	(clipRect.x() + clipRect.width() - m_currentXOffset + 100);

    //!!! The AnalysisHelper guesses chord labels based on a particular
    // key, which it looks for in the segment passed to it.  At present
    // we have no keys in that segment.  Possibly we could stoke it up
    // with the last key preceding "from" in each segment in the
    // composition, plus any between "from" and "to".  Better if the
    // CompositionTimeSliceAdapter could locate keys in its subset of
    // segments, perhaps?

    CompositionTimeSliceAdapter adapter(m_composition, from, to + 1);
    Segment segment;
    AnalysisHelper helper;
    helper.labelChords(adapter, segment);

    QRect boundsForHeight = m_fontMetrics.boundingRect("^j|lM");
    int fontHeight = boundsForHeight.height();
    int textY = (height() - 6)/2 + fontHeight/2;

    double prevX = 0;
    timeT keyAt = from - 1;
    std::string keyText;

    for (Segment::iterator i = segment.begin(); i != segment.end(); ++i) {

	if (!(*i)->isa(Text::EventType)) continue;

	std::string text((*i)->get<String>(Text::TextPropertyName));

	if ((*i)->get<String>(Text::TextTypePropertyName) == Text::KeyName) {
	    timeT myTime = (*i)->getAbsoluteTime();
	    if (myTime == keyAt && text == keyText) continue;
	    else {
		keyAt = myTime;
		keyText = text;
	    }
	}

	double x = m_rulerScale->getXForTime((*i)->getAbsoluteTime());
	(*i)->set<Int>(TEXT_FORMAL_X, (long)x);

	QRect textBounds = m_fontMetrics.boundingRect(strtoqstr(text));
	int width = textBounds.width();

	x -= width / 2;
	if (prevX >= x - 3) x = prevX + 3;
	(*i)->set<Int>(TEXT_ACTUAL_X, (long)x);
	prevX = x + width;
    }

    for (Segment::iterator i = segment.begin(); i != segment.end(); ++i) {
	
	if (!(*i)->isa(Text::EventType)) continue;
	std::string text((*i)->get<String>(Text::TextPropertyName));
	std::string type((*i)->get<String>(Text::TextTypePropertyName));

	if (!(*i)->has(TEXT_FORMAL_X)) continue;

	long formalX = (*i)->get<Int>(TEXT_FORMAL_X);
	long actualX = (*i)->get<Int>(TEXT_ACTUAL_X);

	formalX += m_currentXOffset;
	actualX += m_currentXOffset;

	paint.drawLine(formalX, height() - 4, formalX, height());

	if (type == Text::KeyName) {
	    paint.setFont(m_boldFont);
	} else {
	    paint.setFont(m_font);
	}

	paint.drawText(actualX, textY, strtoqstr(text));
    }
}

