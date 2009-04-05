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

#include "MatrixMover.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "commands/matrix/MatrixModifyCommand.h"
#include "commands/matrix/MatrixInsertionCommand.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "document/CommandHistory.h"
#include "MatrixElement.h"
#include "MatrixScene.h"
#include "MatrixWidget.h"
#include "MatrixTool.h"
#include "MatrixMouseEvent.h"
#include "MatrixViewSegment.h"
#include "misc/Debug.h"


namespace Rosegarden
{

MatrixMover::MatrixMover(MatrixWidget *parent) :
    MatrixTool("matrixmover.rc", "MatrixMover", parent),
    m_currentElement(0),
    m_currentViewSegment(0),
    m_lastPlayedPitch(-1)
{
    createAction("select", SLOT(slotSelectSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("resize", SLOT(slotResizeSelected()));

    createMenu();
}

void
MatrixMover::handleEventRemoved(Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
        m_currentElement = 0;
    }
}

void
MatrixMover::handleLeftButtonPress(const MatrixMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixMover::handleLeftButtonPress() : snapped time = " << e->snappedLeftTime << ", el = " << e->element << endl;

    if (!e->element) return;

    m_currentViewSegment = e->viewSegment;
    m_currentElement = e->element;
    m_clickSnappedLeftTime = e->snappedLeftTime;

    m_quickCopy = (e->modifiers & Qt::ControlModifier);

    if (!m_duplicateElements.empty()) {
        for (size_t i = 0; i < m_duplicateElements.size(); ++i) {
            delete m_duplicateElements[i]->event();
            delete m_duplicateElements[i];
        }
        m_duplicateElements.clear();
    }

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
        
        // if the selection already contains the event, remove it from the
        // selection if shift is pressed
        if (selection->contains(event)) {
            if (e->modifiers & Qt::ShiftModifier) {
                newSelection->removeEvent(event);
            }
        } else {
            newSelection->addEvent(event);
        }
        m_scene->setSelection(newSelection, true);
//            m_mParentView->canvas()->update();
        selection = newSelection;
    } else {
        m_scene->setSingleSelectedEvent(m_currentViewSegment,
                                        m_currentElement, true);
//            m_mParentView->setSingleSelectedEvent(m_currentViewSegment->getSegment(),
//                                                  m_currentElement->event(),
//                                                  true);
//            m_mParentView->canvas()->update();
    }
    
    long velocity = m_widget->getCurrentVelocity();
    event->get<Int>(BaseProperties::VELOCITY, velocity);
    long pitch = 60;
    event->get<Int>(BaseProperties::PITCH, pitch);
    m_scene->playNote(m_currentViewSegment->getSegment(), pitch, velocity);
    m_lastPlayedPitch = pitch;
    
    if (m_quickCopy && selection) {
        for (EventSelection::eventcontainer::iterator i =
                 selection->getSegmentEvents().begin();
             i != selection->getSegmentEvents().end(); ++i) {
            
            MatrixElement *duplicate = new MatrixElement
                (m_scene, new Event(**i), m_widget->isDrumMode());
/*
  duplicate->setLayoutY(element->getLayoutY());
  duplicate->setLayoutX(element->getLayoutX());
  duplicate->setWidth(element->getWidth());
  duplicate->setHeight(element->getHeight());
  duplicate->setCanvasZ(-1);
  m_currentViewSegment->positionElement(duplicate);
*/
            m_duplicateElements.push_back(duplicate);
        }
    }
}

MatrixTool::FollowMode
MatrixMover::handleMouseMove(const MatrixMouseEvent *e)
{
    if (!e) return NoFollow;

    MATRIX_DEBUG << "MatrixMover::handleMouseMove() snapped time = "
                 << e->snappedLeftTime << endl;

    setBasicContextHelp(e->modifiers & Qt::ControlModifier);

    if (!m_currentElement || !m_currentViewSegment) return NoFollow;

    if (getSnapGrid()->getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
    } else {
        clearContextHelp();
    }

    timeT newTime = m_currentElement->getViewAbsoluteTime() +
        (e->snappedLeftTime - m_clickSnappedLeftTime);
    int newPitch = e->pitch;

    emit hoveredOverNoteChanged(newPitch, true, newTime);

    using BaseProperties::PITCH;
    int diffPitch = 0;
    if (m_currentElement->event()->has(PITCH)) {
        diffPitch = newPitch - m_currentElement->event()->get<Int>(PITCH);
    }
/*
    int diffY =
        int(((m_currentViewSegment->getLayoutYForHeight(newPitch) -
              m_currentViewSegment->getElementHeight() / 2) -
             m_currentElement->getLayoutY()));
*/
    EventSelection* selection = m_scene->getSelection();

//    MatrixElement *element = 0;
//    int maxY = m_currentViewSegment->getCanvasYForHeight(0);

    for (EventSelection::eventcontainer::iterator it =
             selection->getSegmentEvents().begin();
         it != selection->getSegmentEvents().end(); ++it) {

//        MatrixElement *element = m_currentViewSegment->getElement(*it);
//        if (!element) continue;


        MatrixElement *element = 0;
        ViewElementList::iterator vi = m_currentViewSegment->findEvent(*it);
        if (vi != m_currentViewSegment->getViewElementList()->end()) {
            element = static_cast<MatrixElement *>(*vi);
        }
        if (!element) continue;

        timeT diffTime = element->getViewAbsoluteTime() -
            m_currentElement->getViewAbsoluteTime();

        int epitch = 0;
        if (element->event()->has(PITCH)) {
            epitch = element->event()->get<Int>(PITCH);
        }

        element->reconfigure(newTime + diffTime,
                             element->getViewDuration(),
                             epitch + diffPitch);
                             
        element->setSelected(true);
            
//        int newX = getSnapGrid().getRulerScale()->
//            getXForTime(newTime + diffTime);

//        if (newX < 0) newX = 0;

//        int newY = int(element->getLayoutY() + diffY);

//            if (newY < 0) newY = 0;
//            if (newY > maxY) newY = maxY;

//            element->setLayoutX(newX);
//            element->setLayoutY(newY);

//            m_currentViewSegment->positionElement(element);
        
//        }
    }

    if (newPitch != m_lastPlayedPitch) {
        long velocity = m_widget->getCurrentVelocity();
        m_currentElement->event()->get<Int>(BaseProperties::VELOCITY, velocity);
        m_scene->playNote(m_currentViewSegment->getSegment(), newPitch, velocity);
        m_lastPlayedPitch = newPitch;
    }

//    m_mParentView->canvas()->update();
    return FollowMode(FollowHorizontal | FollowVertical);
}

void
MatrixMover::handleMouseRelease(const MatrixMouseEvent *e)
{
    if (!e) return;

    MATRIX_DEBUG << "MatrixMover::handleMouseRelease() - newPitch = "
                 << e->pitch << endl;

    if (!m_currentElement || !m_currentViewSegment) return;

    timeT newTime = m_currentElement->getViewAbsoluteTime() +
        (e->snappedLeftTime - m_clickSnappedLeftTime);
    int newPitch = e->pitch;

    if (newPitch > 127) newPitch = 127;
    if (newPitch < 0) newPitch = 0;

    using BaseProperties::PITCH;
    timeT diffTime = newTime - m_currentElement->getViewAbsoluteTime();
    int diffPitch = 0;
    if (m_currentElement->event()->has(PITCH)) {
        diffPitch = newPitch - m_currentElement->event()->get<Int>(PITCH);
    }

    EventSelection *selection = m_scene->getSelection();

    if ((diffTime == 0 && diffPitch == 0) || selection->getAddedEvents() == 0) {
        for (size_t i = 0; i < m_duplicateElements.size(); ++i) {
            delete m_duplicateElements[i]->event();
            delete m_duplicateElements[i];
        }
        m_duplicateElements.clear();
//        m_mParentView->canvas()->update();
        m_currentElement = 0;
        return;
    }

    if (newPitch != m_lastPlayedPitch) {
        long velocity = m_widget->getCurrentVelocity();
        m_currentElement->event()->get<Int>(BaseProperties::VELOCITY, velocity);
        m_scene->playNote(m_currentViewSegment->getSegment(), newPitch, velocity);
        m_lastPlayedPitch = newPitch;
    }

    QString commandLabel;
    if (m_quickCopy) {
        if (selection->getAddedEvents() < 2) {
            commandLabel = tr("Copy and Move Event");
        } else {
            commandLabel = tr("Copy and Move Events");
        }
    } else {
        if (selection->getAddedEvents() < 2) {
            commandLabel = tr("Move Event");
        } else {
            commandLabel = tr("Move Events");
        }
    }

    MacroCommand *macro = new MacroCommand(commandLabel);

    EventSelection::eventcontainer::iterator it =
        selection->getSegmentEvents().begin();

    Segment &segment = m_currentViewSegment->getSegment();

    EventSelection *newSelection = new EventSelection(segment);

    timeT normalizeStart = selection->getStartTime();
    timeT normalizeEnd = selection->getEndTime();

    if (m_quickCopy) {
        for (size_t i = 0; i < m_duplicateElements.size(); ++i) {
            timeT time = m_duplicateElements[i]->getViewAbsoluteTime();
            timeT endTime = time + m_duplicateElements[i]->getViewDuration();
            if (time < normalizeStart) normalizeStart = time;
            if (endTime > normalizeEnd) normalizeEnd = endTime;
            macro->addCommand(new MatrixInsertionCommand
                              (segment, time, endTime, 
                               m_duplicateElements[i]->event()));
            delete m_duplicateElements[i]->event();
            delete m_duplicateElements[i];
        }
        m_duplicateElements.clear();
        m_quickCopy = false;
    }
        
    for (; it != selection->getSegmentEvents().end(); it++) {

        timeT newTime = (*it)->getAbsoluteTime() + diffTime;

        int newPitch = 60;
        if ((*it)->has(PITCH)) {
            newPitch = (*it)->get<Int>(PITCH) + diffPitch;
        }

        Event *newEvent = 0;

        if (newTime < segment.getStartTime()) {
            newTime = segment.getStartTime();
        }

        if (newTime + (*it)->getDuration() >= segment.getEndMarkerTime()) {
            timeT limit = getSnapGrid()->snapTime
                (segment.getEndMarkerTime() - 1, SnapGrid::SnapLeft);
            if (newTime > limit) newTime = limit;
            timeT newDuration = std::min
                ((*it)->getDuration(), segment.getEndMarkerTime() - newTime);
            newEvent = new Event(**it, newTime, newDuration);
        } else {
            newEvent = new Event(**it, newTime);
        }

        newEvent->set<Int>(BaseProperties::PITCH, newPitch);

        macro->addCommand(new MatrixModifyCommand(segment,
                                                  (*it),
                                                  newEvent,
                                                  true,
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

//    m_mParentView->canvas()->update();
    m_currentElement = 0;

    setBasicContextHelp();
}

void MatrixMover::ready()
{
//    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
//            this, SLOT(slotMatrixScrolled(int, int)));

    connect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
            m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));

    m_widget->setCanvasCursor(Qt::sizeAllCursor);
    setBasicContextHelp();
}

void MatrixMover::stow()
{
//    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
//               this, SLOT(slotMatrixScrolled(int, int)));
    disconnect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
               m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));
}

/*
void MatrixMover::slotMatrixScrolled(int newX, int newY)
{
    if (!m_currentElement)
        return ;

    QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint offset = newP1 - oldP1;

    offset = m_mParentView->inverseMapPoint(offset);

    QPoint p(m_currentElement->getCanvasX(), m_currentElement->getCanvasY());
    p += offset;

    timeT newTime = getSnapGrid().snapX(p.x());
    int newPitch = m_currentViewSegment->getHeightAtCanvasCoords(p.x(), p.y());

    handleMouseMove(newTime, newPitch, 0);
}
*/
void MatrixMover::setBasicContextHelp(bool ctrlPressed)
{
    EventSelection *selection = m_scene->getSelection();
    if (!selection || selection->getAddedEvents() < 2) {
        if (!ctrlPressed) {
            setContextHelp(tr("Click and drag to move a note; hold Ctrl as well to copy it"));
        } else {
            setContextHelp(tr("Click and drag to copy a note"));
        }
    } else {
        if (!ctrlPressed) {
            setContextHelp(tr("Click and drag to move selected notes; hold Ctrl as well to copy"));
        } else {
            setContextHelp(tr("Click and drag to copy selected notes"));
        }
    }
}

const QString MatrixMover::ToolName = "mover";

}

#include "MatrixMover.moc"
