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
    m_staffLineThreshold(10),
    m_legerLineOffset(false),
    m_positionTracking(false)
{
    viewport()->setMouseTracking(true);

    m_positionMarker = new QCanvasItemGroup(viewing);

    m_vert1 = new QCanvasLineGroupable(viewing, m_positionMarker);
    m_vert1->setPoints(0, 0, 0, 8);
    m_vert1->setPen(QPen(QColor(64, 64, 64), 1));

    m_vert2 = new QCanvasLineGroupable(viewing, m_positionMarker);
    m_vert2->setPoints(17, 0, 17, 8);
    m_vert2->setPen(QPen(QColor(64, 64, 64), 1));

    m_positionMarker->hide();
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

	int x = e->x() - 8; // magic based on mouse cursor size
	bool needUpdate = (m_positionTracking && (m_positionMarker->x() != x));
	m_positionMarker->setX(x);

	if (prevLine != m_currentHighlightedLine) {

	    if (m_positionTracking) {
		setPositionMarkerHeight(m_currentHighlightedLine);
		m_positionMarker->show();
		needUpdate = true;
	    }

	    emit hoveredOverNoteChange
		(getNoteNameForLine(m_currentHighlightedLine, e->x()));
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

QString
NotationCanvasView::getNoteNameForLine(const StaffLine *line, int x)
{
    int h = line->getHeight();

    // Ideally we'd need to take currently-selected accidental into
    // account, but that information is tricky to get and not essential

    const NotationStaff *staff =
	dynamic_cast<const NotationStaff *>(line->group());

    Rosegarden::Clef clef;
    Rosegarden::Key key;
    staff->getClefAndKeyAtX(x, clef, key);

    std::string noteName =
	Rosegarden::NotationDisplayPitch
	(h, Rosegarden::Accidentals::NoAccidental).getAsString(clef, key);

    return QString(noteName.c_str());
}

int
NotationCanvasView::getStaffLineSpacing(const StaffLine *line)
{
    //!!! There should be a better structure for doing this stuff

    const NotationStaff *staff =
	dynamic_cast<const NotationStaff *>(line->group());

    return staff->getLineSpacing() - 1;
}

int
NotationCanvasView::getLegerLineCount(const StaffLine *line, bool &offset)
{
    //!!! This is far too specifically notation-related to be here, really

    int height = line->getHeight();

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
NotationCanvasView::setPositionMarkerHeight(const StaffLine *line)
{
    int spacing = getStaffLineSpacing(line);
    m_staffLineThreshold = spacing;
    m_vert1->setPoints(0, -spacing/2, 0, spacing/2);
    m_vert2->setPoints(17, -spacing/2, 17, spacing/2); // magic based on mouse cursor size
    m_positionMarker->setY(line->y() + line->startPoint().y());

    bool legerLineOffset = false;
    int  legerLineCount = getLegerLineCount(line, legerLineOffset);

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
		new QCanvasLineGroupable(canvas(), m_positionMarker);

	    line->setPen(QPen(QColor(64, 64, 64), 1));

	    int y = (int)m_positionMarker->y() +
		(above ? -1 : 1) * (i * (spacing + 1));
	    int x = (int)m_positionMarker->x() + 1;

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

    
StaffLine *
NotationCanvasView::findClosestLineWithinThreshold(QMouseEvent* e)
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

	    if (!line->isSignificant()) continue;

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

