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

#include <Qt>


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
    MATRIX_DEBUG << "MatrixMover::handleLeftButtonPress() : snapped time = " << e->snappedLeftTime << ", el = " << e->element;

    if (!e->element) return;

    // Check the scene's current segment (apparently not necessarily the same
    // segment referred to by the scene's current view segment) for this event;
    // return if not found, indicating that this event is from some other,
    // non-active segment.
    //
    // I think notation just makes whatever segment active when you click an
    // event outside the active segment, and I think that's what this code
    // attempted to do too.  I couldn't get that to work at all.  This is better
    // than being able to click on non-active elements to create new events by
    // accident, and will probably fly.  Especially since the multi-segment
    // matrix is new, and we're defining the terms of how it works.
    Segment *segment = m_scene->getCurrentSegment();
    if (!segment) return;
    bool found = false;
    for (Segment::iterator i = segment->begin(); i != segment->end(); ++i) {
        if ((*i) == e->element->event()) found = true;
    }

    if (!found) {
        MATRIX_DEBUG << "Clicked element not owned by active segment.  Returning...";
        return;
    }

    m_currentViewSegment = e->viewSegment;

    m_currentElement = e->element;

    timeT snappedAbsoluteLeftTime =
        getSnapGrid()->snapTime(m_currentElement->getViewAbsoluteTime());
    m_clickSnappedLeftDeltaTime = e->snappedLeftTime - snappedAbsoluteLeftTime;

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
        selection = newSelection;
    } else {
        m_scene->setSingleSelectedEvent(m_currentViewSegment,
                                        m_currentElement, true);
    }
    
    long velocity = m_widget->getCurrentVelocity();
    event->get<Int>(BaseProperties::VELOCITY, velocity);

    long pitchOffset = m_currentViewSegment->getSegment().getTranspose();

    long pitch = 60;
    event->get<Int>(BaseProperties::PITCH, pitch);

    // We used to m_scene->playNote() here, but the new concert pitch matrix was
    // playing chords the first time I clicked a note.  Investigation with
    // KMidiMon revealed two notes firing nearly simultaneously, and with
    // segments of 0 transpose, they were simply identical to each other.  One
    // of them came from here, and this was the one sounding at the wrong pitch
    // in transposed segments.  I've simply removed it with no apparent ill side
    // effects, and a problem solved super cheap.

    m_lastPlayedPitch = pitch;

    if (m_quickCopy && selection) {
        for (EventSelection::eventcontainer::iterator i =
                 selection->getSegmentEvents().begin();
             i != selection->getSegmentEvents().end(); ++i) {

            MatrixElement *duplicate = new MatrixElement
                (m_scene, new Event(**i),
                 m_widget->isDrumMode(), pitchOffset);

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

    timeT newTime = e->snappedLeftTime - m_clickSnappedLeftDeltaTime;
    int newPitch = e->pitch;

    emit hoveredOverNoteChanged(newPitch, true, newTime);

    // get a basic pitch difference calculation comparing the current element's
    // pitch to the clicked pitch (this does not take the transpose factor into
    // account, so in a -9 segment, the initial result winds up being 9
    // semitones too low)
    using BaseProperties::PITCH;
    int diffPitch = 0;
    if (m_currentElement->event()->has(PITCH)) {
        diffPitch = newPitch - m_currentElement->event()->get<Int>(PITCH);
    }

    EventSelection* selection = m_scene->getSelection();

    // factor in transpose to adjust the height calculation
    long pitchOffset = selection->getSegment().getTranspose();
    diffPitch += (pitchOffset * -1);

    for (EventSelection::eventcontainer::iterator it =
             selection->getSegmentEvents().begin();
         it != selection->getSegmentEvents().end(); ++it) {

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
    }

    if (newPitch != m_lastPlayedPitch) {
        long velocity = m_widget->getCurrentVelocity();
        m_currentElement->event()->get<Int>(BaseProperties::VELOCITY, velocity);
        m_scene->playNote(m_currentViewSegment->getSegment(), newPitch + (pitchOffset * -1), velocity);
        m_lastPlayedPitch = newPitch;
    }

    return FollowMode(FollowHorizontal | FollowVertical);
}

void
MatrixMover::handleMouseRelease(const MatrixMouseEvent *e)
{
    if (!e) return;

    MATRIX_DEBUG << "MatrixMover::handleMouseRelease() - newPitch = "
                 << e->pitch << endl;

    if (!m_currentElement || !m_currentViewSegment) return;

    timeT newTime = e->snappedLeftTime - m_clickSnappedLeftDeltaTime;
    int newPitch = e->pitch;

    if (newPitch > 127) newPitch = 127;
    if (newPitch < 0) newPitch = 0;

    // get a basic pitch difference calculation comparing the current element's
    // pitch to the pitch the mouse was released at (see note in
    // handleMouseMove)
    using BaseProperties::PITCH;
    timeT diffTime = newTime - m_currentElement->getViewAbsoluteTime();
    int diffPitch = 0;
    if (m_currentElement->event()->has(PITCH)) {
        diffPitch = newPitch - m_currentElement->event()->get<Int>(PITCH);
    }

    EventSelection* selection = m_scene->getSelection();

    // factor in transpose to adjust the height calculation
    long pitchOffset = selection->getSegment().getTranspose();
    diffPitch += (pitchOffset * -1);

    if ((diffTime == 0 && diffPitch == 0) || selection->getAddedEvents() == 0) {
        for (size_t i = 0; i < m_duplicateElements.size(); ++i) {
            delete m_duplicateElements[i]->event();
            delete m_duplicateElements[i];
        }
        m_duplicateElements.clear();
        m_currentElement = 0;
        return;
    }

    if (newPitch != m_lastPlayedPitch) {
        long velocity = m_widget->getCurrentVelocity();
        m_currentElement->event()->get<Int>(BaseProperties::VELOCITY, velocity);
        m_scene->playNote(m_currentViewSegment->getSegment(), newPitch + (pitchOffset * -1), velocity);
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
        
    for (; it != selection->getSegmentEvents().end(); ++it) {

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
    m_widget->setCanvasCursor(Qt::SizeAllCursor);
    setBasicContextHelp();
}

void MatrixMover::stow()
{
    // Nothing of this vestigial code remains in modern Qt Rosegarden.
}

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

QString MatrixMover::ToolName() { return "mover"; }

}

