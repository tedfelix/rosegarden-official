
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

#include <klocale.h>
#include <kmessagebox.h>

#include "trackscanvas.h"
#include "Track.h"

#include "rosedebug.h"

using Rosegarden::Track;

//////////////////////////////////////////////////////////////////////
//                TrackItem
//////////////////////////////////////////////////////////////////////

TrackItem::TrackItem(int x, int y,
                     int nbSteps,
                     QCanvas* canvas)
    : QCanvasRectangle(x, y,
                       nbStepsToWidth(nbSteps), m_itemHeight,
                       canvas),
      m_track(0)
{
}

unsigned int TrackItem::getItemNbTimeSteps() const
{
    kdDebug(KDEBUG_AREA) << "TrackItem::getItemNbTimeSteps() : "
                         << rect().width() / m_widthToDurationRatio
                         << endl;

    return widthToNbSteps(width());
}

timeT TrackItem::getStartIndex() const
{
    return x() / m_widthToDurationRatio * m_timeStepsResolution;
}

int TrackItem::getInstrument() const
{
    return m_track->getInstrument();
}

void TrackItem::setInstrument(int i)
{
    m_track->setInstrument(i);
}

void TrackItem::setWidthToDurationRatio(unsigned int r)
{
    m_widthToDurationRatio = r;
}

void TrackItem::setTimeStepsResolution(unsigned int r)
{
    m_timeStepsResolution = r;
}

unsigned int TrackItem::getTimeStepsResolution()
{
    return m_timeStepsResolution;
}

void TrackItem::setItemHeight(unsigned int h)
{
    m_itemHeight = h;
}

unsigned int TrackItem::nbStepsToWidth(unsigned int nbSteps)
{
    if (nbSteps < m_timeStepsResolution) nbSteps = m_timeStepsResolution;

    return nbSteps * m_widthToDurationRatio / m_timeStepsResolution;
}

unsigned int TrackItem::widthToNbSteps(unsigned int width)
{
    return width * m_timeStepsResolution / m_widthToDurationRatio;
}


unsigned int TrackItem::m_widthToDurationRatio = 1;
unsigned int TrackItem::m_timeStepsResolution = 384;
unsigned int TrackItem::m_itemHeight = 10;

//////////////////////////////////////////////////////////////////////
//                TracksCanvas
//////////////////////////////////////////////////////////////////////


TracksCanvas::TracksCanvas(int gridH, int gridV,
                           QCanvas& c, QWidget* parent,
                           const char* name, WFlags f) :
    QCanvasView(&c,parent,name,f),
    m_toolType(Pencil),
    m_tool(new TrackPencil(this)),
    m_grid(gridH, gridV),
    m_brush(Qt::blue),
    m_pen(Qt::black),
    m_editMenu(new QPopupMenu(this))
{
    TrackItem::setWidthToDurationRatio(m_grid.hstep());
    TrackItem::setItemHeight(m_grid.vstep());

    m_editMenu->insertItem(I18N_NOOP("Edit"),
                           this, SLOT(onEdit()));
    m_editMenu->insertItem(I18N_NOOP("Edit Small"),
                           this, SLOT(onEditSmall()));
}

TracksCanvas::~TracksCanvas()
{
}

void
TracksCanvas::update()
{
    canvas()->update();
}

void
TracksCanvas::setTool(ToolType t)
{
    if (t == m_toolType) return;

    delete m_tool;
    m_tool = 0;
    m_toolType = t;

    switch(t) {
    case Pencil:
        m_tool = new TrackPencil(this);
        break;
    case Eraser:
        m_tool = new TrackEraser(this);
        break;
    case Mover:
        m_tool = new TrackMover(this);
        break;
    case Resizer:
        m_tool = new TrackResizer(this);
        break;
    default:
        KMessageBox::error(0, QString("TracksCanvas::setTool() : unknown tool id %1").arg(t));
    }
}

TrackItem*
TracksCanvas::findPartClickedOn(QPoint pos)
{
    QCanvasItemList l=canvas()->collisions(pos);

    if (l.count()) {

        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            if (TrackItem *item = dynamic_cast<TrackItem*>(*it))
                return item;
        }

    }

    return 0;
}

void TracksCanvas::contentsMousePressEvent(QMouseEvent* e)
{
    if (e->button() == LeftButton) { // delegate event handling to tool

        m_tool->handleMouseButtonPress(e);

    } else if (e->button() == RightButton) { // popup menu if over a part

        TrackItem *item = findPartClickedOn(e->pos());

        if (item) {
            m_currentItem = item;
            //             kdDebug(KDEBUG_AREA) << "TracksCanvas::contentsMousePressEvent() : edit m_currentItem = "
            //                                  << m_currentItem << endl;

            m_editMenu->exec(QCursor::pos());
        }
    }
}

void TracksCanvas::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    TrackItem *item = findPartClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        emit editTrack(m_currentItem->getTrack());
    }
}

void TracksCanvas::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton) m_tool->handleMouseButtonRelase(e);
}

void TracksCanvas::contentsMouseMoveEvent(QMouseEvent* e)
{
    m_tool->handleMouseMove(e);
}

void
TracksCanvas::wheelEvent(QWheelEvent *e)
{
    e->ignore();
}


void TracksCanvas::clear()
{
    QCanvasItemList list = canvas()->allItems();
    QCanvasItemList::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
	if ( *it )
	    delete *it;
    }
}

/// called when reading a music file
TrackItem*
TracksCanvas::addPartItem(int x, int y, unsigned int nbSteps)
{
    TrackItem* newPartItem = new TrackItem(x, y, nbSteps, canvas());

    newPartItem->setPen(m_pen);
    newPartItem->setBrush(m_brush);
    newPartItem->setVisible(true);     
    newPartItem->setZ(1);           // Track at Z=1, Pointer at Z=10 [rwb]

    return newPartItem;
}


void
TracksCanvas::onEdit()
{
    emit editTrack(m_currentItem->getTrack());
}

void
TracksCanvas::onEditSmall()
{
    emit editTrackSmall(m_currentItem->getTrack());
}

//////////////////////////////////////////////////////////////////////
//                 Track Tools
//////////////////////////////////////////////////////////////////////

TrackTool::TrackTool(TracksCanvas* canvas)
    : m_canvas(canvas),
      m_currentItem(0)
{
}

TrackTool::~TrackTool()
{
    m_canvas->setCursor(Qt::arrowCursor);
}

//////////////////////////////
// TrackPencil
//////////////////////////////

TrackPencil::TrackPencil(TracksCanvas *c)
    : TrackTool(c),
      m_newRect(false)
{
    connect(this, SIGNAL(addTrack(TrackItem*)),
            c,    SIGNAL(addTrack(TrackItem*)));
    connect(this, SIGNAL(deleteTrack(Rosegarden::Track*)),
            c,    SIGNAL(deleteTrack(Rosegarden::Track*)));

    kdDebug(KDEBUG_AREA) << "TrackPencil()\n";
}

void TrackPencil::handleMouseButtonPress(QMouseEvent *e)
{
    m_newRect = false;
    m_currentItem = 0;

    // Check if we're clicking on a rect
    //
    TrackItem *item = m_canvas->findPartClickedOn(e->pos());

    if (item) {
        // we are, so set currentItem to it
        // m_currentItem = item; // leave it alone
        return;

    } else { // we are not, so create one

        int gx = m_canvas->grid().snapX(e->pos().x()),
            gy = m_canvas->grid().snapY(e->pos().y());

        m_currentItem = new TrackItem(gx, gy,
                                      TrackItem::getTimeStepsResolution(),
                                      m_canvas->canvas());
        
        m_currentItem->setPen(m_canvas->pen());
        m_currentItem->setBrush(m_canvas->brush());
        m_currentItem->setVisible(true);
        m_currentItem->setZ(1);          // Track at Z=1, Pointer at Z=10 [rwb]

        m_newRect = true;

        m_canvas->update();
    }

}

void TrackPencil::handleMouseButtonRelase(QMouseEvent*)
{
    if (!m_currentItem) return;

    if (m_currentItem->width() < 0) { // track was drawn from right to left
        double itemX = m_currentItem->x();
        double itemW = m_currentItem->width();
        kdDebug(KDEBUG_AREA) << "TracksCanvas::contentsMouseReleaseEvent() : itemX = "
                             << itemX << " - width : " << itemW << endl;

        m_currentItem->setX(itemX + itemW);
        m_currentItem->setSize(-itemW, m_currentItem->height());

        kdDebug(KDEBUG_AREA) << "TracksCanvas::contentsMouseReleaseEvent() after correction : itemX = "
                             << m_currentItem->x()
                             << " - width : " << m_currentItem->width()
                             << endl;
    }
    

    if (m_currentItem->width() == 0 && ! m_newRect) {

        kdDebug(KDEBUG_AREA) << "TracksCanvas::contentsMouseReleaseEvent() : track deleted"
                             << endl;
        emit deleteTrack(m_currentItem->getTrack());
        delete m_currentItem;
        m_canvas->canvas()->update();

    } else if (m_newRect && m_currentItem->width() > 0) {

        emit addTrack(m_currentItem);

    } else if (m_currentItem->width() > 0) {

        kdDebug(KDEBUG_AREA) << "TracksCanvas::contentsMouseReleaseEvent() : shorten m_currentItem = "
                             << m_currentItem << endl;
        // readjust size of corresponding track
        m_currentItem->getTrack()->setDuration(m_currentItem->getItemNbTimeSteps());
    }

    m_currentItem = 0;
    m_newRect = false;
}

void TrackPencil::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {

	m_currentItem->setSize(m_canvas->grid().snapX(e->pos().x()) - m_currentItem->rect().x(),
                               m_currentItem->height());
	m_canvas->canvas()->update();
    }
}

//////////////////////////////
// TrackEraser
//////////////////////////////

TrackEraser::TrackEraser(TracksCanvas *c)
    : TrackTool(c)
{
    m_canvas->setCursor(Qt::crossCursor);

    connect(this, SIGNAL(deleteTrack(Rosegarden::Track*)),
            c,    SIGNAL(deleteTrack(Rosegarden::Track*)));

    kdDebug(KDEBUG_AREA) << "TrackEraser()\n";
}

void TrackEraser::handleMouseButtonPress(QMouseEvent *e)
{
    m_currentItem = m_canvas->findPartClickedOn(e->pos());
}

void TrackEraser::handleMouseButtonRelase(QMouseEvent*)
{
    if (m_currentItem) emit deleteTrack(m_currentItem->getTrack());
    delete m_currentItem;
    m_canvas->canvas()->update();
    
    m_currentItem = 0;
}

void TrackEraser::handleMouseMove(QMouseEvent*)
{
}

//////////////////////////////
// TrackMover
//////////////////////////////

TrackMover::TrackMover(TracksCanvas *c)
    : TrackTool(c)
{
    m_canvas->setCursor(Qt::pointingHandCursor);

    connect(this, SIGNAL(updateTrackInstrumentAndStartIndex(TrackItem*)),
            c,    SIGNAL(updateTrackInstrumentAndStartIndex(TrackItem*)));

    kdDebug(KDEBUG_AREA) << "TrackMover()\n";
}

void TrackMover::handleMouseButtonPress(QMouseEvent *e)
{
    TrackItem *item = m_canvas->findPartClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        return;
    }
}

void TrackMover::handleMouseButtonRelase(QMouseEvent*)
{
    if (m_currentItem)
        emit updateTrackInstrumentAndStartIndex(m_currentItem);

    m_currentItem = 0;
}

void TrackMover::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {
        m_currentItem->setX(m_canvas->grid().snapX(e->pos().x()));
        m_currentItem->setY(m_canvas->grid().snapY(e->pos().y()));
        m_canvas->canvas()->update();
    }
}

//////////////////////////////
// TrackResizer
//////////////////////////////

TrackResizer::TrackResizer(TracksCanvas *c)
    : TrackTool(c),
      m_edgeThreshold(10)
{
    m_canvas->setCursor(Qt::sizeHorCursor);

    connect(this, SIGNAL(deleteTrack(Rosegarden::Track*)),
            c,    SIGNAL(deleteTrack(Rosegarden::Track*)));

    kdDebug(KDEBUG_AREA) << "TrackResizer()\n";
}

void TrackResizer::handleMouseButtonPress(QMouseEvent *e)
{
    TrackItem* item = m_canvas->findPartClickedOn(e->pos());

    if (item && cursorIsCloseEnoughToEdge(item, e)) {
        m_currentItem = item;
    }
}

void TrackResizer::handleMouseButtonRelase(QMouseEvent*)
{
    if (!m_currentItem) return;

    unsigned int newNbTimeSteps = m_currentItem->getItemNbTimeSteps();

    kdDebug(KDEBUG_AREA) << "TrackResizer: set track nb time steps to "
                         << newNbTimeSteps << endl;
    
    m_currentItem->getTrack()->setDuration(newNbTimeSteps);

    m_currentItem = 0;
}

void TrackResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    // change width only

    m_currentItem->setSize(m_canvas->grid().snapX(e->pos().x()) - m_currentItem->rect().x(),
                           m_currentItem->rect().height());
    
    m_canvas->canvas()->update();
    
}

bool TrackResizer::cursorIsCloseEnoughToEdge(TrackItem* p, QMouseEvent* e)
{
    return ( abs(p->rect().x() + p->rect().width() - e->x()) < m_edgeThreshold);
}
