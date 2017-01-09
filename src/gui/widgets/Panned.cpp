/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Panned]"

#include "Panned.h"

#include "misc/Debug.h"
#include "base/Profiler.h"
#include "gui/general/GUIPalette.h"

#include <QScrollBar>

#include <iostream>

namespace Rosegarden
{

Panned::Panned() :
    m_pointerVisible(false)
{
}

void
Panned::resizeEvent(QResizeEvent *ev)
{
    QPointF near = mapToScene(0, 0);
    QPointF far = mapToScene(width(), height());
    QSizeF sz(far.x()-near.x(), far.y()-near.y());
    QRectF pr(near, sz);

    RG_DEBUG << "Panned::resizeEvent: pr = " << pr;

    if (pr != m_pannedRect) {
        m_pannedRect = pr;
        emit pannedRectChanged(pr);
    }

    QGraphicsView::resizeEvent(ev);
}

void
Panned::paintEvent(QPaintEvent *e)
{
    Profiler profiler("Panned::paintEvent");

    QGraphicsView::paintEvent(e);
}

void
Panned::drawForeground(QPainter *paint, const QRectF &)
{
    Profiler profiler("Panned::drawForeground");

    QPointF near = mapToScene(0, 0);
    QPointF far = mapToScene(width(), height());
    QSizeF sz(far.x()-near.x(), far.y()-near.y());
    QRectF pr(near, sz);

//    RG_DEBUG << "Panned::drawForeground: pr = " << pr;

    if (pr != m_pannedRect) {
        if (pr.x() != m_pannedRect.x()) emit pannedContentsScrolled();
        m_pannedRect = pr;
        emit pannedRectChanged(pr);
    }

    if (m_pointerVisible && scene()) {
        QPoint top = mapFromScene(m_pointerTop);
        float height = m_pointerHeight;
        if (height == 0.f) height = scene()->height();
        QPoint bottom = mapFromScene
            (m_pointerTop + QPointF(0, height));
        paint->save();
        paint->setWorldMatrix(QMatrix());
        paint->setPen(QPen(GUIPalette::getColour(GUIPalette::Pointer), 2));
        paint->drawLine(top, bottom);
        paint->restore();
    }
}

void
Panned::slotSetPannedRect(QRectF pr)
{
    centerOn(pr.center());
//	setSceneRect(pr);
//	m_pannedRect = pr;
}

void
Panned::slotShowPositionPointer(float x) // scene coord; full height
{
    if (m_pointerVisible) {
        QRect oldRect = QRect(mapFromScene(m_pointerTop),
                              QSize(4, viewport()->height()));
        oldRect.moveTop(0);
        oldRect.moveLeft(oldRect.left() - 1);
        viewport()->update(oldRect);
//        RG_DEBUG << "Panned::slotShowPositionPointer: old rect " << oldRect;
    }
    m_pointerVisible = true;
    m_pointerTop = QPointF(x, 0);
    m_pointerHeight = 0;
    QRect newRect = QRect(mapFromScene(m_pointerTop),
                          QSize(4, viewport()->height()));
    newRect.moveTop(0);
    newRect.moveLeft(newRect.left() - 1);
    viewport()->update(newRect);
//    RG_DEBUG << "Panned::slotShowPositionPointer: new rect " << newRect;
}

void
Panned::slotShowPositionPointer(QPointF top, float height) // scene coords
{
    m_pointerVisible = true;
    m_pointerTop = top;
    m_pointerHeight = height;
    viewport()->update(); //!!! should update old and new pointer areas only, as in the previous function
}

void
Panned::slotEnsurePositionPointerInView(bool page)
{
    if (!m_pointerVisible || !scene()) return;

    // scroll horizontally only

    double x = m_pointerTop.x();
    double y = m_pointerTop.y();

    //!!! n.b. should probably behave differently if the pointer is
    //!!! not full height

    int hMin = horizontalScrollBar()->minimum();
    int hMax = horizontalScrollBar()->maximum();

    double leftDist = 0.15;
    double rightDist = 0.15;

    int w = width();                        // View width in pixels
    QRectF r = mapToScene(0, 0, w, 1).boundingRect();
    double ws = r.width();                  // View width in scene units
    double left = r.x();                    // View left x in scene units
    double right = left + ws;               // View right x in scene units

    QRectF sr = sceneRect();
    double length = sr.width();             // Scene horizontal length
    double x1 = sr.x();                     // Scene x minimum value
    double x2 = x1 + length;                // Scene x maximum value

    double leftThreshold = left + ws * leftDist;
    double rightThreshold = right - ws * rightDist;
    // double delta = x - leftThreshold;

    // Is x inside the scene? If not do nothing.
    if ((x < x1) || (x > x2)) return;

//    std::cerr << "page = " << page << std::endl;

    int value = horizontalScrollBar()->value();

//    std::cerr << "x = " << x << ", left = " << left << ", leftThreshold = " << leftThreshold << ", right = " << right << ", rightThreshold = " << rightThreshold << std::endl;

    // Is x inside the view?
//  if (x < leftThreshold || (x > rightThreshold && x < right && page)) {
    // Allow a little room for x to overshoot the left threshold when the scrollbar is updated.
    if (x < leftThreshold - 100 || (x > rightThreshold && x < right && page)) {
//        std::cerr << "big scroll (x is off left, or paging)" << std::endl;
        // scroll to have the left of the view, plus threshold, at x
        value = hMin + (((x - ws * leftDist) - x1) * (hMax - hMin)) / (length - ws);
    } else if (x > rightThreshold) {
//        std::cerr << "small scroll (x is off right and not paging)" << std::endl;
        value = hMin + (((x - ws * (1.0 - rightDist)) - x1) * (hMax - hMin)) / (length - ws);
    }

    if (value < hMin) value = hMin;
    else if (value > hMax) value = hMax;


    // If pointer doesn't fit vertically inside current view,
    // mouseMoveEvent may be called from scene when ensureVisible()
    // is called.
    // Then setupMouseEvent() and setCurrentStaff() will be called from scene.
    // Then slotUpdatePointerPosition() and slotpointerPositionChanged() will
    // be called from NotationWidget.
    // Then again Panned::slotEnsurePositionPointerInView() and probably the
    // is moved again : this is an infinite recursive call loop which leads to
    // crash.

    // To avoid it, ensureVisible() should not be called on a rectangle
    // highter than the view :

    // Convert pointer height from scene coords to pixels
    int hPointerPx = mapFromScene(0, 0, 1,
                                  m_pointerHeight).boundingRect().height();

    // Compute new pointer height and margin to ensure than pointer + margin
    // vertically fit inside current view (default ensureVisible() margins
    // are 50 pixels)
    double ph;        // New height to call ensureVisible()
    double ymargin;   // New vertical margin to call ensureVisible()
    int h = height();
    if (h < hPointerPx) {
        // If pointer is taller than view replace it with a smaller object
        // and use null margins
        ph = (m_pointerHeight * h) / hPointerPx;
        ymargin = 0;
    } else if (h < (hPointerPx + 100)) {
        // If pointer is smaller than view but taller than view + margins
        // keep pointer as is but use null margins
        ph = m_pointerHeight;
        ymargin = 0;
    } else {
        // Sizes are OK : don't change anything
        ph = m_pointerHeight;
        ymargin = 50;             // Keep default margin
    }

    // before h scroll
    if (y != 0) {
        if (ph > 6) ph = ph - 5;      // for safety
        ensureVisible(QRectF(x, y, 1, ph), 50, ymargin);
    }

    horizontalScrollBar()->setValue(value);
}

void
Panned::slotHidePositionPointer()
{
    m_pointerVisible = false;
    viewport()->update(); //!!! should update old pointer area only, really
}

void
Panned::wheelEvent(QWheelEvent *ev)
{
    emit wheelEventReceived(ev);
    QGraphicsView::wheelEvent(ev);
}

void
Panned::slotEmulateWheelEvent(QWheelEvent *ev)
{
    QGraphicsView::wheelEvent(ev);
}

void
Panned::leaveEvent(QEvent *)
{
    emit mouseLeaves();
}

}

