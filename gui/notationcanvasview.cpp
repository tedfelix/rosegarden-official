/***************************************************************************
                          notationcanvasview.cpp  -  description
                             -------------------
    begin                : Thu Sep 28 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
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

#include "notationcanvasview.h"
#include "qcanvasgroupableitem.h"
#include "qcanvasitemgroup.h"
#include "staffline.h"

#include "rosedebug.h"

NotationCanvasView::NotationCanvasView(QCanvas *viewing, QWidget *parent,
                                       const char *name, WFlags f)
    : QCanvasView(viewing, parent, name, f),
      m_currentHighlightedLine(0)
{
    viewport()->setMouseTracking(true);
}

void
NotationCanvasView::contentsMouseReleaseEvent(QMouseEvent*)
{
    canvas()->update();
}

void
NotationCanvasView::contentsMouseMoveEvent(QMouseEvent *e)
{
    bool needUpdate = false;
    
    if (m_currentHighlightedLine) {
        m_currentHighlightedLine->setHighlighted(false);
        needUpdate = true;
    }

    m_currentHighlightedLine = 0;

    QCanvasItemList itemList = canvas()->collisions(e->pos());

    if(itemList.isEmpty()) return;

    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;

        StaffLine *staffLine;
    
        if ((staffLine = dynamic_cast<StaffLine*>(item))) {
            m_currentHighlightedLine = staffLine;
            m_currentHighlightedLine->setHighlighted(true);
            needUpdate = true;
            break;
        }
    }

    if (needUpdate)
        canvas()->update();
}

void
NotationCanvasView::contentsMousePressEvent(QMouseEvent *e)
{
    kdDebug(KDEBUG_AREA) << "mousepress" << endl;

    if (m_currentHighlightedLine) {
        kdDebug(KDEBUG_AREA) << "mousepress : m_currentHighlightedLine != 0 - inserting note - staff pitch :"
                             << m_currentHighlightedLine->associatedPitch() << endl;
        insertNote(m_currentHighlightedLine, e->pos());

        return;
    }
    

    QCanvasItemList itemList = canvas()->collisions(e->pos());

    if(itemList.isEmpty()) { // click was not on an item
        kdDebug(KDEBUG_AREA) << "mousepress : Not on an item" << endl;
        return;
    }

    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;

        StaffLine *staffLine;
    
        if ((staffLine = dynamic_cast<StaffLine*>(item))) {
            kdDebug(KDEBUG_AREA) << "mousepress : on a staff Line - insert note - staff pitch :"
                                 << staffLine->associatedPitch() << endl;
            insertNote(staffLine, e->pos());
            // staffLine->setPen(blue); - debug feedback to confirm which line what clicked on
        
            return;
        }
    }
    

//     QCanvasGroupableItem *gitem;

//     if((gitem = dynamic_cast<QCanvasGroupableItem*>(item))) {
//         kdDebug(KDEBUG_AREA) << "mousepress : Groupable item" << endl;
//         QCanvasItemGroup *t = gitem->group();

//         if(t->active())
//             m_movingItem = t;
//         else {
//             kdDebug(KDEBUG_AREA) << "mousepress : Unmoveable groupable item" << endl;
//             m_movingItem = 0; // this is not a moveable item
//             return;
//         }
//     } else {
//         m_movingItem = item;
//     }

//     m_draggingItem = true;
//     m_movingItem->move(e->x(), e->y());
    canvas()->update();
}


void
NotationCanvasView::insertNote(const StaffLine *line, const QPoint &pos)
{
    kdDebug(KDEBUG_AREA) << "insertNote at pitch " << line->associatedPitch() << endl;

    emit noteInserted(line->associatedPitch(), pos);
    
}
