/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MatrixMover]"
#define RG_NO_DEBUG_PRINT 1

#include "MatrixMover.h"

#include "MatrixElement.h"
#include "MatrixScene.h"
#include "MatrixWidget.h"
#include "MatrixTool.h"
#include "MatrixMouseEvent.h"
#include "MatrixViewSegment.h"

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
#include "misc/Preferences.h"
#include "misc/Debug.h"

#include <QGraphicsRectItem>
#include <QKeyEvent>


namespace Rosegarden
{


MatrixMover::MatrixMover(MatrixWidget *parent) :
    MatrixTool("matrixmover.rc", "MatrixMover", parent)
{
    createAction("select", &MatrixMover::slotSelectSelected);
    createAction("draw", &MatrixMover::slotDrawSelected);
    createAction("erase", &MatrixMover::slotEraseSelected);
    createAction("resize", &MatrixMover::slotResizeSelected);

    createMenu();
}

void
MatrixMover::handleEventRemoved(Event *event)
{
    // NOTE: Do not use m_currentElement in here as it was freed by
    //       ViewSegment::eventRemoved() before we get here.

    // Is it the event we are holding on to?
    if (m_event == event) {
        m_currentElement = nullptr;
        m_event = nullptr;
    }
}

void
MatrixMover::handleLeftButtonPress(const MatrixMouseEvent *e)
{
    RG_DEBUG << "handleLeftButtonPress() : snapped time = " << e->snappedLeftTime << ", el = " << e->element;

    if (!e->element) return;

    m_mousePressPos = e->viewpos;

    Segment *segment = m_scene->getCurrentSegment();
    if (!segment) return;

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
    if (e->element->getSegment() != segment) {

        RG_DEBUG << "handleLeftButtonPress(): Clicked element not owned by active segment.  Returning...";
        return;
    }

    m_currentViewSegment = e->viewSegment;

    m_currentElement = e->element;
    m_event = m_currentElement->event();

    timeT snappedAbsoluteLeftTime =
        getSnapGrid()->snapTime(m_currentElement->getViewAbsoluteTime());
    m_clickSnappedLeftDeltaTime = e->snappedLeftTime - snappedAbsoluteLeftTime;

    m_dragConstrained = Preferences::getMatrixConstrainNotes();
    double horizontalZoomFactor;
    double verticalZoomFactor;
    m_widget->getZoomFactors(horizontalZoomFactor, verticalZoomFactor);
    RG_DEBUG << "handleLeftButtonPress zoom factors" <<
        horizontalZoomFactor << verticalZoomFactor;
    m_constraintH->setRect
        (0, e->sceneY - m_constraintSize / verticalZoomFactor,
         m_scene->width(), 2 * m_constraintSize / verticalZoomFactor);
    m_constraintV->setRect
        (e->sceneX - m_constraintSize / horizontalZoomFactor, 0,
         2* m_constraintSize / horizontalZoomFactor, m_scene->height());

    if (m_dragConstrained) {
        m_constraintH->show();
        m_constraintV->show();
    } else {
        m_constraintH->hide();
        m_constraintV->hide();
    }

    m_quickCopy = (e->modifiers & Qt::ControlModifier);

    if (!m_duplicateElements.empty()) removeDuplicates();

    // Add this element and allow movement
    //
    EventSelection* selection = m_scene->getSelection();

    if (selection) {
        EventSelection *newSelection;

        if ((e->modifiers & Qt::ShiftModifier) ||
            selection->contains(m_event)) {
            newSelection = new EventSelection(*selection);
        } else {
            newSelection = new EventSelection(m_currentViewSegment->getSegment());
        }

        // if the selection already contains the event, remove it from the
        // selection if shift is pressed
        if (selection->contains(m_event)) {
            if (e->modifiers & Qt::ShiftModifier) {
                newSelection->removeEvent(m_event);
            }
        } else {
            newSelection->addEvent(m_event);
        }

        m_scene->setSelection(newSelection, true);
        selection = newSelection;
    } else {
        m_scene->setSingleSelectedEvent(m_currentViewSegment,
                                        m_currentElement, true);
    }

    long velocity = m_widget->getCurrentVelocity();
    m_event->get<Int>(BaseProperties::VELOCITY, velocity);

    long pitch = 60;
    m_event->get<Int>(BaseProperties::PITCH, pitch);

    // We used to m_scene->playNote() here, but the new concert pitch matrix was
    // playing chords the first time I clicked a note.  Investigation with
    // KMidiMon revealed two notes firing nearly simultaneously, and with
    // segments of 0 transpose, they were simply identical to each other.  One
    // of them came from here, and this was the one sounding at the wrong pitch
    // in transposed segments.  I've simply removed it with no apparent ill side
    // effects, and a problem solved super cheap.

    m_lastPlayedPitch = pitch;

    if (m_quickCopy && selection) createDuplicates();
}

FollowMode
MatrixMover::handleMouseMove(const MatrixMouseEvent *e)
{
    if (!e) return NO_FOLLOW;

    if (Preferences::getDynamicDrag()) {
        bool quickCopy = (e->modifiers & Qt::ControlModifier);
        if (quickCopy && ! m_quickCopy) createDuplicates();
        if (!quickCopy && m_quickCopy) removeDuplicates();
        m_quickCopy = quickCopy;
    }

    //RG_DEBUG << "handleMouseMove() snapped time = " << e->snappedLeftTime;
    int dx = e->viewpos.x() - m_mousePressPos.x();
    int dy = e->viewpos.y() - m_mousePressPos.y();

    bool vertical = (abs(dy) > abs(dx));
    // remove constraints
    if (abs(dx) > m_constraintSize &&
        abs(dy) > m_constraintSize) m_dragConstrained = false;

    if (m_dragConstrained) {
        m_constraintH->show();
        m_constraintV->show();
    } else {
        m_constraintH->hide();
        m_constraintV->hide();
    }

    RG_DEBUG << "handleMouseMove vertical" << vertical << m_dragConstrained <<
        e->viewpos << m_mousePressPos;

    setBasicContextHelp(e->modifiers & Qt::ControlModifier);

    if (!m_currentElement || !m_currentViewSegment) return NO_FOLLOW;

    if (getSnapGrid()->getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
    } else {
        clearContextHelp();
    }

    timeT newTime = e->snappedLeftTime - m_clickSnappedLeftDeltaTime;
    int newPitch = e->pitch;

    EventSelection* selection = m_scene->getSelection();

    if (m_dragConstrained) {
        if (vertical) {
            newTime = m_currentElement->getViewAbsoluteTime();
        } else {
            newPitch = m_event->get<Int>(BaseProperties::PITCH);
            // allow for transpose
            long pitchOffset = selection->getSegment().getTranspose();
            newPitch += pitchOffset;
        }
    }

    emit hoveredOverNoteChanged(newPitch, true, newTime);

    // get a basic pitch difference calculation comparing the current element's
    // pitch to the clicked pitch (this does not take the transpose factor into
    // account, so in a -9 segment, the initial result winds up being 9
    // semitones too low)
    using BaseProperties::PITCH;
    int diffPitch = 0;
    if (m_event->has(PITCH)) {
        diffPitch = newPitch - m_event->get<Int>(PITCH);
    }

    // factor in transpose to adjust the height calculation
    long pitchOffset = selection->getSegment().getTranspose();
    diffPitch += (pitchOffset * -1);

    for (EventContainer::iterator it =
             selection->getSegmentEvents().begin();
         it != selection->getSegmentEvents().end(); ++it) {

        MatrixElement *element = nullptr;
        ViewElementList::iterator vi = m_currentViewSegment->findEvent(*it);
        if (vi != m_currentViewSegment->getViewElementList()->end()) {
            element = static_cast<MatrixElement *>(*vi);
        }
        if (!element) continue;

        timeT diffTime = element->getViewAbsoluteTime() -
            m_currentElement->getViewAbsoluteTime();
        timeT newElementTime = newTime + diffTime;

        int epitch = 0;
        if (element->event()->has(PITCH)) {
            epitch = element->event()->get<Int>(PITCH);
        }

        element->reconfigure(newElementTime,
                             element->getViewDuration(),
                             epitch + diffPitch);

        element->setSelected(true);
    }

    if (newPitch != m_lastPlayedPitch) {
        long velocity = m_widget->getCurrentVelocity();
        m_event->get<Int>(BaseProperties::VELOCITY, velocity);
        m_scene->playNote(m_currentViewSegment->getSegment(), newPitch + (pitchOffset * -1), velocity);
        m_lastPlayedPitch = newPitch;
    }

    return (FOLLOW_HORIZONTAL | FOLLOW_VERTICAL);
}

void
MatrixMover::handleMouseRelease(const MatrixMouseEvent *e)
{
    if (!e) return;

    RG_DEBUG << "handleMouseRelease() - newPitch = " << e->pitch;

    int dx = e->viewpos.x() - m_mousePressPos.x();
    int dy = e->viewpos.y() - m_mousePressPos.y();

    bool vertical = (abs(dy) > abs(dx));

    if (!m_currentElement || !m_currentViewSegment) return;

    timeT newTime = e->snappedLeftTime - m_clickSnappedLeftDeltaTime;
    int newPitch = e->pitch;

    EventSelection* selection = m_scene->getSelection();

    if (m_dragConstrained) {
        m_constraintH->hide();
        m_constraintV->hide();
        if (vertical) {
            newTime = m_currentElement->getViewAbsoluteTime();
        } else {
            newPitch = m_event->get<Int>(BaseProperties::PITCH);
            // allow for transpose
            long pitchOffset = selection->getSegment().getTranspose();
            newPitch += pitchOffset;
        }
    }
    m_dragConstrained = false;

    if (newPitch > 127) newPitch = 127;
    if (newPitch < 0) newPitch = 0;

    // get a basic pitch difference calculation comparing the current element's
    // pitch to the pitch the mouse was released at (see note in
    // handleMouseMove)
    using BaseProperties::PITCH;
    timeT diffTime = newTime - m_currentElement->getViewAbsoluteTime();
    int diffPitch = 0;
    if (m_event->has(PITCH)) {
        diffPitch = newPitch - m_event->get<Int>(PITCH);
    }

    // factor in transpose to adjust the height calculation
    long pitchOffset = selection->getSegment().getTranspose();
    diffPitch += (pitchOffset * -1);

    if ((diffTime == 0 && diffPitch == 0) || selection->getAddedEvents() == 0) {
        removeDuplicates();
        m_currentElement = nullptr;
        m_event = nullptr;
        return;
    }

    if (newPitch != m_lastPlayedPitch) {
        long velocity = m_widget->getCurrentVelocity();
        m_event->get<Int>(BaseProperties::VELOCITY, velocity);
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

    EventContainer::iterator it =
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

        Event *newEvent = nullptr;

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

    m_scene->setSelection(nullptr, false);
    CommandHistory::getInstance()->addCommand(macro);
    m_scene->setSelection(newSelection, false);

//    m_mParentView->canvas()->update();
    m_currentElement = nullptr;
    m_event = nullptr;

    setBasicContextHelp();
}

void MatrixMover::ready()
{
    m_widget->setCanvasCursor(Qt::SizeAllCursor);
    setBasicContextHelp();

    if (! m_constraintH) {
        m_constraintH = new QGraphicsRectItem;
        m_constraintH->setPen(QPen(QColor(200,200,0)));
        m_constraintH->setBrush(QBrush(QColor(200,200,0)));
        m_constraintH->setOpacity(0.4);
        m_scene->addItem(m_constraintH);
    }

    if (! m_constraintV) {
        m_constraintV = new QGraphicsRectItem;
        m_constraintV->setPen(QPen(QColor(200,200,0)));
        m_constraintV->setBrush(QBrush(QColor(200,200,0)));
        m_constraintV->setOpacity(0.4);
        m_scene->addItem(m_constraintV);
    }
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

void MatrixMover::createDuplicates()
{
    EventSelection* selection = m_scene->getSelection();
    if (! selection) return;
    if (! m_currentViewSegment) return;
    long pitchOffset = m_currentViewSegment->getSegment().getTranspose();
    for (EventContainer::iterator i =
             selection->getSegmentEvents().begin();
         i != selection->getSegmentEvents().end(); ++i) {

        MatrixElement *duplicate = new MatrixElement
            (m_scene, new Event(**i),
             m_widget->isDrumMode(), pitchOffset,
             m_scene->getCurrentSegment());

        m_duplicateElements.push_back(duplicate);
    }
}

void MatrixMover::removeDuplicates()
{
    for (size_t i = 0; i < m_duplicateElements.size(); ++i) {
        delete m_duplicateElements[i]->event();
        delete m_duplicateElements[i];
    }
    m_duplicateElements.clear();
}

void MatrixMover::keyPressEvent(QKeyEvent *e)
{
    if (Preferences::getDynamicDrag()) {
        bool ctrl = (e->key() == Qt::Key_Control);
        RG_DEBUG << "keyPressEvent" << e->text() << ctrl;
        if (ctrl && ! m_quickCopy) {
            m_quickCopy = true;
            createDuplicates();
        }
    }
}

void MatrixMover::keyReleaseEvent(QKeyEvent *e)
{
    if (Preferences::getDynamicDrag()) {
        bool ctrl = (e->key() == Qt::Key_Control);
        RG_DEBUG << "keyPressRelease" << e->text() << ctrl;
        if (ctrl && m_quickCopy) {
            m_quickCopy = false;
            removeDuplicates();
        }
    }
}

QString MatrixMover::ToolName() { return "mover"; }

}
