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
     * Used to synchronize two views.
     *
     * Connect this to the other view's slotEmulateWheelEvent() and
     * vice versa.
     *
     * ??? Is this redundant?  We're already syncing based on the viewport
     *     with pannedRectChanged().  Why do we need this as well?
     */
    void wheelEventReceived(QWheelEvent *);

    /**
     * 2 connect() calls.
     *
     * ??? Is this redundant?  We're already syncing based on the viewport
     *     with pannedRectChanged().  Why do we need this as well?
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
     * 2 connect() calls.
     *
     * ??? Is this redundant?  We're already syncing based on the viewport
     *     with pannedRectChanged().  Why do we need this as well?
     *     slotSetPannedRect() only centers.  That might be part of the reason.
     */
    void zoomIn();
    /// Emitted on ctrl+wheel.
    /**
     * 2 connect() calls.
     *
     * ??? Is this redundant?  We're already syncing based on the viewport
     *     with pannedRectChanged().  Why do we need this as well?
     *     slotSetPannedRect() only centers.  That might be part of the reason.
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
