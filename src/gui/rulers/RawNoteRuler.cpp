/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RawNoteRuler]"

#include <QPaintEvent>
#include "RawNoteRuler.h"

#include "misc/Debug.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/NotationQuantizer.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "DefaultVelocityColour.h"
#include "gui/general/GUIPalette.h"
#include "misc/Strings.h"

#include <QColor>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QToolTip>
#include <QWidget>


namespace Rosegarden
{

RawNoteRuler::RawNoteRuler(RulerScale *rulerScale,
                           Segment *segment,
                           int height,
                           QWidget *parent) :
        QWidget(parent),
        m_height(height),
        m_currentXOffset(0),
        m_width( -1),
        m_segment(segment),
        m_rulerScale(rulerScale)
{
//    setBackgroundColor(GUIPalette::getColour(GUIPalette::RawNoteRulerBackground));
    this->setToolTip("");
    
    if (m_segment) m_segment->addObserver(this);
}

RawNoteRuler::~RawNoteRuler()
{
    // QToolTip::remove(this);
    
    if (m_segment) m_segment->removeObserver(this);
}

void
RawNoteRuler::setCurrentSegment(Segment *segment)
{
    if (segment == m_segment) return;  // Don't waste CPU time
    
    if (m_segment) m_segment->removeObserver(this);
    m_segment = segment;
    if (m_segment) m_segment->addObserver(this);
}

void
RawNoteRuler::segmentDeleted(const Segment *)
{
   m_segment = nullptr;
}

void
RawNoteRuler::slotScrollHoriz(int x)
{
//###    int w = width(), h = height();
    int dx = x - ( -m_currentXOffset);
    if (dx == 0)
        return ;
    m_currentXOffset = -x;

    update();

//### bitBlt is no more working with Qt4
//     if (dx > w*3 / 4 || dx < -w*3 / 4) {
//         update();
//         return ;
//     }
// 
//     if (dx > 0) { // moving right, so the existing stuff moves left
//         bitBlt(this, 0, 0, this, dx, 0, w - dx, h);
//         repaint(w - dx, 0, dx, h);
//     } else {      // moving left, so the existing stuff moves right
//         bitBlt(this, -dx, 0, this, 0, 0, w + dx, h);
//         repaint(0, 0, -dx, h);
//     }
}

QSize
RawNoteRuler::sizeHint() const
{
    double width =
        m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
        m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
RawNoteRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

std::pair<timeT, timeT>
RawNoteRuler::getExtents(Segment::iterator i)
{
    const Quantizer *q =
        m_segment->getComposition()->getNotationQuantizer();

    timeT u0 = (*i)->getAbsoluteTime();
    timeT u1 = u0 + (*i)->getDuration();

    timeT q0 = q->getQuantizedAbsoluteTime(*i);
    timeT q1 = q0 + q->getQuantizedDuration(*i);

    timeT t0 = std::min(u0, q0);
    timeT t1 = std::max(u1, q1);

    return std::pair<timeT, timeT>(t0, t1);
}

Segment::iterator
RawNoteRuler::addChildren(Segment *s,
                          Segment::iterator to,
                          timeT rightBound,
                          EventTreeNode *node)
{
    Segment::iterator i = node->node;

    std::pair<timeT, timeT> iex = getExtents(i);
    Segment::iterator j = i;
    Segment::iterator rightmost = to;

#ifdef DEBUG_RAW_NOTE_RULER

    RG_DEBUG << "addChildren called for extents " << iex.first << "->" << iex.second << ", rightBound " << rightBound;
#endif

    for (++j; j != to && s->isBeforeEndMarker(j); ) {

        if (!(*j)->isa(Note::EventType)) {
            ++j;
            continue;
        }
        std::pair<timeT, timeT> jex = getExtents(j);

#ifdef DEBUG_RAW_NOTE_RULER

        RG_DEBUG << "addChildren: event at " << (*j)->getAbsoluteTime() << ", extents " << jex.first << "->" << jex.second;
#endif

        if (jex.first == jex.second) {
            ++j;
            continue;
        }
        if (jex.first >= iex.second || jex.first >= rightBound)
            break;

#ifdef DEBUG_RAW_NOTE_RULER

        RG_DEBUG << "addChildren: adding";
#endif

        EventTreeNode *subnode = new EventTreeNode(j);

        Segment::iterator subRightmost = addChildren(s, to, rightBound, subnode);
        if (subRightmost != to)
            rightmost = subRightmost;
        else
            rightmost = j;

        node->children.push_back(subnode);
        j = s->findTime(jex.second);
    }

    return rightmost;
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

        if (!(*i)->isa(Note::EventType)) {
            ++i;
            continue;
        }

        std::pair<timeT, timeT> iex = getExtents(i);

#ifdef DEBUG_RAW_NOTE_RULER

        RG_DEBUG << "buildForest: event at " << (*i)->getAbsoluteTime() << ", extents " << iex.first << "->" << iex.second;
#endif

        if (iex.first == iex.second) {
            ++i;
            continue;
        }
        if (iex.first >= endTime)
            break;

        EventTreeNode *node = new EventTreeNode(i);
        Segment::iterator rightmost = addChildren(s, to, iex.second, node);
        m_forest.push_back(node);

        if (rightmost != to) {
            i = rightmost;
            ++i;
        } else {
            i = s->findTime(iex.second);
        }

#ifdef DEBUG_RAW_NOTE_RULER
        RG_DEBUG << "findTime " << iex.second << " returned iterator at " << (i == s->end() ? -1 : (*i)->getAbsoluteTime());
#endif

    }
}

void
RawNoteRuler::dumpSubtree(EventTreeNode *node, int depth)
{
    if (!node)
        return ;
#ifdef DEBUG_RAW_NOTE_RULER

    for (int i = 0; i < depth; ++i)
        std::cerr << "  ";
    if (depth > 0)
        std::cerr << "->";
    std::cerr << (*node->node)->getAbsoluteTime() << ","
    << (*node->node)->getDuration() << " [";
    long pitch = 0;
    if ((*node->node)->get
            <Int>(PITCH, pitch)) {
        std::cerr << pitch << "]" << std::endl;
    }
    else {
        std::cerr << "no-pitch]" << std::endl;
    }
    for (EventTreeNode::NodeList::iterator i = node->children.begin();
            i != node->children.end(); ++i) {
        dumpSubtree(*i, depth + 1);
    }
#endif
    (void)depth; // avoid warnings
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

    (void)forest; // avoid warnings
}

int
RawNoteRuler::EventTreeNode::getDepth()
{
    int subchildrenDepth = 0;
    for (NodeList::iterator i = children.begin();
            i != children.end(); ++i) {
        int subchildDepth = (*i)->getDepth();
        if (subchildDepth > subchildrenDepth)
            subchildrenDepth = subchildDepth;
    }
    return subchildrenDepth + 1;
}

int
RawNoteRuler::EventTreeNode::getChildrenAboveOrBelow(bool below, int p)
{
    long pitch(p);
    if (pitch < 0)
        (*node)->get
        <Int>(BaseProperties::PITCH, pitch);

    int max = 0;

    for (NodeList::iterator i = children.begin();
            i != children.end(); ++i) {
        int forThisChild = (*i)->getChildrenAboveOrBelow(below, pitch);
        long thisChildPitch = pitch;
        (*(*i)->node)->get
        <Int>(BaseProperties::PITCH, thisChildPitch);
        if (below ? (thisChildPitch < pitch) : (thisChildPitch > pitch)) {
            ++forThisChild;
        }
        if (forThisChild > max)
            max = forThisChild;
    }

    return max;
}

void
RawNoteRuler::drawNode(QPainter &paint, DefaultVelocityColour &vc,
                       EventTreeNode *node, double height, double yorigin)
{
    int depth = node->getDepth();
    int above = node->getChildrenAboveOrBelow(false);

#ifdef DEBUG_RAW_NOTE_RULER

    int below = node->getChildrenAboveOrBelow(true);

    NOTATION_DEBUG << "RawNoteRuler::drawNode: children above: "
    << above << ", below: " << below << endl;
#endif

    int toFit = depth;

    double heightPer = double(height) / toFit;
    if (heightPer > m_height / 4)
        heightPer = m_height / 4;
    if (heightPer < 2)
        heightPer = 2;

    double myOrigin = yorigin + (heightPer * above);
    long myPitch = 60;
    (*node->node)->get
    <Int>(BaseProperties::PITCH, myPitch);

    long velocity = 100;
    (*node->node)->get
    <Int>(BaseProperties::VELOCITY, velocity);
    QColor colour = vc.getColour(velocity);

    timeT start = (*node->node)->getAbsoluteTime();
    timeT end = (*node->node)->getDuration() + start;

    double u0 = m_rulerScale->getXForTime(start);
    double u1 = m_rulerScale->getXForTime(end);

    u0 += m_currentXOffset;
    u1 += m_currentXOffset;

    start = m_segment->getComposition()->getNotationQuantizer()->
            getQuantizedAbsoluteTime(*node->node);
    end = start + m_segment->getComposition()->getNotationQuantizer()->
          getQuantizedDuration(*node->node);

    double q0 = m_rulerScale->getXForTime(start);
    double q1 = m_rulerScale->getXForTime(end);

    q0 += m_currentXOffset;
    q1 += m_currentXOffset;

#ifdef DEBUG_RAW_NOTE_RULER

    NOTATION_DEBUG << "RawNoteRuler: (" << int(start) << "," << myOrigin
    << ") -> (" << int(end) << "," << myOrigin << ")" << endl;
#endif

    int qi0 = int(q0);
    int ui0 = int(u0);
    int qi1 = int(q1);
    int ui1 = int(u1);
    //    int qiw = int(q1-q0) - 1;
    int uiw = int(u1 - u0) - 1;
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

    paint.setPen(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
    paint.setBrush(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
    paint.drawLine(qi0, iy, qi1 - 1, iy);
    paint.drawLine(qi0, iy + ih, qi1 - 1, iy + ih);
    paint.drawLine(ui0, iy + 1, ui0, iy + ih - 1);
    paint.drawLine(ui1 - 1, iy + 1, ui1 - 1, iy + ih - 1);

    for (EventTreeNode::NodeList::iterator i = node->children.begin();
            i != node->children.end(); ++i) {

        long nodePitch = myPitch;
        (*(*i)->node)->get
        <Int>(BaseProperties::PITCH, nodePitch);

        if (nodePitch < myPitch) {

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
    if (!m_segment || !m_segment->getComposition())
        return ;

    // Tooltips
    {
        // QToolTip::remove(this);
        TrackId trackId = m_segment->getTrack();
        Track *track =
            m_segment->getComposition()->getTrackById(trackId);
        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();

        this->setToolTip(tr("Track #%1, Segment \"%2\" (runtime id %3)")
                            .arg(trackPosition + 1)
                            .arg(strtoqstr(m_segment->getLabel()))
                            .arg(m_segment->getRuntimeId()));
    }

    //    START_TIMING;

    QPainter paint(this);
    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalized());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
                 (clipRect.x() - m_currentXOffset - 100);
    timeT to = m_rulerScale->getTimeForX
               (clipRect.x() + clipRect.width() - m_currentXOffset + 100);

    paint.setPen(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
    paint.setBrush(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
    paint.drawLine(0, 0, width(), 0);

    // draw the extent of the segment using its color

    QColor brushColor = m_segment->getComposition()->
                        getSegmentColourMap().getColour(m_segment->getColourIndex());
    paint.setPen(brushColor);
    paint.setBrush(brushColor);
    int x0 = int(m_rulerScale->getXForTime(m_segment->getStartTime()) + 
                 m_currentXOffset);
    int x1 = int(m_rulerScale->getXForTime(m_segment->getEndMarkerTime()) + 
                 m_currentXOffset);
    paint.drawRect(x0, 1, x1-x0+1, height()-1);

    // draw the bar divisions

    int firstBar = m_segment->getComposition()->getBarNumber(from);
    int lastBar = m_segment->getComposition()->getBarNumber(to);
    std::vector<int> divisions;

    for (int barNo = firstBar; barNo <= lastBar; ++barNo) {

        bool isNew = false;
        TimeSignature timeSig =
            m_segment->getComposition()->getTimeSignatureInBar(barNo, isNew);
        if (isNew || barNo == firstBar) {
            timeSig.getDivisions(3, divisions);
            if (timeSig == TimeSignature()) // special case for 4/4
                divisions[0] = 2;
        }

        timeT barStart = m_segment->getComposition()->getBarStart(barNo);
        timeT base = timeSig.getBarDuration();
        timeT barEnd = barStart + base;

        paint.setPen(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
        paint.setBrush(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));

        int x = int(m_rulerScale->getXForTime(barStart) +
                    m_currentXOffset);
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
                                m_currentXOffset);
                    paint.drawLine(x, 1, x, m_height);
                }
                t += base;
            }
        }
    }

    //    PRINT_ELAPSED("RawNoteRuler::paintEvent: drawing bar lines and divisions");

#ifdef DEBUG_RAW_NOTE_RULER
    NOTATION_DEBUG << "RawNoteRuler: from is " << from << ", to is " << to;
#endif

    Segment::iterator i = m_segment->findNearestTime(from);
    if (i == m_segment->end())
        i = m_segment->begin();

    // somewhat experimental, as is this whole class
    Segment::iterator j = m_segment->findTime(to);
    buildForest(m_segment, i, j);

    //    PRINT_ELAPSED("RawNoteRuler::paintEvent: buildForest");

    dumpForest(&m_forest);

    //    PRINT_ELAPSED("RawNoteRuler::paintEvent: dumpForest");

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

    //    PRINT_ELAPSED("RawNoteRuler::paintEvent: complete");
}

}
