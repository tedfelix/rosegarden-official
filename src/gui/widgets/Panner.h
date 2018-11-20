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

#ifndef RG_PANNER_H
#define RG_PANNER_H

#include <QGraphicsView>

namespace Rosegarden
{

class Panner : public QGraphicsView
{
    Q_OBJECT

public:
    Panner();
    virtual ~Panner() { }

    virtual void setScene(QGraphicsScene *);

signals:
    void pannedRectChanged(QRectF);
    void pannerChanged(QRectF);
    void zoomIn();
    void zoomOut();

public slots:
    void slotSetPannedRect(QRectF);

    void slotShowPositionPointer(float x); // scene coord; full height
    void slotShowPositionPointer(QPointF top, float height); // scene coords
    void slotHidePositionPointer();

protected slots:
    void slotSceneRectChanged(const QRectF &);

protected:
    QRectF m_pannedRect;
    QPointF m_pointerTop;
    float m_pointerHeight;
    bool m_pointerVisible;

    void moveTo(QPoint);

    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

    void resizeEvent(QResizeEvent *) override;

    virtual void updateScene(const QList<QRectF> &);
    void drawItems(QPainter *, int, QGraphicsItem *[],
                           const QStyleOptionGraphicsItem []) override;

    bool m_clicked;
    QRectF m_clickedRect;
    QPoint m_clickedPoint;

    QPixmap m_cache;
};

}

#endif

