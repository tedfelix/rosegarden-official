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

#include "ControlSelector.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
//#include "commands/matrix/MatrixModifyCommand.h"
//#include "commands/matrix/MatrixInsertionCommand.h"
//#include "commands/notation/NormalizeRestsCommand.h"
#include "document/CommandHistory.h"
#include "ControlItem.h"
#include "EventControlItem.h"
#include "ControlRuler.h"
#include "ControllerEventsRuler.h"
#include "ControlTool.h"
#include "ControlMouseEvent.h"
#include "misc/Debug.h"

#include <QCursor>
#include <QRectF>

namespace Rosegarden
{

ControlSelector::ControlSelector(ControlRuler *parent) :
    ControlTool("", "ControlSelector", parent)
{
//    createAction("select", SLOT(slotSelectSelected()));
//    createAction("draw", SLOT(slotDrawSelected()));
//    createAction("erase", SLOT(slotEraseSelected()));
//    createAction("resize", SLOT(slotResizeSelected()));
//
//    createMenu();
}

void
ControlSelector::handleLeftButtonPress(const ControlMouseEvent *e)
{
    if (e->itemList.size()) {
        //for (std::vector<ControlItem*>::const_iterator it = e->itemList.begin(); it != e->itemList.end(); it++) {
        std::vector<ControlItem*>::const_iterator it = e->itemList.begin();
        if ((*it)->isSelected()) {
            if (e->modifiers & (Qt::ShiftModifier | Qt::ControlModifier))
                m_ruler->removeFromSelection(*it);
        } else {
            if (!(e->modifiers & (Qt::ShiftModifier | Qt::ControlModifier))) {
                // No add to selection modifiers so clear the current selection
                m_ruler->clearSelectedItems();
            }

            m_ruler->addToSelection(*it);
        }
    } else {
        if (!(e->modifiers & (Qt::ShiftModifier | Qt::ControlModifier))) {
            // No add to selection modifiers so clear the current selection
            m_ruler->clearSelectedItems();
        }

        // Start selection rectangle
        m_ruler->setSelectionRect(new QRectF(e->x,e->y,0.0,0.0));

        // Clear the added items list because we have yet to add any
        m_addedItems.clear();
    }

    m_mouseLastY = e->y;
    m_mouseLastX = e->x;

    m_ruler->update();
}

void
ControlSelector::handleMouseMove(const ControlMouseEvent *e)
{
    if (e->buttons == Qt::NoButton) {
        // No button pressed, set cursor style
        setCursor(e);
    }

    QRectF *pRectF = m_ruler->getSelectionRectangle();
    if (pRectF) {
        // Selection drag is in progress
        // Clear the list of items that this tool has added
        for (ControlItemList::iterator it = m_addedItems.begin(); it != m_addedItems.end(); it++) {
            (*it)->setSelected(false);
        }
        m_addedItems.clear();

        // Update selection rectangle
        pRectF->setBottomRight(QPointF(e->x,e->y));

        // Find items within the range of the new rectangle
        ControlItemMap::iterator itmin =
            m_ruler->findControlItem(std::min(pRectF->left(),
                pRectF->right()));
        ControlItemMap::iterator itmax =
            m_ruler->findControlItem(std::max(pRectF->left(),
                pRectF->right()));

        // Add them if they're within the rectangle
        for (ControlItemMap::iterator it = itmin; it != itmax; it++) {
            if (pRectF->contains(it->second->boundingRect().center())) {
                m_addedItems.push_back(it->second);
                it->second->setSelected(true);
            }
        }

    }

    if ((e->buttons & Qt::LeftButton) && m_overItem) {
        // A drag action is in progress
        m_ruler->setCursor(Qt::ClosedHandCursor);

        float deltaX = (e->x-m_mouseLastX);
        float deltaY = (e->y-m_mouseLastY);
        m_mouseLastX = e->x;
        m_mouseLastY = e->y;

        EventControlItem *item;
        ControlItemList *selected = m_ruler->getSelectedItems();
        for (ControlItemList::iterator it = selected->begin(); it != selected->end(); ++it) {
            item = dynamic_cast <EventControlItem*> (*it);
            if (item) item->reconfigure(item->xStart()+deltaX,item->getValue()+deltaY);
        }
    }

    m_ruler->update();
}

void
ControlSelector::handleMouseRelease(const ControlMouseEvent *e)
{
    QRectF *pRectF = m_ruler->getSelectionRectangle();
    if (pRectF) {
        // Selection drag is now complete
        delete pRectF;
        m_ruler->setSelectionRect(0);

        // Add the selected items to the current selection
        for (ControlItemList::iterator it = m_addedItems.begin(); it != m_addedItems.end(); it++) {
            m_ruler->addToSelection(*it);
        }
    }

    if (m_overItem) {
        // This is the end of a drag event
        // Update the segment to reflect changes
        m_ruler->updateSegment();

        // Reset the cursor to the state that it started
        m_ruler->setCursor(Qt::PointingHandCursor);
    }

    // May have moved off the item during a drag so use setCursor to correct its state
    setCursor(e);

    m_ruler->update();
}

void ControlSelector::setCursor(const ControlMouseEvent *e)
{
    bool isOverItem = false;

    if (e->itemList.size()) isOverItem = true;

    if (!m_overItem) {
        if (isOverItem) {
            m_ruler->setCursor(Qt::PointingHandCursor);
            m_overItem = true;
        }
    } else {
        if (!isOverItem) {
            m_ruler->setCursor(Qt::CrossCursor);
            m_overItem = false;
        }
    }
}

void ControlSelector::ready()
{
    m_ruler->setCursor(Qt::CrossCursor);
    m_overItem = false;
//    connect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
//            m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));

//    m_widget->setCanvasCursor(Qt::sizeAllCursor);
//    setBasicContextHelp();
}

void ControlSelector::stow()
{
//    disconnect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
//               m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));
}

//void PropertyAdjuster::setBasicContextHelp(bool ctrlPressed)
//{
//    EventSelection *selection = m_scene->getSelection();
//    if (!selection || selection->getAddedEvents() < 2) {
//        if (!ctrlPressed) {
//            setContextHelp(tr("Click and drag to move a note; hold Ctrl as well to copy it"));
//        } else {
//            setContextHelp(tr("Click and drag to copy a note"));
//        }
//    } else {
//        if (!ctrlPressed) {
//            setContextHelp(tr("Click and drag to move selected notes; hold Ctrl as well to copy"));
//        } else {
//            setContextHelp(tr("Click and drag to copy selected notes"));
//        }
//    }
//}

const QString ControlSelector::ToolName = "selector";

}

#include "ControlSelector.moc"
