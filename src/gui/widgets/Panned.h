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


/// A QGraphicsView that offers pan synchronization.
class Panned : public QGraphicsView
{
    Q_OBJECT

public:
    Panned();
    ~Panned() override { }

    /// Enable wheel shift pan/ctrl zoom behavior.
    void setWheelZoomPan(bool wheelZoomPan)  { m_wheelZoomPan = wheelZoomPan; }

    /// Scene coords; full height.
    void showPositionPointer(float x);
    /// Scene coords.
    void showPositionPointer(QPointF top, float height);
    void hidePositionPointer();
    /// If visible.
    void ensurePositionPointerInView(bool page);

signals:
    /// Emitted when the panned rect changes.
    /**
     * 5 connect() calls.
     */
    void pannedRectChanged(QRectF);

    /// Use to synchronize two Panned views.
    /**
     * Connect this to the other Panned view's slotEmulateWheelEvent() and
     * vice versa.
     *
     * 2 connect() calls.
     */
    void wheelEventReceived(QWheelEvent *);

    /**
     * 2 connect() calls.
     */
    void pannedContentsScrolled();
    /**
     * 1 connect() call.
     */
    void mouseLeaves();

    /// Emitted on ctrl+wheel.
    /**
     * 2 connect() calls.
     */
    void zoomIn();
    /// Emitted on ctrl+wheel.
    /**
     * 2 connect() calls.
     */
    void zoomOut();

public slots:
    void slotSetPannedRect(QRectF);
    void slotEmulateWheelEvent(QWheelEvent *ev);

protected:
    // QWidget overrides
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void leaveEvent(QEvent *) override;

    // QGraphicsView override.
    void drawForeground(QPainter *, const QRectF &) override;

private:
    /// Cache to detect changes in the panned rect.
    /**
     * See pannedRectChanged().
     */
    QRectF m_pannedRect;

    /// Top of the playback position pointer.
    /**
     * Set by the showPositionPointer() overloaded functions.
     */
    QPointF m_pointerTop;
    /// Height of the playback position pointer.
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
