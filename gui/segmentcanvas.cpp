// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "rosedebug.h"

using Rosegarden::Segment;

//////////////////////////////////////////////////////////////////////
//                SegmentItem
//////////////////////////////////////////////////////////////////////

SegmentItem::SegmentItem(int x, int y,
                     int nbSteps,
                     QCanvas* canvas)
    : QCanvasRectangle(x, y,
                       nbBarsToWidth(nbSteps), m_itemHeight,
                       canvas),
      m_segment(0)
{
}

int SegmentItem::getItemNbBars() const
{
    kdDebug(KDEBUG_AREA) << "SegmentItem::getItemNbBars() : "
                         << rect().width() / m_widthToDurationRatio
                         << endl;

    return widthToNbBars(width());
}

int SegmentItem::getStartBar() const
{
    return (int)(x() / m_widthToDurationRatio * m_barResolution);
}

int SegmentItem::getInstrument() const
{
    return m_segment->getInstrument();
}

void SegmentItem::setInstrument(int i)
{
    m_segment->setInstrument(i);
}

void SegmentItem::setWidthToDurationRatio(unsigned int r)
{
    m_widthToDurationRatio = r;
}

void SegmentItem::setBarResolution(unsigned int r)
{
    m_barResolution = r;
}

unsigned int SegmentItem::getBarResolution()
{
    return m_barResolution;
}

void SegmentItem::setItemHeight(unsigned int h)
{
    m_itemHeight = h;
}

unsigned int SegmentItem::nbBarsToWidth(unsigned int nbBars)
{
    if (nbBars < m_barResolution) nbBars = m_barResolution;

    return nbBars * m_widthToDurationRatio / m_barResolution;
}

unsigned int SegmentItem::widthToNbBars(unsigned int width)
{
    return width * m_barResolution / m_widthToDurationRatio;
}


unsigned int SegmentItem::m_widthToDurationRatio = 1;
unsigned int SegmentItem::m_barResolution = 1;
unsigned int SegmentItem::m_itemHeight = 10;



//////////////////////////////////////////////////////////////////////
//                SegmentCanvas
//////////////////////////////////////////////////////////////////////


SegmentCanvas::SegmentCanvas(int gridH, int gridV,
                           QCanvas& c, QWidget* parent,
                           const char* name, WFlags f) :
    QCanvasView(&c,parent,name,f),
    m_toolType(Pencil),
    m_tool(new SegmentPencil(this)),
    m_grid(gridH, gridV),
    m_brush(Qt::blue),
    m_pen(Qt::black),
    m_editMenu(new QPopupMenu(this))
{
    QWhatsThis::add(this, i18n("Segments Canvas - Create and manipulate your segments here"));

    SegmentItem::setWidthToDurationRatio(m_grid.hstep());
    SegmentItem::setItemHeight(m_grid.vstep());

    m_editMenu->insertItem(I18N_NOOP("Edit as Score"),
                           this, SLOT(onEdit()));
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
    if (t == m_toolType) return;

    delete m_tool;
    m_tool = 0;
    m_toolType = t;

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
    default:
        KMessageBox::error(0, QString("SegmentCanvas::setTool() : unknown tool id %1").arg(t));
    }
}

SegmentItem*
SegmentCanvas::findPartClickedOn(QPoint pos)
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

        m_tool->handleMouseButtonPress(e);

    } else if (e->button() == RightButton) { // popup menu if over a part

        SegmentItem *item = findPartClickedOn(e->pos());

        if (item) {
            m_currentItem = item;
            //             kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMousePressEvent() : edit m_currentItem = "
            //                                  << m_currentItem << endl;

            m_editMenu->exec(QCursor::pos());
        }
    }
}

void SegmentCanvas::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    SegmentItem *item = findPartClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        emit editSegment(m_currentItem->getSegment());
    }
}

void SegmentCanvas::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton) m_tool->handleMouseButtonRelease(e);
}

void SegmentCanvas::contentsMouseMoveEvent(QMouseEvent* e)
{
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

/// called when reading a music file
SegmentItem*
SegmentCanvas::addPartItem(int x, int y, unsigned int nbBars)
{
    SegmentItem* newPartItem = new SegmentItem(x, y, nbBars, canvas());

    newPartItem->setPen(m_pen);
    newPartItem->setBrush(m_brush);
    newPartItem->setVisible(true);     
    newPartItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newPartItem;
}


void
SegmentCanvas::onEdit()
{
    emit editSegment(m_currentItem->getSegment());
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
    SegmentItem *item = m_canvas->findPartClickedOn(e->pos());

    if (item) {
        // we are, so set currentItem to it
        // m_currentItem = item; // leave it alone
        return;

    } else { // we are not, so create one

        int gx = m_canvas->grid().snapX(e->pos().x()),
            gy = m_canvas->grid().snapY(e->pos().y());

        m_currentItem = new SegmentItem(gx, gy,
                                      SegmentItem::getBarResolution(),
                                      m_canvas->canvas());
        
        m_currentItem->setPen(m_canvas->pen());
        m_currentItem->setBrush(m_canvas->brush());
        m_currentItem->setVisible(true);
        m_currentItem->setZ(1);          // Segment at Z=1, Pointer at Z=10 [rwb]

        m_newRect = true;

        m_canvas->update();
    }

}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;

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
    

    if (m_currentItem->width() == 0 && ! m_newRect) {

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : segment deleted"
                             << endl;
        emit deleteSegment(m_currentItem->getSegment());
        delete m_currentItem;
        m_canvas->canvas()->update();

    } else if (m_newRect && m_currentItem->width() > 0) {

        emit addSegment(m_currentItem);

    } else if (m_currentItem->width() > 0) {

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : shorten m_currentItem = "
                             << m_currentItem << endl;

	emit setSegmentDuration(m_currentItem);
    }

    m_currentItem = 0;
    m_newRect = false;
}

void SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {

	m_currentItem->setSize(m_canvas->grid().snapX(e->pos().x()) - m_currentItem->rect().x(),
                               m_currentItem->height());
	m_canvas->canvas()->update();
    }
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
    m_currentItem = m_canvas->findPartClickedOn(e->pos());
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

    connect(this, SIGNAL(updateSegmentInstrumentAndStartIndex(SegmentItem*)),
            c,    SIGNAL(updateSegmentInstrumentAndStartIndex(SegmentItem*)));

    kdDebug(KDEBUG_AREA) << "SegmentMover()\n";
}

void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findPartClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        return;
    }
}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
        emit updateSegmentInstrumentAndStartIndex(m_currentItem);

    m_currentItem = 0;
}

void SegmentMover::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {
        m_currentItem->setX(m_canvas->grid().snapX(e->pos().x()));
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
    SegmentItem* item = m_canvas->findPartClickedOn(e->pos());

    if (item && cursorIsCloseEnoughToEdge(item, e)) {
        m_currentItem = item;
    }
}

void SegmentResizer::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;

    unsigned int newNbBars = m_currentItem->getItemNbBars();

    kdDebug(KDEBUG_AREA) << "SegmentResizer: set segment nb bars to "
                         << newNbBars << endl;
    
    emit setSegmentDuration(m_currentItem);

    m_currentItem = 0;
}

void SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    // change width only

    m_currentItem->setSize(m_canvas->grid().snapX(e->pos().x()) - m_currentItem->rect().x(),
                           m_currentItem->rect().height());
    
    m_canvas->canvas()->update();
    
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(SegmentItem* p, QMouseEvent* e)
{
    return ( abs(p->rect().x() + p->rect().width() - e->x()) < m_edgeThreshold);
}
