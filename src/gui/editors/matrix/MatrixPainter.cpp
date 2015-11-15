/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MatrixPainter.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/SegmentMatrixHelper.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "commands/matrix/MatrixInsertionCommand.h"
#include "commands/matrix/MatrixEraseCommand.h"
#include "commands/matrix/MatrixPercussionInsertionCommand.h"
#include "document/CommandHistory.h"
#include "MatrixElement.h"
#include "MatrixViewSegment.h"
#include "MatrixTool.h"
#include "MatrixWidget.h"
#include "MatrixScene.h"
#include "MatrixMouseEvent.h"

#include "misc/Debug.h"

#include <Qt>

namespace Rosegarden
{

MatrixPainter::MatrixPainter(MatrixWidget *widget) : 
    MatrixTool("matrixpainter.rc", "MatrixPainter", widget),
    m_clickTime(0),
    m_currentElement(0),
    m_currentViewSegment(0)
{
    createAction("select", SLOT(slotSelectSelected()));
    createAction("resize", SLOT(slotResizeSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("move", SLOT(slotMoveSelected()));

    createMenu();
}

void MatrixPainter::handleEventRemoved(Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
        delete m_currentElement;
        m_currentElement = 0;
    }
}



void MatrixPainter::handleMidButtonPress(const MatrixMouseEvent *e)
{
    // note: middle button == third button (== left+right at the same time)
    // pass
    e = e;
}


void MatrixPainter::handleMouseDoubleClick(const MatrixMouseEvent *e){
    /**
    left double click with PainterTool : deletes MatrixElement
    **/
    
    MATRIX_DEBUG << "MatrixPainter::handleThridButtonPress : pitch = "
            << e->pitch << ", time : " << e->time << endl;
    
    m_currentViewSegment = e->viewSegment;
    if (!m_currentViewSegment) return;

    // Don't create an overlapping event on the same note on the same channel
    if (e->element) {
        //std::cerr << "MatrixPainter::handleLeftButtonPress : overlap with an other matrix element" << std::endl;
        // In percussion matrix, we delete the existing event rather
        // than just ignoring it -- this is reasonable as the event
        // has no meaningful duration, so we can just toggle it on and
        // off with repeated clicks
        //if (m_widget->isDrumMode()) {
        if (e->element->event()) {
            MatrixEraseCommand *command =
                    new MatrixEraseCommand(m_currentViewSegment->getSegment(),
                                           e->element->event());
            CommandHistory::getInstance()->addCommand(command);
        }
        //}
        delete m_currentElement;
        m_currentElement = 0;
        return;
    }
    
    /*
    // Grid needed for the event duration rounding
    
    int velocity = m_widget->getCurrentVelocity();
    
    MATRIX_DEBUG << "velocity = " << velocity << endl;
    
    m_clickTime = e->snappedLeftTime;
    
    Event *ev = new Event(Note::EventType, e->snappedLeftTime, e->snapUnit);
    ev->set<Int>(BaseProperties::PITCH, e->pitch);
    ev->set<Int>(BaseProperties::VELOCITY, velocity);
    
    m_currentElement = new MatrixElement(m_scene, ev, m_widget->isDrumMode());
    
    // preview
    m_scene->playNote(m_currentViewSegment->getSegment(), e->pitch, velocity);
    */
        
    
}// end handleMouseDoubleClick()


void MatrixPainter::handleLeftButtonPress(const MatrixMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixPainter::handleLeftButtonPress : pitch = "
                 << e->pitch << ", time : " << e->time << endl;

    m_currentViewSegment = e->viewSegment;
    if (!m_currentViewSegment) return;

    // Don't create an overlapping event on the same note on the same channel
    if (e->element) {
        std::cerr << "MatrixPainter::handleLeftButtonPress : overlap with an other matrix element" << std::endl;
        // In percussion matrix, we delete the existing event rather
        // than just ignoring it -- this is reasonable as the event
        // has no meaningful duration, so we can just toggle it on and
        // off with repeated clicks
        if (m_widget->isDrumMode()) {
            if (e->element->event()) {
                MatrixEraseCommand *command =
                    new MatrixEraseCommand(m_currentViewSegment->getSegment(),
                                           e->element->event());
                CommandHistory::getInstance()->addCommand(command);
            }
        }
        delete m_currentElement;
        m_currentElement = 0;
        return;
    }

    // Grid needed for the event duration rounding

    int velocity = m_widget->getCurrentVelocity();

    MATRIX_DEBUG << "velocity = " << velocity << endl;

    m_clickTime = e->snappedLeftTime;

    // When entering notes, what you click on in concert pitch is what you
    // should see and hear, so we have to alter the event's physical pitch to
    // compensate.  In a concert pitch view of an Eb segment in -9, if you click
    // on a Bb, you should get what will be represented in notation as a G.
    long pitchOffset = m_currentViewSegment->getSegment().getTranspose();
    long adjustedPitch = e->pitch + (pitchOffset * -1);

    Event *ev = new Event(Note::EventType, e->snappedLeftTime, e->snapUnit);
    ev->set<Int>(BaseProperties::PITCH, adjustedPitch);
    ev->set<Int>(BaseProperties::VELOCITY, velocity);

    std::cout << "MATRIX PAINTER: I'm working from segment \"" << m_currentViewSegment->getSegment().getLabel() << "\"" << std::endl
              << "  clicked pitch: " << e->pitch << " adjusted pitch: " << adjustedPitch << std::endl;

    m_currentElement = new MatrixElement(m_scene, ev, m_widget->isDrumMode(), pitchOffset);

    // preview
    m_scene->playNote(m_currentViewSegment->getSegment(), adjustedPitch, velocity);
}

MatrixPainter::FollowMode
MatrixPainter::handleMouseMove(const MatrixMouseEvent *e)
{
    // sanity check
    if (!m_currentElement) return NoFollow;

    if (getSnapGrid()->getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
    } else {
        clearContextHelp();
    }

    timeT time = m_clickTime;
    timeT endTime = e->snappedRightTime;
    if (endTime <= time && e->snappedLeftTime < time) endTime = e->snappedLeftTime;
    if (endTime == time) endTime = time + e->snapUnit;
    if (time > endTime) std::swap(time, endTime);

    MATRIX_DEBUG << "MatrixPainter::handleMouseMove : pitch = "
                 << e->pitch << "time = " << time << ", end time = " << endTime << endl;

    using BaseProperties::PITCH;

    // we need the same pitch/height corrections for dragging notes as for
    // entering new ones
    m_currentViewSegment = e->viewSegment;
    if (!m_currentViewSegment) return NoFollow;
    long pitchOffset = m_currentViewSegment->getSegment().getTranspose();
    long adjustedPitch = e->pitch + (pitchOffset * -1);

    long velocity = m_widget->getCurrentVelocity();
    m_currentElement->event()->get<Int>(BaseProperties::VELOCITY, velocity);

    Event *ev = new Event(Note::EventType, time, endTime - time);
    ev->set<Int>(BaseProperties::PITCH, adjustedPitch);
    ev->set<Int>(BaseProperties::VELOCITY, velocity);

    bool preview = false;
    if (m_currentElement->event()->has(PITCH) &&
        adjustedPitch != m_currentElement->event()->get<Int>(PITCH)) {
        preview = true;
    }

    Event *oldEv = m_currentElement->event();
    delete m_currentElement;
    delete oldEv;

    m_currentElement = new MatrixElement(m_scene, ev, m_widget->isDrumMode(), pitchOffset);

    if (preview) {
        m_scene->playNote(m_currentViewSegment->getSegment(), adjustedPitch, velocity);
    }

    return FollowMode(FollowHorizontal | FollowVertical);
}

void MatrixPainter::handleMouseRelease(const MatrixMouseEvent *e)
{
    // This can happen in case of screen/window capture -
    // we only get a mouse release, the window snapshot tool
    // got the mouse down
    if (!m_currentElement) return;

    timeT time = m_clickTime;
    timeT endTime = e->snappedRightTime;
    if (endTime <= time && e->snappedLeftTime < time) endTime = e->snappedLeftTime;
    if (endTime == time) endTime = time + e->snapUnit;
    if (time > endTime) std::swap(time, endTime);

    if (m_widget->isDrumMode()) {

        MatrixPercussionInsertionCommand *command =
            new MatrixPercussionInsertionCommand(m_currentViewSegment->getSegment(),
                                                 time,
                                                 m_currentElement->event());
        CommandHistory::getInstance()->addCommand(command);

        Event* ev = m_currentElement->event();
        delete m_currentElement;
        delete ev;

        ev = command->getLastInsertedEvent();
        if (ev) {
            m_scene->setSingleSelectedEvent
                (&m_currentViewSegment->getSegment(), ev, false);
        }
    } else {

        SegmentMatrixHelper helper(m_currentViewSegment->getSegment());

        MatrixInsertionCommand* command =
            new MatrixInsertionCommand(m_currentViewSegment->getSegment(),
                                       time,
                                       endTime,
                                       m_currentElement->event());
        
        CommandHistory::getInstance()->addCommand(command);
        
        Event* ev = m_currentElement->event();
        delete m_currentElement;
        delete ev;
        
        ev = command->getLastInsertedEvent();
        if (ev) {
            m_scene->setSingleSelectedEvent
                (&m_currentViewSegment->getSegment(), ev, false);
        }
    }

    m_currentElement = 0;
    m_currentViewSegment = 0;

    setBasicContextHelp();
}

void MatrixPainter::ready()
{
//    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
//            this, SLOT(slotMatrixScrolled(int, int)));

    if (m_widget) m_widget->setCanvasCursor(Qt::CrossCursor);

    setBasicContextHelp();
}

void MatrixPainter::stow()
{
//    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
//               this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixPainter::slotMatrixScrolled(int /* newX */, int /* newY */)
{
    // newX = newY = 42;
/*!!!
    if (!m_currentElement)
        return ;

    QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint offset = newP1 - oldP1;

    offset = m_widget->inverseMapPoint(offset);

    QPoint p(m_currentElement->getCanvasX() + m_currentElement->getWidth(), m_currentElement->getCanvasY());
    p += offset;

    timeT newTime = getSnapGrid()->snapX(p.x());
    int newPitch = m_currentViewSegment->getHeightAtCanvasCoords(p.x(), p.y());

    handleMouseMove(newTime, newPitch, 0);
*/
}

void MatrixPainter::setBasicContextHelp()
{
    if (getSnapGrid()->getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(tr("Click and drag to draw a note; Shift to avoid snapping to grid"));
    } else {
        setContextHelp(tr("Click and drag to draw a note"));
    }        
}

const QString MatrixPainter::ToolName = "painter";

}

