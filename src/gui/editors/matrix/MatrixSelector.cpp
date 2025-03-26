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

#define RG_MODULE_STRING "[MatrixSelector]"
#define RG_NO_DEBUG_PRINT 1

#include "MatrixSelector.h"

#include "misc/Strings.h"
#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "base/ViewElement.h"
#include "base/SnapGrid.h"
#include "commands/edit/EventEditCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/editors/event/EditEvent.h"
#include "gui/general/GUIPalette.h"
#include "MatrixElement.h"
#include "MatrixMover.h"
#include "MatrixPainter.h"
#include "MatrixResizer.h"
#include "MatrixViewSegment.h"
#include "MatrixTool.h"
#include "MatrixToolBox.h"
#include "MatrixWidget.h"
#include "MatrixScene.h"
#include "MatrixMouseEvent.h"
#include "misc/Debug.h"

#include <QSettings>


namespace Rosegarden
{

MatrixSelector::MatrixSelector(MatrixWidget *widget) :
    MatrixTool("matrixselector.rc", "MatrixSelector", widget),
    m_selectionRect(nullptr),
    m_selectionOrigin(),
    m_updateRect(false),
    m_currentViewSegment(nullptr),
    m_clickedElement(nullptr),
    m_event(nullptr),
    m_dispatchTool(nullptr),
    m_justSelectedBar(false),
    m_selectionToMerge(nullptr),
    m_previousCollisions()
{
    //connect(m_widget, SIGNAL(usedSelection()),
    //        this, SLOT(slotHideSelection()));

    createAction("resize", SLOT(slotResizeSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("move", SLOT(slotMoveSelected()));

    createMenu();
}

void
MatrixSelector::handleEventRemoved(Event *event)
{
    // NOTE: Do not use m_clickedElement in here as it was freed by
    //       ViewSegment::eventRemoved() before we get here.

    if (m_dispatchTool)
        m_dispatchTool->handleEventRemoved(event);

    // Is it the event we are holding on to?
    if (m_event == event) {
        m_clickedElement = nullptr;
        m_event = nullptr;
    }
}

void
MatrixSelector::slotClickTimeout()
{
    m_justSelectedBar = false;
}

void
MatrixSelector::handleLeftButtonPress(const MatrixMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixSelector::handleLeftButtonPress";

    m_previousCollisions.clear();

    if (m_justSelectedBar) {
        handleMouseTripleClick(e);
        m_justSelectedBar = false;
        return ;
    }

    m_currentViewSegment = e->viewSegment;

    // Do the merge selection thing
    //
    delete m_selectionToMerge;
    const EventSelection *selectionToMerge = nullptr;
    if (e->modifiers & Qt::ShiftModifier) {
        selectionToMerge = m_scene->getSelection();
    }

    m_selectionToMerge =
        (selectionToMerge ? new EventSelection(*selectionToMerge) : nullptr);

    // Now the rest of the element stuff
    //
    m_clickedElement = e->element;

    if (m_clickedElement) {
        m_event = m_clickedElement->event();

        float x = m_clickedElement->getLayoutX();
        float width = m_clickedElement->getWidth();
        float resizeStart = int(double(width) * 0.85) + x;

        // max size of 10
        if ((x + width) - resizeStart > 10) resizeStart = x + width - 10;

        m_dispatchTool = nullptr;

        if (e->sceneX > resizeStart) {
            m_dispatchTool =
                dynamic_cast<MatrixTool *>
                (m_widget->getToolBox()->getTool(MatrixResizer::ToolName()));
        } else {
            m_dispatchTool =
                dynamic_cast<MatrixTool *>
                (m_widget->getToolBox()->getTool(MatrixMover::ToolName()));
        }

        if (!m_dispatchTool) return;

        m_dispatchTool->ready();
        m_dispatchTool->handleLeftButtonPress(e);
        return;

    } else if (e->modifiers & Qt::ControlModifier) {

        handleMidButtonPress(e);
        return;

    } else {

        // NOTE: if we ever have axis-independent zoom, this (and similar code
        // elsewhere) will have to be refactored to draw a series of lines using
        // two different widths, based on calculating 200 / axisZoomPercent
        // to solve ((w * axisZoomPercent) / 100) = 2
        //
        // (Not sure how to do this now that we do.  It's obnoxious, but oh
        // well.)
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

        // Clear existing selection if we're not merging
        //
        if (!m_selectionToMerge) {
            m_scene->setSelection(nullptr, false);
        }
    }
}

void
MatrixSelector::handleMidButtonPress(const MatrixMouseEvent *e)
{
    // Middle button press draws a new event via MatrixPainter.

    m_clickedElement = nullptr; // should be used for left-button clicks only
    m_event = nullptr;

    // Don't allow overlapping elements on the same channel
    // if (e->element) return;
    if (e->element &&
        e->element->getScene() &&
        e->element->getSegment() ==
        e->element->getScene()->getCurrentSegment()) {
        RG_DEBUG << "handleMidButtonPress(): Will not create note at "
                    "same pitch and time as existing note in active "
                    "segment.";
        return;
    }

    m_dispatchTool =
        dynamic_cast<MatrixTool *>
        (m_widget->getToolBox()->getTool(MatrixPainter::ToolName()));

    if (!m_dispatchTool) return;

    m_dispatchTool->ready();
    m_dispatchTool->handleLeftButtonPress(e);
}

void
MatrixSelector::handleMouseDoubleClick(const MatrixMouseEvent *e)
{
    // Don't use m_clickedElement here, as it's reset to 0 on mouse
    // release, which occurs before our dialog completes (and we need
    // to know the element after that)
    MatrixElement *element = e->element;

    MatrixViewSegment *vs = e->viewSegment;
    if (!vs) return;

    if (element) {
        // Don't allow editing note's parameters if not in active segment
        // because there might be multiple overlapping non-active segment's
        // notes at same pitch/time, and one chosen would be semi-arbitrary
        // (e.g. first segment(??) in lowest numbered track).
        if (!(element &&
              element->getScene() &&
              element->getSegment() ==
              element->getScene()->getCurrentSegment())) {
            RG_DEBUG << "handleMouseDoubleClick(): Note must be "
                        "in active segment.";
            return;
        }

        if (element->event()->isa(Note::EventType) &&
            element->event()->has(BaseProperties::TRIGGER_SEGMENT_ID)) {

            int id = element->event()->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID);
            emit editTriggerSegment(id);
            return;
        }

        EditEvent dialog(m_widget, *element->event());

        // Launch dialog.  Bail if canceled.
        if (dialog.exec() != QDialog::Accepted)
            return;

        Event newEvent = dialog.getEvent();
        // No changes?  Bail.
        if (newEvent == *element->event())
            return;

        CommandHistory::getInstance()->addCommand(new EventEditCommand(
                vs->getSegment(),
                element->event(),  // eventToModify
                newEvent));  // newEvent

    }

#if 0
    // Feature Request #124: multiclick select methods shouldwork in matrix
    // editor (was #988167)

    // Postponing this, as it falls foul of world-matrix transformation
    // etiquette and other such niceties

    else {

        QRect rect = staff->getBarExtents(ev->x(), ev->y());

        m_selectionRect->setX(rect.x() + 2);
        m_selectionRect->setY(rect.y());
        m_selectionRect->setSize(rect.width() - 4, rect.height());

        m_selectionRect->show();
        m_updateRect = false;

        m_justSelectedBar = true;
        QTimer::singleShot(QApplication::doubleClickInterval(), this,
                           SLOT(slotClickTimeout()));
    }
#endif
}

void
MatrixSelector::handleMouseTripleClick(const MatrixMouseEvent *e)
{
    if (!m_justSelectedBar) return;
    m_justSelectedBar = false;

    MatrixViewSegment *vs = e->viewSegment;
    if (!vs) return;

    if (m_clickedElement) {

        // should be safe, as we've already set m_justSelectedBar false
        handleLeftButtonPress(e);
        return;

/*!!! see note above
    } else {

        m_selectionRect->setX(staff->getX());
        m_selectionRect->setY(staff->getY());
        m_selectionRect->setSize(int(staff->getTotalWidth()) - 1,
                                 staff->getTotalHeight() - 1);

        m_selectionRect->show();
        m_updateRect = false;
*/
    }
}

FollowMode
MatrixSelector::handleMouseMove(const MatrixMouseEvent *e)
{
    if (m_dispatchTool) {
        return m_dispatchTool->handleMouseMove(e);
    }

    if (!m_updateRect) {
        setContextHelpFor
            (e, getSnapGrid()->getSnapSetting() == SnapGrid::NoSnap);
        return NO_FOLLOW;
    } else {
        clearContextHelp();
    }

    QPointF p0(m_selectionOrigin);
    QPointF p1(e->sceneX, e->sceneY);
    QRectF r = QRectF(p0, p1).normalized();

    m_selectionRect->setRect(r.x() + 0.5, r.y() + 0.5, r.width(), r.height());
    m_selectionRect->show();

    setViewCurrentSelection(false);



/*
    int w = int(p.x() - m_selectionRect->x());
    int h = int(p.y() - m_selectionRect->y());

    // Qt rectangle dimensions appear to be 1-based
    if (w > 0)
        ++w;
    else
        --w;
    if (h > 0)
        ++h;
    else
        --h;

    // Workaround for #930420 Positional error in sweep-selection box boundary
    int wFix = (w > 0) ? 3 : 0;
    int hFix = (h > 0) ? 3 : 0;
    int xFix = (w < 0) ? 3 : 0;
    m_selectionRect->setSize(w - wFix, h - hFix);
    m_selectionRect->setX(m_selectionRect->x() + xFix);
    setViewCurrentSelection();
    m_selectionRect->setSize(w, h);
    m_selectionRect->setX(m_selectionRect->x() - xFix);
    m_widget->canvas()->update();
*/
    return (FOLLOW_HORIZONTAL | FOLLOW_VERTICAL);
}

void
MatrixSelector::handleMouseRelease(const MatrixMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixSelector::handleMouseRelease";

    if (m_dispatchTool) {
        m_dispatchTool->handleMouseRelease(e);

        m_dispatchTool->stow();
        ready();

        // don't delete the tool as it's still part of the toolbox
        m_dispatchTool = nullptr;

        return;
    }

    m_updateRect = false;

    if (m_clickedElement) {
        m_scene->setSingleSelectedEvent(m_currentViewSegment,
                                        m_clickedElement,
                                        false);
//        m_widget->canvas()->update();
        m_clickedElement = nullptr;
        m_event = nullptr;

    } else if (m_selectionRect) {
        setViewCurrentSelection(true);
        m_previousCollisions.clear();
        m_selectionRect->hide();
//        m_widget->canvas()->update();
    }

    // Tell anyone who's interested that the selection has changed
    emit gotSelection();

    setContextHelpFor(e);
}

void MatrixSelector::keyPressEvent(QKeyEvent *e)
{
    if (m_dispatchTool) m_dispatchTool->keyPressEvent(e);
}

void MatrixSelector::keyReleaseEvent(QKeyEvent *e)
{
    if (m_dispatchTool) m_dispatchTool->keyReleaseEvent(e);
}

void
MatrixSelector::ready()
{
    if (m_widget) m_widget->setCanvasCursor(Qt::ArrowCursor);


/*!!!
    connect(m_widget->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));
*/
    setContextHelp
        (tr("Click and drag to select; middle-click and drag to draw new note"));
}

void
MatrixSelector::stow()
{
    if (m_selectionRect) {
        delete m_selectionRect;
        m_selectionRect = nullptr;
//        m_widget->canvas()->update();
    }
/*!!!
    disconnect(m_widget->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));
*/

}

/* unused
void
MatrixSelector::slotHideSelection()
{
    if (!m_selectionRect) return;
    m_selectionRect->hide();
//!!!    m_selectionRect->setSize(0, 0);
//!!!    m_widget->canvas()->update();
}
*/

/* unused
void
MatrixSelector::slotMatrixScrolled(int , int)
{
*/
/*!!!
    if (m_updateRect) {

        int offsetX = newX - m_widget->getCanvasView()->contentsX();
        int offsetY = newY - m_widget->getCanvasView()->contentsY();

        int w = int(m_selectionRect->width() + offsetX);
        int h = int(m_selectionRect->height() + offsetY);

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
        setViewCurrentSelection();
        m_widget->canvas()->update();
    }
*/
//}

void
MatrixSelector::setViewCurrentSelection(bool always)
{
    if (always) m_previousCollisions.clear();

    MatrixScene::EventWithSegmentMap previewEvents;

    EventSelection* selection = nullptr;
    bool changed = getSelection(selection, &previewEvents);
    if (!changed) {
        delete selection;
        return;
    }

    if (m_selectionToMerge && selection &&
        m_selectionToMerge->getSegment() == selection->getSegment()) {

        selection->addFromSelection(m_selectionToMerge);
        m_scene->setSelection(selection, true);

    } else if (!m_selectionToMerge) {

        m_scene->setSelection(selection, true);
    }
    m_scene->setExtraPreviewEvents(previewEvents);
}

bool
MatrixSelector::getSelection(EventSelection *&selection,
                             MatrixScene::EventWithSegmentMap* previewEvents)
{
    if (!m_selectionRect || !m_selectionRect->isVisible()) return 0;

    if (previewEvents) previewEvents->clear();

    Segment& originalSegment = m_currentViewSegment->getSegment();
    selection = new EventSelection(originalSegment);

    // get the selections
    //
    QList<QGraphicsItem *> l = m_selectionRect->collidingItems
        (Qt::IntersectsItemShape);

    // This is a nasty optimisation, just to avoid re-creating the
    // selection if the items we span are unchanged.  It's not very
    // effective, either, because the colliding items returned
    // includes things like the horizontal and vertical background
    // lines -- and so it changes often: every time we cross a line.
    // More thought needed.

    // It might be better to use the event properties (i.e. time and
    // pitch) to calculate this "from first principles" rather than
    // doing it graphically.  That might also be helpful to avoid us
    // dragging off the logical edges of the scene.

    // (Come to think of it, though, that would be troublesome just
    // because of the requirement to use all events that have any part
    // inside the selection.  Quickly finding all events that start
    // within a time range is trivial, finding all events that
    // intersect one is more of a pain.)
    if (l == m_previousCollisions) return false;
    m_previousCollisions = l;

    if (!l.empty()) {
        for (int i = 0; i < l.size(); ++i) {
            QGraphicsItem *item = l[i];
            MatrixElement *element = MatrixElement::getMatrixElement(item);
            if (element) {
                // The selection should only contain elements from the
                // current segment however for preview play we should
                // have all the elements
                if (element->getSegment() ==
                    element->getScene()->getCurrentSegment()) {
                    selection->addEvent(element->event());
                } else {
                    // previewEvents contains events from other
                    // segments which should also be preview played
                    if (previewEvents) {
                        if (previewEvents->find(element->event()) ==
                            previewEvents->end()) {
                            (*previewEvents)[element->event()] =
                                element->getSegment();
                        }
                    }
                }
            }
        }
    }

    if (selection->getAddedEvents() == 0) {
        delete selection;
        selection = nullptr;
    }

    return true;
}

void
MatrixSelector::setContextHelpFor(const MatrixMouseEvent *e, bool ctrlPressed)
{
    MatrixElement *element = e->element;

    if (!element) {

        setContextHelp
            (tr("Click and drag to select; middle-click and drag to draw new note"));

    } else {

        // same logic as in handleLeftButtonPress

        float x = element->getLayoutX();
        float width = element->getWidth();
        float resizeStart = int(double(width) * 0.85) + x;

        // max size of 10
        if ((x + width) - resizeStart > 10) resizeStart = x + width - 10;

        EventSelection *s = m_scene->getSelection();

        if (e->sceneX > resizeStart) {
            if (s && s->getAddedEvents() > 1) {
                setContextHelp(tr("Click and drag to resize selected notes"));
            } else {
                setContextHelp(tr("Click and drag to resize note"));
            }
        } else {
            if (s && s->getAddedEvents() > 1) {
                if (!ctrlPressed) {
                    setContextHelp(tr("Click and drag to move selected notes; hold Ctrl as well to copy"));
                } else {
                    setContextHelp(tr("Click and drag to copy selected notes"));
                }
            } else {
                if (!ctrlPressed) {
                    setContextHelp(tr("Click and drag to move note; hold Ctrl as well to copy"));
                } else {
                    setContextHelp(tr("Click and drag to copy note"));
                }
            }
        }
    }
}

QString MatrixSelector::ToolName() { return "selector"; }

}
