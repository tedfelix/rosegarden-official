/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2019 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AutoScroller]"

#include "AutoScroller.h"

#include "misc/Debug.h"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QWidget>

#include <algorithm>

#include <math.h>


namespace
{

    // We'll hit MaxScrollRate at this distance outside the viewport.
    // ??? HiDPI: This needs to be bigger for the HiDPI case.
    constexpr double maxDistance = 40;

    double distanceToScrollRate(int distance)
    {
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

}

namespace Rosegarden
{


AutoScroller::AutoScroller() :
    m_abstractScrollArea(nullptr),
    m_viewport(nullptr),
    m_vScrollRate(10),
    m_followMode(NO_FOLLOW)
{
    connect(&m_timer, &QTimer::timeout,
            this, &AutoScroller::slotOnTimer);
}

void
AutoScroller::press()
{
    if (!m_abstractScrollArea) {
        RG_WARNING << "press(): abstract scroll area not specified";
        return;
    }

    if (!m_viewport) {
        RG_WARNING << "press(): viewport not specified";
        return;
    }

    if (!m_timer.isActive())
        m_timer.start(30);  // msecs
}

void
AutoScroller::move(FollowMode i_followMode)
{
    m_followMode = i_followMode;
}

void
AutoScroller::release()
{
    if (m_timer.isActive())
        m_timer.stop();
}

void
AutoScroller::slotOnTimer()
{
    doAutoScroll();
}

void
AutoScroller::doAutoScroll()
{
    const QPoint mousePos = m_viewport->mapFromGlobal(QCursor::pos());

    if (m_followMode & FOLLOW_HORIZONTAL) {

        // The following auto scroll behavior is patterned after Chromium,
        // Eclipse, and the GIMP.  Auto scroll will only happen if the
        // mouse is outside the viewport.  The auto scroll rate is
        // proportional to how far outside the viewport the mouse is.
        // If the right edge is too close to the edge of the screen
        // (e.g. when maximized), the auto scroll area is moved inside
        // of the viewport.

        int scrollX = 0;

        // If the mouse is to the left of the viewport
        if (mousePos.x() < 0) {

            // Set the scroll rate based on how far outside we are.
            scrollX = lround(-distanceToScrollRate(-mousePos.x()));

        } else {

            // Check if the mouse is to the right of the viewport

            // Assume we can place the auto scroll area outside the window.
            int xOffset = 0;

            const int rightSideOfScreen =
                    QApplication::desktop()->availableGeometry(m_viewport).right();

            const int rightSideOfViewport =
                    m_viewport->parentWidget()->mapToGlobal(
                            m_viewport->geometry().bottomRight()).x();

            const int spaceToTheRight = rightSideOfScreen - rightSideOfViewport;

            // If there's not enough space for the auto scroll area, move it
            // inside the viewport.
            if (spaceToTheRight < maxDistance)
                xOffset = static_cast<int>(-maxDistance + spaceToTheRight);

            // Limit where auto scroll begins.
            const int xMax = m_viewport->width() + xOffset;

            // If the mouse is to the right of the auto scroll limit
            if (mousePos.x() > xMax) {
                // Set the scroll rate based on how far outside we are.
                scrollX = lround(distanceToScrollRate(mousePos.x() - xMax));
            }

        }

        // Scroll if needed.
        if (scrollX) {
            QScrollBar *hScrollBar = m_abstractScrollArea->horizontalScrollBar();
            hScrollBar->setValue(hScrollBar->value() + scrollX);
        }
    }

    if (m_followMode & FOLLOW_VERTICAL) {

        // This vertical auto scroll behavior is patterned after
        // Audacity.  Auto scroll will only happen if the mouse is
        // outside the viewport.  The auto scroll rate is fixed.

        int scrollY = 0;

        // If the mouse is above the viewport
        if (mousePos.y() < 0)
            scrollY = -m_vScrollRate;

        // If the mouse is below the viewport
        if (mousePos.y() > m_viewport->height())
            scrollY = +m_vScrollRate;

        // Scroll if needed.
        if (scrollY) {
            QScrollBar *vScrollBar = m_abstractScrollArea->verticalScrollBar();
            vScrollBar->setValue(vScrollBar->value() + scrollY);
        }

    }
}


}
