/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MatrixResizer.h"

#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "commands/matrix/MatrixModifyCommand.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "document/CommandHistory.h"
#include "MatrixElement.h"
#include "MatrixScene.h"
#include "MatrixTool.h"
#include "MatrixWidget.h"
#include "MatrixViewSegment.h"
#include "MatrixMouseEvent.h"
#include "misc/Debug.h"

#include <Qt>


namespace Rosegarden
{

MatrixResizer::MatrixResizer(MatrixWidget *parent) :
    MatrixTool("matrixresizer.rc", "MatrixResizer", parent),
    m_currentElement(0),
    m_currentViewSegment(0)
{
    createAction("select", SLOT(slotSelectSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("move", SLOT(slotMoveSelected()));
    
    createMenu();
}

void
MatrixResizer::handleEventRemoved(Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
        m_currentElement = 0;
    }
}

void
MatrixResizer::handleLeftButtonPress(const MatrixMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixResizer::handleLeftButtonPress() : el = "
                 << e->element << endl;

    if (!e->element) return;

    m_currentViewSegment = e->viewSegment;
    m_currentElement = e->element;

    // Add this element and allow movement
    //
    EventSelection* selection = m_scene->getSelection();
    Event *event = m_currentElement->event();

    if (selection) {
        EventSelection *newSelection;

        if ((e->modifiers & Qt::ShiftModifier) ||
            selection->contains(event)) {
            newSelection = new EventSelection(*selection);
        } else {
            newSelection = new EventSelection(m_currentViewSegment->getSegment());
        }

        newSelection->addEvent(event);
        m_scene->setSelection(newSelection, true);
//        m_mParentView->canvas()->update();
    } else {
        m_scene->setSingleSelectedEvent(m_currentViewSegment,
                                        m_currentElement,
                                        true);
//            m_mParentView->canvas()->update();
    }
}

MatrixResizer::FollowMode
MatrixResizer::handleMouseMove(const MatrixMouseEvent *e)
{
    if (!e) return NoFollow;

    setBasicContextHelp();

    if (!m_currentElement || !m_currentViewSegment) return NoFollow;

    if (getSnapGrid()->getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
    } else {
        clearContextHelp();
    }

    // snap in the closest direction
    timeT snapTime = e->snappedLeftTime;
    if (e->snappedRightTime - e->time < e->time - e->snappedLeftTime) {
        snapTime = e->snappedRightTime;
    }

    timeT newDuration = snapTime - m_currentElement->getViewAbsoluteTime();
    timeT durationDiff = newDuration - m_currentElement->getViewDuration();

    EventSelection* selection = m_scene->getSelection();
    if (!selection || selection->getAddedEvents() == 0) return NoFollow;

    EventSelection::eventcontainer::iterator it =
        selection->getSegmentEvents().begin();

    for (; it != selection->getSegmentEvents().end(); ++it) {

        MatrixElement *element = 0;
        ViewElementList::iterator vi = m_currentViewSegment->findEvent(*it);
        if (vi != m_currentViewSegment->getViewElementList()->end()) {
            element = static_cast<MatrixElement *>(*vi);
        }
        if (!element) continue;

        timeT t = element->getViewAbsoluteTime();
        timeT d = element->getViewDuration();
        
        d = d + durationDiff;
        if (d < 0) {
            t = t + d;
            d = -d;
        } else if (d == 0) {
            d = getSnapGrid()->getSnapTime(t);
        }

        element->reconfigure(t, d);
//            m_currentStaff->positionElement(element);
//        }
    }

//    m_mParentView->canvas()->update();
    return FollowHorizontal;
}

void
MatrixResizer::handleMouseRelease(const MatrixMouseEvent *e)
{
    if (!e || !m_currentElement || !m_currentViewSegment) return;

    // snap in the closest direction
    timeT snapTime = e->snappedLeftTime;
    if (e->snappedRightTime - e->time < e->time - e->snappedLeftTime) {
        snapTime = e->snappedRightTime;
    }

    timeT newDuration = snapTime - m_currentElement->getViewAbsoluteTime();
    timeT durationDiff = newDuration - m_currentElement->getViewDuration();

    EventSelection *selection = m_scene->getSelection();
    if (!selection || selection->getAddedEvents() == 0) return;

    QString commandLabel = tr("Resize Event");
    if (selection->getAddedEvents() > 1) commandLabel = tr("Resize Events");

    MacroCommand *macro = new MacroCommand(commandLabel);

    EventSelection::eventcontainer::iterator it =
        selection->getSegmentEvents().begin();

    Segment &segment = m_currentViewSegment->getSegment();

    EventSelection *newSelection = new EventSelection(segment);

    timeT normalizeStart = selection->getStartTime();
    timeT normalizeEnd = selection->getEndTime();

    for (; it != selection->getSegmentEvents().end(); ++it) {

        timeT t = (*it)->getAbsoluteTime();
        timeT d = (*it)->getDuration();

        MATRIX_DEBUG << "MatrixResizer::handleMouseRelease - "
                     << "Time = " << t
                     << ", Duration = " << d << endl;

        d = d + durationDiff;
        if (d < 0) {
            t = t + d;
            d = -d;
        } else if (d == 0) {
            d = getSnapGrid()->getSnapTime(t);
        }

        if (t + d > segment.getEndMarkerTime()) {
            d = segment.getEndMarkerTime() - t;
            if (d <= 0) {
                d = segment.getEndMarkerTime();
                t = d - getSnapGrid()->getSnapTime(t);
            }
        }

        Event *newEvent = new Event(**it, t, d);

        macro->addCommand(new MatrixModifyCommand(segment,
                                                  *it,
                                                  newEvent,
                                                  false,
                                                  false));

        newSelection->addEvent(newEvent);
    }

    normalizeStart = std::min(normalizeStart, newSelection->getStartTime());
    normalizeEnd = std::max(normalizeEnd, newSelection->getEndTime());

    macro->addCommand(new NormalizeRestsCommand(segment,
                                                normalizeStart,
                                                normalizeEnd));

    m_scene->setSelection(0, false);
    CommandHistory::getInstance()->addCommand(macro);
    m_scene->setSelection(newSelection, false);

//    m_mParentView->update();
    m_currentElement = 0;
    setBasicContextHelp();
}

void MatrixResizer::ready()
{
//    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
//            this, SLOT(slotMatrixScrolled(int, int)));
    m_widget->setCanvasCursor(Qt::SizeHorCursor);
    setBasicContextHelp();
}

void MatrixResizer::stow()
{
//    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
//               this, SLOT(slotMatrixScrolled(int, int)));
}
/*!!!
void MatrixResizer::slotMatrixScrolled(int newX, int newY)
{
    QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint p(newX, newY);

    if (newP1.x() > oldP1.x()) {
        p.setX(newX + m_parentView->getCanvasView()->visibleWidth());
    }

    p = m_mParentView->inverseMapPoint(p);
    int newTime = getSnapGrid()->snapX(p.x());
    handleMouseMove(newTime, 0, 0);
}
*/
void MatrixResizer::setBasicContextHelp()
{
    EventSelection *selection = m_scene->getSelection();
    if (selection && selection->getAddedEvents() > 1) {
        setContextHelp(tr("Click and drag to resize selected notes"));
    } else {
        setContextHelp(tr("Click and drag to resize a note"));
    }
}

QString MatrixResizer::ToolName() { return "resizer"; }

}


