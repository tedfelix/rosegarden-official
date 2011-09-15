/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ROSEGARDENSCROLLVIEW_H_
#define _RG_ROSEGARDENSCROLLVIEW_H_

#include <QPoint>
#include <QAbstractScrollArea>
#include <QDateTime>
#include <QTimer>


class QWidget;
class QWheelEvent;
class QScrollBar;
class QResizeEvent;


namespace Rosegarden
{



/**
 * A QScrollView with more elaborate auto-scrolling capabilities
 * and the ability to have a "fixed" (non-scrolling) widget at its bottom,
 * just above the bottom scrollbar.
 */
class RosegardenScrollView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    RosegardenScrollView(QWidget* parent=0);

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

	// Functions that were missing from QAbstractScrollArea
	int contentsX();	//### todo: when GUI is ready: check the following code
	int contentsY();
	void setContentsPos(int, int ); //### JAS todo: when GUI is ready: check the following code
	int visibleWidth();
	int visibleHeight();
	int contentsWidth();
	int	contentsHeight();
	
	void resizeContents(int, int);	
	void updateContents(int, int, int, int);
	void updateContents(const QRect& r);
	void updateContents();

	void paintEvent( QPaintEvent* pe );
	
	void mousePressEvent( QMouseEvent* );
	void mouseReleaseEvent( QMouseEvent* );
	void mouseDoubleClickEvent( QMouseEvent* );
	void mouseMoveEvent( QMouseEvent* );

	QPoint viewportToContents( QPoint& );
	
	void setDragAutoScroll(bool);

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

    virtual void wheelEvent(QWheelEvent *);

public slots:
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

    void updateScrollBars();

signals:
    void bottomWidgetHeightChanged(int);

    void zoomIn();
    void zoomOut();

protected:
    
    virtual void viewportPaintEvent( QPaintEvent* );

	virtual void contentsMousePressEvent( QMouseEvent* );
	virtual void contentsMouseReleaseEvent( QMouseEvent* );
	virtual void contentsMouseMoveEvent( QMouseEvent* );
	virtual void contentsMouseDoubleClickEvent( QMouseEvent* );

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
    QTime m_scrollShortcuterationTimer;

    QTimer m_autoScrollTimer;
    int m_autoScrollTime;
    int m_autoScrollShortcut;
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
    static const int InitialScrollShortcut;
    static const int MaxScrollDelta;
    static const double ScrollShortcutValue;

	int m_vwidth;
	int m_vheight;
	
private:
	void viewportMousePressEvent( QMouseEvent* );
	void viewportMouseReleaseEvent( QMouseEvent* );
	void viewportMouseDoubleClickEvent( QMouseEvent* );
	void viewportMouseMoveEvent( QMouseEvent* );
	QPoint viewportToContents(const QPoint& vp);
};


}

#endif
