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
#include "Studio.h"
#include "Track.h"
#include "Instrument.h"
#include "Profiler.h"
#include "Quantizer.h"

using Rosegarden::timeT;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::RulerScale;
using Rosegarden::Segment;
using Rosegarden::SegmentSelection;
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
    m_composition(0),
    m_needsRecalculate(true),
    m_currentSegment(0),
    m_studio(0),
    m_chordSegment(0),
    m_fontMetrics(m_boldFont),
    TEXT_FORMAL_X("TextFormalX"),
    TEXT_ACTUAL_X("TextActualX")
{
    m_font.setPointSize(11);
    m_font.setPixelSize(12);
    m_boldFont.setPointSize(11);
    m_boldFont.setPixelSize(12);
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);
    setBackgroundColor(RosegardenGUIColours::ChordNameRulerBackground);
    if (composition) setComposition(composition);
}

ChordNameRuler::~ChordNameRuler()
{
    for (SegmentSelection::iterator si = m_segments.begin();
	 si != m_segments.end(); ++si) {
	(*si)->removeObserver(this);
    }
    delete m_chordSegment;
}

void
ChordNameRuler::setComposition(Rosegarden::Composition *composition)
{
    if (m_composition == composition) return;

    for (SegmentSelection::iterator si = m_segments.begin();
	 si != m_segments.end(); ++si) {
	(*si)->removeObserver(this);
    }
    m_segments.clear();

    m_composition = composition;
    if (m_composition)
	m_compositionRefreshStatusId = composition->getNewRefreshStatusId();
    m_currentSegment = 0;
}

void
ChordNameRuler::setCurrentSegment(Rosegarden::Segment *segment)
{
    m_currentSegment = segment;
}

void
ChordNameRuler::setStudio(Rosegarden::Studio *studio)
{
    m_studio = studio;
}

void
ChordNameRuler::slotScrollHoriz(int x)
{
    int w = width(), h = height();
    int dx = x - (-m_currentXOffset);
    m_currentXOffset = -x;

    if (m_needsRecalculate) {
	update();
	return;
    }
    if (dx == 0) return;

    if (dx > w*7/8 || dx < -w*7/8) {
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

    RG_DEBUG << "Returning chord-label ruler width as " << width << endl;

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
ChordNameRuler::eventAdded(const Rosegarden::Segment *s, Rosegarden::Event *)
{
    if (m_segments.find((Rosegarden::Segment *)s) != m_segments.end())
	m_needsRecalculate = true;
}

void
ChordNameRuler::eventRemoved(const Rosegarden::Segment *s, Rosegarden::Event *)
{
    if (m_segments.find((Rosegarden::Segment *)s) != m_segments.end())
	m_needsRecalculate = true;
}

void
ChordNameRuler::endMarkerTimeChanged(const Rosegarden::Segment *s, bool)
{
    if (m_segments.find((Rosegarden::Segment *)s) != m_segments.end())
	m_needsRecalculate = true;
}

void
ChordNameRuler::segmentDeleted(const Rosegarden::Segment *s)
{
    m_segments.erase((Rosegarden::Segment *)s);
    if (m_currentSegment == s) m_currentSegment = 0;
    m_needsRecalculate = true;
}

void
ChordNameRuler::recalculate(bool regetSegments)
{
    Rosegarden::Profiler profiler("ChordNameRuler::recalculate", true);
    RG_DEBUG << "ChordNameRuler[" << this << "]::recalculate(" << regetSegments << ")" << endl;

    Rosegarden::RefreshStatus &rs =
	m_composition->getRefreshStatus(m_compositionRefreshStatusId);
    if (rs.needsRefresh()) {
	regetSegments = true;
	rs.setNeedsRefresh(false);
    }
    m_needsRecalculate = false;

    if (regetSegments) {

	SegmentSelection ss;

	for (Composition::iterator ci = m_composition->begin();
	     ci != m_composition->end(); ++ci) {

	    if (m_studio) {

		Rosegarden::TrackId ti = (*ci)->getTrack();

		Rosegarden::Instrument *instr = m_studio->getInstrumentById
		    (m_composition->getTrackById(ti)->getInstrument());

		if (instr &&
		    instr->getInstrumentType() == Rosegarden::Instrument::Midi &&
		    instr->isPercussion()) {
		    continue;
		}
	    }

	    ss.insert(*ci);
	}

	for (SegmentSelection::iterator si = m_segments.begin();
	     si != m_segments.end(); ++si) {

	    if (ss.find(*si) == ss.end()) {

		//!!! this is probably unsafe -- the segment has been removed
		// from the composition so it might have been destroyed
		(*si)->removeObserver(this);
		m_segments.erase(*si);

		RG_DEBUG << "Segment deleted, updating (now have " << m_segments.size() << " segments)" << endl;

	    }
	}

	for (SegmentSelection::iterator si = ss.begin();
	     si != ss.end(); ++si) {

	    if (m_segments.find(*si) == m_segments.end()) {
		(*si)->addObserver(this);
		m_segments.insert(*si);
		RG_DEBUG << "Segment created, adding (now have " << m_segments.size() << " segments)" << endl;

	    }
	}

	if (m_currentSegment &&
	    ss.find(m_currentSegment) == ss.end()) {
	    m_currentSegment = 0;
	}
    }	    

    if (m_chordSegment) m_chordSegment->clear();
    else m_chordSegment = new Segment();
    
    if (m_segments.empty()) return;

    if (!m_currentSegment) { //!!! arbitrary, must do better
	//!!! need a segment starting at zero or so with a clef and key in it!
	m_currentSegment = *m_segments.begin();
    }

/*!!!

	for (Composition::iterator ci = m_composition->begin();
	     ci != m_composition->end(); ++ci) {

	    if ((*ci)->getEndMarkerTime() >= from &&
		((*ci)->getStartTime() <= from ||
		 (clefKeySegment &&
		  (*ci)->getStartTime() < clefKeySegment->getStartTime()))) {

		clefKeySegment = *ci;
	    }
	}

	if (!clefKeySegment) return;
    }
*/    
    timeT clefKeyTime = m_currentSegment->getStartTime();
	//(from < m_currentSegment->getStartTime() ?
	//	        m_currentSegment->getStartTime() : from);

    Rosegarden::Clef clef = m_currentSegment->getClefAtTime(clefKeyTime);
    m_chordSegment->insert(clef.getAsEvent(-1));
    
    Rosegarden::Key key = m_currentSegment->getKeyAtTime(clefKeyTime);
    m_chordSegment->insert(key.getAsEvent(-1));

    CompositionTimeSliceAdapter adapter(m_composition, &m_segments);
    
    AnalysisHelper helper;
    helper.labelChords(adapter, *m_chordSegment, m_composition->getNotationQuantizer());
}

void
ChordNameRuler::paintEvent(QPaintEvent* e)
{
    if (!m_composition) return;

    if (m_segments.empty()) {
	
	recalculate(true);

    } else {

	Rosegarden::RefreshStatus &rs =
	    m_composition->getRefreshStatus(m_compositionRefreshStatusId);

	if (rs.needsRefresh()) {

	    recalculate(true);

	} else if (m_needsRecalculate) {

	    recalculate(false);
	}
    }

    Rosegarden::Profiler profiler("ChordNameRuler::paintEvent (body)", true);

    QPainter paint(this);
    paint.setPen(RosegardenGUIColours::ChordNameRulerForeground);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
	(clipRect.x() - m_currentXOffset - m_xorigin - 50);
    timeT   to = m_rulerScale->getTimeForX
	(clipRect.x() + clipRect.width() - m_currentXOffset - m_xorigin + 50);

    QRect boundsForHeight = m_fontMetrics.boundingRect("^j|lM");
    int fontHeight = boundsForHeight.height();
    int textY = (height() - 6)/2 + fontHeight/2;

    double prevX = 0;
    timeT keyAt = from - 1;
    std::string keyText;

    std::cerr << "*** Chord Name Ruler: paint " << from << " -> " << to << std::endl;

    for (Segment::iterator i = m_chordSegment->findTime(from);
	 i != m_chordSegment->findTime(to); ++i) {
	
	std::cerr << "type " << (*i)->getType() << " at " << (*i)->getAbsoluteTime()
		  << std::endl;

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

    for (Segment::iterator i = m_chordSegment->findTime(from);
	 i != m_chordSegment->findTime(to); ++i) {
	
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

