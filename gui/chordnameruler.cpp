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
#include "rosegardenguidoc.h"

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
			       RosegardenGUIDoc *doc,
			       double xorigin,
			       int height,
			       QWidget *parent,
			       const char *name) :
    QWidget(parent, name),
    m_xorigin(xorigin),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_ready(false),
    m_rulerScale(rulerScale),
    m_composition(&doc->getComposition()),
    m_regetSegmentsOnChange(true),
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

    m_compositionRefreshStatusId = m_composition->getNewRefreshStatusId();

    QObject::connect(doc->getCommandHistory(), SIGNAL(commandExecuted()),
		     this, SLOT(update()));
}

ChordNameRuler::ChordNameRuler(RulerScale *rulerScale,
			       RosegardenGUIDoc *doc,
			       std::vector<Rosegarden::Segment *> &segments,
			       double xorigin,
			       int height,
			       QWidget *parent,
			       const char *name) :
    QWidget(parent, name),
    m_xorigin(xorigin),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_ready(false),
    m_rulerScale(rulerScale),
    m_composition(&doc->getComposition()),
    m_regetSegmentsOnChange(false),
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

    m_compositionRefreshStatusId = m_composition->getNewRefreshStatusId();

    QObject::connect(doc->getCommandHistory(), SIGNAL(commandExecuted()),
		     this, SLOT(update()));

    for (std::vector<Rosegarden::Segment *>::iterator i = segments.begin();
	 i != segments.end(); ++i) {
	m_segments.insert(SegmentRefreshMap::value_type
			  (*i, (*i)->getNewRefreshStatusId()));
    }
}

ChordNameRuler::~ChordNameRuler()
{
    delete m_chordSegment;
}

void
ChordNameRuler::setReady()
{
    m_ready = true;
    update();
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
ChordNameRuler::recalculate(timeT from, timeT to)
{
    if (!m_ready) return;

    Rosegarden::Profiler profiler("ChordNameRuler::recalculate", true);
    NOTATION_DEBUG << "ChordNameRuler[" << this << "]::recalculate" << endl;

    bool regetSegments = false;

    enum RecalcLevel { RecalcNone, RecalcVisible, RecalcWhole };
    RecalcLevel level = RecalcNone;

    if (m_segments.empty()) {

	regetSegments = true;

    } else if (m_regetSegmentsOnChange) {

	Rosegarden::RefreshStatus &rs =
	    m_composition->getRefreshStatus(m_compositionRefreshStatusId);

	if (rs.needsRefresh()) {
	    rs.setNeedsRefresh(false);
	    regetSegments = true;
	}
    }

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

	for (SegmentRefreshMap::iterator si = m_segments.begin();
	     si != m_segments.end(); ++si) {

	    if (ss.find(si->first) == ss.end()) {
		m_segments.erase(si);
		level = RecalcWhole;
		NOTATION_DEBUG << "Segment deleted, updating (now have " << m_segments.size() << " segments)" << endl;
	    }
	}

	for (SegmentSelection::iterator si = ss.begin();
	     si != ss.end(); ++si) {

	    if (m_segments.find(*si) == m_segments.end()) {
		m_segments.insert(SegmentRefreshMap::value_type
				  (*si, (*si)->getNewRefreshStatusId()));
		level = RecalcWhole;
		NOTATION_DEBUG << "Segment created, adding (now have " << m_segments.size() << " segments)" << endl;
	    }
	}

	if (m_currentSegment &&
	    ss.find(m_currentSegment) == ss.end()) {
	    m_currentSegment = 0;
	    level = RecalcWhole;
	}
    }	    

    if (!m_chordSegment) m_chordSegment = new Segment();
    if (m_segments.empty()) return;

    Rosegarden::SegmentRefreshStatus overallStatus;
    overallStatus.setNeedsRefresh(false);

    for (SegmentRefreshMap::iterator i = m_segments.begin();
	 i != m_segments.end(); ++i) {
	Rosegarden::SegmentRefreshStatus &status =
	    i->first->getRefreshStatus(i->second);
	if (status.needsRefresh()) {
	    overallStatus.push(status.from(), status.to());
	}
    }

    // We now have the overall area affected by these changes, across
    // all segments.  If it's entirely within our displayed area, just
    // recalculate the displayed area; if it overlaps, calculate the
    // union of the two areas; if it's entirely without, calculate
    // nothing.

    if (level == RecalcNone) {
	if (from == to) {
	    NOTATION_DEBUG << "ChordNameRuler::recalculate: from==to, recalculating all" << endl;
	    level = RecalcWhole;
	} else if (overallStatus.from() == overallStatus.to()) {
	    NOTATION_DEBUG << "ChordNameRuler::recalculate: overallStatus.from==overallStatus.to, ignoring" << endl;
	    level = RecalcNone;
	} else if (overallStatus.from() >= from && overallStatus.to() <= to) {
	    NOTATION_DEBUG << "ChordNameRuler::recalculate: change is " << overallStatus.from() << "->" << overallStatus.to() << ", I show " << from << "->" << to << ", recalculating visible area" << endl;
	    level = RecalcVisible;
	} else if (overallStatus.from() >= to || overallStatus.to() <= from) {
	    NOTATION_DEBUG << "ChordNameRuler::recalculate: change is " << overallStatus.from() << "->" << overallStatus.to() << ", I show " << from << "->" << to << ", ignoring" << endl;
	    level = RecalcNone;
	} else {
	    NOTATION_DEBUG << "ChordNameRuler::recalculate: change is " << overallStatus.from() << "->" << overallStatus.to() << ", I show " << from << "->" << to << ", recalculating whole" << endl;
	    level = RecalcWhole;
	}
    }

    if (level == RecalcNone) return;

    for (SegmentRefreshMap::iterator i = m_segments.begin();
	 i != m_segments.end(); ++i) {
	i->first->getRefreshStatus(i->second).setNeedsRefresh(false);
    }

    if (!m_currentSegment) { //!!! arbitrary, must do better
	//!!! need a segment starting at zero or so with a clef and key in it!
	m_currentSegment = m_segments.begin()->first;
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

    if (level == RecalcWhole) {

	m_chordSegment->clear();
	
	timeT clefKeyTime = m_currentSegment->getStartTime();
	//(from < m_currentSegment->getStartTime() ?
	//	        m_currentSegment->getStartTime() : from);

	Rosegarden::Clef clef = m_currentSegment->getClefAtTime(clefKeyTime);
	m_chordSegment->insert(clef.getAsEvent(-1));
    
	Rosegarden::Key key = m_currentSegment->getKeyAtTime(clefKeyTime);
	m_chordSegment->insert(key.getAsEvent(-1));

	from = 0;
	to = 0;

    } else {
	Segment::iterator i = m_chordSegment->findTime(from);
	Segment::iterator j = m_chordSegment->findTime(to);
	m_chordSegment->erase(i, j);
    }

    SegmentSelection selection;
    for (SegmentRefreshMap::iterator si = m_segments.begin(); si != m_segments.end();
	 ++si) {
	selection.insert(si->first);
    }

    CompositionTimeSliceAdapter adapter(m_composition, &selection, from, to);
    AnalysisHelper helper;
    helper.labelChords(adapter, *m_chordSegment, m_composition->getNotationQuantizer());
}

void
ChordNameRuler::paintEvent(QPaintEvent* e)
{
    if (!m_composition || !m_ready) return;

    NOTATION_DEBUG << "*** Chord Name Ruler: paintEvent" << endl;

    Rosegarden::Profiler profiler1("ChordNameRuler::paintEvent (whole)", true);

    QPainter paint(this);
    paint.setPen(RosegardenGUIColours::ChordNameRulerForeground);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
	(clipRect.x() - m_currentXOffset - m_xorigin - 50);
    timeT   to = m_rulerScale->getTimeForX
	(clipRect.x() + clipRect.width() - m_currentXOffset - m_xorigin + 50);

    recalculate(from, to);

    if (!m_chordSegment) return;

    Rosegarden::Profiler profiler2("ChordNameRuler::paintEvent (paint)", true);

    QRect boundsForHeight = m_fontMetrics.boundingRect("^j|lM");
    int fontHeight = boundsForHeight.height();
    int textY = (height() - 6)/2 + fontHeight/2;

    double prevX = 0;
    timeT keyAt = from - 1;
    std::string keyText;

    NOTATION_DEBUG << "*** Chord Name Ruler: paint " << from << " -> " << to << endl;

    for (Segment::iterator i = m_chordSegment->findTime(from);
	 i != m_chordSegment->findTime(to); ++i) {
	
	NOTATION_DEBUG << "type " << (*i)->getType() << " at " << (*i)->getAbsoluteTime()
		  << endl;

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

