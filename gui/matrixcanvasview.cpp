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

#include "Segment.h"

#include "matrixcanvasview.h"
#include "matrixstaff.h"
#include "matrixelement.h"

using Rosegarden::Segment;
using Rosegarden::timeT;

MatrixCanvasView::MatrixCanvasView(MatrixStaff& staff,
                                   QScrollBar* hsb,
                                   QCanvas *viewing, QWidget *parent,
                                   const char *name, WFlags f)
    : RosegardenCanvasView(hsb, viewing, parent, name, f),
      m_staff(staff),
      m_previousEvTime(0),
      m_previousEvPitch(0),
      m_mouseWasPressed(false),
      m_ignoreClick(false)
{
    viewport()->setMouseTracking(true);
}

MatrixCanvasView::~MatrixCanvasView()
{
}

void MatrixCanvasView::contentsMousePressEvent(QMouseEvent* e)
{
    timeT evTime = m_staff.getTimeForCanvasX(e->x());
    int evPitch = m_staff.getHeightAtCanvasY(e->y());

    
//     kdDebug(KDEBUG_AREA) << "MatrixCanvasView::contentsMousePressEvent() at pitch "
//                          << evPitch << ", time " << evTime << endl;

    QCanvasItemList itemList = canvas()->collisions(e->pos());
    QCanvasItemList::Iterator it;
    MatrixElement* mel = 0;
    QCanvasItem* activeItem = 0;

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;
        QCanvasMatrixRectangle* mRect = 0;
        
        if (item->active()) {
            activeItem = item;
            break;
        }

        if ((mRect = dynamic_cast<QCanvasMatrixRectangle*>(item))) {
            mel = &(mRect->getMatrixElement());
            break;
        }    
    }

    if (activeItem) { // active item takes precedence over notation elements
        emit activeItemPressed(e, activeItem);
        m_mouseWasPressed = true;
        return;
    }

    emit mousePressed(evTime, evPitch, e, mel);
    m_mouseWasPressed = true;

    // Ignore click if it was above the staff and not
    // on an active item
    //
    if (!m_staff.containsCanvasY(e->y()) && !activeItem)
        m_ignoreClick = true;
}

void MatrixCanvasView::contentsMouseMoveEvent(QMouseEvent* e)
{
    if (m_ignoreClick) return;

    timeT evTime = m_staff.getTimeForCanvasX(e->x());
    int evPitch = m_staff.getHeightAtCanvasY(e->y());

    if (evTime != m_previousEvTime) {
        emit hoveredOverAbsoluteTimeChanged(evTime);
        m_previousEvTime = evTime;
    }

    if (evPitch != m_previousEvPitch) {
        emit hoveredOverNoteChanged(m_staff.getNoteNameForPitch(evPitch));
        m_previousEvPitch = evPitch;
    }

    if (m_mouseWasPressed) emit mouseMoved(evTime, e);
    
}

void MatrixCanvasView::contentsMouseDoubleClickEvent (QMouseEvent* e)
{
    if (!m_staff.containsCanvasY(e->y())) {
        m_ignoreClick = true;
        return;
    }

}

void MatrixCanvasView::contentsMouseReleaseEvent(QMouseEvent* e)
{
    if (m_ignoreClick) {
        m_ignoreClick = false;
        return;
    }

    timeT evTime = m_staff.getTimeForCanvasX(e->x());

    emit mouseReleased(evTime, e);
    m_mouseWasPressed = false;
}

void MatrixCanvasView::slotExternalWheelEvent(QWheelEvent* e)
{
    wheelEvent(e);
}
