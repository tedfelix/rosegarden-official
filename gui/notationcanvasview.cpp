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

#include "notationcanvasview.h"
#include "notationelement.h"
#include "qcanvasgroupableitem.h"
#include "qcanvassimplesprite.h"
#include "NotationTypes.h"
#include "notationproperties.h"
#include "notationstaff.h"

#include "rosestrings.h"
#include "rosedebug.h"


NotationCanvasView::NotationCanvasView(const LinedStaffManager<NotationElement> &staffmgr,
                                       QScrollBar *horizBar,
				       QCanvas *viewing, QWidget *parent,
                                       const char *name, WFlags f) :
    RosegardenCanvasView(horizBar, viewing, parent, name, f),
    m_linedStaffManager(staffmgr),
    m_lastYPosNearStaff(0),
    m_currentStaff(0),
    m_currentHeight(-1000),
    m_legerLineOffset(false),
    m_heightTracking(false)
{
// -- switching mandolin-sonatina first staff to page mode:
// default params (I think 16,100): render 1000ms position 1070ms
// 64,100: 1000ms 980ms
// 8, 100: 1140ms 1140ms
// 128, 100: 1060ms 980ms
// 256, 100: 1060ms 980ms / 930ms 920ms

//    canvas()->retune(256, 100);

    viewport()->setMouseTracking(true);

    m_heightMarker = new QCanvasItemGroup(viewing);

    m_vert1 = new QCanvasLineGroupable(viewing, m_heightMarker);
    m_vert1->setPoints(0, 0, 0, 8);
    m_vert1->setPen(QPen(QColor(64, 64, 64), 1));

    m_vert2 = new QCanvasLineGroupable(viewing, m_heightMarker);
    m_vert2->setPoints(17, 0, 17, 8);
    m_vert2->setPen(QPen(QColor(64, 64, 64), 1));

    m_heightMarker->hide();
}

NotationCanvasView::~NotationCanvasView()
{
    // All canvas items are deleted in ~NotationView()
}

void
NotationCanvasView::setHeightTracking(bool t)
{
    m_heightTracking = t;
    if (!t) {
	m_heightMarker->hide();
	canvas()->update();
    }
}
 

void
NotationCanvasView::contentsMouseReleaseEvent(QMouseEvent *e)
{
    emit mouseReleased(e);
}

void
NotationCanvasView::contentsMouseMoveEvent(QMouseEvent *e)
{
    NotationStaff *prevStaff = m_currentStaff;
    int prevHeight = m_currentHeight;
    
    m_currentStaff = dynamic_cast<NotationStaff *>
	(m_linedStaffManager.getStaffForCanvasY(e->y()));

    if (!m_currentStaff) {

	emit hoveredOverNoteChanged(QString::null);
	if (prevStaff) {
	    m_heightMarker->hide();
	    canvas()->update();
	}

    } else {

	m_currentHeight = m_currentStaff->getHeightAtCanvasY(e->y());

	int x = e->x() - 8; // magic based on mouse cursor size
	bool needUpdate = (m_heightTracking && (m_heightMarker->x() != x));
	m_heightMarker->setX(x);

	if (prevStaff  != m_currentStaff ||
	    prevHeight != m_currentHeight) {

	    if (m_heightTracking) {
		setHeightMarkerHeight(e);
		m_heightMarker->show();
		needUpdate = true;
	    }

	    emit hoveredOverNoteChanged
		(strtoqstr
		 (m_currentStaff->getNoteNameAtCanvasCoords(e->x(), e->y())));
	}

	if (needUpdate) canvas()->update();
    }

    NotationElement *elt = getElementAtXCoord(e);
    if (elt) {
	emit hoveredOverAbsoluteTimeChanged(elt->getAbsoluteTime());
    }

    // if(tracking) ??
    emit mouseMoved(e);
}

void NotationCanvasView::contentsMousePressEvent(QMouseEvent *e)
{
    kdDebug(KDEBUG_AREA) << "NotationCanvasView::contentsMousePressEvent() - btn : "
                         << e->button() << " - state : " << e->state()
                         << endl;

    QCanvasItemList itemList = canvas()->collisions(e->pos());

    // We don't want to use m_currentStaff/Height, because we want
    // to make sure the event happens at the point we clicked at
    // rather than the last point for which contentsMouseMoveEvent
    // happened to be called

    NotationStaff *staff = dynamic_cast<NotationStaff *>
	(m_linedStaffManager.getStaffForCanvasY(e->y()));

    if (!staff) {

        if (itemList.count() != 0) // the mouse press occurred on at least one
                                   // item - check if some are active
            processActiveItems(e, itemList);
        else 
            handleMousePress(0, -1, e); // it didn't occur anywhere special

        return;

    }
    
    QCanvasItemList::Iterator it;
    QCanvasNotationSprite* sprite = 0;
    QCanvasItem* activeItem = 0;

    int clickHeight = staff->getHeightAtCanvasY(e->y());

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;

        if (item->active()) {
            activeItem = item;
            break;
        }

        if ((sprite = dynamic_cast<QCanvasNotationSprite*>(item))) {
            NotationElement &el = sprite->getNotationElement();

            if (el.isNote()) { // try to get the right note -- but at worst
			       // we're happy to end up with just any note

		long eventHeight = 0;
		if (el.event()->get<Rosegarden::Int>
		    (NotationProperties::HEIGHT_ON_STAFF, eventHeight)
		    && eventHeight == clickHeight) break;

            } else { // it's not a note, so we don't care about the pitch

                break;
            }
        }
    }

    if (activeItem) { // active item takes precedence over notation elements
        emit activeItemPressed(e, activeItem);
        return;
    }

    int staffNo = -1;

    if (staff)
        staffNo = staff->getId();
    else
        kdDebug(KDEBUG_AREA) << "NotationCanvasView::contentsMousePressEvent() : big problem - couldn't find staff for staff line\n";


    if (sprite)
        handleMousePress(clickHeight, staffNo,
                         e, &(sprite->getNotationElement()));
    else
        handleMousePress(clickHeight, staffNo, e);
}

void NotationCanvasView::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    kdDebug(KDEBUG_AREA) << "NotationCanvasView::contentsMouseDoubleClickEvent()\n";
  
    contentsMousePressEvent(e);
}


void
NotationCanvasView::processActiveItems(QMouseEvent* e,
                                       QCanvasItemList itemList)
{
    QCanvasItem* pressedItem = 0;
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;
        if (item->active() && !pressedItem) {
            kdDebug(KDEBUG_AREA) << "mousepress : got active item\n";
            pressedItem = item;
        }
    }

    if (pressedItem)
        emit activeItemPressed(e, pressedItem);
    
}

void
NotationCanvasView::handleMousePress(int height,
                                     int staffNo,
                                     QMouseEvent *e,
                                     NotationElement *el)
{
    kdDebug(KDEBUG_AREA) << "NotationCanvasView::handleMousePress() at height "
                         << height << endl;

    emit itemPressed(height, staffNo, e, el);
}

bool
NotationCanvasView::posIsTooFarFromStaff(const QPoint &pos)
{
    // return true if pos.y is more than m_staffLineThreshold away from
    // the last pos for which a collision was detected
    //
    return (pos.y() > m_lastYPosNearStaff) ?
        (pos.y() - m_lastYPosNearStaff) > (int)m_staffLineThreshold :
        (m_lastYPosNearStaff - pos.y()) > (int)m_staffLineThreshold;
    
}

int
NotationCanvasView::getLegerLineCount(int height, bool &offset)
{
    //!!! This is far too specifically notation-related to be here, really

    if (height < 0) {

	offset = ((-height % 2) == 1);
	return height / 2;

    } else if (height > 8) {

	offset = ((height % 2) == 1);
	return (height - 8) / 2;
    }

    return 0;
}

void
NotationCanvasView::setHeightMarkerHeight(QMouseEvent *e)
{
    NotationStaff *staff = dynamic_cast<NotationStaff *>
	(m_linedStaffManager.getStaffForCanvasY(e->y()));

    int height = staff->getHeightAtCanvasY(e->y());
    int lineY = staff->getCanvasYForHeight(height, e->y());

//    kdDebug(KDEBUG_AREA) << "NotationCanvasView::setHeightMarkerHeight: "
//			 << e->y() << " snapped to line -> " << lineY
//			 << " (height " << height << ")" << endl;

    int spacing = staff->getLineSpacing() - 1;

    m_staffLineThreshold = spacing;
    m_vert1->setPoints(0, -spacing/2, 0, spacing/2);
    m_vert2->setPoints(17, -spacing/2, 17, spacing/2); // magic based on mouse cursor size
    m_heightMarker->setY(lineY);

    bool legerLineOffset = false;
    int  legerLineCount = getLegerLineCount(height, legerLineOffset);

    if (legerLineCount  != (int)m_legerLines.size() ||
	legerLineOffset != m_legerLineOffset) {

	bool above = false;
	if (legerLineCount < 0) {
	    above = true;
	    legerLineCount = -legerLineCount;
	}

	int i;
	for (i = 0; i < (int)m_legerLines.size(); ++i) {
	    delete m_legerLines[i];
	}
	m_legerLines.clear();

	for (i = 0; i < legerLineCount; ++i) {

	    QCanvasLineGroupable *line = 
		new QCanvasLineGroupable(canvas(), m_heightMarker);

	    line->setPen(QPen(QColor(64, 64, 64), 1));

	    int y = (int)m_heightMarker->y() +
		(above ? -1 : 1) * (i * (spacing + 1));
	    int x = (int)m_heightMarker->x() + 1;

	    if (legerLineOffset) {
		if (above) y -= spacing/2 + 1;
		else y += spacing/2;
	    }

	    line->setPoints(x, y, x+15, y); // magic based on mouse cursor size
	    m_legerLines.push_back(line);
	}

	m_legerLineOffset = legerLineOffset;
    }
}

NotationElement *
NotationCanvasView::getElementAtXCoord(QMouseEvent *e) // any old element
{
    QRect threshold(e->pos(), QSize(4,100)); //!!!
    threshold.moveCenter(e->pos());

    QCanvasItemList itemList = canvas()->collisions(threshold);
    
    QCanvasItemList::Iterator it;
    QCanvasNotationSprite* sprite = 0;

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;

        if ((sprite = dynamic_cast<QCanvasNotationSprite*>(item))) {
            return &(sprite->getNotationElement());
	}
    }

    return 0;
}

void NotationCanvasView::print(QPainter* printpainter)
{
    drawContents(printpainter, 0, 0,
                 canvas()->width(), canvas()->height());
}
