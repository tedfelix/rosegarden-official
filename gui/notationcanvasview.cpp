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

#include "notationcanvasview.h"
#include "notationelement.h"
#include "qcanvasgroupableitem.h"
#include "qcanvassimplesprite.h"
#include "staffline.h"
#include "NotationTypes.h"
#include "notationproperties.h"
#include "notationstaff.h"

#include "rosedebug.h"

using namespace NotationProperties;

NotationCanvasView::NotationCanvasView(QCanvas *viewing, QWidget *parent,
                                       const char *name, WFlags f) :
    QCanvasView(viewing, parent, name, f),
    m_currentHighlightedLine(0),
    m_lastYPosNearStaff(0),
    m_staffLineThreshold(10), //!!!
    m_positionTracking(false)
{
    viewport()->setMouseTracking(true);

    m_positionMarker = new QCanvasItemGroup(viewing);

    QCanvasLineGroupable *line =
	new QCanvasLineGroupable(viewing, m_positionMarker);
    line->setPoints(0, 0, 0, 8); //!!!
    line->setPen(QPen(QColor(64, 64, 64), 1));

    line = new QCanvasLineGroupable(viewing, m_positionMarker);
    line->setPoints(17, 0, 17, 8); //!!!
    line->setPen(QPen(QColor(64, 64, 64), 1));

    m_positionMarker->hide();

    m_legerLinePositionMarker = 0; // for now
}

NotationCanvasView::~NotationCanvasView()
{
    // All canvas items are deleted in ~NotationView()
}

void
NotationCanvasView::setPositionTracking(bool t)
{
    m_positionTracking = t;
    if (!t) {
	m_positionMarker->hide();
	canvas()->update();
    }
}
 

void
NotationCanvasView::contentsMouseReleaseEvent(QMouseEvent *e)
{
    emit mouseRelease(e);
}

void
NotationCanvasView::contentsMouseMoveEvent(QMouseEvent *e)
{
    StaffLine *prevLine = m_currentHighlightedLine;
    m_currentHighlightedLine = findClosestLineWithinThreshold(e);

    if (!m_currentHighlightedLine) {

	emit hoveredOverNoteChange(QString::null);
	if (prevLine) {
	    m_positionMarker->hide();
	    canvas()->update();
	}

    } else {

	int x = e->x() - 8; //!!!
	bool needUpdate = (m_positionTracking && (m_positionMarker->x() != x));
	m_positionMarker->setX(x);

	if (prevLine != m_currentHighlightedLine) {

	    if (m_positionTracking) {
		m_positionMarker->setY
		    (m_currentHighlightedLine->y() +
		     m_currentHighlightedLine->startPoint().y() - 4);//!!!
		m_positionMarker->show();
		needUpdate = true;
	    }

	    QString noteName = getNoteNameForLine(m_currentHighlightedLine,
						  e->x());

	    emit hoveredOverNoteChange(noteName);
	}

	if (needUpdate) canvas()->update();
    }

    NotationElement *elt = getElementAtXCoord(e);
    if (elt) {
	emit hoveredOverAbsoluteTimeChange(elt->getAbsoluteTime());
    }

    // if(tracking) ??
    emit mouseMove(e);
}

void NotationCanvasView::contentsMousePressEvent(QMouseEvent *e)
{
    kdDebug(KDEBUG_AREA) << "NotationCanvasView::contentsMousePressEvent() - btn : "
                         << e->button() << " - state : " << e->state()
                         << endl;

    QCanvasItemList itemList = canvas()->collisions(e->pos());

    // We don't want to use m_currentHighlightedLine, because we want
    // to make sure the event happens at the point we clicked at
    // rather than the last point for which contentsMouseMoveEvent
    // happened to be called

    StaffLine *staffLine = findClosestLineWithinThreshold(e);

    if (!staffLine) {

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

    // Get the actual pitch where the click occurred
    //
    Rosegarden::Key key;
    Rosegarden::Clef clef;

    int clickHeight = staffLine->getHeight();

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
		if (el.event()->get<Rosegarden::Int>(HEIGHT_ON_STAFF,
						     eventHeight)
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

    // Find staff on which the click occurred
    QCanvasItemGroup *group = staffLine->group();
    NotationStaff *staff = dynamic_cast<NotationStaff*>(group);

    if (staff)
        staffNo = staff->getId();
    else
        kdDebug(KDEBUG_AREA) << "NotationCanvasView::contentsMousePressEvent() : big problem - couldn't find staff for staff line\n";


    if (sprite)
        handleMousePress(staffLine, staffNo,
                         e, &(sprite->getNotationElement()));
    else
        handleMousePress(staffLine, staffNo, e);
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
NotationCanvasView::handleMousePress(const StaffLine *line,
                                     int staffNo,
                                     QMouseEvent *e,
                                     NotationElement *el)
{
    int h = line ? line->getHeight() : StaffLine::NoHeight;

    kdDebug(KDEBUG_AREA) << "NotationCanvasView::handleMousePress() at height " << h << endl;

    emit itemPressed(h, staffNo, e, el);
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

QString NotationCanvasView::getNoteNameForLine(const StaffLine *line,
					       int x)
{
    int h = line->getHeight();

    //!!! Need to take currently-selected accidental into account

    const NotationStaff *staff =
	dynamic_cast<const NotationStaff *>(line->group());

    Rosegarden::Clef clef;
    Rosegarden::Key key;
    staff->getClefAndKeyAtX(x, clef, key);

    std::string noteName =
	Rosegarden::NotationDisplayPitch(h, Rosegarden::NoAccidental).
        getAsString(clef, key);

    return QString(noteName.c_str());
}


StaffLine* NotationCanvasView::findClosestLineWithinThreshold(QMouseEvent* e)
{
//    kdDebug(KDEBUG_AREA) << "NotationCanvasView::findClosestLineWithinThreshold()\n";
    
    // Compute a threshold rectangle around the event's position
    //
    QRect threshold(e->pos(), QSize(30,30));
    threshold.moveCenter(e->pos());

    QCanvasItemList nearbyLines = canvas()->collisions(threshold);

    QCanvasItemList::Iterator it;

    unsigned int minDist = canvas()->height();
    StaffLine* closestLine = 0;

    // Scan all StaffLines which collide with the threshold rectangle
    // to find the closest one
    //
    for (it = nearbyLines.begin(); it != nearbyLines.end(); ++it) {

        StaffLine* line = 0;
        QCanvasItem *item = *it;

        if ((line = dynamic_cast<StaffLine*>(item))) {

            unsigned int dist = 0;
	    int y = (int)(line->y() + line->startPoint().y());

            if (y == e->y()) {
                minDist = 0;
                closestLine = line;
                break;
            }

            dist = abs(int(y - e->y()));

            if (dist < minDist) {
                minDist = dist;
                closestLine = line;
            }           
        }
    }

    if (closestLine && minDist <= m_staffLineThreshold) return closestLine;

    return 0;
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

