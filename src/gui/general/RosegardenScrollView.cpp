/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RosegardenScrollView.h"

#include "misc/Debug.h"
#include <QApplication>
#include <QCursor>
#include <QPoint>
#include <QRect>
#include <QScrollBar>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>


namespace Rosegarden
{

// Smooth scroll checks
//

const int RosegardenScrollView::AutoscrollMargin = 16;
const int RosegardenScrollView::InitialScrollTime = 30;
const int RosegardenScrollView::InitialScrollShortcut = 5;
const int RosegardenScrollView::MaxScrollDelta = 100;      // max a.scroll speed
const double RosegardenScrollView::ScrollShortcutValue = 1.04;// shortcuteration rate

RosegardenScrollView::RosegardenScrollView(QWidget* parent,
					const char* name) //, WFlags f)
				: QScrollArea(parent, name), //, f),
        m_bottomWidget(0),
        m_currentBottomWidgetHeight( -1),
        m_smoothScroll(true),
        m_smoothScrollTimeInterval(DefaultSmoothScrollTimeInterval),
        m_minDeltaScroll(DefaultMinDeltaScroll),
        m_autoScrollTime(InitialScrollTime),
        m_autoScrollShortcut(InitialScrollShortcut),
        m_autoScrollXMargin(0),
        m_autoScrollYMargin(0),
        m_currentScrollDirection(None),
        m_scrollDirectionConstraint(NoFollow),
        m_autoScrolling(false)
{
    setDragAutoScroll(true);
    connect( &m_autoScrollTimer, SIGNAL( timeout() ),
             this, SLOT( doAutoScroll() ) );
}

void RosegardenScrollView::setBottomFixedWidget(QWidget* w)
{
    m_bottomWidget = w;
    if (m_bottomWidget) {
        m_bottomWidget->reparent(this, 0, QPoint(0, 0));
        m_bottomWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        setMargins(0, 0, 0, m_bottomWidget->sizeHint().height());
    }
}

void RosegardenScrollView::startAutoScroll()
{
    //     RG_DEBUG << "RosegardenScrollView::startAutoScroll()\n";

    if ( !m_autoScrollTimer.isActive() ) {
        m_autoScrollTime = InitialScrollTime;
        m_autoScrollShortcut = InitialScrollShortcut;
        m_autoScrollTimer.start( m_autoScrollTime );
    }

    QPoint autoScrollStartPoint = viewport()->mapFromGlobal( QCursor::pos() );
    m_autoScrollYMargin = autoScrollStartPoint.y() / 10;
    m_autoScrollXMargin = autoScrollStartPoint.x() / 10;

    m_autoScrolling = true;
}

void RosegardenScrollView::startAutoScroll(int directionConstraint)
{
    setScrollDirectionConstraint(directionConstraint);
    startAutoScroll();
}

void RosegardenScrollView::stopAutoScroll()
{
    //     RG_DEBUG << "RosegardenScrollView::stopAutoScroll()\n";

    m_autoScrollTimer.stop();
    m_minDeltaScroll = DefaultMinDeltaScroll;
    m_currentScrollDirection = None;

    m_autoScrolling = false;
}

void RosegardenScrollView::doAutoScroll()
{
    //     RG_DEBUG << "RosegardenScrollView::doAutoScroll()\n";

    QPoint p = viewport()->mapFromGlobal( QCursor::pos() );
    QPoint dp = p - m_previousP;
    m_previousP = p;

    m_autoScrollTimer.start( m_autoScrollTime );
    ScrollDirection scrollDirection = None;

    int dx = 0, dy = 0;
    if (m_scrollDirectionConstraint & FollowVertical) {
        if ( p.y() < m_autoScrollYMargin ) {
            dy = -(int(m_minDeltaScroll));
            scrollDirection = Top;
        } else if ( p.y() > visibleHeight() - m_autoScrollYMargin ) {
            dy = + (int(m_minDeltaScroll));
            scrollDirection = Bottom;
        }
    }
    bool startDecelerating = false;
    if (m_scrollDirectionConstraint & FollowHorizontal) {

        //        RG_DEBUG << "p.x() : " << p.x() << " - visibleWidth : " << visibleWidth() << " - autoScrollXMargin : " << m_autoScrollXMargin << endl;

        if ( p.x() < m_autoScrollXMargin ) {
            if ( dp.x() > 0 ) {
                startDecelerating = true;
                m_minDeltaScroll /= ScrollShortcutValue;
            }
            dx = -(int(m_minDeltaScroll));
            scrollDirection = Left;
        } else if ( p.x() > visibleWidth() - m_autoScrollXMargin ) {
            if ( dp.x() < 0 ) {
                startDecelerating = true;
                m_minDeltaScroll /= ScrollShortcutValue;
            }
            dx = + (int(m_minDeltaScroll));
            scrollDirection = Right;
        }
    }

    //     RG_DEBUG << "dx: " << dx << ", dy: " << dy << endl;

    if ( (dx || dy) &&
            ((scrollDirection == m_currentScrollDirection) || (m_currentScrollDirection == None)) ) {
        scrollBy(dx, dy);
        if ( startDecelerating )
            m_minDeltaScroll /= ScrollShortcutValue;
        else
            m_minDeltaScroll *= ScrollShortcutValue;
        if (m_minDeltaScroll > MaxScrollDelta )
            m_minDeltaScroll = MaxScrollDelta;
        m_currentScrollDirection = scrollDirection;

    } else {
        // Don't automatically stopAutoScroll() here, the mouse button
        // is presumably still pressed.
        m_minDeltaScroll = DefaultMinDeltaScroll;
        m_currentScrollDirection = None;
    }

}

const int RosegardenScrollView::DefaultSmoothScrollTimeInterval = 10;
const double RosegardenScrollView::DefaultMinDeltaScroll = 1.2;

bool RosegardenScrollView::isTimeForSmoothScroll()
{
    static int desktopWidth = QApplication::desktop()->width(),
                              desktopHeight = QApplication::desktop()->height();

    if (m_smoothScroll) {
        int ta = m_scrollShortcuterationTimer.elapsed();
        int t = m_scrollTimer.elapsed();

        RG_DEBUG << "t = " << t << ", ta = " << ta << ", int " << m_smoothScrollTimeInterval << ", delta " << m_minDeltaScroll << endl;

        if (t < m_smoothScrollTimeInterval) {

            return false;

        } else {

            if (ta > 300) {
                // reset smoothScrollTimeInterval
                m_smoothScrollTimeInterval = DefaultSmoothScrollTimeInterval;
                m_minDeltaScroll = DefaultMinDeltaScroll;
                m_scrollShortcuterationTimer.restart();
            } else if (ta > 50) {
                //                 m_smoothScrollTimeInterval /= 2;
                m_minDeltaScroll *= 1.08;
                m_scrollShortcuterationTimer.restart();
            }

            m_scrollTimer.restart();
            return true;
        }
    }

    return true;
}

void RosegardenScrollView::slotScrollHoriz(int hpos)
{
    QScrollBar* hbar = getMainHorizontalScrollBar();
    int currentContentYPos = contentsY();

    /* Lots of performance hitting debug
    RG_DEBUG << "RosegardenCanvasView::slotScrollHoriz: hpos is " << hpos
      << ", contentsX is " << contentsX() << ", visibleWidth is "
      << visibleWidth() << endl;
             */

    if (hpos == 0) {

        // returning to zero
        //         hbar->setValue(0);
        setContentsPos(0, currentContentYPos);

    } else if (hpos > (contentsX() +
                       visibleWidth() * 1.6) ||
               hpos < (contentsX() -
                       visibleWidth() * 0.7)) {

        // miles off one side or the other
        // 	hbar->setValue(hpos - int(visibleWidth() * 0.4));
        setContentsPos(hpos - int(visibleWidth() * 0.4), currentContentYPos);

    } else if (hpos > (contentsX() +
                       visibleWidth() * 0.9)) {

        // moving off the right hand side of the view
        // 	hbar->setValue(hbar->value() + int(visibleWidth() * 0.6));
        setContentsPos(hbar->value() + int(visibleWidth() * 0.6), currentContentYPos);

    } else if (hpos < (contentsX() +
                       visibleWidth() * 0.1)) {

        // moving off the left
        // 	hbar->setValue(hbar->value() - int(visibleWidth() * 0.6));
        setContentsPos(hbar->value() - int(visibleWidth() * 0.6), currentContentYPos);
    }
}

void RosegardenScrollView::slotScrollHorizSmallSteps(int hpos)
{
    QScrollBar* hbar = getMainHorizontalScrollBar();
    int currentContentYPos = contentsY();

    int diff = 0;

    if (hpos == 0) {

        // returning to zero
        //         hbar->setValue(0);
        setContentsPos(0, currentContentYPos);

    } else if ((diff = int(hpos - (contentsX() +
                                   visibleWidth() * 0.90))) > 0) {

        // moving off the right hand side of the view

        int delta = diff / 6;
        int diff10 = std::min(diff, (int)m_minDeltaScroll);
        delta = std::max(delta, diff10);

        // 	hbar->setValue(hbar->value() + delta);
        setContentsPos(hbar->value() + delta, currentContentYPos);

    } else if ((diff = int(hpos - (contentsX() +
                                   visibleWidth() * 0.10))) < 0) {
        // moving off the left

        int delta = -diff / 6;
        int diff10 = std::min( -diff, (int)m_minDeltaScroll);
        delta = std::max(delta, diff10);

        // 	hbar->setValue(hbar->value() - delta);
        setContentsPos(hbar->value() - delta, currentContentYPos);

    }
}

void RosegardenScrollView::slotScrollVertSmallSteps(int vpos)
{
    QScrollBar* vbar = verticalScrollBar();

    //    RG_DEBUG << "RosegardenCanvasView::slotScrollVertSmallSteps: vpos is " << vpos << ", contentsY is " << contentsY() << ", visibleHeight is " << visibleHeight() << endl;

    // As a special case (or hack), ignore any request made before we've
    // actually been rendered and sized
    if (visibleHeight() <= 1)
        return ;

    int diff = 0;

    if (vpos == 0) {

        // returning to zero
        vbar->setValue(0);

    } else if ((diff = int(vpos - (contentsY() +
                                   visibleHeight() * 0.90))) > 0) {

        // moving off up

        int delta = diff / 6;
        int diff10 = std::min(diff, (int)m_minDeltaScroll);
        delta = std::max(delta, diff10);

        vbar->setValue(vbar->value() + diff);

    } else if ((diff = int(vpos - (contentsY() +
                                   visibleHeight() * 0.10))) < 0) {

        // moving off down

        int delta = -diff / 6;
        int diff10 = std::min( -diff, (int)m_minDeltaScroll);
        delta = std::max(delta, diff10);

        vbar->setValue(vbar->value() - delta);

    }
}

void RosegardenScrollView::slotScrollVertToTop(int vpos)
{
    QScrollBar* vbar = verticalScrollBar();
    if (vpos < visibleHeight() / 3)
        vbar->setValue(0);
    else
        vbar->setValue(vpos - visibleHeight() / 5);
}

void RosegardenScrollView::slotSetScrollPos(const QPoint &pos)
{
    horizontalScrollBar()->setValue(pos.x());
    verticalScrollBar()->setValue(pos.y());
}

void RosegardenScrollView::resizeEvent(QResizeEvent* e)
{
    QScrollView::resizeEvent(e);
    if (!horizontalScrollBar()->isVisible())
        updateBottomWidgetGeometry();

}

void RosegardenScrollView::setHBarGeometry(QScrollBar &hbar, int x, int y, int w, int h)
{
    QScrollView::setHBarGeometry(hbar, x, y, w, h);
    updateBottomWidgetGeometry();
}

void RosegardenScrollView::updateBottomWidgetGeometry()
{
    if (!m_bottomWidget)
        return ;

    int bottomWidgetHeight = m_bottomWidget->sizeHint().height();

    setMargins(0, 0, 0, bottomWidgetHeight);
    QRect r = frameRect();
    int hScrollBarHeight = 0;
    if (horizontalScrollBar()->isVisible())
        hScrollBarHeight = horizontalScrollBar()->height() + 2; // + 2 offset needed to preserve border shadow

    int vScrollBarWidth = 0;
    if (verticalScrollBar()->isVisible())
        vScrollBarWidth = verticalScrollBar()->width();

    m_bottomWidget->setGeometry(r.x(),
                                r.y() + r.height() - bottomWidgetHeight - hScrollBarHeight,
                                r.width() - vScrollBarWidth,
                                bottomWidgetHeight);

    if (bottomWidgetHeight != m_currentBottomWidgetHeight) {
        emit bottomWidgetHeightChanged(bottomWidgetHeight);
        m_currentBottomWidgetHeight = bottomWidgetHeight;
    }

}

void RosegardenScrollView::wheelEvent(QWheelEvent *e)
{
    if (e->state() & ControlButton) {
        if (e->delta() > 0)
            emit zoomIn();
        else if (e->delta() < 0)
            emit zoomOut();
        return ;
    }
    QScrollView::wheelEvent(e);
}

}
#include "RosegardenScrollView.moc"
