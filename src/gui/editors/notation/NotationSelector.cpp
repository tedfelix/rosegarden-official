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

#define RG_MODULE_STRING "[NotationSelector]"
#define RG_NO_DEBUG_PRINT 1

#include "NotationSelector.h"
#include "NotationElement.h"
#include "NotationProperties.h"
#include "NotationStaff.h"
#include "NotationTool.h"
#include "NotationWidget.h"
#include "NotePixmapFactory.h"
#include "NotationMouseEvent.h"
#include "NotationScene.h"

#include "misc/Debug.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Pitch.h"
#include "base/PropertyName.h"
#include "base/Selection.h"
#include "base/ViewElement.h"
#include "base/BaseProperties.h"
#include "base/Profiler.h"

#include "commands/edit/MoveAcrossSegmentsCommand.h"
#include "commands/edit/MoveCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/notation/IncrementDisplacementsCommand.h"

#include "gui/general/GUIPalette.h"
#include "gui/widgets/Panned.h"

#include "document/CommandHistory.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QRect>
#include <QString>
#include <QTimer>

namespace Rosegarden
{

using namespace BaseProperties;

NotationSelector::NotationSelector(NotationWidget *widget, bool ties) :
    NotationTool("notationselector.rc", "NotationSelector", widget),
    m_ties(ties)
{
    connect(this, &NotationSelector::editElement,
            m_widget, &NotationWidget::editElement);

    createAction("insert", SLOT(slotInsertSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("collapse_rests_aggressively", SLOT(slotCollapseRestsHard()));
    createAction("respell_flat", SLOT(slotRespellFlat()));
    createAction("respell_sharp", SLOT(slotRespellSharp()));
    createAction("respell_natural", SLOT(slotRespellNatural()));
    createAction("collapse_notes", SLOT(slotCollapseNotes()));
    createAction("interpret", SLOT(slotInterpret()));
    createAction("move_events_up_staff", SLOT(slotStaffAbove()));
    createAction("move_events_down_staff", SLOT(slotStaffBelow()));
    createAction("make_invisible", SLOT(slotMakeInvisible()));
    createAction("make_visible", SLOT(slotMakeVisible()));

    createMenu();

    m_releaseTimer = new QTimer(this);
    m_releaseTimer->setSingleShot(true);
    connect(m_releaseTimer, &QTimer::timeout,
            this, &NotationSelector::slotMoveInsertionCursor);
}

NotationSelector::~NotationSelector()
{
    delete m_selectionToMerge;
}

void
NotationSelector::handleLeftButtonPress(const NotationMouseEvent *e)
{
    m_doubleClick = false;
    m_tripleClick = false;

    if (m_justSelectedBar) {
        // It's a triple click
        handleMouseTripleClick(e);
        m_justSelectedBar = false;
        return ;
    }

    m_wholeStaffSelectionComplete = false;

    delete m_selectionToMerge;
    const EventSelection *selectionToMerge = nullptr;
    if (e->modifiers & Qt::ShiftModifier) {
        m_clickedShift = true;
        selectionToMerge = m_scene->getSelection();
    } else {
        m_clickedShift = false;
    }
//    std::cout << "NotationSelector::handleLeftButtonPress(): m_clickedShift == " << (m_clickedShift ? "true" : "false") << std::endl;
    m_selectionToMerge =
        (selectionToMerge ? new EventSelection(*selectionToMerge) : nullptr);

    m_selectedStaff = e->staff;
    m_clickedElement = nullptr;

    if (e->exact) {
        m_clickedElement = e->element;
        RG_DEBUG << "exact click" << *(m_clickedElement->event());
        if (m_clickedElement) {
            m_lastDragPitch = -400;
            m_lastDragTime = m_clickedElement->event()->getNotationAbsoluteTime();
        }
    }

    if (!m_selectionRect) {
        m_selectionRect = new QGraphicsRectItem;
        m_scene->addItem(m_selectionRect);
        QColor c = GUIPalette::getColour(GUIPalette::SelectionRectangle);
        m_selectionRect->setPen(QPen(c, 2));
        c.setAlpha(50);
        m_selectionRect->setBrush(c);
    }

    m_selectionOrigin = QPointF(e->sceneX, e->sceneY);
    m_selectionRect->setRect(QRectF(m_selectionOrigin, QSize()));
    m_selectionRect->hide();
    m_updateRect = true;
    m_startedFineDrag = false;
}

void
NotationSelector::handleRightButtonPress(const NotationMouseEvent *e)
{
    m_releaseTimer->stop();     // Useful here ???

    // if nothing selected, permit the possibility of selecting
    // something before showing the menu
    RG_DEBUG << "NotationSelector::handleRightButtonPress";

    const EventSelection *sel = m_scene->getSelection();

    if (!sel || sel->getSegmentEvents().empty()) {

        if (e->element) {
            m_clickedElement = e->element;
            m_selectedStaff = e->staff;
            m_scene->setSingleSelectedEvent
                (m_selectedStaff, m_clickedElement, true);
        }
    }

    NotationTool::handleRightButtonPress(e);
}

void NotationSelector::slotClickTimeout()
{
    m_justSelectedBar = false;
}

void NotationSelector::handleMouseDoubleClick(const NotationMouseEvent *e)
{
    m_releaseTimer->stop();      // Don't move insertion cursor
    m_doubleClick = true;

    RG_DEBUG << "NotationSelector::handleMouseDoubleClick";

    // Only double click on left mouse button is currently used (fix #1493)
    if (e->buttons != Qt::LeftButton) return;

    NotationStaff *staff = e->staff;
    if (!staff) return;
    m_selectedStaff = staff;

    if (e->element && e->exact) {

        emit editElement(staff, e->element);

    } else {

        // Select entire bar

        QRect rect = staff->getBarExtents(e->sceneX, e->sceneY);
        m_selectionRect->setRect(rect.x() + 0.5, rect.y() + 0.5,
                                 rect.width(), rect.height());
        m_selectionRect->show();
        m_updateRect = false;

        m_justSelectedBar = true;
        QTimer::singleShot(QApplication::doubleClickInterval(), this,
                           &NotationSelector::slotClickTimeout);
    }
}

void NotationSelector::handleMouseTripleClick(const NotationMouseEvent *e)
{
    RG_DEBUG << "NotationSelector::handleMouseTripleClick";

    if (!m_justSelectedBar) return;
    m_justSelectedBar = false;
    m_tripleClick = true;

    NotationStaff *staff = e->staff;
    if (!staff) return;
    m_selectedStaff = staff;

    if (e->element && e->exact) {

        // should be safe, as we've already set m_justSelectedBar false
        handleLeftButtonPress(e);
        return;

    } else {

        m_selectionRect->setRect(staff->getX(), staff->getY(),
                                 staff->getTotalWidth() - 1,
                                 staff->getTotalHeight() - 1);

        m_selectionRect->show();
        m_updateRect = false;
    }

    m_wholeStaffSelectionComplete = true;

    return;
}

FollowMode
NotationSelector::handleMouseMove(const NotationMouseEvent *e)
{
    RG_DEBUG << "handleMouseMove" << e;
    if (!m_updateRect) return NO_FOLLOW;

//    std::cout << "NotationSelector::handleMouseMove: staff is "
//              << m_selectedStaff << ", m_updateRect is " << m_updateRect
//              << std::endl;

    if (!m_selectedStaff) m_selectedStaff = e->staff;

    int w = int(e->sceneX - m_selectionOrigin.x());
    int h = int(e->sceneY - m_selectionOrigin.y());

    RG_DEBUG << "NotationSelector::handleMouseMove: "
                   << e->sceneX << "-" << m_selectionOrigin.x() << "=> w: " << w << " h: " << h;

    if (m_clickedElement /* && !m_clickedElement->isRest() */) {

        if ((w > 3 || w < -3 || h > 3 || h < -3) &&
            !m_clickedShift) {
//            std::cout << "Dragging from Code Point Bravo: w: " << w << " h: " << h << std::endl;
            drag(e->sceneX, e->sceneY, false);
        }

    } else {
        const QPointF p0(m_selectionOrigin);
        const QPointF p1(e->sceneX, e->sceneY);
        const QRectF r = QRectF(p0, p1).normalized();

        m_selectionRect->setRect(r.x() + 0.5, r.y() + 0.5, r.width(), r.height());
        m_selectionRect->show();

        setViewCurrentSelection(true);
    }

    return (FOLLOW_HORIZONTAL | FOLLOW_VERTICAL);
}

void NotationSelector::handleMouseRelease(const NotationMouseEvent *e)
{
    //RG_DEBUG << "NotationSelector::handleMouseRelease.";
    m_updateRect = false;

    // We can lose m_selectionRect since the click under some
    // conditions.
    if (!m_selectionRect) { return; }

    // Test how far we've moved from the original click position -- not
    // how big the rectangle is (if we were dragging an event, the
    // rectangle size will still be zero).

    int w = int(e->sceneX - m_selectionOrigin.x());
    int h = int(e->sceneY - m_selectionOrigin.y());

    //RG_DEBUG << "e->sceneX =" << e->sceneX << "sceneY =" << e->sceneY
    //         << "m_selectionOrigin =" << m_selectionOrigin
    //         << "w =" << w << "h =" << h << "m_startedFineDrag =" << m_startedFineDrag;

    if ((w > -3 && w < 3 && h > -3 && h < 3 && !m_startedFineDrag) ||
        (m_clickedShift)) {

        if (m_clickedElement != nullptr && m_selectedStaff) {

            // If we didn't drag out a meaningful area, but _did_
            // click on an individual event, then select just that
            // event

            if (m_selectionToMerge &&
                m_selectionToMerge->getSegment() == m_selectedStaff->getSegment()) {

                // if the event was already part of the selection, we want to
                // remove it
                if (m_selectionToMerge->contains(m_clickedElement->event())) {
                    m_selectionToMerge->removeEvent(m_clickedElement->event(),
                                                    m_ties);
                } else {
                    m_selectionToMerge->addEvent(m_clickedElement->event(),
                                                 m_ties);
                }

                m_scene->setSelection(m_selectionToMerge, true);
                m_selectionToMerge = nullptr;

            } else {
                RG_DEBUG << "setSingleSelectedEvent" <<
                    m_clickedElement->event()->getType() <<
                    m_clickedElement->event()->getAbsoluteTime();
                m_scene->setSingleSelectedEvent(m_selectedStaff, m_clickedElement, true);
            }

        } else {
            // Nothing selected or a group of events added to selection
            setViewCurrentSelection(false);
        }

    } else {

        if (m_clickedElement &&
            !m_clickedShift) {
            //RG_DEBUG << "Dragging from Code Point Foxtrot: w =" << w << "h =" << h;
            // drag() must be called here from Foxtrot, whether attempting to
            // click to select a note head or to click and drag a note.  It's
            // required in both cases, and neither case works without this call
            // here:
            drag(e->sceneX, e->sceneY, true);
        } else {
            // A group of events is selected
            setViewCurrentSelection(false);
        }
    }

    m_clickedElement = nullptr;
    m_selectionRect->hide();
    m_selectionOrigin = QPointF();
    m_wholeStaffSelectionComplete = false;

    // If we clicked on no event but on a staff, move the insertion cursor
    // to the point where we clicked. In such a case nothing has been selected.

    // If double or triple click, something is selected or the click was
    // outside any staff.
    if (m_doubleClick || m_tripleClick) return;

    // If simple click, look at clicked staff and current selection
    if (e->staff && !m_scene->getSelection()) {
        m_pointerStaff = e->staff;
        m_pointerTime = e->time;
        // Wait a possible double click before executing the code
        m_releaseTimer->start(QApplication::doubleClickInterval());
    }
}

void NotationSelector::slotMoveInsertionCursor()
{
    // Move the insertion cursor to the mouse pointer position.

    // We just clicked on a staff in the window, so no scroll is needed
    // and we don't want to see any move of the staves
    m_widget->setScroll(false);

    // ! Warning, this short-circuits NotationView::setCurrentStaff...
    m_scene->setCurrentStaff(m_pointerStaff);
    m_widget->setPointerPosition(m_pointerTime);

    m_widget->setScroll(true);
}

void NotationSelector::drag(int x, int y, bool final)
{
    RG_DEBUG << "NotationSelector::drag " << x << ", " << y;

    // ??? INVALID READ (confirmed, m_clickedElement pointing to freed memory)
    //     Select a note, drag it slightly and without releasing the
    //     mouse button, press the delete key.  Now release the mouse button.
    //     This routine is entered with m_clickedElement pointing to
    //     freed memory.  It's a rare case, so not critical to fix.

    if (!m_clickedElement || !m_selectedStaff) return ;

    EventSelection *selection = m_scene->getSelection();

    RG_DEBUG << "NotationSelector::drag: scene currently has selection with " << (selection ? selection->getSegmentEvents().size() : 0) << " event(s)";

    if (!selection || !selection->contains(m_clickedElement->event())) {
        selection = new EventSelection(m_selectedStaff->getSegment());
        RG_DEBUG << "(selection does not contain our event " << m_clickedElement->event() << " of type " << m_clickedElement->event()->getType() << ", adding it)";
        selection->addEvent(m_clickedElement->event(), m_ties);
    }
    m_scene->setSelection(selection, false);

    RG_DEBUG << "Sorted out selection";

    NotationStaff *targetStaff = m_scene->getStaffForSceneCoords(x, y);
    if (!targetStaff) targetStaff = m_selectedStaff;

    // Calculate time and height

    timeT clickedTime = m_clickedElement->event()->getNotationAbsoluteTime();

    Accidental clickedAccidental = Accidentals::NoAccidental;
    (void)m_clickedElement->event()->get<String>(ACCIDENTAL, clickedAccidental);

    // get the pitch from the element that was clicked on
    long clickedPitch = 0;
    (void)m_clickedElement->event()->get<Int>(PITCH, clickedPitch);

    // get the height from the element that was clicked on
    long clickedHeight = 0;
    (void)m_clickedElement->event()->get<Int>
            (NotationProperties::HEIGHT_ON_STAFF, clickedHeight);

    Event *clefEvt = nullptr, *keyEvt = nullptr;
    Clef clef;
    ::Rosegarden::Key key;

    timeT dragTime = clickedTime;
    double layoutX = m_clickedElement->getLayoutX();
    timeT duration = m_clickedElement->getViewDuration();

    NotationElementList::iterator itr =
        targetStaff->getElementUnderSceneCoords(x, y, clefEvt, keyEvt);

    if (itr != targetStaff->getViewElementList()->end()) {

        NotationElement *elt = dynamic_cast<NotationElement *>(*itr);
        dragTime = elt->getViewAbsoluteTime();
        layoutX = elt->getLayoutX();

        if (elt->isRest() && duration > 0 && elt->getItem()) {

            double restX = 0, restWidth = 0;
            elt->getSceneAirspace(restX, restWidth);

            timeT restDuration = elt->getViewDuration();

            if (restWidth > 0 && restDuration >= duration * 2) {

                int parts = restDuration / duration;
                double encroachment = x - restX;
                RG_DEBUG << "encroachment is " << encroachment << ", restWidth is " << restWidth;
                int part = (int)((encroachment / restWidth) * parts);
                if (part >= parts) part = parts - 1;

                dragTime += part * restDuration / parts;
                layoutX += part * restWidth / parts + (restX - elt->getSceneX());
            }
        }
    }

    if (clefEvt)
        clef = Clef(*clefEvt);
    if (keyEvt)
        key = ::Rosegarden::Key(*keyEvt);

    // set height to the height of y at the mouse click that got us here,
    // weighted toward clickedHeight (the height-on-staff property of the
    // element that was clicked) to make it more difficult to move notes by
    // mistake
    int height = targetStaff->getWeightedHeightAtSceneCoords(clickedHeight, x, y);

    // set pitch to the pitch of the notation element that got us here
    int pitch = clickedPitch;

    // if the height of y isn't the same as the height of the element that got
    // us here, we're doing a move
    if (height != clickedHeight) {
        pitch = Pitch(height, clef, key,
                      clickedAccidental).getPerformancePitch();
    }

    if (pitch < clickedPitch) {
        if (height < -10) {
            height = -10;
            pitch = Pitch(height, clef, key,
                          clickedAccidental).getPerformancePitch();
        }
    } else if (pitch > clickedPitch) {
        if (height > 18) {
            height = 18;
            pitch = Pitch(height, clef, key,
                          clickedAccidental).getPerformancePitch();
        }
    }

    bool singleNonNotePreview = !m_clickedElement->isNote() &&
                                selection->getSegmentEvents().size() == 1;

    if (!final && !singleNonNotePreview) {

        if ((pitch != m_lastDragPitch || dragTime != m_lastDragTime) &&
            m_clickedElement->isNote()) {

            m_scene->showPreviewNote(targetStaff, layoutX, pitch, height,
                                     Note::getNearestNote(duration),
                                     m_clickedElement->isGrace());
            m_lastDragPitch = pitch;
            m_lastDragTime = dragTime;
        }

    } else {

        m_scene->clearPreviewNote();

        MacroCommand *command = new MacroCommand(MoveCommand::getGlobalName());
        bool haveSomething = false;

        MoveCommand *mc = nullptr;
        Event *lastInsertedEvent = nullptr;

        if (pitch != clickedPitch && m_clickedElement->isNote()) {
            command->addCommand(new TransposeCommand(pitch - clickedPitch,
                                *selection));
            haveSomething = true;
        }

        if (targetStaff != m_selectedStaff) {
            command->addCommand(new MoveAcrossSegmentsCommand(
                    &targetStaff->getSegment(),  // secondSegment
                    dragTime - clickedTime + selection->getStartTime(),  // newStartTime
                    true,  // notation
                    selection));
            haveSomething = true;
        } else {
            timeT endTime = m_selectedStaff->getSegment().getEndTime();
            timeT newEndTime = dragTime + selection->getTotalDuration();

            if (dragTime != clickedTime && newEndTime <= endTime) {
                mc = new MoveCommand
                     (m_selectedStaff->getSegment(),  //!!!sort
                      dragTime - clickedTime, true, *selection);
                command->addCommand(mc);
                haveSomething = true;
            }
        }

        if (haveSomething) {

            CommandHistory::getInstance()->addCommand(command);

            // Moving the event will cause a new event to be created,
            // so our clicked element will no longer be valid.  But we
            // can't always recreate it, so as a precaution clear it
            // here so at least it isn't set to something bogus
            m_clickedElement = nullptr;

            if (mc && singleNonNotePreview) {

                lastInsertedEvent = mc->getLastInsertedEvent();

                if (lastInsertedEvent) {
                    m_scene->setSingleSelectedEvent(&targetStaff->getSegment(),
                                                    lastInsertedEvent,
						    true);

                    ViewElementList::iterator vli =
                        targetStaff->findEvent(lastInsertedEvent);

                    if (vli != targetStaff->getViewElementList()->end()) {
                        m_clickedElement = dynamic_cast<NotationElement *>(*vli);
                    } else {
                        m_clickedElement = nullptr;
                    }

                }
            }
        } else {
            delete command;
	}
    }
}

void NotationSelector::ready()
{
    m_widget->setCanvasCursor(Qt::ArrowCursor);

    // The arrow tool doesn't use the wheel.
    m_widget->getView()->setWheelZoomPan(true);
}

void NotationSelector::stow()
{
    delete m_selectionRect;
    m_selectionRect = nullptr;
}

void NotationSelector::handleEventRemoved(Event *event)
{
    if (m_clickedElement && m_clickedElement->event() == event) {
        m_clickedElement = nullptr;
    }
}

void NotationSelector::slotInsertSelected()
{
    invokeInParentView("draw");
}

void NotationSelector::slotEraseSelected()
{
    invokeInParentView("erase");
}

void NotationSelector::slotCollapseRestsHard()
{
    invokeInParentView("collapse_rests_aggressively");
}

void NotationSelector::slotRespellFlat()
{
    invokeInParentView("respell_flat");
}

void NotationSelector::slotRespellSharp()
{
    invokeInParentView("respell_sharp");
}

void NotationSelector::slotRespellNatural()
{
    invokeInParentView("respell_natural");
}

void NotationSelector::slotCollapseNotes()
{
    invokeInParentView("collapse_notes");
}

void NotationSelector::slotInterpret()
{
    invokeInParentView("interpret");
}

void NotationSelector::slotStaffAbove()
{
    invokeInParentView("move_events_up_staff");
}

void NotationSelector::slotStaffBelow()
{
    invokeInParentView("move_events_down_staff");
}

void NotationSelector::slotMakeInvisible()
{
    invokeInParentView("make_invisible");
}

void NotationSelector::slotMakeVisible()
{
    invokeInParentView("make_visible");
}

void NotationSelector::setViewCurrentSelection(bool preview)
{
    NotationScene::EventWithSegmentMap previewEvents;
    NotationScene::EventWithSegmentMap* previewEventsPtr = nullptr;
    if (preview) previewEventsPtr = &previewEvents;
    EventSelection *selection =
        getEventsInSelectionRect(previewEventsPtr);

    if (m_selectionToMerge) {
        if (selection &&
            m_selectionToMerge->getSegment() == selection->getSegment()) {
            selection->addFromSelection(m_selectionToMerge);
        } else {
            return;
        }
    }

    m_scene->setSelection(selection, preview);
    if (preview) m_scene->setExtraPreviewEvents(previewEvents);
}
/*!!!
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
*/
EventSelection *
NotationSelector::getEventsInSelectionRect
(NotationScene::EventWithSegmentMap* previewEvents)
{
    // If selection rect is not visible or too small,
    // return 0
    //
    if (!m_selectionRect->isVisible()) return nullptr;

    if (!m_selectedStaff) return nullptr;

    Profiler profiler("NotationSelector::getEventsInSelectionRect");

    //    RG_DEBUG << "Selection x,y: " << m_selectionRect->x() << ","
    //                         << m_selectionRect->y() << "; w,h: " << m_selectionRect->width() << "," << m_selectionRect->height() << endl;

    QRectF rect = m_selectionRect->rect();

    if (rect.width()  > -3 &&
        rect.width()  <  3 &&
        rect.height() > -3 &&
        rect.height() <  3) return nullptr;

    QList<QGraphicsItem *> l = m_selectionRect->collidingItems
        (Qt::IntersectsItemShape);

    Segment& segment = m_selectedStaff->getSegment();

    // If we selected the whole staff, force that to happen explicitly
    // rather than relying on collisions with the rectangle -- because
    // events way off the currently visible area might not even have
    // been drawn yet, and so will not appear in the collision list.
    // (We did still need the collision list to determine which staff
    // to use though.)

    if (m_wholeStaffSelectionComplete) {
        EventSelection *selection = new EventSelection(segment,
                                                       segment.getStartTime(),
                                                       segment.getEndMarkerTime());
        return selection;
    }

    EventSelection *selection = new EventSelection(segment);
    int nbw = m_selectedStaff->getNotePixmapFactory(false).getNoteBodyWidth();

    if (previewEvents) previewEvents->clear();
    for (int i = 0; i < l.size(); ++i) {

        QGraphicsItem *item = l[i];
        NotationElement *element = NotationElement::getNotationElement(item);
        if (!element) continue;

        double x = element->getSceneX();
        double y = element->getSceneY();

        bool shifted = false;

        // #957364 (Notation: Hard to select upper note in chords
        // of seconds) -- adjust x-coord for shifted note head
        if (element->event()->get<Bool>(m_selectedStaff->getProperties().
                                       NOTE_HEAD_SHIFTED, shifted) && shifted) {
            x += nbw;
        }

        // check if the element's rect
        // is actually included in the selection rect.
        //
        if (!rect.contains(x, y))  {
            // #988217 (Notation: Special column of pixels
            // prevents sweep selection) -- for notes, test again
            // with centred x-coord
            if (!element->isNote() || !rect.contains(x + nbw/2, y)) {
                continue;
            }
        }

        // must be in the same segment as we first started on,
        // we can't select events across multiple segments
        if (selection->getSegment().findSingle(element->event()) !=
            selection->getSegment().end()) {
            selection->addEvent(element->event(), m_ties);
        } else {
            if (previewEvents) {
                // previewEvents should contain all notes from other segments
                if (! element->isNote()) continue;
                (*previewEvents)[element->event()] =
                    element->getSegment();
            }
       }
    }

    if (selection->getAddedEvents() > 0) {
        return selection;
    } else {
        delete selection;
        return nullptr;
    }
}

QString NotationSelector::ToolName() { return "notationselector"; }

QString NotationSelectorNoTies::ToolName() { return "notationselectornoties"; }

}
