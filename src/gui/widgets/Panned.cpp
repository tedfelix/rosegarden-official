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
#include "gui/general/GUIPalette.h"

#include <QScrollBar>

#include <iostream>

namespace Rosegarden
{
	
Panned::Panned() :
    m_pointerVisible(false)
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

    if (m_pointerVisible && scene()) {
        QPoint top = mapFromScene(m_pointerTop);
        float height = m_pointerHeight;
        if (height == 0.f) height = scene()->height();
        QPoint bottom = mapFromScene
            (QPointF(m_pointerTop.x(), m_pointerTop.y() + height));
        paint->save();
        paint->setWorldMatrix(QMatrix());
        paint->setPen(QPen(GUIPalette::getColour(GUIPalette::Pointer), 2));
        paint->drawLine(top, bottom);
        paint->restore();
    }
}

void
Panned::scrollContentsBy(int dx, int dy)
{
    // used to scroll rulers
    emit pannedContentsScrolled(dx, dy);

    QGraphicsView::scrollContentsBy(dx, dy);
}

void
Panned::slotSetPannedRect(QRectF pr)
{
    centerOn(pr.center());
}

void
Panned::slotShowPositionPointer(float x) // scene coord; full height
{
    m_pointerVisible = true;
    m_pointerTop = QPointF(x, 0);
    m_pointerHeight = 0;
    update(); //!!! should update old and new pointer areas only, really
}

void
Panned::slotShowPositionPointer(QPointF top, float height) // scene coords
{
    m_pointerVisible = true;
    m_pointerTop = top;
    m_pointerHeight = height;
    update(); //!!! should update old and new pointer areas only, really
}

void
Panned::slotEnsurePositionPointerInView(bool page)
{
    if (!m_pointerVisible || !scene()) return;

    // scroll horizontally only

    double x = m_pointerTop.x();

    //!!! n.b. should probably behave differently if the pointer is
    //!!! not full height

    int hMin = horizontalScrollBar()->minimum();
    int hMax = horizontalScrollBar()->maximum();

    double leftDist = 0.15;
    double rightDist = 0.20;

    int w = width();                        // View width in pixels    
    QRectF r = mapToScene(0, 0, w, 1).boundingRect();
    double ws = r.width();                  // View width in scene units
    double left = r.x();                    // View left x in scene units
    double right = left + ws;               // View right x in scene units

    QRectF sr = sceneRect();
    double length = sr.width();             // Scene horizontal length
    double x1 = sr.x();                     // Scene x minimum value
    double x2 = x1 + length;                // Scene x maximum value

    double leftThreshold = left + ws * leftDist;
    double rightThreshold = right - ws * rightDist;
    double delta = x - leftThreshold;
  
    // Is x inside the scene? If not do nothing.
    if ((x < x1) || (x > x2)) return;

    std::cerr << "page = " << page << std::endl;

    int value = horizontalScrollBar()->value();

//    std::cerr << "x = " << x << ", left = " << left << ", leftThreshold = " << leftThreshold << ", right = " << right << ", rightThreshold = " << rightThreshold << std::endl;

    // Is x inside the view?
    if (x < left || (x > rightThreshold && x < right && page)) {
//        std::cerr << "big scroll (x is off left, or paging)" << std::endl;
        // scroll to have the left of the view, plus threshold, at x
        value = hMin + (((x - ws * leftDist) - x1) * (hMax - hMin)) / (length - ws);
    } else if (x > rightThreshold) {
//        std::cerr << "small scroll (x is off right and not paging)" << std::endl;
        value = hMin + (((x - ws * (1.0 - rightDist)) - x1) * (hMax - hMin)) / (length - ws);
    }
            
    if (value < hMin) value = hMin;
    else if (value > hMax) value = hMax;
    horizontalScrollBar()->setValue(value);
}

void
Panned::slotHidePositionPointer()
{
    m_pointerVisible = false;
    update(); //!!! should update old pointer area only, really
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
