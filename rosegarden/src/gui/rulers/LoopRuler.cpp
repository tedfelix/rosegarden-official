/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "LoopRuler.h"

#include "misc/Debug.h"
#include "base/RulerScale.h"
#include "base/SnapGrid.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/HZoomable.h"
#include "gui/general/RosegardenCanvasView.h"
#include <qpainter.h>
#include <qrect.h>
#include <qsize.h>
#include <qwidget.h>
#include <qtooltip.h>
#include <klocale.h>
#include <kaction.h>
#include <qpainter.h>
#include <qpointarray.h>
#include "document/RosegardenGUIDoc.h"


namespace Rosegarden
{

LoopRuler::LoopRuler(RosegardenGUIDoc *doc,
                     RulerScale *rulerScale,
                     int height,
                     double xorigin,
                     bool invert,
                     QWidget *parent,
                     const char *name) : 
    QWidget(parent, name),
    m_doc(doc),
    m_height(height),
    m_xorigin(xorigin),
    m_invert(invert),
    m_currentXOffset(0),
    m_width( -1),
    m_activeMousePress(false),
    m_rulerScale(rulerScale),
    m_defaultGrid(rulerScale),
    m_loopGrid(rulerScale),
    m_grid(&m_defaultGrid),
    m_loopingMode(false),
    m_startLoop(0), m_endLoop(0),
    m_quickMarkerPen(QPen(GUIPalette::getColour(GUIPalette::QuickMarker), 4))
{
    /*
     * I need to understand if this ruler is being built for the main
     * window (Track Editor) or for any of the segment editors. Apparently
     * the name parameter is supplied (non-NULL) only for the main window.
     * I can't find of any other (more decent) way to do this. Sorry.
     */
    m_mainWindow = (name != 0 && std::string(name).length() != 0);
    
    setBackgroundColor(GUIPalette::getColour(GUIPalette::LoopRulerBackground));

    // Always snap loop extents to beats; by default apply no snap to
    // pointer position
    //
    m_defaultGrid.setSnapTime(SnapGrid::NoSnap);
    m_loopGrid.setSnapTime(SnapGrid::SnapToBeat);

    QToolTip::add
        (this, i18n("Click and drag to move the playback pointer.\nShift-click and drag to set a range for looping or editing.\nShift-click to clear the loop or range.\nDouble-click to start playback."));
}

LoopRuler::~LoopRuler()
{}

void
LoopRuler::setSnapGrid(SnapGrid *grid)
{
    if (grid == 0) m_grid = &m_defaultGrid;
    else m_grid = grid;
}

void LoopRuler::scrollHoriz(int x)
{
    if (getHScaleFactor() != 1.0) {
        m_currentXOffset = static_cast<int>( -x / getHScaleFactor());
        repaint();
        return;
    }

    int w = width(), h = height();
    int dx = x - ( -m_currentXOffset);
    m_currentXOffset = -x;

    if (dx > w*3 / 4 || dx < -w*3 / 4) {
        update();
        return ;
    }

    if (dx > 0) { // moving right, so the existing stuff moves left
        bitBlt(this, 0, 0, this, dx, 0, w - dx, h);
        repaint(w - dx, 0, dx, h);
    } else {      // moving left, so the existing stuff moves right
        bitBlt(this, -dx, 0, this, 0, 0, w + dx, h);
        repaint(0, 0, -dx, h);
    }
}

QSize LoopRuler::sizeHint() const
{
    double width =
        m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
        m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) +
        m_xorigin;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize LoopRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;

    QSize res = QSize(int(firstBarWidth), m_height);

    return res;
}

void LoopRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);

    if (getHScaleFactor() != 1.0)
        paint.scale(getHScaleFactor(), 1.0);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.setBrush(colorGroup().foreground());
    drawBarSections(&paint);
    drawLoopMarker(&paint);
    
    if (m_mainWindow) {
        timeT tQM = m_doc->getQuickMarkerTime();
        if (tQM >= 0) {
            // draw quick marker
            double xQM = m_rulerScale->getXForTime(tQM)
                       + m_xorigin + m_currentXOffset;
            
            paint.setPen(m_quickMarkerPen);
            
            // looks necessary to compensate for shift in the CompositionView (cursor)
            paint.translate(1, 0);
            
            // draw red segment
            paint.drawLine(int(xQM), 1, int(xQM), m_height-1);
        }
    }
}

void LoopRuler::drawBarSections(QPainter* paint)
{
    QRect clipRect = paint->clipRegion().boundingRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
                                            m_currentXOffset -
                                            m_xorigin);
    int lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
        firstBar = m_rulerScale->getFirstVisibleBar();
    }

    paint->setPen(GUIPalette::getColour(GUIPalette::LoopRulerForeground));

    for (int i = firstBar; i <= lastBar; ++i) {

        double x = m_rulerScale->getBarPosition(i) + m_currentXOffset + m_xorigin;
        if ((x * getHScaleFactor()) > clipRect.x() + clipRect.width())
            break;

        double width = m_rulerScale->getBarWidth(i);
        if (width == 0)
            continue;

        if (x + width < clipRect.x())
            continue;

        if (m_invert) {
            paint->drawLine(int(x), 0, int(x), 5*m_height / 7);
        } else {
            paint->drawLine(int(x), 2*m_height / 7, int(x), m_height);
        }

        double beatAccumulator = m_rulerScale->getBeatWidth(i);
        double inc = beatAccumulator;
        if (inc == 0)
            continue;

        for (; beatAccumulator < width; beatAccumulator += inc) {
            if (m_invert) {
                paint->drawLine(int(x + beatAccumulator), 0,
                                int(x + beatAccumulator), 2 * m_height / 7);
            } else {
                paint->drawLine(int(x + beatAccumulator), 5*m_height / 7,
                                int(x + beatAccumulator), m_height);
            }
        }
    }
}

void
LoopRuler::drawLoopMarker(QPainter* paint)
{
    double x1 = (int)m_rulerScale->getXForTime(m_startLoop);
    double x2 = (int)m_rulerScale->getXForTime(m_endLoop);

    if (x1 > x2) {
        x2 = x1;
        x1 = (int)m_rulerScale->getXForTime(m_endLoop);
    }

    x1 += m_currentXOffset + m_xorigin;
    x2 += m_currentXOffset + m_xorigin;

    paint->save();
    paint->setBrush(GUIPalette::getColour(GUIPalette::LoopHighlight));
    paint->setPen(GUIPalette::getColour(GUIPalette::LoopHighlight));
    paint->drawRect(static_cast<int>(x1), 0, static_cast<int>(x2 - x1), m_height);
    paint->restore();

}

void
LoopRuler::mousePressEvent(QMouseEvent *mE)
{
    RG_DEBUG << "LoopRuler::mousePressEvent: x = " << mE->x() << endl;

    Qt::ButtonState bs = mE->state();
    setLoopingMode((bs & Qt::ShiftButton) != 0);

    if (mE->button() == LeftButton) {
        double x = mE->pos().x() / getHScaleFactor() - m_currentXOffset - m_xorigin;

        if (m_loopingMode) {
            m_endLoop = m_startLoop = m_loopGrid.snapX(x);
        } else {
            // No -- now that we're emitting when the button is
            // released, we _don't_ want to emit here as well --
            // otherwise we get an irritating stutter when simply
            // clicking on the ruler during playback
//            RG_DEBUG << "emitting setPointerPosition(" << m_rulerScale->getTimeForX(x) << ")" << endl;
//            emit setPointerPosition(m_rulerScale->getTimeForX(x));
        }

        m_activeMousePress = true;
        emit startMouseMove(RosegardenCanvasView::FollowHorizontal);
    }
}

void
LoopRuler::mouseReleaseEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton) {
        if (m_loopingMode) {
            // Cancel the loop if there was no drag
            //
            if (m_endLoop == m_startLoop) {
                m_endLoop = m_startLoop = 0;

                // to clear any other loop rulers
                emit setLoop(m_startLoop, m_endLoop);
                update();
            }

            // emit with the args around the right way
            //
            if (m_endLoop < m_startLoop)
                emit setLoop(m_endLoop, m_startLoop);
            else
                emit setLoop(m_startLoop, m_endLoop);
        } else {
            // we need to re-emit this signal so that when the user releases the button
            // after dragging the pointer, the pointer's position is updated again in the
            // other views (typically, in the seg. canvas while the user has dragged the pointer
            // in an edit view)
            //
            double x = mE->pos().x() / getHScaleFactor() - m_currentXOffset - m_xorigin;
            emit setPointerPosition(m_grid->snapX(x));
        }
        emit stopMouseMove();
        m_activeMousePress = false;
    }
}

void
LoopRuler::mouseDoubleClickEvent(QMouseEvent *mE)
{
    double x = mE->pos().x() / getHScaleFactor() - m_currentXOffset - m_xorigin;
    if (x < 0)
        x = 0;

    RG_DEBUG << "LoopRuler::mouseDoubleClickEvent: x = " << x << ", looping = " << m_loopingMode << endl;

    if (mE->button() == LeftButton && !m_loopingMode)
        emit setPlayPosition(m_grid->snapX(x));
}

void
LoopRuler::mouseMoveEvent(QMouseEvent *mE)
{
    double x = mE->pos().x() / getHScaleFactor() - m_currentXOffset - m_xorigin;
    if (x < 0)
        x = 0;

    if (m_loopingMode) {
        if (m_grid->snapX(x) != m_endLoop) {
            m_endLoop = m_loopGrid.snapX(x);
            emit dragLoopToPosition(m_endLoop);
            update();
        }
    } else {
        emit dragPointerToPosition(m_grid->snapX(x));
    }

    emit mouseMove();
}

void LoopRuler::slotSetLoopMarker(timeT startLoop,
                                  timeT endLoop)
{
    m_startLoop = startLoop;
    m_endLoop = endLoop;

    QPainter paint(this);
    paint.setBrush(colorGroup().foreground());
    drawBarSections(&paint);
    drawLoopMarker(&paint);

    update();
}

}
#include "LoopRuler.moc"
