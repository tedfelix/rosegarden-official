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

#include "chordnameruler.h"
#include "colours.h"
#include "notationproperties.h"
#include "rosestrings.h"
#include "rosedebug.h"

#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "AnalysisTypes.h"
#include "SegmentNotationHelper.h"
#include "Composition.h"
#include "CompositionTimeSliceAdapter.h"
#include "RulerScale.h"

using Rosegarden::timeT;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::RulerScale;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Composition;
using Rosegarden::CompositionTimeSliceAdapter;
using Rosegarden::AnalysisHelper;
using Rosegarden::Text;


ChordNameRuler::ChordNameRuler(RulerScale *rulerScale,
			       Composition *composition,
			       double xorigin,
			       int height,
			       QWidget *parent,
			       const char *name) :
    QWidget(parent, name),
    m_xorigin(xorigin),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_rulerScale(rulerScale),
    m_composition(composition),
    m_currentSegment(0),
    m_fontMetrics(m_boldFont),
    TEXT_FORMAL_X("TextFormalX"),
    TEXT_ACTUAL_X("TextActualX"),
    m_labelSegment(0),
    m_needRecalc(true)
{
    m_font.setPointSize(11);
    m_font.setPixelSize(12);
    m_boldFont.setPointSize(11);
    m_boldFont.setPixelSize(12);
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);
    setBackgroundColor(RosegardenGUIColours::ChordNameRulerBackground);
}

ChordNameRuler::~ChordNameRuler()
{
    if (m_currentSegment) {
	m_currentSegment->removeObserver(this);
    }
}

void
ChordNameRuler::setComposition(Rosegarden::Composition *composition)
{
    if (m_currentSegment) {
	m_currentSegment->removeObserver(this);
    }
    m_composition = composition;
    m_currentSegment = 0;
}

void
ChordNameRuler::setCurrentSegment(Rosegarden::Segment *segment)
{
    if (m_currentSegment) {
	m_currentSegment->removeObserver(this);
    }
    m_currentSegment = segment;
    m_currentSegment->addObserver(this);
}

void
ChordNameRuler::eventAdded(const Segment *s, Rosegarden::Event *)
{
    if (s == m_currentSegment) m_needRecalc = true;
}

void
ChordNameRuler::eventRemoved(const Segment *s, Rosegarden::Event *)
{
    if (s == m_currentSegment) m_needRecalc = true;
}

void
ChordNameRuler::endMarkerTimeChanged(const Segment *s, bool)
{
    if (s == m_currentSegment) m_needRecalc = true;
}

void
ChordNameRuler::slotScrollHoriz(int x)
{
    int w = width(), h = height();
    int dx = x - (-m_currentXOffset);
    m_currentXOffset = -x;

    if (dx > w*3/4 || dx < -w*3/4) {
	update();
	return;
    }

    if (dx > 0) { // moving right, so the existing stuff moves left
	bitBlt(this, 0, 0, this, dx, 0, w-dx, h);
	repaint(w-dx, 0, dx, h);
    } else {      // moving left, so the existing stuff moves right
	bitBlt(this, -dx, 0, this, 0, 0, w+dx, h);
	repaint(0, 0, -dx, h);
    }
}

QSize
ChordNameRuler::sizeHint() const
{
    double width =
	m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    NOTATION_DEBUG << "Returning chord-label ruler width as " << width << endl;

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
    if (!m_composition) return;

    QPainter paint(this);
    paint.setPen(RosegardenGUIColours::ChordNameRulerForeground);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
	(clipRect.x() - m_currentXOffset - m_xorigin - 100);
    timeT   to = m_rulerScale->getTimeForX
	(clipRect.x() + clipRect.width() - m_currentXOffset - m_xorigin + 100);

//!!! This optimisation doesn't work -- it'd demand that we were
//notified each time anything in any segment changed (as we might not
//have a current segment and in any case show chords from across all
//segments) -- remove segment observer stuff from this class unless 
//a cleverer way turns up
//   if (m_needRecalc || !m_labelSegment) {

    if (true) {

	if (m_labelSegment) m_labelSegment->clear();
	else m_labelSegment = new Segment();

	CompositionTimeSliceAdapter adapter(m_composition, from, to + 1);

	// Populate the segment with the current clef and key at the
	// time at which analysis starts, taken from the "current segment"
	// if we have one.  If we don't have one, use the first segment
	// that contains our start time

	Segment *clefKeySegment = m_currentSegment;
	if (!clefKeySegment) {
	    for (Composition::iterator ci = m_composition->begin();
		 ci != m_composition->end(); ++ci) {
		if ((*ci)->getStartTime() <= from &&
		    (*ci)->getEndMarkerTime() >= from) {
		    clefKeySegment = *ci;
		    break;
		}
	    }
	    if (!clefKeySegment) return;
	}
	
	Rosegarden::Clef clef = clefKeySegment->getClefAtTime(from);
	m_labelSegment->insert(clef.getAsEvent(from - 1));
	
	Rosegarden::Key key = clefKeySegment->getKeyAtTime(from);
	m_labelSegment->insert(key.getAsEvent(from - 1));
	
	AnalysisHelper helper;
	helper.labelChords(adapter, *m_labelSegment);

	m_needRecalc = false;
    }

    QRect boundsForHeight = m_fontMetrics.boundingRect("^j|lM");
    int fontHeight = boundsForHeight.height();
    int textY = (height() - 6)/2 + fontHeight/2;

    double prevX = 0;
    timeT keyAt = from - 1;
    std::string keyText;

    for (Segment::iterator i = m_labelSegment->findTime(from);
	 i != m_labelSegment->findTime(to) &&
	     m_labelSegment->isBeforeEndMarker(i); ++i) {
	
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
	(*i)->set<Int>(TEXT_ACTUAL_X, long(x));
	prevX = x + width;
    }

    for (Segment::iterator i = m_labelSegment->findTime(from);
	 i != m_labelSegment->findTime(to) &&
	     m_labelSegment->isBeforeEndMarker(i); ++i) {
	
	if (!(*i)->isa(Text::EventType)) continue;
	std::string text((*i)->get<String>(Text::TextPropertyName));
	std::string type((*i)->get<String>(Text::TextTypePropertyName));

	if (!(*i)->has(TEXT_FORMAL_X)) continue;

	long formalX = (*i)->get<Int>(TEXT_FORMAL_X);
	long actualX = (*i)->get<Int>(TEXT_ACTUAL_X);

	formalX += m_currentXOffset + long(m_xorigin);
	actualX += m_currentXOffset + long(m_xorigin);

	paint.drawLine(formalX, height() - 4, formalX, height());

	if (type == Text::KeyName) {
	    paint.setFont(m_boldFont);
	} else {
	    paint.setFont(m_font);
	}

	paint.drawText(actualX, textY, strtoqstr(text));
    }
}

