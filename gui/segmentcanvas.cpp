/***************************************************************************
                          trackscanvas.cpp  -  description
                             -------------------
    begin                : Mon May 7 2001
    copyright            : (C) 2001 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qpopupmenu.h>

#include <klocale.h>

#include "trackscanvas.h"

#include "rosedebug.h"


TrackPartItem::TrackPartItem(QCanvas* canvas)
    : QCanvasRectangle(canvas),
      m_part(0)
{
}

TrackPartItem::TrackPartItem(const QRect &r, QCanvas* canvas)
    : QCanvasRectangle(r, canvas),
      m_part(0)
{
}

TrackPartItem::TrackPartItem(int x, int y,
                             int width, int height,
                             QCanvas* canvas)
    : QCanvasRectangle(x, y, width, height, canvas),
      m_part(0)
{
}




TrackPart::TrackPart(TrackPartItem *r, unsigned int widthToLengthRatio)
    : m_trackNb(0),
      m_length(0),
      m_widthToLengthRatio(widthToLengthRatio),
      m_canvasPartItem(r)
{
    if (m_canvasPartItem) {
        m_length = m_canvasPartItem->width() / m_widthToLengthRatio;
        m_canvasPartItem->setPart(this);
    }

    kdDebug(KDEBUG_AREA) << "TrackPart::TrackPart : Length = "
                         << m_length << endl;

}

TrackPart::~TrackPart()
{
    delete m_canvasPartItem;
}

void
TrackPart::updateLength()
{
    m_length = m_canvasPartItem->width() / m_widthToLengthRatio;
    kdDebug(KDEBUG_AREA) << "TrackPart::updateLength : Length = "
                         << m_length << endl;
}



TracksCanvas::TracksCanvas(int gridH, int gridV,
                           QCanvas& c, QWidget* parent,
                           const char* name, WFlags f) :
    QCanvasView(&c,parent,name,f),
    m_newRect(false),
    m_grid(gridH, gridV),
    m_brush(new QBrush(Qt::blue)),
    m_pen(new QPen(Qt::black)),
    m_editMenu(new QPopupMenu(this))
{
    m_editMenu->insertItem(I18N_NOOP("Edit"),
                           this, SLOT(onEdit()));
}

TracksCanvas::~TracksCanvas()
{
    delete m_brush;
    delete m_pen;
}

void
TracksCanvas::update()
{
    canvas()->update();
}

TrackPartItem*
TracksCanvas::findPartClickedOn(QPoint pos)
{
    QCanvasItemList l=canvas()->collisions(pos);

    if (l.count()) {

        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            if (TrackPartItem *item = dynamic_cast<TrackPartItem*>(*it))
                return item;
        }

    }

    return 0;
}

void
TracksCanvas::contentsMousePressEvent(QMouseEvent* e)
{
    m_newRect = false;
    m_currentItem = 0;

    if (e->button() == LeftButton) {

        // Check if we're clicking on a rect
        //
        TrackPartItem *item = findPartClickedOn(e->pos());

        if (item) {
             // we are, so set currentItem to it
            m_currentItem = item;
            return;

        } else { // we are not, so create one

            int gx = m_grid.snapX(e->pos().x()),
                gy = m_grid.snapY(e->pos().y());

            //         qDebug("Creating new rect. at %d, %d - h = %d, v = %d",
            //                gx, gy, m_grid.hstep(), m_grid.vstep());

            m_currentItem = new TrackPartItem(gx, gy,
                                              m_grid.hstep(),
                                              m_grid.vstep(),
                                              canvas());

            m_currentItem->setPen(*m_pen);
            m_currentItem->setBrush(*m_brush);
            m_currentItem->setVisible(true);

            m_newRect = true;

            update();
        }

    } else if (e->button() == RightButton) { // popup menu if over a part

        TrackPartItem *item = findPartClickedOn(e->pos());

        if (item) {
            m_currentItem = item;
            m_editMenu->exec(QCursor::pos());
        }
    }
}

void TracksCanvas::contentsMouseReleaseEvent(QMouseEvent*)
{
    if (!m_currentItem) return;

    if (m_currentItem->width() == 0) {
        qDebug("rect deleted");
        // delete m_currentItem; - TODO emit signal
    }

    if (m_newRect) {

        TrackPart *newPart = new TrackPart(m_currentItem, gridHStep());
        emit addTrackPart(newPart);

    } else {
        // readjust size of corresponding track
        TrackPart *part = m_currentItem->part();
        part->updateLength();
    }


    m_currentItem = 0;
}

void TracksCanvas::contentsMouseMoveEvent(QMouseEvent* e)
{

    if ( m_currentItem ) {

//         qDebug("Enlarging rect. to h = %d, v = %d",
//                gpos.x() - m_currentItem->rect().x(),
//                gpos.y() - m_currentItem->rect().y());

	m_currentItem->setSize(m_grid.snapX(e->pos().x()) - m_currentItem->rect().x(),
                               m_currentItem->rect().height());
	canvas()->update();
    }
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

TrackPartItem*
TracksCanvas::addPartItem(int x, int y, unsigned int nbBars)
{
    TrackPartItem* newPartItem = new TrackPartItem(x, y,
                                                   gridHStep() * nbBars,
                                                   grid().vstep(),
                                                   canvas());
    newPartItem->setPen(*m_pen);
    newPartItem->setBrush(*m_brush);
    newPartItem->setVisible(true);     

    return newPartItem;
}


void
TracksCanvas::onEdit()
{
    emit editTrackPart(m_currentItem->part());
}
