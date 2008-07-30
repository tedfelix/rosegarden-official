
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

#ifndef _RG_ROSEGARDENCANVASVIEW_H_
#define _RG_ROSEGARDENCANVASVIEW_H_

#include <qpoint.h>
#include <qtimer.h>
#include <qcanvas.h>
#include <qdatetime.h>
#include <qwmatrix.h>

class QWidget;
class QWheelEvent;
class QScrollBar;
class QResizeEvent;


namespace Rosegarden
{

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

    /**
     * Sets the widget which will be between the scrollable part of the view
     * and the left edge of the view.
     */
    void setLeftFixedWidget(QWidget*);

    void updateLeftWidgetGeometry();

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

    QWidget* m_leftWidget;

    bool m_smoothScroll;
    int m_smoothScrollTimeInterval;
    float m_minDeltaScroll;
    QTime m_scrollTimer;
    QTime m_scrollAccelerationTimer;

    QTimer m_autoScrollTimer;
    int m_autoScrollTime;
    int m_autoScrollAccel;
    QPoint m_previousP;
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


}

#endif
