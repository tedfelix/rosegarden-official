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

namespace Rosegarden
{
	
Panner::Panner()
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setInteractive(false);
    setViewport(new PannerViewport(this));
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
Panner::PannerViewport::paintEvent(QPaintEvent *e)
{
    QPainter paint;
    paint.begin(this);

    QPainterPath path;
    path.addRect(rect());
    path.addPolygon(m_p->mapFromScene(m_p->m_pannedRect));

    QColor c(GUIPalette::getColour(GUIPalette::PannerOverlay));
    c.setAlpha(40);
    paint.setPen(Qt::NoPen);
    paint.setBrush(c);
    paint.drawPath(path);

    paint.setBrush(Qt::NoBrush);
    paint.setPen(QPen(GUIPalette::getColour(GUIPalette::PannerOverlay), 0));
    paint.drawConvexPolygon(m_p->mapFromScene(m_p->m_pannedRect));

    RG_DEBUG << "draw polygon: " << m_p->mapFromScene(m_p->m_pannedRect) << endl;
    paint.end();
}
 
void
Panner::mousePressEvent(QMouseEvent *e)
{
}

void
Panner::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }

    moveTo(e->pos());
}

void
Panner::mouseMoveEvent(QMouseEvent *e)
{
}

void
Panner::mouseReleaseEvent(QMouseEvent *e)
{
}

void
Panner::moveTo(QPoint p)
{
    QPointF sp = mapToScene(p);
    QRectF nr = m_pannedRect;
    double d = sp.x() - nr.center().x();
    nr.adjust(d, 0, d, 0);
    slotSetPannedRect(nr);
    emit pannedRectChanged(m_pannedRect);
}
   
}

#include "Panner.moc"
