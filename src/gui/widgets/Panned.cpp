/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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

#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>


namespace Rosegarden
{


Panned::Panned() :
    m_pannedRect(),
    m_pointerTop(),
    m_pointerHeight(0),
    m_pointerVisible(false),
    m_wheelZoomPan(false)
{
}

void
Panned::resizeEvent(QResizeEvent *ev)
{
    const QRectF viewportScene = mapToScene(rect()).boundingRect();

    //RG_DEBUG << "resizeEvent(): viewportScene = " << viewportScene;

    // If the viewport has changed (resized)...
    if (viewportScene != m_pannedRect) {
        m_pannedRect = viewportScene;
        emit pannedRectChanged(viewportScene);
    }

    QGraphicsView::resizeEvent(ev);
}

void
Panned::paintEvent(QPaintEvent *e)
{
    Profiler profiler("Panned::paintEvent");

    QGraphicsView::paintEvent(e);
}

#if 0
// It would be nice to do this instead of detecting scrolling in
// drawForeground().  Unfortunately, this is unreliable.  Need to
// track down why.  Specifically, scrolling with the arrow keys ends
// up out of sync.
void
Panned::scrollContentsBy(int dx, int dy)
{
    //RG_DEBUG << "scrollContentsBy()";

    // Determine whether we've scrolled and emit appropriate signals.

    const QRectF viewportScene = mapToScene(rect()).boundingRect();

    // If the viewport has changed (scrolled)...
    if (viewportScene != m_pannedRect) {
        // If we've moved horizontally
        if (viewportScene.x() != m_pannedRect.x())
            emit pannedContentsScrolled();

        m_pannedRect = viewportScene;

        emit pannedRectChanged(viewportScene);
    }

    QGraphicsView::scrollContentsBy(dx, dy);
}
#endif

void
Panned::drawForeground(QPainter *paint, const QRectF &)
{
    Profiler profiler("Panned::drawForeground");

    // Detect viewport scrolling and emit appropriate signals

    const QRectF viewportScene = mapToScene(rect()).boundingRect();

    // If the viewport has changed (scrolled)...
    if (viewportScene != m_pannedRect) {
        // If we've moved horizontally
        if (viewportScene.x() != m_pannedRect.x())
            emit pannedContentsScrolled();

        m_pannedRect = viewportScene;

        emit pannedRectChanged(viewportScene);
    }

    // Draw the Playback Position Pointer

    if (!m_pointerVisible)
        return;

    if (!scene())
        return;

    QPoint top = mapFromScene(m_pointerTop);

    float height = m_pointerHeight;
    if (height == 0)
        height = scene()->height();

    QPoint bottom = mapFromScene(m_pointerTop + QPointF(0, height));

    paint->save();
    paint->setWorldMatrix(QMatrix());
    paint->setPen(QPen(GUIPalette::getColour(GUIPalette::Pointer), 2));
    paint->drawLine(top, bottom);
    paint->restore();
}

void
Panned::slotSetPannedRect(QRectF viewportScene)
{
    // ??? We're just centering.  That explains why zoom has to travel by
    //     a different path (zoomIn() and zoomOut() signals).
    centerOn(viewportScene.center());

    // ??? Wouldn't this eliminate the need for the zoom signals?
    //setSceneRect(viewportScene);
}

void
Panned::showPositionPointer(float x) // scene coord; full height
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
Panned::showPositionPointer(QPointF top, float height) // scene coords
{
    m_pointerVisible = true;
    m_pointerTop = top;
    m_pointerHeight = height;
    viewport()->update(); //!!! should update old and new pointer areas only, as in the previous function
}

void
Panned::ensurePositionPointerInView(bool page)
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

    //RG_DEBUG << "ensurePositionPointerInView(): page = " << page;

    int value = horizontalScrollBar()->value();

    //RG_DEBUG << "x = " << x << ", left = " << left << ", leftThreshold = " << leftThreshold << ", right = " << right << ", rightThreshold = " << rightThreshold;

    // Is x inside the view?
//  if (x < leftThreshold || (x > rightThreshold && x < right && page)) {
    // Allow a little room for x to overshoot the left threshold when the scrollbar is updated.
    if (x < leftThreshold - 100 || (x > rightThreshold && x < right && page)) {
        //RG_DEBUG << "big scroll (x is off left, or paging)" <<;
        // scroll to have the left of the view, plus threshold, at x
        value = hMin + (((x - ws * leftDist) - x1) * (hMax - hMin)) / (length - ws);
    } else if (x > rightThreshold) {
        //RG_DEBUG << "small scroll (x is off right and not paging)";
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
    // Then again Panned::ensurePositionPointerInView() and probably the ?
    // is moved again : this is an infinite recursive call loop which leads to
    // crash.

    // To avoid it, ensureVisible() should not be called on a rectangle
    // higher than the view :

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
Panned::hidePositionPointer()
{
    m_pointerVisible = false;
    viewport()->update(); //!!! should update old pointer area only, really
}

void
Panned::wheelEvent(QWheelEvent *ev)
{
    emit wheelEventReceived(ev);

    if (m_wheelZoomPan)
        processWheelEvent(ev);
    else
        QGraphicsView::wheelEvent(ev);
}

void
Panned::slotEmulateWheelEvent(QWheelEvent *ev)
{
    if (m_wheelZoomPan)
        processWheelEvent(ev);
    else
        QGraphicsView::wheelEvent(ev);
}

void
Panned::processWheelEvent(QWheelEvent *e)
{
    //RG_DEBUG << "processWheelEvent()";
    //RG_DEBUG << "  delta(): " << e->delta();
    //RG_DEBUG << "  angleDelta(): " << e->angleDelta();
    //RG_DEBUG << "  pixelDelta(): " << e->pixelDelta();

    // See also RosegardenScrollView::wheelEvent().

    // We'll handle this.  Don't pass to parent.
    e->accept();

    QPoint angleDelta = e->angleDelta();

    // Ctrl+wheel to zoom
    if (e->modifiers() & Qt::CTRL) {
        // Wheel down
        if (angleDelta.y() > 0)
            emit zoomOut();
        else if (angleDelta.y() < 0)  // Wheel up
            emit zoomIn();

        return;
    }

    // Shift+wheel to scroll left/right.
    // If shift is pressed and we are scrolling vertically...
    if ((e->modifiers() & Qt::SHIFT)  &&  angleDelta.y() != 0) {
        // Transform the incoming vertical scroll event into a
        // horizontal scroll event.

        // Swap x/y
        QPoint pixelDelta2(e->pixelDelta().y(), e->pixelDelta().x());
        QPoint angleDelta2(angleDelta.y(), angleDelta.x());

        // Create a new event.
        // We remove the Qt::SHIFT modifier otherwise we end up
        // moving left/right a page at a time.
        QWheelEvent e2(
                e->pos(),  // pos
                e->globalPosF(),  // globalPos
                pixelDelta2,  // pixelDelta
                angleDelta2,  // angleDelta
                e->delta(),  // qt4Delta
                Qt::Horizontal,  // qt4Orientation
                e->buttons(),  // buttons
                e->modifiers() & ~Qt::SHIFT,  // modifiers
                e->phase(),  // phase
                e->source(),  // source
                e->inverted());  // inverted

        // Let baseclass handle as usual.
        QGraphicsView::wheelEvent(&e2);

        return;
    }

    // Let baseclass handle normal scrolling.
    QGraphicsView::wheelEvent(e);
}

void
Panned::leaveEvent(QEvent *)
{
    emit mouseLeaves();
}


}
