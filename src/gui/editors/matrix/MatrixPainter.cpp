/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MatrixPainter]"
#define RG_NO_DEBUG_PRINT

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
    m_currentElement(nullptr),
    m_currentViewSegment(nullptr),
    m_previewElement(nullptr)
{
    createAction("select", SLOT(slotSelectSelected()));
    createAction("resize", SLOT(slotResizeSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("move", SLOT(slotMoveSelected()));

    createMenu();
    m_previewEvent = new Event(Note::EventType, 0);
}

MatrixPainter::~MatrixPainter()
{
    delete m_previewElement;
    delete m_previewEvent;
}

void MatrixPainter::handleEventRemoved(Event * /*event*/)
{
    // Unlike all the other handleEventRemoved() handlers, when we
    // get into this one, m_currentElement is either nullptr or
    // points to something completely valid.  However, it is never
    // pointing to the event that comes in.  I assume this is
    // because m_currentElement always points to the temporary
    // MatrixElement that is used while drawing.  And that one is
    // never selected and therefore can never be deleted.

    // Nothing to do here.  The incoming Event is never related
    // to m_currentElement.
}

void MatrixPainter::handleMidButtonPress(const MatrixMouseEvent * /*event*/)
{
    // note: middle button == third button (== left+right at the same time)
}

void MatrixPainter::handleMouseDoubleClick(const MatrixMouseEvent *e){
    /**
    left double click with PainterTool : deletes MatrixElement
    **/

    RG_DEBUG << "handleMouseDoubleClick(): pitch = " << e->pitch << ", time : " << e->time;

    m_currentViewSegment = e->viewSegment;
    if (!m_currentViewSegment) return;

    // Don't create an overlapping event on the same note on the same channel
    if (e->element) {
        //RG_DEBUG << "handleMouseDoubleClick(): overlap with another matrix element";
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
        m_currentElement = nullptr;
        return;
    }

    /*
    // Grid needed for the event duration rounding

    int velocity = m_widget->getCurrentVelocity();

    RG_DEBUG << "handleMouseDoubleClick(): velocity = " << velocity;

    m_clickTime = e->snappedLeftTime;

    Event *ev = new Event(Note::EventType, e->snappedLeftTime, e->snapUnit);
    ev->set<Int>(BaseProperties::PITCH, e->pitch);
    ev->set<Int>(BaseProperties::VELOCITY, velocity);

    m_currentElement = new MatrixElement(m_scene, ev, m_widget->isDrumMode(),
                                         m_scene->getCurrentSegment());

    // preview
    m_scene->playNote(m_currentViewSegment->getSegment(), e->pitch, velocity);
    */


}// end handleMouseDoubleClick()


void MatrixPainter::handleLeftButtonPress(const MatrixMouseEvent *e)
{
    RG_DEBUG << "handleLeftButtonPress(): pitch = " << e->pitch << ", time : " << e->time;

    // no preview as long as the button is pressed
    clearPreview();

    m_currentViewSegment = e->viewSegment;
    if (!m_currentViewSegment) return;

    const Segment *eventSegment = nullptr;
    if (e->element) eventSegment = e->element->getSegment();

    // Don't create an overlapping event on the same note on the same channel
    if (e->element && eventSegment == m_scene->getCurrentSegment()) {
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
        else {
            RG_DEBUG << "handleLeftButtonPress(): Will not create note at "
                        "same pitch and time as existing note in active "
                        "segment.";
        }
        delete m_currentElement;
        m_currentElement = nullptr;
        return;
    }

    // Grid needed for the event duration rounding

    int velocity = m_widget->getCurrentVelocity();

    RG_DEBUG << "handleLeftButtonPress(): velocity = " << velocity;

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

    RG_DEBUG << "handleLeftButtonPress(): I'm working from segment \"" << m_currentViewSegment->getSegment().getLabel() << "\"" << "  clicked pitch: " << e->pitch << " adjusted pitch: " << adjustedPitch;

    m_currentElement = new MatrixElement(m_scene, ev, m_widget->isDrumMode(),
                                         pitchOffset,
                                         m_scene->getCurrentSegment());

    // preview
    m_scene->playNote(m_currentViewSegment->getSegment(), adjustedPitch, velocity);
}

FollowMode
MatrixPainter::handleMouseMove(const MatrixMouseEvent *e)
{
    if (!m_currentElement) {
        // show preview
        showPreview(e);
        return NO_FOLLOW;
    }

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

    //RG_DEBUG << "handleMouseMove(): pitch = " << e->pitch << "time = " << time << ", end time = " << endTime;

    using BaseProperties::PITCH;

    // we need the same pitch/height corrections for dragging notes as for
    // entering new ones
    m_currentViewSegment = e->viewSegment;
    if (!m_currentViewSegment) return NO_FOLLOW;
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

    // const Segment *segment = e->element ? e->element->getSegment() : nullptr;
    m_currentElement = new MatrixElement(m_scene, ev, m_widget->isDrumMode(),
                                         pitchOffset,
                                         m_scene->getCurrentSegment());

    if (preview) {
        m_scene->playNote(m_currentViewSegment->getSegment(), adjustedPitch, velocity);
    }

    return (FOLLOW_HORIZONTAL | FOLLOW_VERTICAL);
}

void MatrixPainter::handleMouseRelease(const MatrixMouseEvent *e)
{
    showPreview(e);

    // This can happen in case of screen/window capture -
    // we only get a mouse release, the window snapshot tool
    // got the mouse down
    if (!m_currentElement)
        return;

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

    m_currentElement = nullptr;
    m_currentViewSegment = nullptr;

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
    clearPreview();
}

//void MatrixPainter::slotMatrixScrolled(int /* newX */, int /* newY */)
/* unused
{
    // newX = newY = 42;
    */ /*!!! */ /*
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
*/ /*
}
*/

void MatrixPainter::setBasicContextHelp()
{
    if (getSnapGrid()->getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(tr("Click and drag to draw a note; Shift to avoid snapping to grid"));
    } else {
        setContextHelp(tr("Click and drag to draw a note"));
    }
}

QString MatrixPainter::ToolName() { return "painter"; }

void MatrixPainter::showPreview(const MatrixMouseEvent *e)
{
    if (! m_previewElement) {
        m_previewElement = new MatrixElement(m_scene,
                                             m_previewEvent,
                                             m_widget->isDrumMode(),
                                             0,
                                             nullptr,
                                             true);
    }

    m_previewEvent->set<Int>(BaseProperties::PITCH, e->pitch);
    m_previewElement->reconfigure(e->snappedLeftTime, e->snapUnit);
}

void MatrixPainter::clearPreview()
{
    if (m_previewElement) {
        delete m_previewElement;
        m_previewElement = nullptr;
    }
}

}
