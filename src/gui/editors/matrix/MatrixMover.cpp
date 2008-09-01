/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <Q3CanvasPixmap>
#include "MatrixMover.h"

#include "base/BaseProperties.h"
#include <klocale.h>
#include <kstandarddirs.h>
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "commands/matrix/MatrixModifyCommand.h"
#include "commands/matrix/MatrixInsertionCommand.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "gui/general/EditTool.h"
#include "gui/general/RosegardenCanvasView.h"
#include "MatrixElement.h"
#include "MatrixStaff.h"
#include "MatrixTool.h"
#include "MatrixView.h"
#include "MatrixVLayout.h"
#include <kaction.h>
#include <kglobal.h>
#include <QIcon>
#include <QPoint>
#include <QString>
#include "misc/Debug.h"


namespace Rosegarden
{

MatrixMover::MatrixMover(MatrixView* parent) :
    MatrixTool("MatrixMover", parent),
    m_currentElement(0),
    m_currentStaff(0),
    m_lastPlayedPitch(-1)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    Q3CanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIcon icon = QIcon(pixmap);

    new KAction(i18n("Switch to Select Tool"), icon, Key_F2, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Draw Tool"), "pencil", Key_F3, this,
                SLOT(slotDrawSelected()), actionCollection(),
                "draw");

    new KAction(i18n("Switch to Erase Tool"), "eraser", Key_F4, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    pixmap.load(pixmapDir + "/toolbar/resize.xpm");
    icon = QIcon(pixmap);
    new KAction(i18n("Switch to Resize Tool"), icon, Key_F6, this,
                SLOT(slotResizeSelected()), actionCollection(),
                "resize");

    createMenu("matrixmover.rc");
}

void MatrixMover::handleEventRemoved(Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
        m_currentElement = 0;
    }
}

void MatrixMover::handleLeftButtonPress(timeT time,
                                        int pitch,
                                        int staffNo,
                                        QMouseEvent* e,
                                        ViewElement* el)
{
    MATRIX_DEBUG << "MatrixMover::handleLeftButtonPress() : time = " << time << ", el = " << el << endl;
    if (!el) return;

    m_quickCopy = (e->state() & Qt::ControlModifier);

    if (!m_duplicateElements.empty()) {
        for (size_t i = 0; i < m_duplicateElements.size(); ++i) {
            delete m_duplicateElements[i]->event();
            delete m_duplicateElements[i];
        }
        m_duplicateElements.clear();
    }

    m_currentElement = dynamic_cast<MatrixElement*>(el);
    m_currentStaff = m_mParentView->getStaff(staffNo);

    if (m_currentElement) {

        // Add this element and allow movement
        //
        EventSelection* selection = m_mParentView->getCurrentSelection();

        if (selection) {
            EventSelection *newSelection;

            if ((e->state() & Qt::ShiftModifier) ||
                    selection->contains(m_currentElement->event()))
                newSelection = new EventSelection(*selection);
            else
                newSelection = new EventSelection(m_currentStaff->getSegment());

            // if the selection already contains the event, remove it from the
            // selection if shift is pressed
            if (selection->contains(m_currentElement->event())){
                if (e->state() & Qt::ShiftModifier)
                    newSelection->removeEvent(m_currentElement->event());
            } else {
                newSelection->addEvent(m_currentElement->event());
            }
            m_mParentView->setCurrentSelection(newSelection, true, true);
            m_mParentView->canvas()->update();
            selection = newSelection;
        } else {
            m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
                                                  m_currentElement->event(),
                                                  true);
            m_mParentView->canvas()->update();
        }

        long velocity = m_mParentView->getCurrentVelocity();
        m_currentElement->event()->get<Int>(BaseProperties::VELOCITY, velocity);
        m_mParentView->playNote(m_currentStaff->getSegment(), pitch, velocity);
        m_lastPlayedPitch = pitch;

        if (m_quickCopy && selection) {
            for (EventSelection::eventcontainer::iterator i =
                     selection->getSegmentEvents().begin();
                 i != selection->getSegmentEvents().end(); ++i) {

                MatrixElement *element = m_currentStaff->getElement(*i);
                if (!element) continue;

                MatrixElement *duplicate = new MatrixElement
                    (new Event(**i), m_mParentView->isDrumMode());
                duplicate->setLayoutY(element->getLayoutY());
                duplicate->setLayoutX(element->getLayoutX());
                duplicate->setWidth(element->getWidth());
                duplicate->setHeight(element->getHeight());
                duplicate->setCanvasZ(-1);
                m_currentStaff->positionElement(duplicate);
                m_duplicateElements.push_back(duplicate);
            }
        }
    }
    
    m_clickX = m_mParentView->inverseMapPoint(e->pos()).x();
}

timeT
MatrixMover::getDragTime(QMouseEvent *e, timeT candidate)
{
    int x = m_mParentView->inverseMapPoint(e->pos()).x();
    int xdiff = x - m_clickX;

    const SnapGrid &grid = getSnapGrid();
    const RulerScale &scale = *grid.getRulerScale();
    
    timeT eventTime = m_currentElement->getViewAbsoluteTime();
    int eventX = scale.getXForTime(eventTime);
    timeT preSnapTarget = scale.getTimeForX(eventX + xdiff);
    
    candidate = grid.snapTime(preSnapTarget, SnapGrid::SnapEither);
    
    if (xdiff == 0 ||
        (abs(eventTime - preSnapTarget) < abs(candidate - preSnapTarget))) {
        candidate = eventTime;
    }

    return candidate;
}

int MatrixMover::handleMouseMove(timeT newTime,
                                 int newPitch,
                                 QMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixMover::handleMouseMove() time = "
    << newTime << endl;

    if (e) {
        setBasicContextHelp(e->state() & Qt::ControlModifier);
    }

    if (!m_currentElement || !m_currentStaff)
        return RosegardenCanvasView::NoFollow;

    if (getSnapGrid().getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(i18n("Hold Shift to avoid snapping to beat grid"));
    } else {
        clearContextHelp();
    }

    if (e) newTime = getDragTime(e, newTime);

    emit hoveredOverNoteChanged(newPitch, true, newTime);

    using BaseProperties::PITCH;
    int diffPitch = 0;
    if (m_currentElement->event()->has(PITCH)) {
        diffPitch = newPitch - m_currentElement->event()->get<Int>(PITCH);
    }

    int diffY =
        int(((m_currentStaff->getLayoutYForHeight(newPitch) -
              m_currentStaff->getElementHeight() / 2) -
             m_currentElement->getLayoutY()));

    EventSelection* selection = m_mParentView->getCurrentSelection();
    EventSelection::eventcontainer::iterator it =
        selection->getSegmentEvents().begin();

    MatrixElement *element = 0;
    int maxY = m_currentStaff->getCanvasYForHeight(0);

    for (; it != selection->getSegmentEvents().end(); it++) {
        element = m_currentStaff->getElement(*it);

        if (element) {

            timeT diffTime = element->getViewAbsoluteTime() -
                m_currentElement->getViewAbsoluteTime();

            int newX = getSnapGrid().getRulerScale()->
                getXForTime(newTime + diffTime);

            if (newX < 0) newX = 0;

            int newY = int(element->getLayoutY() + diffY);

            if (newY < 0) newY = 0;
            if (newY > maxY) newY = maxY;

            element->setLayoutX(newX);
            element->setLayoutY(newY);

            m_currentStaff->positionElement(element);
        }
    }

    if (newPitch != m_lastPlayedPitch) {
        long velocity = m_mParentView->getCurrentVelocity();
        m_currentElement->event()->get<Int>(BaseProperties::VELOCITY, velocity);
        m_mParentView->playNote(m_currentStaff->getSegment(), newPitch, velocity);
        m_lastPlayedPitch = newPitch;
    }

    m_mParentView->canvas()->update();
    return RosegardenCanvasView::FollowHorizontal |
           RosegardenCanvasView::FollowVertical;
}

void MatrixMover::handleMouseRelease(timeT newTime,
                                     int newPitch,
                                     QMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixMover::handleMouseRelease() - newPitch = "
    << newPitch << endl;

    if (!m_currentElement || !m_currentStaff)
        return;

    if (newPitch > MatrixVLayout::maxMIDIPitch)
        newPitch = MatrixVLayout::maxMIDIPitch;
    if (newPitch < 0)
        newPitch = 0;

    if (e) newTime = getDragTime(e, newTime);

    using BaseProperties::PITCH;
    timeT diffTime = newTime - m_currentElement->getViewAbsoluteTime();
    int diffPitch = 0;
    if (m_currentElement->event()->has(PITCH)) {
        diffPitch = newPitch - m_currentElement->event()->get<Int>(PITCH);
    }

    EventSelection *selection = m_mParentView->getCurrentSelection();

    if ((diffTime == 0 && diffPitch == 0) || selection->getAddedEvents() == 0) {
        for (size_t i = 0; i < m_duplicateElements.size(); ++i) {
            delete m_duplicateElements[i]->event();
            delete m_duplicateElements[i];
        }
        m_duplicateElements.clear();
        m_mParentView->canvas()->update();
        m_currentElement = 0;
        return;
    }

    if (newPitch != m_lastPlayedPitch) {
        long velocity = m_mParentView->getCurrentVelocity();
        m_currentElement->event()->get<Int>(BaseProperties::VELOCITY, velocity);
        m_mParentView->playNote(m_currentStaff->getSegment(), newPitch, velocity);
        m_lastPlayedPitch = newPitch;
    }

    QString commandLabel;
    if (m_quickCopy) {
        if (selection->getAddedEvents() < 2) {
            commandLabel = i18n("Copy and Move Event");
        } else {
            commandLabel = i18n("Copy and Move Events");
        }
    } else {
        if (selection->getAddedEvents() < 2) {
            commandLabel = i18n("Move Event");
        } else {
            commandLabel = i18n("Move Events");
        }
    }

    MacroCommand *macro = new MacroCommand(commandLabel);

    EventSelection::eventcontainer::iterator it =
        selection->getSegmentEvents().begin();

    Segment &segment = m_currentStaff->getSegment();

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
            timeT limit = getSnapGrid().snapTime
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
    
    m_mParentView->setCurrentSelection(0, false, false);
    m_mParentView->addCommandToHistory(macro);
    m_mParentView->setCurrentSelection(newSelection, false, false);

    m_mParentView->canvas()->update();
    m_currentElement = 0;

    setBasicContextHelp();
}

void MatrixMover::ready()
{
    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));
    connect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
            m_mParentView, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));
    m_mParentView->setCanvasCursor(Qt::sizeAllCursor);
    setBasicContextHelp();
}

void MatrixMover::stow()
{
    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));
    disconnect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
               m_mParentView, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));
}

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
    int newPitch = m_currentStaff->getHeightAtCanvasCoords(p.x(), p.y());

    handleMouseMove(newTime, newPitch, 0);
}

void MatrixMover::setBasicContextHelp(bool ctrlPressed)
{
    EventSelection *selection = m_mParentView->getCurrentSelection();
    if (!selection || selection->getAddedEvents() < 2) {
        if (!ctrlPressed) {
            setContextHelp(i18n("Click and drag to move a note; hold Ctrl as well to copy it"));
        } else {
            setContextHelp(i18n("Click and drag to copy a note"));
        }
    } else {
        if (!ctrlPressed) {
            setContextHelp(i18n("Click and drag to move selected notes; hold Ctrl as well to copy"));
        } else {
            setContextHelp(i18n("Click and drag to copy selected notes"));
        }
    }
}

const QString MatrixMover::ToolName = "mover";

}
#include "MatrixMover.moc"
