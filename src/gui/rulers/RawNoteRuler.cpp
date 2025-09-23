/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RawNoteRuler]"
#define RG_NO_DEBUG_PRINT

#include "RawNoteRuler.h"

#include "DefaultVelocityColour.h"

#include "misc/Debug.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/NotationQuantizer.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "gui/general/GUIPalette.h"
#include "misc/Strings.h"

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QRect>
#include <QSize>
#include <QToolTip>
#include <QWidget>


namespace Rosegarden
{


RawNoteRuler::RawNoteRuler(RulerScale *rulerScale,
                           Segment *segment,
                           int height) :
    m_rulerScale(rulerScale),
    m_segment(segment),
    m_height(height)
{
    if (m_segment)
        m_segment->addObserver(this);
}

RawNoteRuler::~RawNoteRuler()
{
    if (m_segment)
        m_segment->removeObserver(this);

    // ??? std::shared_ptr would help.
    for (const EventTreeNode *node : m_forest) {
        delete node;
    }
}

void
RawNoteRuler::setCurrentSegment(Segment *segment)
{
    // No change?  Bail.
    if (segment == m_segment)
        return;
    
    // Out with the old.
    if (m_segment)
        m_segment->removeObserver(this);

    // In with the new.
    m_segment = segment;
    if (m_segment)
        m_segment->addObserver(this);
}

void
RawNoteRuler::segmentDeleted(const Segment *)
{
   m_segment = nullptr;
}

void
RawNoteRuler::scrollHoriz(int x)
{
    // No change?  Bail.
    if (m_currentXOffset == -x)
        return;

    m_currentXOffset = -x;

    update();
}

QSize
RawNoteRuler::sizeHint() const
{
    double width =
        m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
        m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    return QSize(width, m_height);
}

QSize
RawNoteRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

std::pair<timeT /*start*/, timeT /*duration*/>
RawNoteRuler::getExtents(const Event *event) const
{
    const Quantizer *q =
        m_segment->getComposition()->getNotationQuantizer();

    const timeT uStart = event->getAbsoluteTime();
    const timeT qStart = q->getQuantizedAbsoluteTime(event);
    const timeT start = std::min(uStart, qStart);

    const timeT uDuration = uStart + event->getDuration();
    const timeT qDuration = qStart + q->getQuantizedDuration(event);
    const timeT duration = std::max(uDuration, qDuration);

    return std::pair<timeT, timeT>(start, duration);
}

Segment::const_iterator
RawNoteRuler::addChildren(const Segment *s,
                          const Segment::const_iterator to,
                          const timeT rightBound,
                          EventTreeNode * const parentNode)
{
    const Segment::const_iterator i = parentNode->eventIter;
    const std::pair<timeT, timeT> iex = getExtents(*i);
    const timeT iexDuration = iex.second;

    Segment::const_iterator eventIter = i;
    Segment::const_iterator rightmost = to;

#ifdef DEBUG_RAW_NOTE_RULER
    RG_DEBUG << "addChildren called for extents " << iex.first << "->" << iexDuration << ", rightBound " << rightBound;
#endif

    for (++eventIter; eventIter != to  &&  s->isBeforeEndMarker(eventIter); ) {

        if (!(*eventIter)->isa(Note::EventType)) {
            ++eventIter;
            continue;
        }
        const std::pair<timeT, timeT> jex = getExtents(*eventIter);
        const timeT jexStart = jex.first;
        const timeT jexDuration = jex.second;

#ifdef DEBUG_RAW_NOTE_RULER
        RG_DEBUG << "addChildren: event at " << (*eventIter)->getAbsoluteTime() << ", extents " << jexStart << "->" << jexDuration;
#endif

        if (jexStart == jexDuration) {
            ++eventIter;
            continue;
        }
        if (jexStart >= iexDuration  ||  jexStart >= rightBound)
            break;

#ifdef DEBUG_RAW_NOTE_RULER
        RG_DEBUG << "addChildren: adding";
#endif

        EventTreeNode *subnode = new EventTreeNode(eventIter);

        Segment::const_iterator subRightmost = addChildren(s, to, rightBound, subnode);
        if (subRightmost != to)
            rightmost = subRightmost;
        else
            rightmost = eventIter;

        parentNode->children.push_back(subnode);
        eventIter = s->findTime(jexDuration);
    }

    return rightmost;
}

void
RawNoteRuler::buildForest(const Segment *s,
                          Segment::const_iterator from,
                          Segment::const_iterator to)
{
    for (const EventTreeNode *node : m_forest) {
        delete node;
    }
    m_forest.clear();

    timeT endTime = (s->isBeforeEndMarker(to) ? (*to)->getAbsoluteTime() :
                     s->getEndMarkerTime());

    for (Segment::const_iterator i = from; i != to && s->isBeforeEndMarker(i); ) {

        if (!(*i)->isa(Note::EventType)) {
            ++i;
            continue;
        }

        std::pair<timeT, timeT> iex = getExtents(*i);

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
        Segment::const_iterator rightmost = addChildren(s, to, iex.second, node);
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
RawNoteRuler::dumpSubtree(const EventTreeNode *node, int depth)
{
    if (!node)
        return ;

    for (int i = 0; i < depth; ++i)
        std::cerr << "  ";
    if (depth > 0)
        std::cerr << "->";
    std::cerr << (*node->eventIter)->getAbsoluteTime() << "," << (*node->eventIter)->getDuration() << " [";
    long pitch = 0;
    if ((*node->eventIter)->get<Int>(BaseProperties::PITCH, pitch)) {
        std::cerr << pitch << "]" << std::endl;
    }
    else {
        std::cerr << "no-pitch]" << std::endl;
    }
    for (const EventTreeNode *node2 : node->children) {
        dumpSubtree(node2, depth + 1);
    }
}

void
RawNoteRuler::dumpForest(const EventTreeNodeList *forest)
{
    std::cerr << "\nFOREST:\n" << std::endl;

    for (unsigned int i = 0; i < forest->size(); ++i) {
        std::cerr << "\nTREE " << i << ":\n" << std::endl;
        dumpSubtree((*forest)[i], 0);
    }

    std::cerr << std::endl;
}

int
RawNoteRuler::EventTreeNode::getDepth() const
{
    int subchildrenDepth = 0;

    for (const EventTreeNode *node : children) {
        const int subchildDepth = node->getDepth();
        if (subchildDepth > subchildrenDepth)
            subchildrenDepth = subchildDepth;
    }

    return subchildrenDepth + 1;
}

int
RawNoteRuler::EventTreeNode::getChildrenAboveOrBelow(bool below, int p) const
{
    long pitch(p);
    if (pitch < 0)
        (*eventIter)->get<Int>(BaseProperties::PITCH, pitch);

    int max = 0;

    for (const EventTreeNode *node : children) {
        int forThisChild = node->getChildrenAboveOrBelow(below, pitch);
        long thisChildPitch = pitch;
        (*node->eventIter)->get<Int>(BaseProperties::PITCH, thisChildPitch);
        if (below ? (thisChildPitch < pitch) : (thisChildPitch > pitch)) {
            ++forThisChild;
        }
        if (forThisChild > max)
            max = forThisChild;
    }

    return max;
}

void
RawNoteRuler::drawNode(QPainter &painter,
                       const EventTreeNode *node,
                       double height,
                       double yorigin)
{
    int depth = node->getDepth();
    int above = node->getChildrenAboveOrBelow(false);

#ifdef DEBUG_RAW_NOTE_RULER

    int below = node->getChildrenAboveOrBelow(true);

    NOTATION_DEBUG << "RawNoteRuler::drawNode: children above: " << above << ", below: " << below << endl;
#endif

    int toFit = depth;

    double heightPer = double(height) / toFit;
    if (heightPer > m_height / 4)
        heightPer = m_height / 4;
    if (heightPer < 2)
        heightPer = 2;

    double myOrigin = yorigin + (heightPer * above);
    long myPitch = 60;
    (*node->eventIter)->get<Int>(BaseProperties::PITCH, myPitch);

    long velocity = 100;
    (*node->eventIter)->get<Int>(BaseProperties::VELOCITY, velocity);

    timeT start = (*node->eventIter)->getAbsoluteTime();
    timeT end = (*node->eventIter)->getDuration() + start;

    double u0 = m_rulerScale->getXForTime(start);
    double u1 = m_rulerScale->getXForTime(end);

    u0 += m_currentXOffset;
    u1 += m_currentXOffset;

    start = m_segment->getComposition()->getNotationQuantizer()->
            getQuantizedAbsoluteTime(*node->eventIter);
    end = start + m_segment->getComposition()->getNotationQuantizer()->
          getQuantizedDuration(*node->eventIter);

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

    QColor colour = DefaultVelocityColour::getInstance()->getColour(velocity);
    painter.setPen(colour);
    painter.setBrush(colour);
    painter.drawRect(ui0 + 1, iy + 1, uiw, ih - 1);

    painter.setPen(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
    painter.setBrush(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
    painter.drawLine(qi0, iy, qi1 - 1, iy);
    painter.drawLine(qi0, iy + ih, qi1 - 1, iy + ih);
    painter.drawLine(ui0, iy + 1, ui0, iy + ih - 1);
    painter.drawLine(ui1 - 1, iy + 1, ui1 - 1, iy + ih - 1);

    for (const EventTreeNode *node : node->children) {

        long nodePitch = myPitch;
        (*node->eventIter)->get<Int>(BaseProperties::PITCH, nodePitch);

        if (nodePitch < myPitch) {

            drawNode(painter,
                     node,
                     height - heightPer - myOrigin,
                     myOrigin + heightPer);

        } else {

            drawNode(painter,
                     node,
                     myOrigin - yorigin,
                     yorigin);
        }
    }
}

void
RawNoteRuler::paintEvent(QPaintEvent *paintEvent2)
{
    if (!m_segment || !m_segment->getComposition())
        return ;

    // Tooltips
    // ??? Setting this on every paint seems a bit much.  Can we at least
    //     detect a change to the segment and update then?  How about in
    //     setCurrentSegment()?
    {
        // QToolTip::remove(this);
        TrackId trackId = m_segment->getTrack();
        Track *track =
            m_segment->getComposition()->getTrackById(trackId);
        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();

        setToolTip(tr("Track #%1, Segment \"%2\" (runtime id %3)").
                       arg(trackPosition + 1).
                       arg(strtoqstr(m_segment->getLabel())).
                       arg(m_segment->getRuntimeId()));
    }

    QPainter painter(this);
    painter.setClipRegion(paintEvent2->region());
    painter.setClipRect(paintEvent2->rect().normalized());

    QRect clipRect = painter.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
                 (clipRect.x() - m_currentXOffset - 100);
    timeT to = m_rulerScale->getTimeForX
               (clipRect.x() + clipRect.width() - m_currentXOffset + 100);

    painter.setPen(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
    painter.setBrush(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
    painter.drawLine(0, 0, width(), 0);

    // draw the extent of the segment using its color

    QColor brushColor = m_segment->getComposition()->
                        getSegmentColourMap().getColour(m_segment->getColourIndex());
    painter.setPen(brushColor);
    painter.setBrush(brushColor);
    int x0 = int(m_rulerScale->getXForTime(m_segment->getStartTime()) + 
                 m_currentXOffset);
    int x1 = int(m_rulerScale->getXForTime(m_segment->getEndMarkerTime()) + 
                 m_currentXOffset);
    painter.drawRect(x0, 1, x1-x0+1, height()-1);

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

        painter.setPen(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));
        painter.setBrush(GUIPalette::getColour(GUIPalette::RawNoteRulerForeground));

        int x = int(m_rulerScale->getXForTime(barStart) +
                    m_currentXOffset);
        painter.drawLine(x, 1, x, m_height);

        for (int depth = 0; depth < 3; ++depth) {

            int grey = depth * 60 + 60;
            painter.setPen(QColor(grey, grey, grey));
            painter.setBrush(QColor(grey, grey, grey));

            base /= divisions[depth];
            timeT t(barStart + base);
            while (t < barEnd) {
                if ((t - barStart) % (base * divisions[depth]) != 0) {
                    int x = int(m_rulerScale->getXForTime(t) +
                                m_currentXOffset);
                    painter.drawLine(x, 1, x, m_height);
                }
                t += base;
            }
        }
    }

#ifdef DEBUG_RAW_NOTE_RULER
    NOTATION_DEBUG << "RawNoteRuler: from is " << from << ", to is " << to;
#endif

    Segment::const_iterator i = m_segment->findNearestTime(from);
    if (i == m_segment->end())
        i = m_segment->begin();

    // somewhat experimental, as is this whole class
    Segment::const_iterator j = m_segment->findTime(to);
    buildForest(m_segment, i, j);

#ifdef DEBUG_RAW_NOTE_RULER
    dumpForest(&m_forest);
#endif

    for (const EventTreeNode *node : m_forest) {

        // Each tree in the forest should represent a note that starts
        // at a time when no other notes are playing (at least of
        // those that started no earlier than the paint start time).
        // Each node in that tree represents a note that starts
        // playing during its parent node's note, or at the same time
        // as it.

        drawNode(painter,
                 node,
                 m_height - 3,  // height
                 2);  // yorigin

    }
}


}
