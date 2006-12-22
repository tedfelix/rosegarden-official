/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MatrixCanvasView.h"

#include "base/SnapGrid.h"
#include "gui/general/MidiPitchLabel.h"
#include "gui/general/RosegardenCanvasView.h"
#include "MatrixElement.h"
#include "MatrixStaff.h"
#include "QCanvasMatrixRectangle.h"
#include <qcanvas.h>
#include <qpoint.h>
#include <qwidget.h>
#include "misc/Debug.h"



namespace Rosegarden
{

MatrixCanvasView::MatrixCanvasView(MatrixStaff& staff,
                                   SnapGrid *snapGrid,
                                   bool drumMode,
                                   QCanvas *viewing, QWidget *parent,
                                   const char *name, WFlags f)
        : RosegardenCanvasView(viewing, parent, name, f),
        m_staff(staff),
        m_snapGrid(snapGrid),
        m_drumMode(drumMode),
        m_previousEvTime(0),
        m_previousEvPitch(0),
        m_mouseWasPressed(false),
        m_ignoreClick(false),
        m_smoothModifier(Qt::ShiftButton),
        m_lastSnap(SnapGrid::SnapToBeat),
        m_isSnapTemporary(false)
{
    viewport()->setMouseTracking(true);
}

MatrixCanvasView::~MatrixCanvasView()
{}

void MatrixCanvasView::contentsMousePressEvent(QMouseEvent* e)
{
    QPoint p = inverseMapPoint(e->pos());

    updateGridSnap(e);

    MATRIX_DEBUG << "MatrixCanvasView::contentsMousePressEvent: snap time is " << m_snapGrid->getSnapTime(double(p.x())) << endl;

    timeT evTime;

    if (m_drumMode) {
        evTime = m_snapGrid->snapX(p.x(), SnapGrid::SnapEither);
        MATRIX_DEBUG << "MatrixCanvasView: drum mode: snapEither " << p.x() << " -> " << evTime << endl;
    } else {
        evTime = m_snapGrid->snapX(p.x(), SnapGrid::SnapLeft);
        MATRIX_DEBUG << "MatrixCanvasView: normal mode: snapLeft " << p.x() << " -> " << evTime << endl;
    }

    int evPitch = m_staff.getHeightAtCanvasCoords(p.x(), p.y());

    timeT emTime = m_staff.getSegment().getEndMarkerTime();
    if (evTime > emTime)
        evTime = emTime;
    timeT esTime = m_staff.getSegment().getStartTime();
    if (evTime < esTime)
        evTime = esTime;

    //     MATRIX_DEBUG << "MatrixCanvasView::contentsMousePressEvent() at pitch "
    //                          << evPitch << ", time " << evTime << endl;

    QCanvasItemList itemList = canvas()->collisions(p);
    QCanvasItemList::Iterator it;
    MatrixElement* mel = 0;
    QCanvasItem* activeItem = 0;

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;

        QCanvasMatrixRectangle *mRect = 0;

        if (item->active()) {
            activeItem = item;
            break;
        }

        if ((mRect = dynamic_cast<QCanvasMatrixRectangle*>(item))) {

            // QCanvas::collisions() can be a bit optimistic and report
            // items which are close to the point but not actually under it.
            // So a little sanity check helps.
            if (! mRect->rect().contains(p, true))
                continue;

            mel = &(mRect->getMatrixElement());
            MATRIX_DEBUG << "MatrixCanvasView::contentsMousePressEvent: collision with an existing matrix element" << endl;
            break;
        }
    }

    if (activeItem) { // active item takes precedence over notation elements
        emit activeItemPressed(e, activeItem);
        m_mouseWasPressed = true;
        return ;
    }

    emit mousePressed(evTime, evPitch, e, mel);
    m_mouseWasPressed = true;

    // Ignore click if it was above the staff and not
    // on an active item
    //
    if (!m_staff.containsCanvasCoords(p.x(), p.y()) && !activeItem)
        m_ignoreClick = true;
}

void MatrixCanvasView::contentsMouseMoveEvent(QMouseEvent* e)
{
    QPoint p = inverseMapPoint(e->pos());
    /*
        if (m_snapGrid->getSnapTime(double(p.x())))
            m_lastSnap = m_snapGrid->getSnapTime(double(p.x()));
    */
    updateGridSnap(e);

    if (m_ignoreClick)
        return ;

    timeT evTime;
    if (m_drumMode) {
        evTime = m_snapGrid->snapX(p.x(), SnapGrid::SnapEither);
    } else {
        evTime = m_snapGrid->snapX(p.x(), SnapGrid::SnapLeft);
    }

    int evPitch = m_staff.getHeightAtCanvasCoords(p.x(), p.y());

    timeT emTime = m_staff.getSegment().getEndMarkerTime();
    if (evTime > emTime)
        evTime = emTime;

    timeT stTime = m_staff.getSegment().getStartTime();
    if (evTime < stTime)
        evTime = stTime;

    if (evTime != m_previousEvTime) {
        emit hoveredOverAbsoluteTimeChanged(evTime);
        m_previousEvTime = evTime;
    }

    QCanvasItemList itemList = canvas()->collisions(p);
    MatrixElement* mel = 0;

    for (QCanvasItemList::iterator it = itemList.begin();
            it != itemList.end(); ++it) {

        QCanvasItem *item = *it;
        QCanvasMatrixRectangle *mRect = 0;

        if ((mRect = dynamic_cast<QCanvasMatrixRectangle*>(item))) {
            if (!mRect->rect().contains(p, true))
                continue;
            mel = &(mRect->getMatrixElement());
            MATRIX_DEBUG << "have element" << endl;
            break;
        }
    }

    if (!m_mouseWasPressed && // if mouse pressed, leave this to the tool
        (evPitch != m_previousEvPitch || mel)) {
        MidiPitchLabel label(evPitch);
        if (mel) {
            emit hoveredOverNoteChanged(evPitch, true,
                                        mel->event()->getAbsoluteTime());
        } else {
            emit hoveredOverNoteChanged(evPitch, false, 0);
        }
        m_previousEvPitch = evPitch;
    }

//    if (m_mouseWasPressed)
        emit mouseMoved(evTime, evPitch, e);

}

void MatrixCanvasView::contentsMouseDoubleClickEvent (QMouseEvent* e)
{
    QPoint p = inverseMapPoint(e->pos());

    if (!m_staff.containsCanvasCoords(p.x(), p.y())) {
        m_ignoreClick = true;
        return ;
    }

    contentsMousePressEvent(e);
}

void MatrixCanvasView::contentsMouseReleaseEvent(QMouseEvent* e)
{
    QPoint p = inverseMapPoint(e->pos());

    if (m_ignoreClick) {
        m_ignoreClick = false;
        return ;
    }

    timeT evTime;
    if (m_drumMode) {
        evTime = m_snapGrid->snapX(p.x(), SnapGrid::SnapEither);
    } else {
        evTime = m_snapGrid->snapX(p.x(), SnapGrid::SnapLeft);
    }

    int evPitch = m_staff.getHeightAtCanvasCoords(p.x(), p.y());

    timeT emTime = m_staff.getSegment().getEndMarkerTime();
    if (evTime > emTime)
        evTime = emTime;

    emit mouseReleased(evTime, evPitch, e);
    m_mouseWasPressed = false;
}

void MatrixCanvasView::slotExternalWheelEvent(QWheelEvent* e)
{
    wheelEvent(e);
}

void MatrixCanvasView::updateGridSnap(QMouseEvent *e)
{
    Qt::ButtonState bs = e->state();

    //    MATRIX_DEBUG << "MatrixCanvasView::updateGridSnap : bs = "
    //		 << bs << " - sm = " << getSmoothModifier() << ", is temporary " << m_isSnapTemporary << ", saved is " << m_lastSnap << endl;

    if (bs & getSmoothModifier()) {

        if (!m_isSnapTemporary) {
            m_lastSnap = m_snapGrid->getSnapSetting();
        }
        m_snapGrid->setSnapTime(SnapGrid::NoSnap);
        m_isSnapTemporary = true;

    } else if (m_isSnapTemporary) {

        m_snapGrid->setSnapTime(m_lastSnap);
        m_isSnapTemporary = false;
    }
}

void MatrixCanvasView::enterEvent(QEvent *e)
{
    emit mouseEntered();
}

void MatrixCanvasView::leaveEvent(QEvent *e)
{
    emit mouseLeft();
}

}
#include "MatrixCanvasView.moc"
