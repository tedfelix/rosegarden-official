/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

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

#include "gui/general/AutoScroller.h"

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

    QPoint viewportToContents(const QPoint &);

    void updateContents();

    void startAutoScroll();
    void setFollowMode(FollowMode followMode)
            { m_autoScroller.setFollowMode(followMode); }
    void stopAutoScroll();

    bool isAutoScrolling() const  { return m_autoScroller.isRunning(); }

    /// Playback scrolling.
    /**
     * Scroll horizontally to make the given contents position visible,
     * paging so as to get some visibility of the next screenful
     * (for playback etc)
     */
    void scrollHoriz(int x);

    /// Track select scrolling.
    /**
     * Scroll vertically to make the given contents position visible.
     *
     * The main test case for this is selecting tracks with the
     * arrow keys and making sure the view scrolls to show the
     * selected track.
     */
    void scrollVert(int y);

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

    /// Viewport resize.
    void resizeEvent(QResizeEvent *) override;

    void wheelEvent(QWheelEvent *) override;

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

    AutoScroller m_autoScroller;

};


}

#endif
