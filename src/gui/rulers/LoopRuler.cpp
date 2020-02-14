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

#define RG_MODULE_STRING "[LoopRuler]"

#include "LoopRuler.h"

#include "misc/Debug.h"
#include "base/RulerScale.h"
#include "base/SnapGrid.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/RosegardenScrollView.h"
#include "document/RosegardenDocument.h"

#include <QPainter>
#include <QRect>
#include <QSize>
#include <QWidget>
#include <QToolTip>
#include <QAction>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QBrush>

#include <utility>  // std::swap()

namespace Rosegarden
{

LoopRuler::LoopRuler(RosegardenDocument *doc,
                     RulerScale *rulerScale,
                     int height,
                     bool invert,
                     bool isForMainWindow,
                     QWidget *parent) :
    QWidget(parent),
    m_height(height),
    m_invert(invert),
    m_isForMainWindow(isForMainWindow),
    m_currentXOffset(0),
    m_width( -1),
    m_activeMousePress(false),
    m_doc(doc),
    m_rulerScale(rulerScale),
    m_defaultGrid(rulerScale),
    m_loopGrid(new SnapGrid(rulerScale)),
    m_grid(&m_defaultGrid),
    m_quickMarkerPen(QPen(GUIPalette::getColour(GUIPalette::QuickMarker), 4)),
    m_loopingMode(false),
    m_startLoop(0),
    m_endLoop(0)
{
    // Always snap loop extents to beats; by default apply no snap to
    // pointer position
    //
    m_defaultGrid.setSnapTime(SnapGrid::NoSnap);
    m_loopGrid->setSnapTime(SnapGrid::SnapToBeat);

    setToolTip(tr("<qt><p>Click and drag to move the playback pointer.</p><p>Right-click and drag to set a range for looping or editing.</p><p>Right-click to clear the loop or range.</p><p>Ctrl-click and drag to move the playback pointer with snap to beat.</p><p>Double-click to start playback.</p></qt>"));
}

LoopRuler::~LoopRuler()
{
    delete m_loopGrid;
}

void
LoopRuler::setSnapGrid(const SnapGrid *grid)
{
    delete m_loopGrid;
    if (grid == nullptr) {
        m_grid = &m_defaultGrid;
        m_loopGrid = new SnapGrid(m_defaultGrid);
    } else {
        m_grid = grid;
        m_loopGrid = new SnapGrid(*grid);
    }
    m_loopGrid->setSnapTime(SnapGrid::SnapToBeat);
}

void LoopRuler::scrollHoriz(int x)
{
    // int w = width(); //, h = height();
    // int dx = x - ( -m_currentXOffset);

    m_currentXOffset = -x;

//    if (dx > w*3 / 4 || dx < -w*3 / 4) {
//        update();
//        return ;
//    }

/*### These bitBlts are not working
    RG_DEBUG << "LoopRuler::scrollHoriz > Dodgy bitBlt start?";
    if (dx > 0) { // moving right, so the existing stuff moves left
        bitBlt(this, 0, 0, this, dx, 0, w - dx, h);
        repaint(w - dx, 0, dx, h);
    } else {      // moving left, so the existing stuff moves right
        bitBlt(this, -dx, 0, this, 0, 0, w + dx, h);
        repaint(0, 0, -dx, h);
    }
    RG_DEBUG << "LoopRuler::scrollHoriz > Dodgy bitBlt end?";
*/
    update();
}

QSize LoopRuler::sizeHint() const
{
    double width =
        m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
        m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize LoopRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);

    QSize res = QSize(int(firstBarWidth), m_height);

    return res;
}

void LoopRuler::paintEvent(QPaintEvent* e)
{
//    RG_DEBUG << "LoopRuler::paintEvent";

    QPainter paint(this);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalized());

    // In a stylesheet world, we have to draw the ruler backgrounds.  Hopefully
    // this won't be too flickery.  (Seems OK, and best of all it actually
    // worked!)
    QBrush bg = QBrush(GUIPalette::getColour(GUIPalette::LoopRulerBackground));
    paint.fillRect(e->rect(), bg);

    paint.setBrush(palette().foreground());
    drawBarSections(&paint);
    drawLoopMarker(&paint);
    
    if (m_isForMainWindow) {
        timeT tQM = m_doc->getQuickMarkerTime();
        if (tQM >= 0) {
            // draw quick marker
            double xQM = m_rulerScale->getXForTime(tQM)
                       + m_currentXOffset;
            
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
                                            m_currentXOffset);
    int lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
        firstBar = m_rulerScale->getFirstVisibleBar();
    }

    paint->setPen(GUIPalette::getColour(GUIPalette::LoopRulerForeground));

    for (int i = firstBar; i < lastBar; ++i) {

        double x = m_rulerScale->getBarPosition(i) + m_currentXOffset;
        if (x > clipRect.x() + clipRect.width())
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
LoopRuler::drawLoopMarker(QPainter *paint)
{
    double x1 = (int)m_rulerScale->getXForTime(m_startLoop);
    double x2 = (int)m_rulerScale->getXForTime(m_endLoop);

    if (x1 > x2)
        std::swap(x1, x2);

    x1 += m_currentXOffset;
    x2 += m_currentXOffset;

    paint->save();
    paint->setBrush(GUIPalette::getColour(GUIPalette::LoopHighlight));
    paint->setPen(GUIPalette::getColour(GUIPalette::LoopHighlight));
    paint->drawRect(static_cast<int>(x1), 0, static_cast<int>(x2 - x1), m_height);
    paint->restore();

}

double
LoopRuler::mouseEventToSceneX(QMouseEvent *mE)
{
    double x = mE->pos().x() - m_currentXOffset;
    return x;
}

void
LoopRuler::mousePressEvent(QMouseEvent *mouseEvent)
{
    //RG_DEBUG << "LoopRuler::mousePressEvent: x = " << mouseEvent->x();

    const double x = mouseEventToSceneX(mouseEvent);

    const bool leftButton = (mouseEvent->button() == Qt::LeftButton);
    const bool rightButton = (mouseEvent->button() == Qt::RightButton);
    const bool shift = ((mouseEvent->modifiers() & Qt::ShiftModifier) != 0);

    // If loop mode has been requested
    if ((shift  &&  leftButton)  ||  rightButton) {
        // Loop mode
        m_loopingMode = true;
        m_startLoop = m_loopGrid->snapX(x);
        m_endLoop = m_startLoop;
        m_activeMousePress = true;

        emit startMouseMove(FOLLOW_HORIZONTAL);

        return;
    }

    // Left button pointer drag
    if (leftButton) {

        // If we are still using the default grid, that means we are being
        // used by the TrackEditor (instead of the MatrixEditor).
        if (m_grid == &m_defaultGrid) {
            // If the ctrl key is pressed, enable snap to beat
            if ((mouseEvent->modifiers() & Qt::ControlModifier) != 0)
                m_defaultGrid.setSnapTime(SnapGrid::SnapToBeat);
            else
                m_defaultGrid.setSnapTime(SnapGrid::NoSnap);
        }

        // No -- now that we're emitting when the button is
        // released, we _don't_ want to emit here as well --
        // otherwise we get an irritating stutter when simply
        // clicking on the ruler during playback
        //emit setPointerPosition(m_rulerScale->getTimeForX(x));

        // But we want to see the pointer under the mouse as soon as the
        // button is pressed, before we begin to drag it.
        emit dragPointerToPosition(m_grid->snapX(x));

        m_lastMouseXPos = x;

        m_activeMousePress = true;

        // ??? This signal is never emitted with any other argument.
        //     Remove the parameter.  This gets a little tricky because
        //     some clients need this and share slots with other signal
        //     sources.  It would probably be best to connect this signal
        //     to a slot in the client that is specific to LoopRuler.
        emit startMouseMove(FOLLOW_HORIZONTAL);

    }

}

void
LoopRuler::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
    // If we were in looping mode
    if (m_loopingMode) {
        m_loopingMode = false;

        // If there was no drag, cancel the loop.
        if (m_endLoop == m_startLoop) {
            m_startLoop = 0;
            m_endLoop = 0;

            // to clear any other loop rulers
            emit setLoop(m_startLoop, m_endLoop);
            update();
        } else {  // There was drag
            // Make sure start < end
            if (m_endLoop < m_startLoop)
                std::swap(m_startLoop, m_endLoop);

            emit setLoop(m_startLoop, m_endLoop);
        }

        emit stopMouseMove();
        m_activeMousePress = false;

        return;
    }

	if (mouseEvent->button() == Qt::LeftButton) {
        // we need to re-emit this signal so that when the user releases
        // the button after dragging the pointer, the pointer's position
        // is updated again in the other views (typically, in the seg.
        // canvas while the user has dragged the pointer in an edit view)
        emit setPointerPosition(m_grid->snapX(m_lastMouseXPos));

        emit stopMouseMove();
        m_activeMousePress = false;
    }
}

void
LoopRuler::mouseDoubleClickEvent(QMouseEvent *mE)
{
    double x = mouseEventToSceneX(mE);
    if (x < 0)
        x = 0;

    RG_DEBUG << "LoopRuler::mouseDoubleClickEvent: x = " << x << ", looping = " << m_loopingMode;

	if (mE->button() == Qt::LeftButton  &&  !m_loopingMode)
        emit setPlayPosition(m_grid->snapX(x));
}

void
LoopRuler::mouseMoveEvent(QMouseEvent *mE)
{
    // If we are still using the default grid, that means we are being
    // used by the TrackEditor (instead of the MatrixEditor).
    if (m_grid == &m_defaultGrid) {
        // If the ctrl key is pressed, enable snap to beat
        if ((mE->modifiers() & Qt::ControlModifier) != 0)
            m_defaultGrid.setSnapTime(SnapGrid::SnapToBeat);
        else
            m_defaultGrid.setSnapTime(SnapGrid::NoSnap);
    }

    double x = mouseEventToSceneX(mE);
    if (x < 0)
        x = 0;

    if (m_loopingMode) {
        if (m_loopGrid->snapX(x) != m_endLoop) {
            m_endLoop = m_loopGrid->snapX(x);
            emit dragLoopToPosition(m_endLoop);
            update();
        }
    } else {
        emit dragPointerToPosition(m_grid->snapX(x));

        m_lastMouseXPos = x;

    }

    emit mouseMove();
}

void LoopRuler::slotSetLoopMarker(timeT startLoop,
                                  timeT endLoop)
{
    m_startLoop = startLoop;
    m_endLoop = endLoop;

    update();
}

}
