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

#define RG_MODULE_STRING "[ControlSelector]"
#define RG_NO_DEBUG_PRINT

#include "ControlSelector.h"

#include "ControlMouseEvent.h"
#include "ControlRuler.h"

#include <QRectF>


namespace Rosegarden
{


ControlSelector::ControlSelector(ControlRuler *parent) :
    ControlMover(parent, "ControlSelector")
{
}

void
ControlSelector::handleLeftButtonPress(const ControlMouseEvent *e)
{
    // If nothing is under the cursor...
    if (e->itemList.empty()) {
        // Start selection rectangle
        m_ruler->setSelectionRect(new QRectF(e->x, e->y, 0.0, 0.0));

        // Clear the added items list because we have yet to add any
        m_addedItems.clear();
    }

    ControlMover::handleLeftButtonPress(e);
}

FollowMode
ControlSelector::handleMouseMove(const ControlMouseEvent *e)
{
    QRectF *pRectF = m_ruler->getSelectionRectangle();

    // If selection drag is in progress...
    if (pRectF) {
        // Clear the list of items that this tool has added
        for (ControlItemList::iterator it = m_addedItems.begin();
             it != m_addedItems.end();
             ++it) {
            (*it)->setSelected(false);
        }
        m_addedItems.clear();

        // Update selection rectangle
        pRectF->setBottomRight(QPointF(e->x, e->y));

        // Find items within the range of the new rectangle
        ControlItemMap::iterator iterMin = m_ruler->findControlItem(
                std::min(pRectF->left(), pRectF->right()));
        const ControlItemMap::iterator iterMax = m_ruler->findControlItem(
                std::max(pRectF->left(), pRectF->right()));

        // For each item...
        for (ControlItemMap::iterator it = iterMin; it != iterMax; ++it) {
            // If this item is within the rubber-band rectangle....
            if (pRectF->contains(it->second->boundingRect().center())  &&
                it->second->active()) {
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
    // Selection drag is now complete

    QRectF *pRectF = m_ruler->getSelectionRectangle();

    // If we had a rubber-band rect...
    if (pRectF) {
        // Delete it.
        delete pRectF;
        m_ruler->setSelectionRect(nullptr);

        // Add the selected items to the current selection.
        for (ControlItemList::iterator it = m_addedItems.begin();
             it != m_addedItems.end();
             ++it) {
            m_ruler->addToSelection(*it);
        }
    }

    ControlMover::handleMouseRelease(e);
}


}
