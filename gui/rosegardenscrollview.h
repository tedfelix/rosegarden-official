// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef ROSEGARDENSCROLLVIEW_H
#define ROSEGARDENSCROLLVIEW_H

#include <vector>

#include <qscrollview.h>
#include <qdatetime.h>
#include <qtimer.h>


class QScrollBar;
class QGridLayout;

/**
 * A QScrollView with an auxiliary horiz. scrollbar
 * That scrollbar should be provided by the parent widget
 * (typically an EditView). The RosegardenCanvasView keeps
 * the auxilliary horiz. scrollbar range in sync with the
 * one of its own scrollbar with slotUpdate().
 */
class RosegardenScrollView : public QScrollView
{
    Q_OBJECT
public:
    RosegardenScrollView(QWidget* parent=0, const char* name=0, WFlags f=0);

    /**
     * EditTool::handleMouseMove() returns a OR-ed combination of these
     * to indicate which direction to scroll to
     */
    enum {
        NoFollow = 0x0,
        FollowHorizontal = 0x1,
        FollowVertical = 0x2
    };

    /**
     * Sets the canvas width to be exactly the width needed to show
     * all the items
     */
//     void fitWidthToContents();

    /**
     * Sets the widget which will be between the scrollable part of the view
     * and the horizontal scrollbar
     */
    void setBottomFixedWidget(QWidget*);

    void updateBottomWidgetGeometry();

    /// Map a point with the inverse world matrix
//     QPoint inverseMapPoint(const QPoint& p) { return inverseWorldMatrix().map(p); }

    void setSmoothScroll(bool s) { m_smoothScroll = s; }

    bool isTimeForSmoothScroll();

    void setScrollDirectionConstraint(int d) { m_scrollDirectionConstraint = d; }

    int getDeltaScroll() { return m_minDeltaScroll; }

public slots:
    /// Update the RosegardenScrollView after a change of content
    virtual void slotUpdate();

    /**
     * Scroll horizontally to make the given position visible,
     * paging to as to get some visibility of the next screenful
     * (for playback etc)
     */
    void slotScrollHoriz(int hpos);

    /**
     * Scroll horizontally to make the given position somewhat
     * nearer to visible, scrolling by only "a small distance"
     * at a time
     */
    void slotScrollHorizSmallSteps(int hpos);

    /**
     * Scroll vertically to make the given position somewhat
     * nearer to visible, scrolling by only "a small distance"
     * at a time
     */
    void slotScrollVertSmallSteps(int vpos);

    /**
     * Scroll vertically so as to place the given position
     * somewhere near the top of the viewport.
     */
    void slotScrollVertToTop(int vpos);

    /**
     * Set the x and y scrollbars to a particular position
     */
    void slotSetScrollPos(const QPoint &);

    void startAutoScroll();
    void startAutoScroll(int directionConstraint);
    void stopAutoScroll();
    void doAutoScroll();

    bool isAutoScrolling() const { return m_autoScrolling; }

signals:
    void bottomWidgetHeightChanged(int);

protected:
    
    virtual void resizeEvent(QResizeEvent*);
    virtual void setHBarGeometry(QScrollBar &hbar, int x, int y, int w, int h);
    
    virtual QScrollBar* getMainHorizontalScrollBar() { return horizontalScrollBar(); }

    //--------------- Data members ---------------------------------
    enum ScrollDirection { None, Top, Bottom, Left, Right };

    QWidget* m_bottomWidget;
    int m_currentBottomWidgetHeight;

    bool m_smoothScroll;
    int m_smoothScrollTimeInterval;
    float m_minDeltaScroll;
    QTime m_scrollTimer;
    QTime m_scrollAccelerationTimer;

    QTimer m_autoScrollTimer;
    int m_autoScrollTime;
    int m_autoScrollAccel;
    QPoint m_autoScrollStartPoint;
    int m_autoScrollXMargin;
    int m_autoScrollYMargin;
    ScrollDirection m_currentScrollDirection;
    int m_scrollDirectionConstraint;
    bool m_autoScrolling;

    static const int DefaultSmoothScrollTimeInterval;
    static const int DefaultMinDeltaScroll;

    static const int AutoscrollMargin;
    static const int InitialScrollTime;
    static const int InitialScrollAccel;
    static const int MaxScrollDelta;
    static const double ScrollAccelValue;

};

#endif

