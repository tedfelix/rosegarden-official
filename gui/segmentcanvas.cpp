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
#include "rulerscale.h"

#include <cassert>

using Rosegarden::Segment;
using Rosegarden::Note;

//////////////////////////////////////////////////////////////////////
//                SegmentItem
//////////////////////////////////////////////////////////////////////

SegmentItem::SegmentItem(int y, timeT startTime, timeT duration,
			 double unitsPerPixel, QCanvas *canvas) :
    QCanvasRectangle((double)startTime / unitsPerPixel, y, 
		     (double)duration / unitsPerPixel, m_itemHeight, canvas),
    m_segment(0),
    m_startTime(startTime),
    m_duration(duration),
    m_selected(false),
    m_unitsPerPixel(unitsPerPixel)
{
    // nothing else
}

SegmentItem::SegmentItem(int y, Segment *segment,
			 double unitsPerPixel, QCanvas *canvas) :
    QCanvasRectangle((double)segment->getStartIndex() / unitsPerPixel, y,
		     (double)segment->getDuration() / unitsPerPixel,
		     m_itemHeight, canvas),
    m_segment(segment),
    m_startTime(segment->getStartIndex()),
    m_duration(segment->getDuration()),
    m_selected(false),
    m_unitsPerPixel(unitsPerPixel)
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

    m_startTime = segment->getStartIndex();
    m_duration = segment->getDuration();

    setX((double)m_startTime / m_unitsPerPixel);
    setSize((double)m_duration / m_unitsPerPixel, height());
}

void
SegmentItem::setStartTime(timeT t)
{
    m_startTime = t;
    setX((double)m_startTime / m_unitsPerPixel);
}

void
SegmentItem::setDuration(timeT d)
{
    m_duration = d;
    setSize((double)m_duration / m_unitsPerPixel, height());
}

void
SegmentItem::setUnitsPerPixel(double upp)
{
    m_unitsPerPixel = upp;
    setX((double)m_startTime / m_unitsPerPixel);
    setSize((double)m_duration / m_unitsPerPixel, height());
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
//                SegmentCanvas
//////////////////////////////////////////////////////////////////////


SegmentCanvas::SegmentCanvas(RulerScale *rulerScale, int vStep,
			     QCanvas& c, QWidget* parent,
			     const char* name, WFlags f) :
    QCanvasView(&c, parent, name, f),
    m_tool(0),
    m_grid(dynamic_cast<SimpleRulerScale *>(rulerScale), vStep),
    m_brush(RosegardenGUIColours::SegmentBlock),
    m_highlightBrush(RosegardenGUIColours::SegmentHighlightBlock),
    m_pen(RosegardenGUIColours::SegmentBorder),
    m_editMenu(new QPopupMenu(this)),
    m_recordingSegment(0)
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

SegmentItem *
SegmentCanvas::addSegmentItem(int y, timeT startTime, timeT duration)
{
    SegmentItem* newItem = new SegmentItem
	(y, startTime, duration, m_grid.getUnitsPerPixel(), canvas());

    newItem->setPen(m_pen);
    newItem->setBrush(m_brush);
    newItem->setVisible(true);     
    newItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newItem;
}

void
SegmentCanvas::showRecordingSegmentItem(int y, timeT startTime, timeT duration)
{
    //!!! I think it'd be highly advisable to make the recording
    // segment item a real SegmentItem rather than just a rectangle

    double startX = startTime / m_grid.getUnitsPerPixel();
    double endX = (startTime + duration) / m_grid.getUnitsPerPixel();

    if(m_recordingSegment)
    {
	m_recordingSegment->move(startX, y);
        m_recordingSegment->setSize(endX - startX, m_grid.getYSnap());
    }
    else
    {
        m_recordingSegment = new QCanvasRectangle
	    (startX, y, endX - startX, m_grid.getYSnap(), canvas());

        m_recordingSegment->
            setPen(RosegardenGUIColours::RecordingSegmentBorder);
        m_recordingSegment->
            setBrush(RosegardenGUIColours::RecordingSegmentCanvas);

        m_recordingSegment->setVisible(true);     
        m_recordingSegment->setZ(1);           // Segment at Z=1,
    }
}

void
SegmentCanvas::destroyRecordingSegmentItem()
{
    if(m_recordingSegment)
    {
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


//////////////////////////////////////////////////////////////////////
//                 SnapGrid
//////////////////////////////////////////////////////////////////////

const timeT SegmentCanvas::SnapGrid::NoSnap     = -3;
const timeT SegmentCanvas::SnapGrid::SnapToBar  = -2;
const timeT SegmentCanvas::SnapGrid::SnapToBeat = -1;

SegmentCanvas::SnapGrid::SnapGrid(SimpleRulerScale *rulerScale, int vstep) :
    m_rulerScale(rulerScale),
    m_snapTime(SnapToBeat),
    m_vstep(vstep)
{
    // nothing else 
}

void
SegmentCanvas::SnapGrid::setSnapTime(timeT snap)
{
    assert(snap > 0 ||
	   snap == NoSnap ||
	   snap == SnapToBar ||
	   snap == SnapToBeat);
    m_snapTime = snap;
}

timeT
SegmentCanvas::SnapGrid::getSnapTime(double x) const
{
    if (m_snapTime == NoSnap) return 0;
    timeT time = m_rulerScale->getTimeForX(x);

    Rosegarden::Composition *composition = m_rulerScale->getComposition();
    int barNo = composition->getBarNumber(time, false);
    std::pair<timeT, timeT> barRange = composition->getBarRange(barNo, false);

    timeT snapTime = barRange.second - barRange.first;

    if (m_snapTime == SnapToBeat) {
	snapTime = composition->getTimeSignatureAt(time).getBeatDuration();
    } else if (m_snapTime != SnapToBar && m_snapTime < snapTime) {
	snapTime = m_snapTime;
    }

    kdDebug(KDEBUG_AREA) << "SnapGrid: snap time is " << snapTime << endl;
    return snapTime;
}

timeT
SegmentCanvas::SnapGrid::snapX(double x) const
{
    timeT time = m_rulerScale->getTimeForX(x);
    if (m_snapTime == NoSnap) return time;

    Rosegarden::Composition *composition = m_rulerScale->getComposition();
    int barNo = composition->getBarNumber(time, false);
    std::pair<timeT, timeT> barRange = composition->getBarRange(barNo, false);

    timeT snapTime = barRange.second - barRange.first;

    if (m_snapTime == SnapToBeat) {
	snapTime = composition->getTimeSignatureAt(time).getBeatDuration();
    } else if (m_snapTime != SnapToBar && m_snapTime < snapTime) {
	snapTime = m_snapTime;
    }

    timeT offset = (time - barRange.first);
    timeT rounded = (offset / snapTime) * snapTime;

    timeT snapped;
    if ((offset - rounded) > (rounded + snapTime - offset)) {
	snapped = rounded + snapTime + barRange.first;
    } else {
	snapped = rounded + barRange.first;
    }

    kdDebug(KDEBUG_AREA) << "SnapGrid: before: " << x << " = " << time << "; after: " << snapped << endl;
    return snapped;
}
	
double
SegmentCanvas::SnapGrid::getUnitsPerPixel() const
{
    // requires a SimpleRulerScale, not just a RulerScale -- which
    // is quite correct, as SegmentItem requires x-coordinate to be
    // strictly proportional to time, just as in SimpleRulerScale
    return m_rulerScale->getUnitsPerPixel();
}

int
SegmentCanvas::SnapGrid::snapY(int y) const
{
    return y / m_vstep * m_vstep;
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
      m_newRect(false)
{
//    m_canvas->setCursor(Qt::ibeamCursor);

    connect(this, SIGNAL(addSegment(SegmentItem*)),
            c,    SIGNAL(addSegment(SegmentItem*)));
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
        // m_currentItem = item; // leave it alone
        return;

    } else { // we are not, so create one

	int y = m_canvas->grid().snapY(e->pos().y());
	timeT time = m_canvas->grid().snapX(e->pos().x());
//	timeT duration = m_canvas->grid().snapWidth(0);
	timeT duration = m_canvas->grid().getSnapTime(e->pos().x());
	if (duration == 0) duration = Note(Note::Shortest).getDuration();

	m_currentItem = m_canvas->addSegmentItem(y, time, duration);
        m_newRect = true;

        m_canvas->update();
    }
}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;
//!!!    m_currentItem->normalize();


/*!!!
    if (m_currentItem->width() < 0) { // segment was drawn from right to left
        double itemX = m_currentItem->x();
        double itemW = m_currentItem->width();
        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : itemX = "
                             << itemX << " - width : " << itemW << endl;

        m_currentItem->setX(itemX + itemW);
        m_currentItem->setSize(int(-itemW), m_currentItem->height());

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() after correction : itemX = "
                             << m_currentItem->x()
                             << " - width : " << m_currentItem->width()
                             << endl;
    }
*/
/*
    if (m_currentItem->width() == 0 && ! m_newRect) {

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : segment deleted"
                             << endl;
        emit deleteSegment(m_currentItem->getSegment());
        delete m_currentItem;
        m_canvas->canvas()->update();

	} else*/ if (m_newRect /*&& m_currentItem->width() > 0*/) {

        emit addSegment(m_currentItem);

	} else /* if (m_currentItem->width() > 0)*/ {

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : shorten m_currentItem = "
                             << m_currentItem << endl;

	emit setSegmentDuration(m_currentItem);
    }

    m_currentItem = 0;
    m_newRect = false;
}

void SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    timeT time = m_canvas->grid().snapX(e->pos().x());

    if (time < m_currentItem->getStartTime()) {

	timeT duration = m_currentItem->getDuration() +
	    (m_currentItem->getStartTime() - time);
	m_currentItem->setStartTime(time);
	m_currentItem->setDuration(duration);

    } else {
	timeT duration = time - m_currentItem->getStartTime();
	m_currentItem->setDuration(duration);
    }

    timeT snap = m_canvas->grid().getSnapTime(e->pos().x());
    if (snap == 0) snap = Note(Note::Shortest).getDuration();
    if (m_currentItem->getDuration() < snap) {
	m_currentItem->setDuration(snap);
    }

/*
	int width = m_canvas->grid().snapX(e->pos().x()) -
	    m_currentItem->rect().x();
	if ((width >= 0) && (width < SegmentItem::getBarResolution())) {
	    width = SegmentItem::getBarResolution(); // cc
	}

	m_currentItem->setSize(width, m_currentItem->height());
*/

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

    connect(this, SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)),
            c,    SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)));

    kdDebug(KDEBUG_AREA) << "SegmentMover()\n";
}

void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        return;
    }
}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
        emit updateSegmentTrackAndStartIndex(m_currentItem);

    m_currentItem = 0;
}

void SegmentMover::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {
	//!!! as SegmentSelector::handleMouseMove -- move into SegmentItem
//        m_currentItem->setX(m_canvas->grid().snapX(e->pos().x()));
	m_currentItem->setStartTime(m_canvas->grid().snapX(e->pos().x()));
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
    emit setSegmentDuration(m_currentItem);
    m_currentItem = 0;
}

void SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    //!!! as SegmentPencil::handleMouseMove
    //!!! move into SegmentItem

    timeT time = m_canvas->grid().snapX(e->pos().x());

    if (time < m_currentItem->getStartTime()) {

	timeT duration = m_currentItem->getDuration() +
	    (m_currentItem->getStartTime() - time);
	m_currentItem->setStartTime(time);
	m_currentItem->setDuration(duration);

    } else {
	timeT duration = time - m_currentItem->getStartTime();
	m_currentItem->setDuration(duration);
    }

    timeT snap = m_canvas->grid().getSnapTime(e->pos().x());
    if (snap == 0) snap = Note(Note::Shortest).getDuration();
    if (m_currentItem->getDuration() < snap) {
	m_currentItem->setDuration(snap);
    }


//!!!    m_currentItem->setSize(m_canvas->grid().snapX(e->pos().x()) - m_currentItem->rect().x(),
//                           m_currentItem->rect().height());
    
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

    connect(this, SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)),
            c,    SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)));
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
    std::list<SegmentItem*>::iterator it;
    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         it++)
    {
        (*it)->setSelected(false, m_canvas->getSegmentBrush());
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
        selectSegmentItem(m_currentItem);
        emit updateSegmentTrackAndStartIndex(m_currentItem);
    }

}

void
SegmentSelector::selectSegmentItem(SegmentItem *selectedItem)
{
    // If we're selecting a Segment through this method
    // then don't set the m_currentItem
    //
    selectedItem->setSelected(true, m_canvas->getHighlightBrush());
    m_selectedItems.push_back(selectedItem);
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
    if (m_currentItem) {

        if (m_segmentCopyMode)
        {
            std::cout << "Segment quick copy mode not implemented" << std::endl;
        }
        else
        {
            if (m_currentItem->isSelected())
            {
                std::list<SegmentItem*>::iterator it;

                for (it = m_selectedItems.begin();
                     it != m_selectedItems.end();
                     it++)
                {
//                    (*it)->setX(m_canvas->grid().snapX(e->pos().x()));
		    (*it)->setStartTime(m_canvas->grid().snapX(e->pos().x()));
                    (*it)->setY(m_canvas->grid().snapY(e->pos().y()));
                    m_canvas->canvas()->update();
                    emit updateSegmentTrackAndStartIndex(*it);
                }
            }
        }
    }
}

