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

#include <qpopupmenu.h>
#include <qwhatsthis.h>
#include <qfont.h>
#include <qfontmetrics.h>

#include <klocale.h>
#include <kmessagebox.h>

#include "rosegardenguidoc.h"
#include "segmentcanvas.h"
#include "Segment.h"
#include "Composition.h"
#include "NotationTypes.h"

#include "rosedebug.h"
#include "colours.h"
#include "RulerScale.h"

#include <cassert>

using Rosegarden::Segment;
using Rosegarden::Note;
using Rosegarden::RulerScale;
using Rosegarden::SnapGrid;
using Rosegarden::TrackId;
using Rosegarden::timeT;

//////////////////////////////////////////////////////////////////////
//                SegmentItem
//////////////////////////////////////////////////////////////////////

QFont *SegmentItem::m_font = 0;
QFontMetrics *SegmentItem::m_fontMetrics = 0;
int SegmentItem::m_fontHeight = 0;

SegmentItem::SegmentItem(TrackId track, timeT startTime, timeT duration,
                         SnapGrid *snapGrid, QCanvas *canvas) :
    QCanvasRectangle(0, 0, 1, 1, canvas),
    m_segment(0),
    m_track(track),
    m_startTime(startTime),
    m_duration(duration),
    m_selected(false),
    m_snapGrid(snapGrid),
    m_repeatRectangle(0)
    //m_label(new QCanvasText("", canvas))
{
    /*
    if (!m_font) makeFont();
    m_label->setFont(*m_font);
    */

    recalculateRectangle(true);
}

SegmentItem::SegmentItem(Segment *segment,
                         SnapGrid *snapGrid, QCanvas *canvas) :
    QCanvasRectangle(0, 0, 1, 1, canvas),
    m_segment(segment),
    m_selected(false),
    m_snapGrid(snapGrid),
    m_repeatRectangle(0)
    //m_label(new QCanvasText("", canvas))
{
    /*
    if (!m_font) makeFont();
    m_label->setFont(*m_font);
    */

    recalculateRectangle(true);
}

SegmentItem::~SegmentItem()
{
    kdDebug(KDEBUG_AREA) << "SegmentItem::~SegmentItem" << endl;
    if (m_repeatRectangle) delete m_repeatRectangle;
    //if (m_label) delete m_label;
}

void
SegmentItem::makeFont()
{
    m_font = new QFont();
    m_font->setPixelSize(m_snapGrid->getYSnap() / 2);
    m_font->setWeight(QFont::Bold);
    m_font->setItalic(false);
    m_fontMetrics = new QFontMetrics(*m_font);
    m_fontHeight = m_fontMetrics->boundingRect("|^M,g").height();
}

void
SegmentItem::recalculateRectangle(bool inheritFromSegment)
{
    if (m_segment && inheritFromSegment) {

	m_track = m_segment->getTrack();
	m_startTime = m_segment->getStartTime();
	m_duration = m_segment->getDuration();
        //m_labelText = m_segment->getLabel().c_str();

	if (m_segment->isRepeating()) {

	    timeT repeatStart = m_startTime + m_duration;
	    timeT repeatEnd = m_segment->getRepeatEndTime();

	    if (!m_repeatRectangle) {
		m_repeatRectangle = new QCanvasRectangle(0, 0, 1, 1, canvas());
	    }

	    m_repeatRectangle->setX
		(m_snapGrid->getRulerScale()->getXForTime(repeatStart));
	    m_repeatRectangle->setY
		(m_snapGrid->getYBinCoordinate(m_track));
	    m_repeatRectangle->setSize
		((int)m_snapGrid->getRulerScale()->getWidthForDuration
		 (repeatStart, repeatEnd - repeatStart) + 1,
		 m_snapGrid->getYSnap());

	    m_repeatRectangle->setZ(-1);
	    m_repeatRectangle->show();
	    
	} else if (m_repeatRectangle) {
	    delete m_repeatRectangle;
	    m_repeatRectangle = 0;
	}
    }

    setX(m_snapGrid->getRulerScale()->getXForTime(m_startTime));
    setY(m_snapGrid->getYBinCoordinate(m_track));

    setSize((int)m_snapGrid->getRulerScale()->
	    getWidthForDuration(m_startTime, m_duration) + 1,
	    m_snapGrid->getYSnap());

    /*
    QString text = m_labelText;
    bool dots = false;

    while (text.length() > 0 &&
	   (m_fontMetrics->boundingRect(dots ? (text + "...") : text).width() >
	    width() - 5)) {
	if (!dots && text.length() > 6) {
	    text.truncate(text.length() - 4);
	    dots = true;
	} else if (dots && text.length() < 2) {
	    dots = false;
	} else {
	    text.truncate(text.length() - 1);
	}
    }
    if (dots) m_label->setText(text + "...");
    else m_label->setText(text);

    m_label->setX(m_snapGrid->getRulerScale()->getXForTime(m_startTime) + 3);
    m_label->setY(m_snapGrid->getYBinCoordinate(m_track) +
		  (m_snapGrid->getYSnap()/2 - m_fontHeight/2) * 2 / 3);
    m_label->setZ(2);
    m_label->show();
    */
}

Segment *
SegmentItem::getSegment() const
{
    return m_segment;
}

void
SegmentItem::setSegment(Segment *segment)
{
    m_segment = segment;
    recalculateRectangle(true);
}

void
SegmentItem::setStartTime(timeT t)
{
    m_startTime = t;
    recalculateRectangle(false);
}

void
SegmentItem::setDuration(timeT d)
{
    m_duration = d;
    recalculateRectangle(false);
}

void
SegmentItem::setTrack(TrackId track)
{
    m_track = track;
    recalculateRectangle(false);
}

void
SegmentItem::normalize()
{
    if (m_duration < 0) {
	m_duration = -m_duration;
	m_startTime -= m_duration;
	recalculateRectangle(false);
    }
}

// Set this SegmentItem as selected/highlighted - we send
// in the QBrush we need at the same time
//
void
SegmentItem::setSelected(const bool &select, const QBrush &brush)
{
    setBrush(brush);
    m_selected = select;
}


//////////////////////////////////////////////////////////////////////
////             SegmentSplitLine
//////////////////////////////////////////////////////////////////////
SegmentSplitLine::SegmentSplitLine(int x, int y, int height,
                                   Rosegarden::RulerScale *rulerScale,
                                   QCanvas* canvas):
                                   QCanvasLine(canvas),
                                   m_rulerScale(rulerScale),
                                   m_height(height)
{
    setPen(Qt::black);
    setBrush(Qt::black);
    setZ(3);
    moveLine(x, y);
}

void
SegmentSplitLine::moveLine(int x, int y)
{
    setPoints(x, y, x, y + m_height);
    show();
}

void
SegmentSplitLine::hideLine()
{
    hide();
}



//////////////////////////////////////////////////////////////////////
//                SegmentCanvas
//////////////////////////////////////////////////////////////////////


SegmentCanvas::SegmentCanvas(RosegardenGUIDoc *doc,
                             RulerScale *rulerScale, QScrollBar* hsb,
                             int vStep,
			     QCanvas* c, QWidget* parent,
			     const char* name, WFlags f) :
    RosegardenCanvasView(hsb, c, parent, name, f),
    m_tool(0),
    m_grid(rulerScale, vStep),
    m_currentItem(0),
    m_recordingSegment(0),
    m_splitLine(0),
    m_brush(RosegardenGUIColours::SegmentBlock),
    m_highlightBrush(RosegardenGUIColours::SegmentHighlightBlock),
    m_pen(RosegardenGUIColours::SegmentBorder),
    m_editMenu(new QPopupMenu(this)),
    m_fineGrain(false),
    m_doc(doc)
{
    QWhatsThis::add(this, i18n("Segments Canvas - Create and manipulate your segments here"));

}

void SegmentCanvas::slotSetTool(ToolType t)
{
    kdDebug(KDEBUG_AREA) << "SegmentCanvas::slotSetTool(" << t << ")"
                         << this << "\n";

    if (m_tool)
      delete m_tool;

    m_tool = 0;

    switch(t) {
    case Pencil:
        m_tool = new SegmentPencil(this);
        break;
    case Eraser:
        m_tool = new SegmentEraser(this);
        break;
    case Mover:
        m_tool = new SegmentMover(this);
        break;
    case Resizer:
        m_tool = new SegmentResizer(this);
        break;
    case Selector:
        m_tool = new SegmentSelector(this);
        break;
    case Splitter:
        m_tool = new SegmentSplitter(this);
        break;
    case Joiner:
        m_tool = new SegmentJoiner(this);
        break;

    default:
        KMessageBox::error(0, QString("SegmentCanvas::slotSetTool() : unknown tool id %1").arg(t));
    }
}

void SegmentCanvas::updateAllSegmentItems()
{
    // store the segments we currently show here to speed up
    // determining if new segments were added to the composition
    // 
    std::vector<Segment*> currentSegments;
    bool foundOneSegmentDeleted = false;

    QCanvasItemList l = canvas()->allItems();
    
    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
        SegmentItem *item = dynamic_cast<SegmentItem*>(*it);
        if (item) {
            Segment* segment = item->getSegment();

            if (!segment->getComposition()) { // this segment has been deleted
                delete item;
                foundOneSegmentDeleted = true;
            }
            else {
                item->recalculateRectangle(true);
                currentSegments.push_back(segment);
            }
        }
    }

    unsigned int nbSegmentsInComposition = m_doc->getComposition().getNbSegments();
    unsigned int nbCurrentSegments = currentSegments.size();
    
    if ((nbSegmentsInComposition != nbCurrentSegments) ||
        (foundOneSegmentDeleted))
        // if the composition and the segment canvas have the same
        // number of segments, but one of them got deleted they can't
        // be all the same segments, so we also have to look
        {
            using Rosegarden::Composition;

            // check all composition's segments if there's a SegmentItem for it
            //
            const Composition::segmentcontainer& compositionSegments = m_doc->getComposition().getSegments();
            for(Composition::segmentcontainer::const_iterator i = compositionSegments.begin();
                i != compositionSegments.end(); ++i) {

                Segment* seg = (*i);

                if (std::find(currentSegments.begin(), currentSegments.end(), seg) == currentSegments.end()) {
                    // found one - update
                    addSegmentItem(seg);
                }

            }

        }

    slotUpdate();
}

void SegmentCanvas::updateSegmentItem(Segment *segment)
{
    SegmentItem *item = findSegmentItem(segment);
    if (!item) {
	addSegmentItem(segment);
    } else {
	item->recalculateRectangle(true);
    }
}

void SegmentCanvas::removeSegmentItem(Segment *segment)
{
    SegmentItem *item = findSegmentItem(segment);
    delete item;
}

SegmentItem*
SegmentCanvas::findSegmentItem(Rosegarden::Segment *segment)
{
    //!!! slow.
    QCanvasItemList l = canvas()->allItems();
    
    if (l.count()) {
        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            SegmentItem *item = dynamic_cast<SegmentItem*>(*it);
	    if (item && (item->getSegment() == segment)) {
                return item;
	    }
        }
    }

    return 0;
}

SegmentItem*
SegmentCanvas::findSegmentClickedOn(QPoint pos)
{
    QCanvasItemList l=canvas()->collisions(pos);

    if (l.count()) {
        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            if (SegmentItem *item = dynamic_cast<SegmentItem*>(*it))
                return item;
        }
    }

    return 0;
}

void SegmentCanvas::contentsMousePressEvent(QMouseEvent* e)
{
    if (e->button() == LeftButton) { // delegate event handling to tool

        // ensure that we have a valid tool
        //
        if (m_tool)
            m_tool->handleMouseButtonPress(e);
        else
            kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMousePressEvent() :"
                                 << this << " no tool\n";

    } else if (e->button() == RightButton) { // popup menu if over a part

        SegmentItem *item = findSegmentClickedOn(e->pos());

        if (item) {
            m_currentItem = item;
            //             kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMousePressEvent() : edit m_currentItem = "
            //                                  << m_currentItem << endl;

            if (m_currentItem->getSegment()->getType() == 
                                             Rosegarden::Segment::Audio)
            {
                m_editMenu->clear();
                m_editMenu->insertItem(i18n("Edit Audio"),
                                       this, SLOT(slotOnEditAudio()));
            }
            else
            {
                m_editMenu->clear();
                m_editMenu->insertItem(i18n("Edit as Notation"),
                                       this, SLOT(slotOnEditNotation()));

                m_editMenu->insertItem(i18n("Edit as Matrix"),
                                       this, SLOT(slotOnEditMatrix()));
            }

            m_editMenu->exec(QCursor::pos());
        }
    }
}

void SegmentCanvas::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    SegmentItem *item = findSegmentClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        // TODO : edit style should be user configurable

        if (m_currentItem->getSegment()->getType() == Rosegarden::Segment::Audio)
            emit editSegmentAudio(m_currentItem->getSegment());
        else
        {
            if (m_doc->getConfiguration().getDoubleClickClient() == 
                    Rosegarden::Configuration::NotationView)
                emit editSegmentNotation(m_currentItem->getSegment());
            else
                emit editSegmentMatrix(m_currentItem->getSegment());
        }
    }
}

void SegmentCanvas::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (!m_tool) return;

    if (e->button() == LeftButton) m_tool->handleMouseButtonRelease(e);
}

void SegmentCanvas::contentsMouseMoveEvent(QMouseEvent* e)
{
    if (!m_tool) return;

    m_tool->handleMouseMove(e);
}

void SegmentCanvas::clear()
{
    QCanvasItemList list = canvas()->allItems();
    QCanvasItemList::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
	if ( *it )
	    delete *it;
    }
}

// Show the split line. This is where we perform Segment splits.
//
void SegmentCanvas::slotShowSplitLine(int x, int y)
{
    if (m_splitLine == 0)
        m_splitLine = new SegmentSplitLine(x, y,
					   m_grid.getYSnap() - 1,
                                           m_grid.getRulerScale(),
                                           canvas());
    else
        m_splitLine->moveLine(x, y);

    slotUpdate();
}

// Hide the split line
//
void SegmentCanvas::slotHideSplitLine()
{
    m_splitLine->hideLine();
    slotUpdate();
}


SegmentItem *
SegmentCanvas::addSegmentItem(TrackId track, timeT startTime, timeT duration)
{
    SegmentItem *newItem = new SegmentItem
	(track, startTime, duration, &m_grid, canvas());

    newItem->setPen(m_pen);
    newItem->setBrush(m_brush);
    newItem->setVisible(true);     
    newItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newItem;
}

SegmentItem *
SegmentCanvas::addSegmentItem(Segment *segment)
{
    SegmentItem *newItem = new SegmentItem(segment, &m_grid, canvas());

    newItem->setPen(m_pen);
    newItem->setBrush(m_brush);
    newItem->setVisible(true);     
    newItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newItem;
}

void SegmentCanvas::showRecordingSegmentItem(TrackId track,
                                             timeT startTime, timeT duration)
{
    if (m_recordingSegment) {

	m_recordingSegment->setStartTime(startTime);
	m_recordingSegment->setDuration(duration);
	m_recordingSegment->setTrack(track);

    } else {
	
	m_recordingSegment = addSegmentItem(track, startTime, duration);
	m_recordingSegment->
            setPen(RosegardenGUIColours::RecordingSegmentBorder);
        m_recordingSegment->
            setBrush(RosegardenGUIColours::RecordingSegmentBlock);
	m_recordingSegment->setZ(2);
    }
}


void SegmentCanvas::deleteRecordingSegmentItem()
{
    if (m_recordingSegment) {
	m_recordingSegment->setVisible(false);
	delete m_recordingSegment;
	m_recordingSegment = 0;
        canvas()->update();
    }
}



void SegmentCanvas::slotOnEditNotation()
{
    emit editSegmentNotation(m_currentItem->getSegment());
}

void SegmentCanvas::slotOnEditMatrix()
{
    emit editSegmentMatrix(m_currentItem->getSegment());
}

void SegmentCanvas::slotOnEditAudio()
{
    emit editSegmentAudio(m_currentItem->getSegment());
}



// Select a SegmentItem on the canvas according to a
// passed Segment pointer
//
//
void SegmentCanvas::slotSelectSegments(std::vector<Rosegarden::Segment*> segments)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    std::vector<Rosegarden::Segment*>::iterator segIt;
    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    // clear any SegmentItems currently selected
    //
    selTool->clearSelected();

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        SegmentItem* segItem = dynamic_cast<SegmentItem*>(*it);
        
        if (segItem) { 

            for (segIt = segments.begin(); segIt != segments.end(); segIt++) {

                if (segItem->getSegment() == (*segIt)) {

                    selTool->slotSelectSegmentItem(segItem);
                }
            }
        }
    }
}

// Get a vector of selected Segments if we're using a Selector tool
//
std::vector<Rosegarden::Segment*>
SegmentCanvas::getSelectedSegments()
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (selTool)
        return selTool->getSelectedSegments();

    return *(new std::vector<Rosegarden::Segment*>);
}

void
SegmentCanvas::clearSelected()
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (selTool)
        return selTool->clearSelected();
}



// enter/exit selection add mode - this means that the SHIFT key
// (or similar) has been depressed and if we're in Select mode we
// can add Selections to the one we currently have
//
//
void SegmentCanvas::slotSetSelectAdd(const bool &value)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    selTool->setSegmentAdd(value);
}


// enter/exit selection copy mode - this means that the CTRL key
// (or similar) has been depressed and if we're in Select mode we
// can copy the current selection with a click and drag (overrides
// the default movement behaviour for selection).
//
//
void SegmentCanvas::slotSetSelectCopy(const bool &value)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    selTool->setSegmentCopy(value);
}


void SegmentCanvas::setSnapGrain(bool fine)
{
    if (m_fineGrain) {
	grid().setSnapTime(SnapGrid::NoSnap);
    } else {
	grid().setSnapTime(fine ? SnapGrid::SnapToBeat : SnapGrid::SnapToBar);
    }
}


void SegmentCanvas::slotSetFineGrain(bool value)
{
    m_fineGrain = value;
}

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

SegmentTool::SegmentTool(SegmentCanvas* canvas)
    : m_canvas(canvas),
      m_currentItem(0)
{
    m_canvas->setCursor(Qt::arrowCursor);
}

SegmentTool::~SegmentTool()
{
}


//////////////////////////////
// SegmentPencil
//////////////////////////////

SegmentPencil::SegmentPencil(SegmentCanvas *c)
    : SegmentTool(c),
      m_newRect(false),
      m_track(0),
      m_startTime(0),
      m_duration(0)
{
//    m_canvas->setCursor(Qt::ibeamCursor);

    connect(this, SIGNAL(addSegment(Rosegarden::TrackId, Rosegarden::timeT, Rosegarden::timeT)),
            c,    SIGNAL(addSegment(Rosegarden::TrackId, Rosegarden::timeT, Rosegarden::timeT)));

    kdDebug(KDEBUG_AREA) << "SegmentPencil()\n";
}

void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    m_newRect = false;
    m_currentItem = 0;

    // Check if we're clicking on a rect
    //
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item) return;

    m_canvas->setSnapGrain(false);

    TrackId track = m_canvas->grid().getYBin(e->pos().y());
    timeT time = m_canvas->grid().snapX(e->pos().x(), SnapGrid::SnapLeft);
    timeT duration = m_canvas->grid().getSnapTime(e->pos().x());
    if (duration == 0) duration = Note(Note::Shortest).getDuration();
    
    m_currentItem = m_canvas->addSegmentItem(track, time, duration);
    m_newRect = true;
    
    m_canvas->slotUpdate();
}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;
    m_currentItem->normalize();

    if (m_newRect) {
	emit addSegment(m_currentItem->getTrack(),
			m_currentItem->getStartTime(),
			m_currentItem->getDuration());
    }

    delete m_currentItem;
    m_currentItem = 0;
    m_newRect = false;
}

void SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    m_canvas->setSnapGrain(false);

    SnapGrid::SnapDirection direction = SnapGrid::SnapRight;
    if (e->pos().x() < m_currentItem->x()) direction = SnapGrid::SnapLeft;

    timeT time = m_canvas->grid().snapX(e->pos().x(), direction);
    timeT duration = time - m_currentItem->getStartTime();

    timeT snap = m_canvas->grid().getSnapTime(e->pos().x());
    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    if ((duration >= 0 && duration <  snap) ||
	(duration <  0 && duration > -snap)) {
	duration = (duration < 0 ? -snap : snap);
    }

    if (direction == SnapGrid::SnapLeft) {
	duration += std::max(m_currentItem->getDuration(), timeT(0));
    }

    m_currentItem->setDuration(duration);
    m_canvas->slotUpdate();
}

//////////////////////////////
// SegmentEraser
//////////////////////////////

SegmentEraser::SegmentEraser(SegmentCanvas *c)
    : SegmentTool(c)
{
    m_canvas->setCursor(Qt::pointingHandCursor);

    connect(this, SIGNAL(deleteSegment(Rosegarden::Segment*)),
            c,    SIGNAL(deleteSegment(Rosegarden::Segment*)));

    kdDebug(KDEBUG_AREA) << "SegmentEraser()\n";
}

void SegmentEraser::handleMouseButtonPress(QMouseEvent *e)
{
    m_currentItem = m_canvas->findSegmentClickedOn(e->pos());
}

void SegmentEraser::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem) emit deleteSegment(m_currentItem->getSegment());
    m_canvas->canvas()->update();
    
    m_currentItem = 0;
}

void SegmentEraser::handleMouseMove(QMouseEvent*)
{
}

//////////////////////////////
// SegmentMover
//////////////////////////////

SegmentMover::SegmentMover(SegmentCanvas *c)
    : SegmentTool(c)
{
    m_canvas->setCursor(Qt::sizeAllCursor);

    connect(this, SIGNAL(changeSegmentTrackAndStartTime(Rosegarden::Segment *, Rosegarden::TrackId, Rosegarden::timeT)),
            c,    SIGNAL(changeSegmentTrackAndStartTime(Rosegarden::Segment *, Rosegarden::TrackId, Rosegarden::timeT)));

    kdDebug(KDEBUG_AREA) << "SegmentMover()\n";
}

void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
	m_currentItemStartX = item->x();
	m_clickPoint = e->pos();
        return;
    }
}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
        emit changeSegmentTrackAndStartTime(m_currentItem->getSegment(),
					    m_currentItem->getTrack(),
					    m_currentItem->getStartTime());

    m_currentItem = 0;
}

void SegmentMover::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {

	m_canvas->setSnapGrain(true);

	int x = e->pos().x() - m_clickPoint.x();
	m_currentItem->setStartTime(m_canvas->grid().snapX
				    (m_currentItemStartX + x));

	TrackId track = m_canvas->grid().getYBin(e->pos().y());
        m_currentItem->setTrack(track);
        m_canvas->canvas()->update();
    }
}

//////////////////////////////
// SegmentResizer
//////////////////////////////

SegmentResizer::SegmentResizer(SegmentCanvas *c)
    : SegmentTool(c),
      m_edgeThreshold(10)
{
    m_canvas->setCursor(Qt::sizeHorCursor);

    connect(this, SIGNAL(deleteSegment(Rosegarden::Segment*)),
            c,    SIGNAL(deleteSegment(Rosegarden::Segment*)));

    connect(this, SIGNAL(changeSegmentTimes(Rosegarden::Segment*, Rosegarden::timeT, Rosegarden::timeT)),
            c,    SIGNAL(changeSegmentTimes(Rosegarden::Segment*, Rosegarden::timeT, Rosegarden::timeT)));

    kdDebug(KDEBUG_AREA) << "SegmentResizer()\n";
}

void SegmentResizer::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem* item = m_canvas->findSegmentClickedOn(e->pos());

    if (item && cursorIsCloseEnoughToEdge(item, e)) {
        m_currentItem = item;
    }
}

void SegmentResizer::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;
    m_currentItem->normalize();

    // normalisation may mean start time has changed as well as duration
    emit changeSegmentTimes(m_currentItem->getSegment(),
			    m_currentItem->getStartTime(),
			    m_currentItem->getDuration());

    m_currentItem = 0;
}

void SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    m_canvas->setSnapGrain(true);

    timeT time = m_canvas->grid().snapX(e->pos().x());
    timeT duration = time - m_currentItem->getStartTime();

    timeT snap = m_canvas->grid().getSnapTime(e->pos().x());
    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    if ((duration > 0 && duration <  snap) ||
	(duration < 0 && duration > -snap)) {
	m_currentItem->setDuration(duration < 0 ? -snap : snap);
    } else {
	m_currentItem->setDuration(duration);
    }

    m_canvas->canvas()->update();
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(SegmentItem* p, QMouseEvent* e)
{
    return ( abs(p->rect().x() + p->rect().width() - e->x()) < int(m_edgeThreshold));
}

//////////////////////////////
// SegmentSelector (bo!)
//////////////////////////////

SegmentSelector::SegmentSelector(SegmentCanvas *c)
    : SegmentTool(c), m_segmentAddMode(false), m_segmentCopyMode(false)

{
    kdDebug(KDEBUG_AREA) << "SegmentSelector()\n";

    connect(this, SIGNAL(changeSegmentTrackAndStartTime(Rosegarden::Segment *, Rosegarden::TrackId, Rosegarden::timeT)),
            c,    SIGNAL(changeSegmentTrackAndStartTime(Rosegarden::Segment *, Rosegarden::TrackId, Rosegarden::timeT)));

    connect(this, SIGNAL(selectedSegments(std::vector<Rosegarden::Segment*>)),
            c,     SIGNAL(selectedSegments(std::vector<Rosegarden::Segment*>)));
}

SegmentSelector::~SegmentSelector()
{
    clearSelected();
}

void
SegmentSelector::clearSelected()
{
    // For the moment only clear all selected from the vector
    //
    SegmentItemList::iterator it;
    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         it++)
    {
        it->second->setSelected(false, m_canvas->getSegmentBrush());
    }

    // now clear the vector
    //
    m_selectedItems.clear();

    // clear the current item
    //
    m_currentItem = 0;

    // send update
    //
    m_canvas->canvas()->update();
}

void
SegmentSelector::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    // If we're in segmentAddMode then we don't clear the
    // selection vector
    //
    if (!m_segmentAddMode)
       clearSelected();

    if (item)
    {
        m_currentItem = item;
        m_clickPoint = e->pos();
        slotSelectSegmentItem(m_currentItem);

        // emit this time for movement tracking (as we can move
        // through the Selector)
        //
        emit changeSegmentTrackAndStartTime(m_currentItem->getSegment(),
					    m_currentItem->getTrack(),
					    m_currentItem->getStartTime());

    }
 
    // Tell the RosegardenGUIView that we've selected some new Segments -
    // when the list is empty we're just unselecting.
    //
    emit selectedSegments(getSelectedSegments());

}

std::vector<Rosegarden::Segment*>
SegmentSelector::getSelectedSegments()
{
    std::vector<Rosegarden::Segment*> segments;
    SegmentItemList::iterator it;

    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         it++)
    {
        segments.push_back(it->second->getSegment());
    }

    return segments;
}


void
SegmentSelector::slotSelectSegmentItem(SegmentItem *selectedItem)
{
    // If we're selecting a Segment through this method
    // then don't set the m_currentItem
    //
    selectedItem->setSelected(true, m_canvas->getHighlightBrush());
    m_selectedItems.push_back(SegmentItemPair
                 (QPoint((int)selectedItem->x(), (int)selectedItem->y()),
                  selectedItem));
    m_canvas->canvas()->update();
}


// Don't need to do anything - for the moment we do this
// all on click, not release
//
void
SegmentSelector::handleMouseButtonRelease(QMouseEvent * /*e*/)
{
    if (!m_currentItem) return;

    if (m_segmentCopyMode)
    {
	std::cout << "Segment quick copy mode not implemented" << std::endl;
	return;
    }

    if (m_currentItem->isSelected())
    {
	SegmentItemList::iterator it;
	
	for (it = m_selectedItems.begin();
	     it != m_selectedItems.end();
	     it++)
	{
	    //!!! we don't really want a command to happen for every
	    //single segment.  The trackeditor is using a command that
	    //can change the positions of many tracks together; we
	    //need to get our data to it somehow and let it collate
	    // the segments into a single command

	    if ((it->second->getSegment()->getStartTime() !=
		 it->second->getStartTime()) ||
                (it->second->getSegment()->getTrack() !=
                 it->second->getTrack())) {
		emit changeSegmentTrackAndStartTime(it->second->getSegment(),
						    it->second->getTrack(),
						    it->second->getStartTime());
	    }
	}

	m_canvas->canvas()->update();
    }
    
    m_currentItem = 0;
}

// In Select mode we implement movement on the Segment
// as movement _of_ the Segment - as with SegmentMover
//
void
SegmentSelector::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    if (m_segmentCopyMode)
    {
	std::cout << "Segment quick copy mode not implemented" << std::endl;
	return;
    }

    m_canvas->setSnapGrain(true);

    if (m_currentItem->isSelected())
    {
	SegmentItemList::iterator it;
	
	for (it = m_selectedItems.begin();
	     it != m_selectedItems.end();
	     it++)
	{
	    int x = e->pos().x() - m_clickPoint.x(),
		y = e->pos().y() - m_clickPoint.y();
	    it->second->setStartTime(m_canvas->grid().snapX(it->first.x() + x));

	    TrackId track = m_canvas->grid().getYBin(it->first.y() + y);
	    it->second->setTrack(track);
	}

	m_canvas->canvas()->update();
    }
}

//////////////////////////////
//
// SegmentSplitter
//
//////////////////////////////


SegmentSplitter::SegmentSplitter(SegmentCanvas *c)
    : SegmentTool(c)
{
    kdDebug(KDEBUG_AREA) << "SegmentSplitter()\n";
    m_canvas->setCursor(Qt::splitHCursor);

    connect(this,
            SIGNAL(splitSegment(Rosegarden::Segment*, Rosegarden::timeT)),
            c,
            SIGNAL(splitSegment(Rosegarden::Segment*, Rosegarden::timeT)));
}

SegmentSplitter::~SegmentSplitter()
{
}

void
SegmentSplitter::handleMouseButtonPress(QMouseEvent *e)
{
    // Remove cursor and replace with line on a SegmentItem
    // at where the cut will be made
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
        m_canvas->setCursor(Qt::blankCursor);
        drawSplitLine(e);
    }

}

// Actually perform a split if we're on a Segment.
// Return the Segment pointer and the desired split
// time to the Document level.
//
void
SegmentSplitter::handleMouseButtonRelease(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
	m_canvas->setSnapGrain(true);

        // Split this Segment at snapped time
        emit splitSegment(item->getSegment(),
                          m_canvas->grid().snapX(e->pos().x()));
    }
 
    // Reinstate the cursor
    m_canvas->setCursor(Qt::splitHCursor);
    m_canvas->slotHideSplitLine();
}


void
SegmentSplitter::handleMouseMove(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
        m_canvas->setCursor(Qt::blankCursor);
        drawSplitLine(e);
    }
    else
    {
        m_canvas->setCursor(Qt::splitHCursor);
        m_canvas->slotHideSplitLine();
    }
}

// Draw the splitting line
//
void
SegmentSplitter::drawSplitLine(QMouseEvent *e)
{ 
    m_canvas->setSnapGrain(true);

    // Turn the real X into a snapped X
    //
    timeT xT = m_canvas->grid().snapX(e->pos().x());
    int x = (int)(m_canvas->grid().getRulerScale()->getXForTime(xT));

    // Need to watch y doesn't leak over the edges of the
    // current Segment.
    //
    int y = m_canvas->grid().snapY(e->pos().y());

    m_canvas->slotShowSplitLine(x, y);
}


void
SegmentSplitter::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
}


//////////////////////////////
//
// SegmentJoiner
//
//////////////////////////////
SegmentJoiner::SegmentJoiner(SegmentCanvas *c)
    : SegmentTool(c)
{
    kdDebug(KDEBUG_AREA) << "SegmentJoiner()\n";
}

SegmentJoiner::~SegmentJoiner()
{
}

void
SegmentJoiner::handleMouseButtonPress(QMouseEvent*)
{
}

void
SegmentJoiner::handleMouseButtonRelease(QMouseEvent*)
{
}


void
SegmentJoiner::handleMouseMove(QMouseEvent*)
{
}

void
SegmentJoiner::contentsMouseDoubleClickEvent(QMouseEvent*)
{
}



