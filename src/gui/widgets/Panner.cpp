/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Panner.h"

#include "gui/general/GUIPalette.h"
#include "misc/Debug.h"

#include <QPolygon>
#include <QMouseEvent>

#include <iostream>

namespace Rosegarden
{
	
Panner::Panner() :
    m_clicked(false)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    setMouseTracking(true);
    setInteractive(false);
}

void
Panner::slotSetPannedRect(QRectF rect) 
{
    RG_DEBUG << "Panner::slotSetPannedRect(" << rect << ")" << endl;
    
    m_pannedRect = rect;
    update();
}

void
Panner::resizeEvent(QResizeEvent *)
{
    if (scene()) fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void
Panner::paintEvent(QPaintEvent *e)
{
    QGraphicsView::paintEvent(e);

    QPainter paint;
    paint.begin(viewport());

    QPainterPath path;
    path.addRect(rect());
    path.addPolygon(mapFromScene(m_pannedRect));

    QColor c(GUIPalette::getColour(GUIPalette::PannerOverlay));
    c.setAlpha(80);
    paint.setPen(Qt::NoPen);
    paint.setBrush(c);
    paint.drawPath(path);

    paint.setBrush(Qt::NoBrush);
    paint.setPen(QPen(GUIPalette::getColour(GUIPalette::PannerOverlay), 0));
    paint.drawConvexPolygon(mapFromScene(m_pannedRect));

    RG_DEBUG << "draw polygon: " << mapFromScene(m_pannedRect) << endl;
    paint.end();
}
 
void
Panner::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QGraphicsView::mouseDoubleClickEvent(e);
        return;
    }
    m_clicked = true;
    m_clickedRect = m_pannedRect;
    m_clickedPoint = e->pos();
}

void
Panner::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QGraphicsView::mouseDoubleClickEvent(e);
        return;
    }

    moveTo(e->pos());
}

void
Panner::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_clicked) return;
    QPointF cp = mapToScene(m_clickedPoint);
    QPointF mp = mapToScene(e->pos());
    QPointF delta = mp - cp;
    QRectF nr = m_clickedRect;
    nr.translate(delta);
    slotSetPannedRect(nr);
    emit pannedRectChanged(m_pannedRect);
}

void
Panner::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QGraphicsView::mouseDoubleClickEvent(e);
        return;
    }

    if (m_clicked) {
        mouseMoveEvent(e);
    }

    m_clicked = false;
}

void
Panner::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0) {
        emit zoomOut();
    } else {
        emit zoomIn();
    }
}

void
Panner::moveTo(QPoint p)
{
    QPointF sp = mapToScene(p);
    QRectF nr = m_pannedRect;
    double d = sp.x() - nr.center().x();
    nr.translate(d, 0);
    slotSetPannedRect(nr);
    emit pannedRectChanged(m_pannedRect);
}
   
}

#include "Panner.moc"
