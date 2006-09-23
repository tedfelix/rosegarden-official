// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef ROSEGARDENCANVASVIEW_H
#define ROSEGARDENCANVASVIEW_H

#include <vector>

#include <qcanvas.h>
#include <qwmatrix.h>
#include <qdatetime.h>
#include <qtimer.h>

class QScrollBar;
class QGridLayout;

/**
 * A QCanvasView with an auxiliary horiz. scrollbar
 * That scrollbar should be provided by the parent widget
 * (typically an EditView). The RosegardenCanvasView keeps
 * the auxilliary horiz. scrollbar range in sync with the
 * one of its own scrollbar with slotUpdate().
 */
class RosegardenCanvasView : public QCanvasView
{
    Q_OBJECT
public:
    RosegardenCanvasView(QCanvas*,
                         QWidget* parent=0, const char* name=0, WFlags f=0);

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
    void fitWidthToContents();

    /**
     * Sets the widget which will be between the scrollable part of the view
     * and the horizontal scrollbar
     */
    void setBottomFixedWidget(QWidget*);

    void updateBottomWidgetGeometry();

    /// Map a point with the inverse world matrix
    QPoint inverseMapPoint(const QPoint& p) { return inverseWorldMatrix().map(p); }

    void setSmoothScroll(bool s) { m_smoothScroll = s; }

    bool isTimeForSmoothScroll();

    void setScrollDirectionConstraint(int d) { m_scrollDirectionConstraint = d; }

    bool isAutoScrolling() const { return m_autoScrolling; }

    virtual void wheelEvent(QWheelEvent *);

public slots:
    /// Update the RosegardenCanvasView after a change of content
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

signals:
    void bottomWidgetHeightChanged(int);

    void zoomIn();
    void zoomOut();

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

    static const int    DefaultSmoothScrollTimeInterval;
    static const double DefaultMinDeltaScroll;

    static const int AutoscrollMargin;
    static const int InitialScrollTime;
    static const int InitialScrollAccel;
    static const int MaxScrollDelta;
    static const double ScrollAccelValue;

};

class CanvasCursor : public QCanvasRectangle
{
public:
    CanvasCursor(QCanvas*, int width);
    void updateHeight();
//     virtual QRect boundingRect() const;
protected:
    int m_width;
};



/**
 * A pseudo GC in which CanvasItems whose ownership isn't clear cut
 * can be put for periodical removal.
 *
 * This is especially for SegmentItems which can put their repeat
 * rectangles when they're being deleted.
 *
 * The problem this solves is a classic ownership/double deletion
 * case. The SegmentCanvas deletes all its items on destruction. But
 * the SegmentItems have an auxiliary "repeat rectangle" which is a
 * QCanvasRectangle, that needs to be deleted when the SegmentItem is
 * itself deleted.
 *
 * However, if the SegmentItem deletes its repeat rectangle, then when
 * the SegmentCanvas destruction occurs, the SegmentCanvas dtor will
 * get a list of all its children (QCanvas::allItems()), containing
 * both SegmentItems and their repeat rectangles. Deleting a
 * SegmentItem will delete its repeat rectangle, which will still be
 * present in the all children list which the SegmentCanvas dtor is
 * iterating over.
 * 
 * So a solution is simply to push to-be-deleted repeat rectangles on
 * this GC, which should be processed on canvas updates, for instance.
 *
 */
class CanvasItemGC
{
public:
    /// mark the given item for GC
    static void mark(QCanvasItem*);

    /// GC all marked items
    static void gc();

    /// Forget all marked items - don't delete them
    static void flush();

protected:
    static std::vector<QCanvasItem*> m_garbage;
};


#endif

