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

#include <QAbstractScrollArea>
#include <QDateTime>
#include <QPoint>
#include <QTimer>


class QMouseEvent;
class QPaintEvent;
class QRect;
class QResizeEvent;
class QScrollBar;
class QWheelEvent;
class QWidget;


namespace Rosegarden
{


class StandardRuler;

/// QAbstractScrollArea with auto-scroll and bottom ruler.
/**
 * A QAbstractScrollArea with more elaborate auto-scrolling capabilities
 * and the ability to have a vertically "fixed" StandardRuler at its
 * bottom, just above the horizontal scrollbar.
 *
 * Some Q3ScrollView compatibility is provided to ease the transition
 * from Q3ScrollView to QAbstractScrollArea.
 *
 * CompositionView derives from this class.
 */
class RosegardenScrollView : public QAbstractScrollArea
{
    Q_OBJECT

public:

    RosegardenScrollView(QWidget *parent);

    /// Connect the bottom StandardRuler.
    /**
     * Sets the ruler widget which will be between the scrollable part of
     * the view and the horizontal scrollbar.
     *
     * This is called by TrackEditor::init() to connect a StandardRuler
     * instance.
     */
    void setBottomRuler(StandardRuler *);

    /// X coordinate of the contents that are at the left edge of the viewport.
    int contentsX();
    /// Y coordinate of the contents that are at the top edge of the viewport.
    int contentsY();

    void updateContents();

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

    /**
     * Called by TrackEditor::handleAutoScroll().
     */
    void doAutoScroll();
    bool isAutoScrolling() const  { return m_autoScrolling; }

    /// Playback, pointer drag, and loop drag scrolling.
    /**
     * Scroll horizontally to make the given position visible,
     * paging so as to get some visibility of the next screenful
     * (for playback etc)
     */
    void scrollHoriz(int hpos);

    /// Mouse move, pointer drag, and loop drag scrolling.
    /**
     * Scroll horizontally to make the given position somewhat
     * nearer to visible, scrolling by only "a small distance"
     * at a time
     */
    void scrollHorizSmallSteps(int hpos);

    /// Mouse move and track select scrolling.
    /**
     * Scroll vertically to make the given position somewhat
     * nearer to visible, scrolling by only "a small distance"
     * at a time
     */
    void scrollVertSmallSteps(int vpos);

public slots:
    /// Handle top and bottom StandardRuler::startMouseMove() signals.
    /**
     * See enum FollowMode for valid mask values.
     */
    void slotStartAutoScroll(int followMode);
    /// Handle top and bottom StandardRuler::stopMouseMove() signals.
    void slotStopAutoScroll();

signals:
    /// Used by TrackEditor to keep TrackButtons the right size.
    void viewportResize();

    /// Emitted on Ctrl-Scroll Wheel Up.
    /**
     * TrackEditor connects this to RosegardenMainWindow::slotZoomIn().
     */
    void zoomIn();
    /// Emitted on Ctrl-Scroll Wheel Down.
    /**
     * TrackEditor connects this to RosegardenMainWindow::slotZoomOut().
     */
    void zoomOut();

protected:

    /// Sets the size of the contents area and updates the viewport accordingly.
    /**
     * Q3ScrollView compatible function.
     */
    void resizeContents(int width, int height);
    /// Width of the contents area.
    int contentsWidth()  { return m_contentsWidth; }
    /// Height of the contents area.
    int contentsHeight()  { return m_contentsHeight; }

    void updateContents(const QRect &);

    QPoint viewportToContents(const QPoint &);

    /// See enum FollowMode.
    void setFollowMode(int followMode)  { m_followMode = followMode; }
    void startAutoScroll();

    /// Viewport resize.
    virtual void resizeEvent(QResizeEvent *);

    virtual void wheelEvent(QWheelEvent *);

private slots:

    /// Handler for m_autoScrollTimer.
    void slotOnAutoScrollTimer();

private:

    StandardRuler *m_bottomRuler;
    /// Make sure the bottom ruler stays in the proper place.
    void updateBottomRulerGeometry();

    int m_contentsWidth;
    int m_contentsHeight;

    /// Calls update() on a rectangle defined by x, y, w, h, translated appropriately.
    /**
     * Q3ScrollView compatible function.
     */
    void updateContents(int x, int y, int width, int height);

    /// Adjust the scrollbars' max and page step.
    void updateScrollBars();

    // *** Auto Scrolling

    /// Current auto scroll rate.
    double m_scrollRate;
    static const double MinScrollRate;
    static const double MaxScrollRate;
    static const double ScrollAccelerationFactor;

    /// Calls slotOnAutoScrollTimer().
    QTimer m_autoScrollTimer;
    /// m_autoScrollTimer interval.
    static const int AutoScrollTimerInterval;

    // Margins around the edges of the viewport where auto scrolling
    // will begin.
    int m_autoScrollXMargin;
    int m_autoScrollYMargin;

    /// Used by doAutoScroll() to detect mouse movement.
    int m_previousX;

    enum ScrollDirection { None, Top, Bottom, Left, Right };
    ScrollDirection m_currentScrollDirection;
    /// See enum FollowMode for valid mask values.
    int m_followMode;
    bool m_autoScrolling;

    // *** UNUSED

    //void bottomWidgetHeightChanged(int newHeight);
    /**
     * Scroll vertically so as to place the given position
     * somewhere near the top of the viewport.
     */
    //void slotScrollVertToTop(int vpos);
    /// Set the x and y scrollbars to a particular position
    //void slotSetScrollPos(const QPoint &);
    //QPoint viewportToContents(QPoint &);
    // From Q3ScrollView
    //void setDragAutoScroll(bool);
    /// Map a point with the inverse world matrix
    //QPoint inverseMapPoint(const QPoint& p) { return inverseWorldMatrix().map(p); }
    //void setSmoothScroll(bool s)  { m_smoothScroll = s; }
    //int getDeltaScroll() const  { return m_scrollRate; }
    //QTime m_scrollTimer;
    //bool m_smoothScroll;  // always true
    //int m_smoothScrollTimeInterval;
    //QTime m_scrollShortcuterationTimer;
    //bool isTimeForSmoothScroll();
    //void setHBarGeometry(QScrollBar &hbar, int x, int y, int w, int h);
    //int m_autoScrollShortcut;
    //static const int DefaultSmoothScrollTimeInterval;
    //static const int AutoscrollMargin;
    //static const int InitialScrollShortcut;

};


}

#endif
