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

#include <klocale.h>
#include <kmessagebox.h>

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

//////////////////////////////////////////////////////////////////////
//                SegmentItem
//////////////////////////////////////////////////////////////////////

SegmentItem::SegmentItem(int y, timeT startTime, timeT duration,
                         RulerScale *rulerScale, QCanvas *canvas) :
    QCanvasRectangle((int)rulerScale->getXForTime(startTime),
                     y, 
                     (int)rulerScale->getWidthForDuration(startTime, duration),
                     m_itemHeight, canvas),
    m_segment(0),
    m_startTime(startTime),
    m_duration(duration),
    m_selected(false),
    m_rulerScale(rulerScale)
{
    // nothing else
}

SegmentItem::SegmentItem(int y, Segment *segment,
                         RulerScale *rulerScale, QCanvas *canvas) :
    QCanvasRectangle((int)rulerScale->getXForTime(segment->getStartTime()),
                     y,
                     (int)rulerScale->
                          getWidthForDuration(segment->getStartTime(),
                                              segment->getDuration()),
                     m_itemHeight, canvas),
    m_segment(segment),
    m_startTime(segment->getStartTime()),
    m_duration(segment->getDuration()),
    m_selected(false),
    m_rulerScale(rulerScale)
{
    // nothing else 
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

    m_startTime = segment->getStartTime();
    m_duration = segment->getDuration();

    setX(m_rulerScale->getXForTime(m_startTime));
    setSize((int)m_rulerScale->getWidthForDuration(m_startTime, m_duration),
            height());
}

void
SegmentItem::setStartTime(timeT t)
{
    m_startTime = t;
    setX(m_rulerScale->getXForTime(m_startTime));
}

void
SegmentItem::setDuration(timeT d)
{
    m_duration = d;
    setSize((int)m_rulerScale->getWidthForDuration(m_startTime, m_duration),
            height());
}

void
SegmentItem::recalculateRectangle()
{
    setX(m_rulerScale->getXForTime(m_startTime));
    setSize((int)m_rulerScale->getWidthForDuration(m_startTime, m_duration),
            height());
}

void
SegmentItem::normalize()
{
    if (m_duration < 0) {
	m_duration = -m_duration;
	m_startTime -= m_duration;
	recalculateRectangle();
    }
}

int SegmentItem::getTrack() const
{
    return m_segment->getTrack();
}

void SegmentItem::setItemHeight(unsigned int h)
{
    m_itemHeight = h;
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

unsigned int SegmentItem::m_itemHeight = 10;


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


SegmentCanvas::SegmentCanvas(RulerScale *rulerScale, int vStep,
			     QCanvas& c, QWidget* parent,
			     const char* name, WFlags f) :
    QCanvasView(&c, parent, name, f),
    m_tool(0),
    m_grid(rulerScale, vStep),
    m_currentItem(0),
    m_recordingSegment(0),
    m_splitLine(0),
    m_brush(RosegardenGUIColours::SegmentBlock),
    m_highlightBrush(RosegardenGUIColours::SegmentHighlightBlock),
    m_pen(RosegardenGUIColours::SegmentBorder),
    m_editMenu(new QPopupMenu(this))
{
    QWhatsThis::add(this, i18n("Segments Canvas - Create and manipulate your segments here"));

    SegmentItem::setItemHeight(vStep);
}

SegmentCanvas::~SegmentCanvas()
{
}

void
SegmentCanvas::update()
{
    canvas()->update();
}

void
SegmentCanvas::setTool(ToolType t)
{
    kdDebug(KDEBUG_AREA) << "SegmentCanvas::setTool(" << t << ")"
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
        KMessageBox::error(0, QString("SegmentCanvas::setTool() : unknown tool id %1").arg(t));
    }
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
                                       this, SLOT(onEditAudio()));
            }
            else
            {
                m_editMenu->clear();
                m_editMenu->insertItem(i18n("Edit as Notation"),
                                       this, SLOT(onEditNotation()));

                m_editMenu->insertItem(i18n("Edit as Matrix"),
                                       this, SLOT(onEditMatrix()));
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
            emit editSegmentNotation(m_currentItem->getSegment());
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

void
SegmentCanvas::wheelEvent(QWheelEvent *e)
{
    e->ignore();
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
void
SegmentCanvas::showSplitLine(int x, int y)
{
    if (m_splitLine == 0)
        m_splitLine = new SegmentSplitLine(x, y,
                                           SegmentItem::getItemHeight() - 1,
                                           m_grid.getRulerScale(),
                                           canvas());
    else
        m_splitLine->moveLine(x, y);

    update();
}

// Hide the split line
//
void
SegmentCanvas::hideSplitLine()
{
    m_splitLine->hideLine();
    update();
}



SegmentItem *
SegmentCanvas::addSegmentItem(int y, timeT startTime, timeT duration)
{
    SegmentItem* newItem = new SegmentItem
	(y, startTime, duration, m_grid.getRulerScale(), canvas());

    newItem->setPen(m_pen);
    newItem->setBrush(m_brush);
    newItem->setVisible(true);     
    newItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newItem;
}

void
SegmentCanvas::showRecordingSegmentItem(int y, timeT startTime, timeT duration)
{
    if (m_recordingSegment) {

	m_recordingSegment->setStartTime(startTime);
	m_recordingSegment->setDuration(duration);
	m_recordingSegment->setY(y);

    } else {
	
	m_recordingSegment = addSegmentItem(y, startTime, duration);
	m_recordingSegment->
            setPen(RosegardenGUIColours::RecordingSegmentBorder);
        m_recordingSegment->
            setBrush(RosegardenGUIColours::RecordingSegmentBlock);
	m_recordingSegment->setZ(2);
    }
}


void
SegmentCanvas::deleteRecordingSegmentItem()
{
    if (m_recordingSegment) {
	m_recordingSegment->setVisible(false);
	delete m_recordingSegment;
	m_recordingSegment = 0;
    }
}



void SegmentCanvas::onEditNotation()
{
    emit editSegmentNotation(m_currentItem->getSegment());
}

void SegmentCanvas::onEditMatrix()
{
    emit editSegmentMatrix(m_currentItem->getSegment());
}

void SegmentCanvas::onEditAudio()
{
    emit editSegmentAudio(m_currentItem->getSegment());
}



// Select a SegmentItem on the canvas according to a
// passed Segment pointer
//
//
void
SegmentCanvas::selectSegments(std::list<Rosegarden::Segment*> segments)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    std::list<Rosegarden::Segment*>::iterator segIt;
    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    // clear any SegmentItems currently selected
    //
    selTool->clearSelected();

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        if ((*it)->rtti() == SegmentItem::SegmentItemRTTI) { 

            for (segIt = segments.begin(); segIt != segments.end(); segIt++) {

                if (dynamic_cast<SegmentItem*>(*it)->getSegment() == (*segIt)) {

                    selTool->selectSegmentItem(dynamic_cast<SegmentItem*>(*it));
                }
            }
        }
    }
}

// enter/exit selection add mode - this means that the SHIFT key
// (or similar) has been depressed and if we're in Select mode we
// can add Selections to the one we currently have
//
//
void
SegmentCanvas::setSelectAdd(const bool &value)
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
void
SegmentCanvas::setSelectCopy(const bool &value)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    selTool->setSegmentCopy(value);
}

void
SegmentCanvas::setFineGrain(bool value)
{
    kdDebug(KDEBUG_AREA) << "SegmentCanvas::setFineGrain(" << value << ")" << endl;
    m_tool->setFineGrain(value);
}


//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

SegmentTool::SegmentTool(SegmentCanvas* canvas)
    : m_canvas(canvas),
      m_currentItem(0),
      m_fineGrain(false)
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
      m_y(0),
      m_startTime(0),
      m_duration(0)
{
//    m_canvas->setCursor(Qt::ibeamCursor);

    connect(this, SIGNAL(addSegment(int, Rosegarden::timeT, Rosegarden::timeT)),
            c,    SIGNAL(addSegment(int, Rosegarden::timeT, Rosegarden::timeT)));
    connect(this, SIGNAL(deleteSegment(Rosegarden::Segment*)),
            c,    SIGNAL(deleteSegment(Rosegarden::Segment*)));
    connect(this, SIGNAL(setSegmentDuration(SegmentItem*)),
            c,    SIGNAL(updateSegmentDuration(SegmentItem*)));

    kdDebug(KDEBUG_AREA) << "SegmentPencil()\n";
}

void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    m_newRect = false;
    m_currentItem = 0;

    // Check if we're clicking on a rect
    //
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item) {
        // we are, so set currentItem to it
        m_currentItem = item;
        return;

    } else { // we are not, so create one

	// a little hardcoded, for now
	if (m_fineGrain) {
	    m_canvas->grid().setSnapTime(SnapGrid::NoSnap);
	} else {
	    m_canvas->grid().setSnapTime(SnapGrid::SnapToBar);
	}

	int y = m_canvas->grid().snapY(e->pos().y());
	timeT time = m_canvas->grid().snapX(e->pos().x());
	timeT duration = m_canvas->grid().getSnapTime(e->pos().x());
	if (duration == 0) duration = Note(Note::Shortest).getDuration();

        // Don't use the m_currentItem for the moment as we
        // don't want to create a SegmentItem at this level
        //
	    //m_currentItem = m_canvas->addSegmentItem(y, time, duration);
        m_newRect = true;
        m_y = y;
        m_startTime = time;
        m_duration = duration;

        m_canvas->update();
    }
}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent*)
{
    //if (!m_currentItem) return;
    //m_currentItem->normalize();

    if (m_newRect) {

        emit addSegment(m_y, m_startTime, m_duration);
        
    }
    /*else {

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : shorten m_currentItem = "
                             << m_currentItem << endl;

	emit setSegmentDuration(m_currentItem);
    }*/

    m_currentItem = 0;
    m_newRect = false;
}

void SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    if (m_fineGrain) {
	m_canvas->grid().setSnapTime(SnapGrid::NoSnap);
    } else {
	m_canvas->grid().setSnapTime(SnapGrid::SnapToBar);
    }

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
    delete m_currentItem;
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

    connect(this, SIGNAL(updateSegmentTrackAndStartTime(SegmentItem*)),
            c,    SIGNAL(updateSegmentTrackAndStartTime(SegmentItem*)));

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
        emit updateSegmentTrackAndStartTime(m_currentItem);

    m_currentItem = 0;
}

void SegmentMover::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {

	if (m_fineGrain) {
	    m_canvas->grid().setSnapTime(SnapGrid::NoSnap);
	} else {
	    m_canvas->grid().setSnapTime(SnapGrid::SnapToBeat);
	}

	int x = e->pos().x() - m_clickPoint.x();
	m_currentItem->setStartTime(m_canvas->grid().snapX
				    (m_currentItemStartX + x));
        m_currentItem->setY(m_canvas->grid().snapY(e->pos().y()));
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

    connect(this, SIGNAL(setSegmentDuration(Rosegarden::Segment*)),
            c,    SIGNAL(updateSegmentDuration(Rosegarden::Segment*)));

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
    emit setSegmentDuration(m_currentItem);
    m_currentItem = 0;
}

void SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    if (m_fineGrain) {
	m_canvas->grid().setSnapTime(SnapGrid::NoSnap);
    } else {
	m_canvas->grid().setSnapTime(SnapGrid::SnapToBeat);
    }

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

    connect(this, SIGNAL(updateSegmentTrackAndStartTime(SegmentItem*)),
            c,    SIGNAL(updateSegmentTrackAndStartTime(SegmentItem*)));
}

SegmentSelector::~SegmentSelector()
{
    clearSelected();
}

void
SegmentSelector::clearSelected()
{
    // For the moment only clear all selected from the list
    //
    SegmentItemList::iterator it;
    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         it++)
    {
        it->second->setSelected(false, m_canvas->getSegmentBrush());
    }

    // now clear the list
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
    // selection list
    //
    if (!m_segmentAddMode)
       clearSelected();

    if (item)
    {
        m_currentItem = item;
        m_clickPoint = e->pos();
        selectSegmentItem(m_currentItem);
        emit updateSegmentTrackAndStartTime(m_currentItem);
    }

}

void
SegmentSelector::selectSegmentItem(SegmentItem *selectedItem)
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

    if (m_fineGrain) {
	m_canvas->grid().setSnapTime(SnapGrid::NoSnap);
    } else {
	m_canvas->grid().setSnapTime(SnapGrid::SnapToBeat);
    }

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
	    it->second->setY(m_canvas->grid().snapY(it->first.y() + y));
	    m_canvas->canvas()->update();
	    emit updateSegmentTrackAndStartTime(it->second);
	}
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
        if (m_fineGrain)
            m_canvas->grid().setSnapTime(SnapGrid::NoSnap);
        else
            m_canvas->grid().setSnapTime(SnapGrid::SnapToBeat);

        // Split this Segment at snapped time
        emit splitSegment(item->getSegment(),
                          m_canvas->grid().snapX(e->pos().x()));
    }
 
    // Reinstate the cursor
    m_canvas->setCursor(Qt::splitHCursor);
    m_canvas->hideSplitLine();
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
        m_canvas->hideSplitLine();
    }
}

// Draw the splitting line
//
void
SegmentSplitter::drawSplitLine(QMouseEvent *e)
{ 
    if (m_fineGrain)
        m_canvas->grid().setSnapTime(SnapGrid::NoSnap);
    else
        m_canvas->grid().setSnapTime(SnapGrid::SnapToBeat);

    // Turn the real X into a snapped X
    //
    timeT xT = m_canvas->grid().snapX(e->pos().x());
    int x = (int)(m_canvas->grid().getRulerScale()->getXForTime(xT));

    // Need to watch y doesn't leak over the edges of the
    // current Segment.
    //
    int y = m_canvas->grid().snapY(e->pos().y());

    m_canvas->showSplitLine(x, y);
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



