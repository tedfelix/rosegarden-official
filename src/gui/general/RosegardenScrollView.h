/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENSCROLLVIEW_H
#define RG_ROSEGARDENSCROLLVIEW_H

#include <QPoint>
#include <QAbstractScrollArea>
#include <QDateTime>
#include <QTimer>


class QWidget;
class QWheelEvent;
class QScrollBar;
class QResizeEvent;
class QPaintEvent;
class QMouseEvent;


namespace Rosegarden
{


/// QAbstractScrollArea with auto-scroll and bottom ruler.
/**
 * A QAbstractScrollArea with more elaborate auto-scrolling capabilities
 * and the ability to have a vertically "fixed" widget (ruler) at its
 * bottom, just above the horizontal scrollbar.
 *
 * CompositionView derives from this class.
 */
class RosegardenScrollView : public QAbstractScrollArea
{
    Q_OBJECT

public:

    RosegardenScrollView(QWidget *parent);

    // Q3ScrollView functions that were missing from QAbstractScrollArea

    int contentsX();
    int contentsY();
    void setContentsPos(int x, int y);
    int visibleWidth();
    int visibleHeight();
    int contentsWidth();
    int contentsHeight();

    void resizeContents(int width, int height);
    void updateContents(int x, int y, int width, int height);
    void updateContents(const QRect &);
    void updateContents();

    //QPoint viewportToContents(QPoint &);

    // From Q3ScrollView
    //void setDragAutoScroll(bool);

    /// Connect the bottom ruler.
    /**
     * Sets the ruler widget which will be between the scrollable part of
     * the view and the horizontal scrollbar.
     */
    void setBottomFixedWidget(QWidget *);

    /// Map a point with the inverse world matrix
    //QPoint inverseMapPoint(const QPoint& p) { return inverseWorldMatrix().map(p); }

    //void setSmoothScroll(bool s)  { m_smoothScroll = s; }
    bool isTimeForSmoothScroll();

    /// Follow Mode
    /**
     * Derivers from SegmentTool override SegmentTool::handleMouseMove() and
     * return an OR-ed combination of these to indicate the auto-scroll
     * direction.
     *
     * See MatrixTool::FollowMode, NotationTool::FollowMode, and
     * ControlTool::FollowMode.
     *
     * Would this make more sense in SegmentTool?
     */
    enum FollowMode {
        NoFollow = 0x0,
        FollowHorizontal = 0x1,
        FollowVertical = 0x2
    };

    /// See enum FollowMode.
    void setFollowMode(int followMode)  { m_followMode = followMode; }

    //int getDeltaScroll() const  { return m_minDeltaScroll; }

public slots:
    /**
     * Scroll horizontally to make the given position visible,
     * paging so as to get some visibility of the next screenful
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
    /// See enum FollowMode for valid mask values.
    void startAutoScroll(int followMode);
    void stopAutoScroll();
    /// Handler for m_autoScrollTimer.
    /**
     * Also called by TrackEditor::handleAutoScroll().
     */
    void doAutoScroll();

    bool isAutoScrolling() const  { return m_autoScrolling; }

    void updateScrollBars();

signals:
    //void bottomWidgetHeightChanged(int newHeight);

    void zoomIn();
    void zoomOut();

protected:

    virtual void resizeEvent(QResizeEvent *);

    virtual void paintEvent(QPaintEvent *);

    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);

    virtual void wheelEvent(QWheelEvent *);

    // Q3ScrollView compatibility functions.

    virtual void viewportPaintEvent(QPaintEvent *);

    virtual void contentsMousePressEvent(QMouseEvent *);
    virtual void contentsMouseReleaseEvent(QMouseEvent *);
    virtual void contentsMouseMoveEvent(QMouseEvent *);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent *);

private:

    // ??? These are called exactly once.  They can be inlined into
    //     their callers.
    void viewportMousePressEvent(QMouseEvent *);
    void viewportMouseReleaseEvent(QMouseEvent *);
    void viewportMouseMoveEvent(QMouseEvent *);
    void viewportMouseDoubleClickEvent(QMouseEvent *);

    QPoint viewportToContents(const QPoint &);

    //void setHBarGeometry(QScrollBar &hbar, int x, int y, int w, int h);
    QScrollBar *getMainHorizontalScrollBar()  { return horizontalScrollBar(); }

    QWidget *m_bottomWidget;
    int m_currentBottomWidgetHeight;
    void updateBottomWidgetGeometry();

    bool m_smoothScroll;  // always true
    int m_smoothScrollTimeInterval;
    float m_minDeltaScroll;
    QTime m_scrollTimer;
    QTime m_scrollShortcuterationTimer;

    /// Calls doAutoScroll().
    QTimer m_autoScrollTimer;
    int m_autoScrollTime;
    int m_autoScrollShortcut;
    QPoint m_previousP;
    int m_autoScrollXMargin;
    int m_autoScrollYMargin;
    enum ScrollDirection { None, Top, Bottom, Left, Right };
    ScrollDirection m_currentScrollDirection;
    /// See enum FollowMode for valid mask values.
    int m_followMode;
    bool m_autoScrolling;    

    static const int DefaultSmoothScrollTimeInterval;
    static const double DefaultMinDeltaScroll;

    //static const int AutoscrollMargin;
    static const int InitialScrollTime;
    static const int InitialScrollShortcut;
    static const int MaxScrollDelta;
    static const double ScrollShortcutValue;

    int m_vwidth;
    int m_vheight;

};


}

#endif
