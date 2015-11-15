/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

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

#include <cmath>

#define CONTROL_SMALL_DISTANCE 10

namespace Rosegarden
{

ControlSelector::ControlSelector(ControlRuler *parent) :
    ControlMover(parent,"ControlSelector")
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
    if (!(e->itemList.size())) {
        // Start selection rectangle
        m_ruler->setSelectionRect(new QRectF(e->x,e->y,0.0,0.0));

        // Clear the added items list because we have yet to add any
        m_addedItems.clear();
    }

    ControlMover::handleLeftButtonPress(e);
}

ControlTool::FollowMode
ControlSelector::handleMouseMove(const ControlMouseEvent *e)
{
    QRectF *pRectF = m_ruler->getSelectionRectangle();
    if (pRectF) {
        // Selection drag is in progress
        // Clear the list of items that this tool has added
        for (ControlItemList::iterator it = m_addedItems.begin(); it != m_addedItems.end(); ++it) {
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
        for (ControlItemMap::iterator it = itmin; it != itmax; ++it) {
            if (pRectF->contains(it->second->boundingRect().center())) {
                m_addedItems.push_back(it->second);
                it->second->setSelected(true);
            }
        }

    }
    
    return ControlMover::handleMouseMove(e);
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
        for (ControlItemList::iterator it = m_addedItems.begin(); it != m_addedItems.end(); ++it) {
            m_ruler->addToSelection(*it);
        }
    }

    ControlMover::handleMouseRelease(e);
}

const QString ControlSelector::ToolName = "selector";

}

