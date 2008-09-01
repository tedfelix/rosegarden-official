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


#include <Q3CanvasItemList>
#include <Q3CanvasRectangle>
#include "NotationSelector.h"
#include "misc/Debug.h"

#include <klocale.h>
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/Selection.h"
#include "base/ViewElement.h"
#include "base/BaseProperties.h"
#include "commands/edit/MoveAcrossSegmentsCommand.h"
#include "commands/edit/MoveCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/notation/IncrementDisplacementsCommand.h"
#include "gui/general/EditTool.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/LinedStaff.h"
#include "gui/general/RosegardenCanvasView.h"
#include "gui/kdeext/QCanvasSimpleSprite.h"
#include "NotationElement.h"
#include "NotationProperties.h"
#include "NotationStaff.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"
#include <kaction.h>
#include <QApplication>
#include <QIcon>
#include <QRect>
#include <QString>
#include <QTimer>


namespace Rosegarden
{
 
using namespace BaseProperties;

NotationSelector::NotationSelector(NotationView* view)
        : NotationTool("NotationSelector", view),
        m_selectionRect(0),
        m_updateRect(false),
        m_selectedStaff(0),
        m_clickedElement(0),
        m_selectionToMerge(0),
        m_justSelectedBar(false),
        m_wholeStaffSelectionComplete(false)
{
    connect(m_parentView, SIGNAL(usedSelection()),
            this, SLOT(slotHideSelection()));

    connect(this, SIGNAL(editElement(NotationStaff *, NotationElement *, bool)),
            m_parentView, SLOT(slotEditElement(NotationStaff *, NotationElement *, bool)));

    QIcon icon
    (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                  makeToolbarPixmap("crotchet")));
    new KToggleAction(i18n("Switch to Insert Tool"), icon, 0, this,
                      SLOT(slotInsertSelected()), actionCollection(),
                      "insert");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    // (this crashed, and it might be superfluous with ^N anyway, so I'm
    // commenting it out, but leaving it here in case I change my mind about
    // fooling with it.)  (DMM)
    //    new KAction(i18n("Normalize Rests"), 0, 0, this,
    //                SLOT(slotCollapseRests()), actionCollection(),
    //                "collapse_rests");

    new KAction(i18n("Collapse Rests"), 0, 0, this,
                SLOT(slotCollapseRestsHard()), actionCollection(),
                "collapse_rests_aggressively");

    new KAction(i18n("Respell as Flat"), 0, 0, this,
                SLOT(slotRespellFlat()), actionCollection(),
                "respell_flat");

    new KAction(i18n("Respell as Sharp"), 0, 0, this,
                SLOT(slotRespellSharp()), actionCollection(),
                "respell_sharp");

    new KAction(i18n("Respell as Natural"), 0, 0, this,
                SLOT(slotRespellNatural()), actionCollection(),
                "respell_natural");

    new KAction(i18n("Collapse Notes"), 0, 0, this,
                SLOT(slotCollapseNotes()), actionCollection(),
                "collapse_notes");

    new KAction(i18n("Interpret"), 0, 0, this,
                SLOT(slotInterpret()), actionCollection(),
                "interpret");

    new KAction(i18n("Move to Staff Above"), 0, 0, this,
                SLOT(slotStaffAbove()), actionCollection(),
                "move_events_up_staff");

    new KAction(i18n("Move to Staff TicksBelow"), 0, 0, this,
                SLOT(slotStaffBelow()), actionCollection(),
                "move_events_down_staff");

    new KAction(i18n("Make Invisible"), 0, 0, this,
                SLOT(slotMakeInvisible()), actionCollection(),
                "make_invisible");

    new KAction(i18n("Make Visible"), 0, 0, this,
                SLOT(slotMakeVisible()), actionCollection(),
                "make_visible");

    createMenu("notationselector.rc");
}

NotationSelector::~NotationSelector()
{
    delete m_selectionToMerge;
}

void NotationSelector::handleLeftButtonPress(timeT t,
        int height,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    std::cerr << "NotationSelector::handleMousePress: time is " << t << ", staffNo is "
              << staffNo << ", e and element are " << e << " and " << element << std::endl;

    if (m_justSelectedBar) {
        handleMouseTripleClick(t, height, staffNo, e, element);
        m_justSelectedBar = false;
        return ;
    }

    m_wholeStaffSelectionComplete = false;

    delete m_selectionToMerge;
    const EventSelection *selectionToMerge = 0;
    if (e->state() & ShiftButton) {
        m_clickedShift = true;
        selectionToMerge = m_nParentView->getCurrentSelection();
    } else {
        m_clickedShift = false;
    }
    m_selectionToMerge =
        (selectionToMerge ? new EventSelection(*selectionToMerge) : 0);

    m_clickedElement = dynamic_cast<NotationElement*>(element);
    if (m_clickedElement) {
        m_selectedStaff = getStaffForElement(m_clickedElement);
        m_lastDragPitch = -400;
        m_lastDragTime = m_clickedElement->event()->getNotationAbsoluteTime();
    } else {
        m_selectedStaff = 0; // don't know yet; wait until we have an element
    }

    m_selectionRect->setX(e->x());
    m_selectionRect->setY(e->y());
    m_selectionRect->setSize(0, 0);

    m_selectionRect->show();
    m_updateRect = true;
    m_startedFineDrag = false;

    //m_parentView->setCursorPosition(p.x());
}

void NotationSelector::handleRightButtonPress(timeT t,
        int height,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    std::cerr << "NotationSelector::handleRightButtonPress" << std::endl;

    const EventSelection *sel = m_nParentView->getCurrentSelection();

    if (!sel || sel->getSegmentEvents().empty()) {

        // if nothing selected, permit the possibility of selecting
        // something before showing the menu

        if (element) {
            m_clickedElement = dynamic_cast<NotationElement*>(element);
            m_selectedStaff = getStaffForElement(m_clickedElement);
            m_nParentView->setSingleSelectedEvent
                (m_selectedStaff->getId(), m_clickedElement->event(),
                 true, true);
        }

        handleLeftButtonPress(t, height, staffNo, e, element);
    }

    EditTool::handleRightButtonPress(t, height, staffNo, e, element);
}

void NotationSelector::slotClickTimeout()
{
    m_justSelectedBar = false;
}

void NotationSelector::handleMouseDoubleClick(timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    NOTATION_DEBUG << "NotationSelector::handleMouseDoubleClick" << endl;
    m_clickedElement = dynamic_cast<NotationElement*>(element);

    NotationStaff *staff = m_nParentView->getNotationStaff(staffNo);
    if (!staff)
        return ;
    m_selectedStaff = staff;

    bool advanced = (e->state() & ShiftButton);

    if (m_clickedElement) {

        emit editElement(staff, m_clickedElement, advanced);

    } else {

        QRect rect = staff->getBarExtents(e->x(), e->y());

        m_selectionRect->setX(rect.x() + 1);
        m_selectionRect->setY(rect.y());
        m_selectionRect->setSize(rect.width() - 1, rect.height());

        m_selectionRect->show();
        m_updateRect = false;

        m_justSelectedBar = true;
        QTimer::singleShot(QApplication::doubleClickInterval(), this,
                           SLOT(slotClickTimeout()));
    }

    return ;
}

void NotationSelector::handleMouseTripleClick(timeT t,
        int height,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    if (!m_justSelectedBar)
        return ;
    m_justSelectedBar = false;

    NOTATION_DEBUG << "NotationSelector::handleMouseTripleClick" << endl;
    m_clickedElement = dynamic_cast<NotationElement*>(element);

    NotationStaff *staff = m_nParentView->getNotationStaff(staffNo);
    if (!staff)
        return ;
    m_selectedStaff = staff;

    if (m_clickedElement) {

        // should be safe, as we've already set m_justSelectedBar false
        handleLeftButtonPress(t, height, staffNo, e, element);
        return ;

    } else {

        m_selectionRect->setX(staff->getX());
        m_selectionRect->setY(staff->getY());
        m_selectionRect->setSize(int(staff->getTotalWidth()) - 1,
                                 staff->getTotalHeight() - 1);

        m_selectionRect->show();
        m_updateRect = false;
    }

    m_wholeStaffSelectionComplete = true;

    return ;
}

int NotationSelector::handleMouseMove(timeT, int,
                                      QMouseEvent* e)
{
    if (!m_updateRect)
        return RosegardenCanvasView::NoFollow;

    int w = int(e->x() - m_selectionRect->x());
    int h = int(e->y() - m_selectionRect->y());

    if (m_clickedElement /* && !m_clickedElement->isRest() */) {

        if (m_startedFineDrag) {
            dragFine(e->x(), e->y(), false);
        } else if (m_clickedShift) {
            if (w > 2 || w < -2 || h > 2 || h < -2) {
                dragFine(e->x(), e->y(), false);
            }
        } else if (w > 3 || w < -3 || h > 3 || h < -3) {
            drag(e->x(), e->y(), false);
        }

    } else {

        // Qt rectangle dimensions appear to be 1-based
        if (w > 0)
            ++w;
        else
            --w;
        if (h > 0)
            ++h;
        else
            --h;

        m_selectionRect->setSize(w, h);
        setViewCurrentSelection(true);
        m_nParentView->canvas()->update();
    }

    return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
}

void NotationSelector::handleMouseRelease(timeT, int, QMouseEvent *e)
{
    NOTATION_DEBUG << "NotationSelector::handleMouseRelease" << endl;
    m_updateRect = false;

    NOTATION_DEBUG << "selectionRect width, height: " << m_selectionRect->width()
    << ", " << m_selectionRect->height() << endl;

    // Test how far we've moved from the original click position -- not
    // how big the rectangle is (if we were dragging an event, the
    // rectangle size will still be zero).

    if (((e->x() - m_selectionRect->x()) > -3 &&
            (e->x() - m_selectionRect->x()) < 3 &&
            (e->y() - m_selectionRect->y()) > -3 &&
            (e->y() - m_selectionRect->y()) < 3) &&
            !m_startedFineDrag) {

        if (m_clickedElement != 0 && m_selectedStaff) {
            
            // If we didn't drag out a meaningful area, but _did_
            // click on an individual event, then select just that
            // event

            if (m_selectionToMerge &&
                    m_selectionToMerge->getSegment() ==
                    m_selectedStaff->getSegment()) {

                // if the event was already part of the selection, we want to
                // remove it
                if (m_selectionToMerge->contains(m_clickedElement->event())) {
                    m_selectionToMerge->removeEvent(m_clickedElement->event());
                } else {
                    m_selectionToMerge->addEvent(m_clickedElement->event());
                }
                
                m_nParentView->setCurrentSelection(m_selectionToMerge,
                                                   true, true);
                m_selectionToMerge = 0;

            } else {

                m_nParentView->setSingleSelectedEvent
                (m_selectedStaff->getId(), m_clickedElement->event(),
                 true, true);
            }
            /*
                    } else if (m_selectedStaff) {
             
                        // If we clicked on no event but on a staff, move the
                        // insertion cursor to the point where we clicked. 
                        // Actually we only really want this to happen if
                        // we aren't double-clicking -- consider using a timer
                        // to establish whether a double-click is going to happen
             
                        m_nParentView->slotSetInsertCursorPosition(e->x(), (int)e->y());
            */
        } else {
            setViewCurrentSelection(false);
        }

    } else {

        if (m_startedFineDrag) {
            dragFine(e->x(), e->y(), true);
        } else if (m_clickedElement /* && !m_clickedElement->isRest() */) {
            drag(e->x(), e->y(), true);
        } else {
            setViewCurrentSelection(false);
        }
    }

    m_clickedElement = 0;
    m_selectionRect->hide();
    m_wholeStaffSelectionComplete = false;
    m_nParentView->canvas()->update();
}

void NotationSelector::drag(int x, int y, bool final)
{
    NOTATION_DEBUG << "NotationSelector::drag " << x << ", " << y << endl;

    if (!m_clickedElement || !m_selectedStaff)
        return ;

    EventSelection *selection = m_nParentView->getCurrentSelection();
    if (!selection || !selection->contains(m_clickedElement->event())) {
        selection = new EventSelection(m_selectedStaff->getSegment());
        selection->addEvent(m_clickedElement->event());
    }
    m_nParentView->setCurrentSelection(selection);

    LinedStaff *targetStaff = m_nParentView->getStaffForCanvasCoords(x, y);
    if (!targetStaff)
        targetStaff = m_selectedStaff;

    // Calculate time and height

    timeT clickedTime = m_clickedElement->event()->getNotationAbsoluteTime();

    Accidental clickedAccidental = Accidentals::NoAccidental;
    (void)m_clickedElement->event()->get<String>(ACCIDENTAL, clickedAccidental);

    long clickedPitch = 0;
    (void)m_clickedElement->event()->get<Int>(PITCH, clickedPitch);

    long clickedHeight = 0;
    (void)m_clickedElement->event()->get<Int>
    (NotationProperties::HEIGHT_ON_STAFF, clickedHeight);

    Event *clefEvt = 0, *keyEvt = 0;
    Clef clef;
    ::Rosegarden::Key key;

    timeT dragTime = clickedTime;
    double layoutX = m_clickedElement->getLayoutX();
    timeT duration = m_clickedElement->getViewDuration();

    NotationElementList::iterator itr =
        targetStaff->getElementUnderCanvasCoords(x, y, clefEvt, keyEvt);

    if (itr != targetStaff->getViewElementList()->end()) {

        NotationElement *elt = dynamic_cast<NotationElement *>(*itr);
        dragTime = elt->getViewAbsoluteTime();
        layoutX = elt->getLayoutX();

        if (elt->isRest() && duration > 0 && elt->getCanvasItem()) {

            double restX = 0, restWidth = 0;
            elt->getCanvasAirspace(restX, restWidth);

            timeT restDuration = elt->getViewDuration();

            if (restWidth > 0 &&
                    restDuration >= duration * 2) {

                int parts = restDuration / duration;
                double encroachment = x - restX;
                NOTATION_DEBUG << "encroachment is " << encroachment << ", restWidth is " << restWidth << endl;
                int part = (int)((encroachment / restWidth) * parts);
                if (part >= parts)
                    part = parts - 1;

                dragTime += part * restDuration / parts;
                layoutX += part * restWidth / parts +
                           (restX - elt->getCanvasX());
            }
        }
    }

    if (clefEvt)
        clef = Clef(*clefEvt);
    if (keyEvt)
        key = ::Rosegarden::Key(*keyEvt);

    int height = targetStaff->getHeightAtCanvasCoords(x, y);
    int pitch = clickedPitch;

    if (height != clickedHeight)
        pitch =
            Pitch
            (height, clef, key, clickedAccidental).getPerformancePitch();

    if (pitch < clickedPitch) {
        if (height < -10) {
            height = -10;
            pitch = Pitch
                    (height, clef, key, clickedAccidental).getPerformancePitch();
        }
    } else if (pitch > clickedPitch) {
        if (height > 18) {
            height = 18;
            pitch = Pitch
                    (height, clef, key, clickedAccidental).getPerformancePitch();
        }
    }

    bool singleNonNotePreview = !m_clickedElement->isNote() &&
                                selection->getSegmentEvents().size() == 1;

    if (!final && !singleNonNotePreview) {

        if ((pitch != m_lastDragPitch || dragTime != m_lastDragTime) &&
            m_clickedElement->isNote()) {

            m_nParentView->showPreviewNote(targetStaff->getId(),
                                           layoutX, pitch, height,
                                           Note::getNearestNote(duration),
                                           m_clickedElement->isGrace());
            m_lastDragPitch = pitch;
            m_lastDragTime = dragTime;
        }

    } else {

        m_nParentView->clearPreviewNote();

        MacroCommand *command = new MacroCommand(MoveCommand::getGlobalName());
        bool haveSomething = false;

        MoveCommand *mc = 0;
        Event *lastInsertedEvent = 0;

        if (pitch != clickedPitch && m_clickedElement->isNote()) {
            command->addCommand(new TransposeCommand(pitch - clickedPitch,
                                *selection));
            haveSomething = true;
        }

        if (targetStaff != m_selectedStaff) {
            command->addCommand(new MoveAcrossSegmentsCommand
                                (m_selectedStaff->getSegment(),
                                 targetStaff->getSegment(),
                                 dragTime - clickedTime + selection->getStartTime(),
                                 true,
                                 *selection));
            haveSomething = true;
        } else {
            if (dragTime != clickedTime) {
                mc = new MoveCommand
                     (m_selectedStaff->getSegment(),  //!!!sort
                      dragTime - clickedTime, true, *selection);
                command->addCommand(mc);
                haveSomething = true;
            }
        }

        if (haveSomething) {

            m_nParentView->addCommandToHistory(command);

            if (mc && singleNonNotePreview) {

                lastInsertedEvent = mc->getLastInsertedEvent();

                if (lastInsertedEvent) {
                    m_nParentView->setSingleSelectedEvent(targetStaff->getId(),
                                                          lastInsertedEvent);

                    ViewElementList::iterator vli =
                        targetStaff->findEvent(lastInsertedEvent);

                    if (vli != targetStaff->getViewElementList()->end()) {
                        m_clickedElement = dynamic_cast<NotationElement *>(*vli);
                    } else {
                        m_clickedElement = 0;
                    }

                    m_selectionRect->setX(x);
                    m_selectionRect->setY(y);
                }
            }
        } else {
            delete command;
        }
    }
}

void NotationSelector::dragFine(int x, int y, bool final)
{
    NOTATION_DEBUG << "NotationSelector::drag " << x << ", " << y << endl;

    if (!m_clickedElement || !m_selectedStaff)
        return ;

    EventSelection *selection = m_nParentView->getCurrentSelection();
    if (!selection)
        selection = new EventSelection(m_selectedStaff->getSegment());
    if (!selection->contains(m_clickedElement->event()))
        selection->addEvent(m_clickedElement->event());
    m_nParentView->setCurrentSelection(selection);

    // Fine drag modifies the DISPLACED_X and DISPLACED_Y properties on
    // each event.  The modifications have to be relative to the previous
    // values of these properties, not to zero, so for each event we need
    // to store the previous value at the time the drag starts.

    static PropertyName xProperty("temporary-displaced-x");
    static PropertyName yProperty("temporary-displaced-y");

    if (!m_startedFineDrag) {
        // back up original properties

        for (EventSelection::eventcontainer::iterator i =
                    selection->getSegmentEvents().begin();
                i != selection->getSegmentEvents().end(); ++i) {
            long prevX = 0, prevY = 0;
            (*i)->get
            <Int>(DISPLACED_X, prevX);
            (*i)->get
            <Int>(DISPLACED_Y, prevY);
            (*i)->setMaybe<Int>(xProperty, prevX);
            (*i)->setMaybe<Int>(yProperty, prevY);
        }

        m_startedFineDrag = true;
    }

    // We want the displacements in 1/1000ths of a staff space

    double dx = x - m_selectionRect->x();
    double dy = y - m_selectionRect->y();

    double noteBodyWidth = m_nParentView->getNotePixmapFactory()->getNoteBodyWidth();
    double lineSpacing = m_nParentView->getNotePixmapFactory()->getLineSpacing();
    dx = (1000.0 * dx) / noteBodyWidth;
    dy = (1000.0 * dy) / lineSpacing;

    if (final) {

        // reset original values (and remove backup values) before
        // applying command

        for (EventSelection::eventcontainer::iterator i =
                    selection->getSegmentEvents().begin();
                i != selection->getSegmentEvents().end(); ++i) {
            long prevX = 0, prevY = 0;
            (*i)->get
            <Int>(xProperty, prevX);
            (*i)->get
            <Int>(yProperty, prevY);
            (*i)->setMaybe<Int>(DISPLACED_X, prevX);
            (*i)->setMaybe<Int>(DISPLACED_Y, prevY);
            (*i)->unset(xProperty);
            (*i)->unset(yProperty);
        }

        IncrementDisplacementsCommand *command = new IncrementDisplacementsCommand
                (*selection, long(dx), long(dy));
        m_nParentView->addCommandToHistory(command);

    } else {

        timeT startTime = 0, endTime = 0;

        for (EventSelection::eventcontainer::iterator i =
                    selection->getSegmentEvents().begin();
                i != selection->getSegmentEvents().end(); ++i) {
            long prevX = 0, prevY = 0;
            (*i)->get
            <Int>(xProperty, prevX);
            (*i)->get
            <Int>(yProperty, prevY);
            (*i)->setMaybe<Int>(DISPLACED_X, prevX + long(dx));
            (*i)->setMaybe<Int>(DISPLACED_Y, prevY + long(dy));
            if (i == selection->getSegmentEvents().begin()) {
                startTime = (*i)->getAbsoluteTime();
            }
            endTime = (*i)->getAbsoluteTime() + (*i)->getDuration();
        }

        if (startTime == endTime)
            ++endTime;
        selection->getSegment().updateRefreshStatuses(startTime, endTime);
        m_nParentView->update();
    }
}

void NotationSelector::ready()
{
    m_selectionRect = new Q3CanvasRectangle(m_nParentView->canvas());

    m_selectionRect->hide();
    m_selectionRect->setPen(GUIPalette::getColour(GUIPalette::SelectionRectangle));

    m_nParentView->setCanvasCursor(Qt::arrowCursor);
    m_nParentView->setHeightTracking(false);
}

void NotationSelector::stow()
{
    delete m_selectionRect;
    m_selectionRect = 0;
    m_nParentView->canvas()->update();
}

void NotationSelector::slotHideSelection()
{
    if (!m_selectionRect)
        return ;
    m_selectionRect->hide();
    m_selectionRect->setSize(0, 0);
    m_nParentView->canvas()->update();
}

void NotationSelector::slotInsertSelected()
{
    m_nParentView->slotLastNoteAction();
}

void NotationSelector::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void NotationSelector::slotCollapseRestsHard()
{
    m_parentView->actionCollection()->action("collapse_rests_aggressively")->activate();
}

void NotationSelector::slotRespellFlat()
{
    m_parentView->actionCollection()->action("respell_flat")->activate();
}

void NotationSelector::slotRespellSharp()
{
    m_parentView->actionCollection()->action("respell_sharp")->activate();
}

void NotationSelector::slotRespellNatural()
{
    m_parentView->actionCollection()->action("respell_natural")->activate();
}

void NotationSelector::slotCollapseNotes()
{
    m_parentView->actionCollection()->action("collapse_notes")->activate();
}

void NotationSelector::slotInterpret()
{
    m_parentView->actionCollection()->action("interpret")->activate();
}

void NotationSelector::slotStaffAbove()
{
    m_parentView->actionCollection()->action("move_events_up_staff")->activate();
}

void NotationSelector::slotStaffBelow()
{
    m_parentView->actionCollection()->action("move_events_down_staff")->activate();
}

void NotationSelector::slotMakeInvisible()
{
    m_parentView->actionCollection()->action("make_invisible")->activate();
}

void NotationSelector::slotMakeVisible()
{
    m_parentView->actionCollection()->action("make_visible")->activate();
}

void NotationSelector::setViewCurrentSelection(bool preview)
{
    EventSelection *selection = getSelection();

    if (m_selectionToMerge) {
        if (selection &&
                m_selectionToMerge->getSegment() == selection->getSegment()) {
            selection->addFromSelection(m_selectionToMerge);
        } else {
            return ;
        }
    }

    m_nParentView->setCurrentSelection(selection, preview, true);
}

NotationStaff *
NotationSelector::getStaffForElement(NotationElement *elt)
{
    for (int i = 0; i < m_nParentView->getStaffCount(); ++i) {
        NotationStaff *staff = m_nParentView->getNotationStaff(i);
        if (staff->getSegment().findSingle(elt->event()) !=
                staff->getSegment().end())
            return staff;
    }
    return 0;
}

EventSelection* NotationSelector::getSelection()
{
    // If selection rect is not visible or too small,
    // return 0
    //
    if (!m_selectionRect->visible()) return 0;

    //    NOTATION_DEBUG << "Selection x,y: " << m_selectionRect->x() << ","
    //                         << m_selectionRect->y() << "; w,h: " << m_selectionRect->width() << "," << m_selectionRect->height() << endl;

    if (m_selectionRect->width()  > -3 &&
        m_selectionRect->width()  <  3 &&
        m_selectionRect->height() > -3 &&
        m_selectionRect->height() <  3) return 0;

    Q3CanvasItemList itemList = m_selectionRect->collisions(false);
    Q3CanvasItemList::Iterator it;

    QRect rect = m_selectionRect->rect().normalize();
    QCanvasNotationSprite *sprite = 0;

    if (!m_selectedStaff) {

        // Scan the list of collisions, looking for a valid notation
        // element; if we find one, initialise m_selectedStaff from it.
        // If we don't find one, we have no selection.  This is a little
        // inefficient but we only do it for the first event in the
        // selection.

        for (it = itemList.begin(); it != itemList.end(); ++it) {
            
            if ((sprite = dynamic_cast<QCanvasNotationSprite*>(*it))) {

            NotationElement &el = sprite->getNotationElement();

            NotationStaff *staff = getStaffForElement(&el);
            if (!staff) continue;
            
            int x = (int)(*it)->x();
            bool shifted = false;
            int nbw = staff->getNotePixmapFactory(false).getNoteBodyWidth();

            
            // #957364 (Notation: Hard to select upper note in
            // chords of seconds) -- adjust x-coord for shifted
            // note head
            if (el.event()->get<Rosegarden::Bool>
                (staff->getProperties().NOTE_HEAD_SHIFTED, shifted) && shifted) {
                x += nbw;
            }

            if (!rect.contains(x, int((*it)->y()), true)) {
                // #988217 (Notation: Special column of pixels
                // prevents sweep selection) -- for notes, test
                // again with centred x-coord
                if (!el.isNote() || !rect.contains(x + nbw/2, int((*it)->y()), true)) {
                    continue;
                }
            }
                
            m_selectedStaff = staff;
            break;
            }
        }
    }

    if (!m_selectedStaff) return 0;
    Segment& originalSegment = m_selectedStaff->getSegment();

    // If we selected the whole staff, force that to happen explicitly
    // rather than relying on collisions with the rectangle -- because
    // events way off the currently visible area might not even have
    // been drawn yet, and so will not appear in the collision list.
    // (We did still need the collision list to determine which staff
    // to use though.)
    
    if (m_wholeStaffSelectionComplete) {
    EventSelection *selection = new EventSelection(originalSegment,
                               originalSegment.getStartTime(),
                               originalSegment.getEndMarkerTime());
        return selection;
    }
    
    EventSelection* selection = new EventSelection(originalSegment);

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        if ((sprite = dynamic_cast<QCanvasNotationSprite*>(*it))) {

            NotationElement &el = sprite->getNotationElement();
            
            int x = (int)(*it)->x();
            bool shifted = false;
            int nbw = m_selectedStaff->getNotePixmapFactory(false).getNoteBodyWidth();

            // #957364 (Notation: Hard to select upper note in chords
            // of seconds) -- adjust x-coord for shifted note head
            if (el.event()->get<Rosegarden::Bool>
                            (m_selectedStaff->getProperties().NOTE_HEAD_SHIFTED, shifted)
                            && shifted) {
                x += nbw;
            }

            // check if the element's rect
            // is actually included in the selection rect.
            //
            if (!rect.contains(x, int((*it)->y()), true))  {
                // #988217 (Notation: Special column of pixels
                // prevents sweep selection) -- for notes, test again
                // with centred x-coord
                if (!el.isNote() || !rect.contains(x + nbw/2, int((*it)->y()), true)) {
                    continue;
                }
            }
                
            // must be in the same segment as we first started on,
            // we can't select events across multiple segments
            if (selection->getSegment().findSingle(el.event()) !=
            selection->getSegment().end()) {
                selection->addEvent(el.event());
            }
        }
    }

    if (selection->getAddedEvents() > 0) {
        return selection;
    } else {
        delete selection;
        return 0;
    }
}

const QString NotationSelector::ToolName = "notationselector";

}
#include "NotationSelector.moc"
