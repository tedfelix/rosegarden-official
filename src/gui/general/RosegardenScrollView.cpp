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

#define RG_MODULE_STRING "[RosegardenScrollView]"

#include "RosegardenScrollView.h"

#include "misc/Debug.h"
#include "gui/rulers/StandardRuler.h"

#include <QApplication>
#include <QCursor>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QRect>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>

#include <algorithm>  // std::swap(), std::min(), std::max()

#include <math.h>


namespace Rosegarden
{


const int RosegardenScrollView::AutoScrollTimerInterval = 30;  // msecs


RosegardenScrollView::RosegardenScrollView(QWidget *parent)
    : QAbstractScrollArea(parent),

      m_bottomRuler(nullptr),
      m_contentsWidth(0),
      m_contentsHeight(0),
      m_followMode(NoFollow),
      m_autoScrolling(false)
{
    // Turn off the frame which causes positioning issues.
    // The rest of the code assumes there is no frame.
    setFrameStyle(QFrame::NoFrame);

    connect(&m_autoScrollTimer, &QTimer::timeout,
            this, &RosegardenScrollView::slotOnAutoScrollTimer);
}

int RosegardenScrollView::contentsX()
{
    return horizontalScrollBar()->value();
}

int RosegardenScrollView::contentsY()
{
    return verticalScrollBar()->value();
}

void RosegardenScrollView::resizeContents(int w, int h)
{
    // Code lifted from Q3ScrollView::resizeContents().

    // Hold on to the old values.
    int ow = m_contentsWidth;
    int oh = m_contentsHeight;

    // We need to set these before we do the swaps, otherwise we may be
    // storing the wrong (post-swap) values.
    m_contentsWidth = w;
    m_contentsHeight = h;

    // This was necessary until I fixed the resizeEvent connection
    //d->scrollbar_timer.start(0, true);

    //### CJ - Don't think this is necessary - slightly confused as we're
    //         resizing the content, not the widget
    //if (d->children.isEmpty() && d->policy == Default)
    //    setResizePolicy(Manual);

    // Make sure "w" is the larger one.
    // ??? This would be easier to read using minWidth and maxWidth for the
    //     smaller and the larger.
    if (ow > w) {
        std::swap(w, ow);
    }

    // Refresh area ow..w (minWidth..maxWidth)
    if (ow < viewport()->width()  &&  w >= 0) {
        if (ow < 0)
            ow = 0;
        if (w > viewport()->width())
            w = viewport()->width();
        viewport()->update(contentsX()+ow, 0, w-ow, viewport()->height());
    }

    // Make sure "h" is the larger one.
    // ??? This would be easier to read using minHeight and maxHeight for
    //     the smaller and the larger.
    if (oh > h) {
        std::swap(h, oh);
    }

    // Refresh area oh..h (minHeight..maxHeight)
    if (oh < viewport()->height()  &&  h >= 0) {
        if (oh < 0)
            oh = 0;
        if (h > viewport()->height())
            h = viewport()->height();
        viewport()->update(0, contentsY()+oh, viewport()->width(), h-oh);
    }

    // Since the contents size has changed, make sure the
    // scrollbars are updated.
    updateScrollBars();
}

void RosegardenScrollView::updateContents(int x, int y, int w, int h)
{
    // Code lifted from Q3ScrollView::updateContents().

    if (!isVisible() || !updatesEnabled())
        return;

    // Translate contents coords to viewport coords.
    x -= contentsX();
    y -= contentsY();

    // Cut off any portion left of the left edge.
    if (x < 0) {
        w += x;
        x = 0;
    }
    // Cut off any portion above the top edge.
    if (y < 0) {
        h += y;
        y = 0;
    }

    if (w < 0 || h < 0)
        return;

    // If x or y are beyond the viewport, bail.
    if (x > viewport()->width()  ||  y > viewport()->height())
        return;

    // No need to update more than can be seen.
    if (w > viewport()->width())
        w = viewport()->width();
    if (h > viewport()->height())
        h = viewport()->height();

    //### CJ - I don't think we used a clipped_viewport on Q3ScrollView
    //if (d->clipped_viewport) {
    //    // Translate clipper() to viewport()
    //    x -= d->clipped_viewport->x();
    //    y -= d->clipped_viewport->y();
    //}

    viewport()->update(x, y, w, h);
}

void RosegardenScrollView::updateContents(const QRect &r)
{
    updateContents(r.x(), r.y(), r.width(), r.height());
}

void RosegardenScrollView::updateContents()
{
    viewport()->update();
}

void RosegardenScrollView::updateScrollBars()
{
    horizontalScrollBar()->setMaximum(
        std::max(m_contentsWidth - viewport()->width(), 0));
    horizontalScrollBar()->setPageStep(viewport()->width());
    horizontalScrollBar()->setSingleStep(viewport()->width() / 10);

    verticalScrollBar()->setMaximum(
        std::max(m_contentsHeight - viewport()->height(), 0));
    verticalScrollBar()->setPageStep(viewport()->height());
    // Note: The vertical scrollbar's single step is set to the track
    //       height in TrackEditor::init().
}

QPoint RosegardenScrollView::viewportToContents(const QPoint &vp)
{
    return QPoint(vp.x() + contentsX(),
                  vp.y() + contentsY());
}

void RosegardenScrollView::setBottomRuler(StandardRuler *ruler)
{
    m_bottomRuler = ruler;
    if (m_bottomRuler) {
        m_bottomRuler->setParent(this);
        m_bottomRuler->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        // ??? Why do we have to add 1 to get enough room?
        //     - Are the viewport's limits being ignored?  Is someone
        //       overdrawing the bottom by 1?
        //     - Is the bottom ruler expanding by 1?  No.  The hint is 25,
        //       we size it to 25, and it stays at 25.
        //     - Inclusive vs. exclusive math?  Don't think so.
        setViewportMargins( 0, 0, 0, m_bottomRuler->sizeHint().height() + 1 );
    }
}

void RosegardenScrollView::startAutoScroll()
{
    if (!m_autoScrollTimer.isActive()) {
        m_autoScrollTimer.start(AutoScrollTimerInterval);
    }

    m_autoScrolling = true;
}

void RosegardenScrollView::slotStartAutoScroll(int followMode)
{
    setFollowMode(followMode);
    startAutoScroll();
}

void RosegardenScrollView::slotStopAutoScroll()
{
    m_autoScrollTimer.stop();
    m_autoScrolling = false;
}

double RosegardenScrollView::distanceToScrollRate(int distance)
{
    // We'll hit MaxScrollRate at this distance outside the viewport.
    const double maxDistance = 40;
    const double distanceNormalized = distance / maxDistance;
    // Apply a curve to reduce the touchiness.
    // Simple square curve.  Something more pronounced might be better.
    const double distanceWithCurve = distanceNormalized * distanceNormalized;

    const double minScrollRate = 1.2;
    const double maxScrollRate = 100;
    const double scrollRateRange = (maxScrollRate - minScrollRate);

    const double scrollRate = distanceWithCurve * scrollRateRange + minScrollRate;

    return std::min(scrollRate, maxScrollRate);
}

void RosegardenScrollView::doAutoScroll()
{
    const QPoint mousePos = viewport()->mapFromGlobal(QCursor::pos());

    m_autoScrollTimer.start(AutoScrollTimerInterval);

    if (m_followMode & FollowHorizontal) {

        // The following auto scroll behavior is patterned after Chromium,
        // Eclipse, and the GIMP.  Auto scroll will only happen if the
        // mouse is outside the viewport.  The auto scroll rate is
        // proportional to how far outside the viewport the mouse is.

        int scrollX = 0;

        // If the mouse is to the left of the viewport
        if (mousePos.x() < 0) {
            // Set the scroll rate based on how far outside we are.
            scrollX = lround(-distanceToScrollRate(-mousePos.x()));
        }

        // If the mouse is to the right of the viewport
        if (mousePos.x() > viewport()->width()) {
            // Set the scroll rate based on how far outside we are.
            scrollX = lround(distanceToScrollRate(mousePos.x() - viewport()->width()));
        }

        // Scroll if needed.
        if (scrollX)
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + scrollX);
    }

    if (m_followMode & FollowVertical) {

        // This vertical auto scroll behavior is patterned after
        // Audacity.  Auto scroll will only happen if the mouse is
        // outside the viewport.  The auto scroll rate is fixed.

        int scrollY = 0;

        // If the mouse is above the viewport
        if (mousePos.y() < 0) {
            scrollY = -5;
        }

        // If the mouse is below the viewport
        if (mousePos.y() > viewport()->height()) {
            scrollY = +5;
        }

        // Scroll if needed.
        if (scrollY)
            verticalScrollBar()->setValue(verticalScrollBar()->value() + scrollY);
    }
}

void RosegardenScrollView::slotOnAutoScrollTimer()
{
    doAutoScroll();
}

void RosegardenScrollView::scrollHoriz(int x)
{
    QScrollBar *hbar = horizontalScrollBar();

    const int contentsX = hbar->value();

    if (x == 0) {

        hbar->setValue(0);

    } else if (x > (contentsX + viewport()->width() * 1.6)  ||
               x < (contentsX - viewport()->width() * 0.7)) {
        // The requested x is relatively far away.

        // Put it slightly to the left of center.
        hbar->setValue(x - int(viewport()->width() * 0.4));

    } else if (x > (contentsX + viewport()->width() * 0.9)) {
        // The requested x is slightly off to the right.

        // Go right a little more than half of a page.
        hbar->setValue(contentsX + int(viewport()->width() * 0.6));

    } else if (x < (contentsX + viewport()->width() * 0.1)) {
        // The requested x is slightly off to the left.

        // Go left a little more than half of a page.
        hbar->setValue(contentsX - int(viewport()->width() * 0.6));
    }
}

void RosegardenScrollView::scrollVert(int y)
{
    // Ignore any request made before we've actually been rendered and sized.
    if (viewport()->height() <= 1)
        return;

    QScrollBar *vbar = verticalScrollBar();

    // One "line" (track) from the bottom.
    int bottomMargin = contentsY() + viewport()->height() - vbar->singleStep();

    // If the requested y is below the bottom margin.
    if (y > bottomMargin) {
        int scrollY = y - bottomMargin;

        // Scroll to make sure the requested y is visible.
        vbar->setValue(vbar->value() + scrollY);

        return;
    }

    // If the requested y is above the top.
    if (y < contentsY()) {
        int scrollY = y - contentsY();

        // Scroll to make sure the requested y is at the top.
        vbar->setValue(vbar->value() + scrollY);

        return;
    }
}

void RosegardenScrollView::resizeEvent(QResizeEvent *e)
{
    QAbstractScrollArea::resizeEvent(e);

    // Since the viewport size has changed, we need to update
    // the scrollbars.
    updateScrollBars();

    // Make sure the bottom ruler is where it needs to be.
    updateBottomRulerGeometry();

    // Let TrackEditor know so it can resize the TrackButtons to match.
    emit viewportResize();
}

void RosegardenScrollView::updateBottomRulerGeometry()
{
    if (!m_bottomRuler)
        return;

    int bottomRulerHeight = m_bottomRuler->sizeHint().height();
    // Since there's no margin (see the call to setFrameStyle() in
    // the ctor), we can assume the viewport coords match up with
    // the parent coords.  No need to transform.
    QRect viewportRect = viewport()->rect();

    // Move the bottom ruler to below the viewport.
    m_bottomRuler->setGeometry(
            viewportRect.left(),
            viewportRect.bottom() + 1,  // +1 to be just under
            viewportRect.width(),
            bottomRulerHeight);  // See the call to setViewportMargins().
}

void RosegardenScrollView::wheelEvent(QWheelEvent *e)
{
    // We'll handle this.  Don't pass to parent.
    e->accept();

    // Ctrl+wheel to zoom
    if (e->modifiers() & Qt::CTRL) {
        if (e->delta() > 0)
            emit zoomIn();
        else if (e->delta() < 0)
            emit zoomOut();

        return;
    }

    // Shift+wheel to scroll left/right.
    if (e->modifiers() == Qt::SHIFT)
        QApplication::sendEvent(horizontalScrollBar(), e);
    else  // Scroll up/down
        QApplication::sendEvent(verticalScrollBar(), e);
}


}
