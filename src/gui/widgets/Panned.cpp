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

#include "Panned.h"

#include "misc/Debug.h"

namespace Rosegarden
{
	
Panned::Panned()
{
}

void
Panned::resizeEvent(QResizeEvent *)
{
    QPointF near = mapToScene(0, 0);
    QPointF far = mapToScene(width(), height());
    QSizeF sz(far.x()-near.x(), far.y()-near.y());
    QRectF pr(near, sz);

    RG_DEBUG << "Panned::resizeEvent: pr = " << pr << endl;

    if (pr != m_pannedRect) {
        m_pannedRect = pr;
        emit pannedRectChanged(pr);
    }
}
    
void
Panned::drawForeground(QPainter *paint, const QRectF &)
{
    QPointF near = mapToScene(0, 0);
    QPointF far = mapToScene(width(), height());
    QSizeF sz(far.x()-near.x(), far.y()-near.y());
    QRectF pr(near, sz);

    RG_DEBUG << "Panned::drawForeground: pr = " << pr << endl;

    if (pr != m_pannedRect) {
        m_pannedRect = pr;
        emit pannedRectChanged(pr);
    }
}

void
Panned::slotSetPannedRect(QRectF pr)
{
    centerOn(pr.center());
}

void
Panned::wheelEvent(QWheelEvent *ev)
{
    emit wheelEventReceived(ev);
    QGraphicsView::wheelEvent(ev);
}

void
Panned::slotEmulateWheelEvent(QWheelEvent *ev)
{
    QGraphicsView::wheelEvent(ev);
}
    
}

#include "Panned.moc"
