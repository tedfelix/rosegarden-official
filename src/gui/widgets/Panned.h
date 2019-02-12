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

#ifndef RG_PANNED_H
#define RG_PANNED_H

#include <QGraphicsView>
#include <QPointF>
#include <QRectF>

class QEvent;
class QPainter;
class QPaintEvent;
class QResizeEvent;
class QWheelEvent;

namespace Rosegarden
{


/// A QGraphicsView that offers viewport synchronization.
/**
 * Rename: SynchronizedGraphicsView
 */
class Panned : public QGraphicsView
{
    Q_OBJECT

public:
    Panned();
    ~Panned() override { }

    /// Enable wheel shift pan/ctrl zoom behavior.
    void setWheelZoomPan(bool wheelZoomPan)  { m_wheelZoomPan = wheelZoomPan; }


    // *** Playback Position Pointer

    /// MatrixWidget full-height pointer.  Scene coords.
    void showPositionPointer(float x);
    /// NotationWidget specific height pointer.  Scene coords.
    void showPositionPointer(QPointF top, float height);
    void hidePositionPointer();
    /// If visible.
    void ensurePositionPointerInView(bool page);

signals:
    /// Emitted when the viewport changes.  Scene coords.
    /**
     * This is emitted when the foreground is redrawn, or when the window
     * is resized and the visible scene coords change.
     *
     * Handlers of this typically use the viewport rect to stay in sync
     * with the main view and each other.
     *
     * rename: viewportChanged()
     */
    void pannedRectChanged(QRectF viewportScene);

    /// Emitted when a wheel event is received.
    /**
     * Used to synchronize a primary view (e.g. the matrix view) to a
     * secondary view (e.g. the matrix widget's piano view) when the user
     * scrolls within the secondary view using the wheel.
     *
     * MatrixWidget connects the piano view's wheelEventReceived() signal to
     * the matrix view's slotEmulateWheelEvent() slot.  This ensures that the
     * matrix view stays in sync with the piano view.
     *
     * ??? Since we already have a pannedRectChanged() signal, why not also
     *     offer a slotSyncVertical() that can handle that signal and keep
     *     two views in sync vertically (position and zoom)?  That would
     *     eliminate the need for the wheelEventReceived() signal.  This
     *     would also fix the bug where using the scroll wheel with Shift
     *     or Ctrl causes the matrix view to scroll and zoom and sometimes
     *     end up out of sync with the piano view.
     *
     * NotationWidget connects the track headers view's wheelEventReceived()
     * signal to the notation view's slotEmulateWheelEvent() slot.  This
     * ensures that the notation view stays in sync with the track headers
     * view.
     *
     * ??? As with the MatrixWidget, we might be able to get rid of this
     *     signal and instead connect the track headers view's
     *     pannedRectChanged() to a new slotSyncVertical() in the
     *     notation view.  That should be simpler and fix some bugs.
     */
    void wheelEventReceived(QWheelEvent *);

    /**
     * MatrixWidget::slotHScroll() fields this one from the view and then
     * forwards the position info to the various rulers.
     *
     * NotationWidget::slotHScroll() fields this one from the view and then
     * forwards the position info to the various rulers.  It does other things
     * as well.
     *
     * ??? Perhaps we can get rid of this and instead use pannedRectChanged().
     *     Since this provides absolutely no info, we should be able to easily
     *     do this in a pannedRectChanged() handler within the Widget classes.
     *     pannedRectChanged() is sent more frequently, but who cares?  If
     *     we want to be more efficient, we can detect horizontal scrolling
     *     in the handlers.
     *
     * rename: horizontalScroll()
     */
    void pannedContentsScrolled();

    /// Emitted when a leaveEvent() is received.
    /**
     * Used by the MatrixWidget to remove the pitch highlight.
     */
    void mouseLeaves();

    /// Emitted on ctrl+wheel.
    /**
     * This drives the zoom behavior in MatrixWidget and NotationWidget.
     * See MatrixWidget::slotZoomIn() and NotationWidget::slotZoomIn().
     *
     * ??? Might combine the two signals into a single zoom() signal with
     *     a bool to indicate "in" vs. "out".
     */
    void zoomIn();
    /// Emitted on ctrl+wheel.
    /**
     * This drives the zoom behavior in MatrixWidget and NotationWidget.
     * See MatrixWidget::slotZoomOut() and NotationWidget::slotZoomOut().
     */
    void zoomOut();

public slots:
    /// Actually, this only centers the viewport.
    /**
     * rename: slotSetViewport();
     */
    void slotSetPannedRect(QRectF viewportScene);
    void slotEmulateWheelEvent(QWheelEvent *ev);

protected:
    // QWidget overrides
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void leaveEvent(QEvent *) override;

    // QAbstractScrollArea override.
    //void scrollContentsBy(int dx, int dy) override;

    // QGraphicsView override.
    void drawForeground(QPainter *, const QRectF &) override;

private:
    /// Cache to detect changes in the viewport.  Scene coords.
    /**
     * See pannedRectChanged().
     *
     * rename: m_viewportScene
     */
    QRectF m_pannedRect;

    /// Top of the playback position pointer.  Scene coords.
    /**
     * Set by the showPositionPointer() overloaded functions.
     */
    QPointF m_pointerTop;
    /// Height of the playback position pointer.  Scene coords.
    /**
     * 0 means full height.
     *
     * Set by the showPositionPointer() overloaded functions.
     */
    float m_pointerHeight;
    /// Whether to draw the playback position pointer.
    /**
     * Used by drawForeground() to draw the PPP.  Used by other functions
     * to make sure the appropriate portions of the view are updated.
     */
    bool m_pointerVisible;

    /// Whether wheel shift pan/ctrl zoom behavior is enabled.
    bool m_wheelZoomPan;

    /// Custom processing providing standard modifier key functions.
    /**
     * Used when m_wheelZoomPan is true.
     */
    void processWheelEvent(QWheelEvent *e);

};


}

#endif
