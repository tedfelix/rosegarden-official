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

#include "rawnoteruler.h"
#include "colours.h"
#include "velocitycolour.h"
#include "rosedebug.h"

#include "Composition.h"
#include "Segment.h"
#include "RulerScale.h"
#include "Quantizer.h"
#include "BaseProperties.h"

using Rosegarden::RulerScale;
using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::BaseProperties;
using Rosegarden::BaseProperties::PITCH;
using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::timeT;


RawNoteRuler::RawNoteRuler(RulerScale *rulerScale,
			   Segment *segment,
			   double xorigin,
			   int height,
			   QWidget *parent,
			   const char *name) :
    QWidget(parent, name),
    m_xorigin(xorigin),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_segment(segment),
    m_rulerScale(rulerScale)
{
    setBackgroundColor(RosegardenGUIColours::RawNoteRulerBackground);
}

RawNoteRuler::~RawNoteRuler()
{
    // nothing
}

void
RawNoteRuler::slotScrollHoriz(int x)
{
    m_currentXOffset = -x;
    repaint();
}

QSize
RawNoteRuler::sizeHint() const
{
    double width =
	m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) +
	m_xorigin;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
RawNoteRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

std::pair<timeT, timeT>
RawNoteRuler::getExtents(Segment::iterator i)
{
    const Rosegarden::Quantizer *q =
	m_segment->getComposition()->getNotationQuantizer();

    timeT u0 = (*i)->getAbsoluteTime();
    timeT u1 = u0 + (*i)->getDuration();
    
    timeT q0 = q->getQuantizedAbsoluteTime(*i);
    timeT q1 = q0 + q->getQuantizedDuration(*i);
    
    timeT t0 = std::min(u0, q0);
    timeT t1 = std::max(u1, q1);
    
    return std::pair<timeT, timeT>(t0, t1);
}

void
RawNoteRuler::addChildren(Segment *s,
			  Segment::iterator to,
			  timeT rightBound,
			  EventTreeNode *node)
{
    Segment::iterator i = node->node;

    std::pair<timeT, timeT> iex = getExtents(i);
    Segment::iterator j = i;
    
    for (++j; j != to && s->isBeforeEndMarker(j); ) {
	
	if (!(*j)->isa(Note::EventType)) { ++j; continue; }
	
	std::pair<timeT, timeT> jex = getExtents(j);
	if (jex.first == jex.second) { ++j; continue; }
	if (jex.first >= iex.second || jex.first >= rightBound) break;

	EventTreeNode *subnode = new EventTreeNode(j);
	addChildren(s, to, rightBound, subnode);
	node->children.push_back(subnode);
	j = s->findTime(jex.second);
    }
}    
	
void
RawNoteRuler::buildForest(Segment *s,
			  Segment::iterator from,
			  Segment::iterator to)
{
    for (EventTreeNode::NodeList::iterator i = m_forest.begin();
	 i != m_forest.end(); ++i) {
	delete *i;
    }
    m_forest.clear();

    timeT endTime = (s->isBeforeEndMarker(to) ? (*to)->getAbsoluteTime() :
		     s->getEndMarkerTime());

    for (Segment::iterator i = from; i != to && s->isBeforeEndMarker(i); ) {
	if (!(*i)->isa(Note::EventType)) { ++i; continue; }

	std::pair<timeT, timeT> iex = getExtents(i);

#ifdef DEBUG_RAW_NOTE_RULER
	std::cerr << "buildForest: event at " << (*i)->getAbsoluteTime() << ", extents " << iex.first << "->" << iex.second << std::endl;
#endif
	
	if (iex.first == iex.second) { ++i; continue; }
	if (iex.first >= endTime) break;

	EventTreeNode *node = new EventTreeNode(i);
	addChildren(s, to, iex.second, node);
	m_forest.push_back(node);

	i = s->findTime(iex.second);

#ifdef DEBUG_RAW_NOTE_RULER
	std::cerr << "findTime " << iex.second << " returned iterator at " << (i == s->end() ? -1 : (*i)->getAbsoluteTime()) << std::endl;
#endif
    }
}

void
RawNoteRuler::dumpSubtree(EventTreeNode *node, int depth)
{
    if (!node) return;
#ifdef DEBUG_RAW_NOTE_RULER
    for (int i = 0; i < depth; ++i) std::cerr << "  ";
    if (depth > 0) std::cerr << "->";
    std::cerr << (*node->node)->getAbsoluteTime() << ","
	      << (*node->node)->getDuration() << " [";
    long pitch = 0;
    if ((*node->node)->get<Int>(PITCH, pitch)) {
	std::cerr << pitch << "]" << std::endl;
    } else {
	std::cerr << "no-pitch]" << std::endl;
    }
    for (EventTreeNode::NodeList::iterator i = node->children.begin();
	 i != node->children.end(); ++i) {
	dumpSubtree(*i, depth+1);
    }
#endif
}

void
RawNoteRuler::dumpForest(EventTreeNode::NodeList *forest)
{
#ifdef DEBUG_RAW_NOTE_RULER
    std::cerr << "\nFOREST:\n" << std::endl;

    for (unsigned int i = 0; i < forest->size(); ++i) {

	std::cerr << "\nTREE " << i << ":\n" << std::endl;
	dumpSubtree((*forest)[i], 0);
    }

    std::cerr << std::endl;
#endif
}

int
RawNoteRuler::EventTreeNode::getDepth()
{
    int subchildrenDepth = 0;
    for (NodeList::iterator i = children.begin();
	 i != children.end(); ++i) {
	int subchildDepth = (*i)->getDepth();
	if (subchildDepth > subchildrenDepth) subchildrenDepth = subchildDepth;
    }
    return subchildrenDepth + 1;
}

int
RawNoteRuler::EventTreeNode::getChildrenAboveOrBelow(bool below, int p)
{
    long pitch(p);
    if (pitch < 0) (*node)->get<Int>(PITCH, pitch);
    
    int max = 0;

    for (NodeList::iterator i = children.begin();
	 i != children.end(); ++i) {
	int forThisChild = (*i)->getChildrenAboveOrBelow(below, pitch);
	long thisChildPitch = pitch;
	(*(*i)->node)->get<Int>(PITCH, thisChildPitch);
	if (below ? (thisChildPitch < pitch) : (thisChildPitch > pitch)) {
	    ++forThisChild;
	}
	if (forThisChild > max) max = forThisChild;
    }

    return max;
}

void
RawNoteRuler::drawNode(QPainter &paint, DefaultVelocityColour &vc,
		       EventTreeNode *node, double height, double yorigin)
{
    int depth = node->getDepth();
    int above = node->getChildrenAboveOrBelow(false);
    int below = node->getChildrenAboveOrBelow(true);

#ifdef DEBUG_RAW_NOTE_RULER
    NOTATION_DEBUG << "RawNoteRuler::drawNode: children above: "
		   << above << ", below: " << below << endl;
#endif

    int toFit = depth;

    double heightPer = double(height) / toFit;
    if (heightPer > m_height/4) heightPer = m_height/4;
    if (heightPer < 2) heightPer = 2;

    double myOrigin = yorigin + (heightPer * above);
    int myPitch = (*node->node)->get<Int>(PITCH);

    long velocity = 100;
    (*node->node)->get<Int>(BaseProperties::VELOCITY, velocity);
    QColor colour = vc.getColour(velocity);
    
    timeT start = (*node->node)->getAbsoluteTime();
    timeT   end = (*node->node)->getDuration() + start;
    
    double u0 = m_rulerScale->getXForTime(start);
    double u1 = m_rulerScale->getXForTime(end);
    
    u0 += m_currentXOffset + m_xorigin;
    u1 += m_currentXOffset + m_xorigin;

    start = m_segment->getComposition()->getNotationQuantizer()->
	getQuantizedAbsoluteTime(*node->node);
    end = start + m_segment->getComposition()->getNotationQuantizer()->
	getQuantizedDuration(*node->node);

    double q0 = m_rulerScale->getXForTime(start);
    double q1 = m_rulerScale->getXForTime(end);
    
    q0 += m_currentXOffset + m_xorigin;
    q1 += m_currentXOffset + m_xorigin;

#ifdef DEBUG_RAW_NOTE_RULER
    NOTATION_DEBUG << "RawNoteRuler: (" << int(start) << "," << myOrigin
		   << ") -> (" << int(end) << "," << myOrigin << ")" << endl;
#endif
    
    int qi0 = int(q0);
    int ui0 = int(u0);
    int qi1 = int(q1);
    int ui1 = int(u1);
    int qiw = int(q1-q0) - 1;
    int uiw = int(u1-u0) - 1;
//    int iy = int(myOrigin + (height - heightPer) / 2);
    int iy = int(myOrigin);
    int ih = int(heightPer);

#ifdef DEBUG_RAW_NOTE_RULER
    NOTATION_DEBUG << "RawNoteRuler: height " << height << ", heightPer "
		   << heightPer << ", iy " << iy << endl;
#endif
    
    paint.setPen(colour);
    paint.setBrush(colour);
    paint.drawRect(ui0 + 1, iy + 1, uiw, ih - 1);
    
    paint.setPen(RosegardenGUIColours::RawNoteRulerForeground);
    paint.setBrush(RosegardenGUIColours::RawNoteRulerForeground);
    paint.drawLine(qi0,     iy,      qi1 - 1, iy);
    paint.drawLine(qi0,     iy + ih, qi1 - 1, iy + ih);
    paint.drawLine(ui0,     iy + 1,  ui0,     iy + ih - 1);
    paint.drawLine(ui1 - 1, iy + 1,  ui1 - 1, iy + ih - 1);

    for (EventTreeNode::NodeList::iterator i = node->children.begin();
	 i != node->children.end(); ++i) {

	//!!! need to ensure nothing goes in the tree that doesn't
	// have a pitch, or else make these calls safe

	if ((*(*i)->node)->get<Int>(PITCH) < myPitch) {

	    drawNode(paint, vc, *i,
		     height - heightPer - myOrigin, myOrigin + heightPer);
	    
	} else {

	    drawNode(paint, vc, *i,
		     myOrigin - yorigin, yorigin);
	}
    }
}    

void
RawNoteRuler::paintEvent(QPaintEvent* e)
{
    if (!m_segment || !m_segment->getComposition()) return;
    
    START_TIMING;

    QPainter paint(this);
    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
	(clipRect.x() - m_currentXOffset - 100 - m_xorigin);
    timeT   to = m_rulerScale->getTimeForX
	(clipRect.x() + clipRect.width() - m_currentXOffset + 100 - m_xorigin);

    paint.setPen(RosegardenGUIColours::RawNoteRulerForeground);
    paint.setBrush(RosegardenGUIColours::RawNoteRulerForeground);
    paint.drawLine(0, 0, width(), 0);

    // draw the bar divisions

    int firstBar = m_segment->getComposition()->getBarNumber(from);
    int lastBar  = m_segment->getComposition()->getBarNumber(to);
    std::vector<int> divisions;

    for (int barNo = firstBar; barNo <= lastBar; ++barNo) {

	bool isNew = false;
	Rosegarden::TimeSignature timeSig =
	    m_segment->getComposition()->getTimeSignatureInBar(barNo, isNew);
	if (isNew || barNo == firstBar) {
	    divisions = timeSig.getDivisions(3);
	}

	timeT barStart = m_segment->getComposition()->getBarStart(barNo);
	timeT base = timeSig.getBarDuration();
	timeT barEnd = barStart + base;

	paint.setPen(RosegardenGUIColours::RawNoteRulerForeground);
	paint.setBrush(RosegardenGUIColours::RawNoteRulerForeground);

	int x = int(m_rulerScale->getXForTime(barStart) +
		    m_currentXOffset + m_xorigin);
	paint.drawLine(x, 1, x, m_height);

	for (int depth = 0; depth < 3; ++depth) {

	    int grey = depth * 60 + 60;
	    paint.setPen(QColor(grey, grey, grey));
	    paint.setBrush(QColor(grey, grey, grey));

	    base /= divisions[depth];
	    timeT t(barStart + base);
	    while (t < barEnd) {
		if ((t - barStart) % (base * divisions[depth]) != 0) {
		    int x = int(m_rulerScale->getXForTime(t) +
				m_currentXOffset + m_xorigin);
		    paint.drawLine(x, 1, x, m_height);
		}
		t += base;
	    }
	}
    }

    PRINT_ELAPSED("RawNoteRuler::paintEvent: drawing bar lines and divisions");

#ifdef DEBUG_RAW_NOTE_RULER
    NOTATION_DEBUG << "RawNoteRuler: from is " << from << ", to is " << to << endl;
#endif

    Segment::iterator i = m_segment->findNearestTime(from);
    if (i == m_segment->end()) i = m_segment->begin();

    // somewhat experimental, as is this whole class
    Segment::iterator j = m_segment->findTime(to);
    buildForest(m_segment, i, j);

    PRINT_ELAPSED("RawNoteRuler::paintEvent: buildForest");

    dumpForest(&m_forest);

    PRINT_ELAPSED("RawNoteRuler::paintEvent: dumpForest");

    for (EventTreeNode::NodeList::iterator fi = m_forest.begin();
	 fi != m_forest.end(); ++fi) {

	// Each tree in the forest should represent a note that starts
	// at a time when no other notes are playing (at least of
	// those that started no earlier than the paint start time).
	// Each node in that tree represents a note that starts
	// playing during its parent node's note, or at the same time
	// as it.

	drawNode(paint, *DefaultVelocityColour::getInstance(), *fi, m_height - 3, 2);

    }

    PRINT_ELAPSED("RawNoteRuler::paintEvent: complete");
}

