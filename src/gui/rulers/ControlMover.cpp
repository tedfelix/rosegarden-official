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

#define RG_MODULE_STRING "[ControlMover]"
#define RG_NO_DEBUG_PRINT 1

#include "ControlMover.h"

#include "EventControlItem.h"
#include "ControlItem.h"
#include "ControlMouseEvent.h"
#include "ControlRuler.h"
#include "ControllerEventsRuler.h"

#include "base/SnapGrid.h"
#include "base/RulerScale.h"
#include "misc/Debug.h"

#include <math.h>


namespace Rosegarden
{


ControlMover::ControlMover(ControlRuler *parent, const QString &menuName) :
    ControlTool("", menuName, parent),
    m_snapGrid(parent->getSnapGrid()),
    m_rulerScale(parent->getRulerScale())
{
}

void
ControlMover::handleLeftButtonPress(const ControlMouseEvent *e)
{
    if (m_overItem) {

        // Hide the cursor.
        m_ruler->setCursor(Qt::BlankCursor);

        // Find the ControlItem that was clicked if any.

        QSharedPointer<ControlItem> controlItem;

        // For each ControlItem in the item list...
        for (QSharedPointer<ControlItem> controlItemLoop : e->itemList) {
            // If this one is active, go with it.
            if (controlItemLoop->active()) {
                controlItem = controlItemLoop;
                break;
            }
        }

        if (!controlItem) {
            // If the user is not holding down Shift for "Add to selection",
            // clear the current selection.
            if (!(e->modifiers & Qt::ShiftModifier))
                m_ruler->clearSelectedItems();
            return;
        }

        // Already selected?  Toggle it.
        if (controlItem->isSelected()) {
            if (e->modifiers & Qt::ShiftModifier)
                m_ruler->removeFromSelection(controlItem);
        } else {  // Not selected.
            // If the user is not holding down Shift for "Add to selection",
            // clear the current selection.
            if (!(e->modifiers & Qt::ShiftModifier))
                m_ruler->clearSelectedItems();

            m_ruler->addToSelection(controlItem);
        }

        // Build the start point list.

        m_startPointList.clear();

        ControlItemList *selected = m_ruler->getSelectedItems();
        // For each selected it, add its position to m_startPointList.
        for (QSharedPointer<ControlItem> controlItem : *selected) {
            m_startPointList.push_back(
                    QPointF(controlItem->xStart(), controlItem->y()));
        }

    } else {  // Not over an item.
        // If the user is not holding down Shift for "Add to selection",
        // clear the current selection.
        if (!(e->modifiers & Qt::ShiftModifier))
            m_ruler->clearSelectedItems();
    }

    // Track the mouse click point.
    m_mouseStartX = e->x;
    m_mouseStartY = e->y;

    m_lastDScreenX = 0.0f;
    m_lastDScreenY = 0.0f;

    m_ruler->update();
}

void
ControlMover::handleMidButtonPress(const ControlMouseEvent * /*e*/)
{
    ControllerEventsRuler *controllerEventsRuler =
            dynamic_cast<ControllerEventsRuler *>(m_ruler);
    if (!controllerEventsRuler)
        return;

    controllerEventsRuler->slotSetToDefault();
}

FollowMode
ControlMover::handleMouseMove(const ControlMouseEvent *e)
{
    emit showContextHelp(tr("Click and drag a value. Shift suppresses grid snap. Ctrl constrains to horizontal or vertical"));

    // No button pressed?  Set appropriate cursor.
    if (e->buttons == Qt::NoButton)
        setCursor(e);

    // If a drag action is in progress...
    if ((e->buttons & Qt::LeftButton)  &&  m_overItem) {

        float deltaX = (e->x-m_mouseStartX);
        float deltaY = (e->y-m_mouseStartY);

        const double xscale = m_ruler->getXScale();
        const double yscale = m_ruler->getYScale();

        float dScreenX = deltaX / xscale;
        float dScreenY = deltaY / yscale;

        // If the control key is held down, restrict movement to either
        // horizontal or vertical depending on the direction the item has
        // been moved.
        if (e->modifiers & Qt::ControlModifier) {

            constexpr int CONTROL_SMALL_DISTANCE{10};

            // For small displacements from the starting position, use the
            // direction of this movement rather than the actual displacement.
            // Makes dragging through the original position less likely to
            // switch constraint axis.
            if ((fabs(dScreenX) < CONTROL_SMALL_DISTANCE)  &&
                (fabs(dScreenY) < CONTROL_SMALL_DISTANCE)) {
                dScreenX = dScreenX - m_lastDScreenX;
                dScreenY = dScreenY - m_lastDScreenY;
            }

            if (fabs(dScreenX) > fabs(dScreenY))
                deltaY = 0;
            else
                deltaX = 0;

        }

        m_lastDScreenX = dScreenX;
        m_lastDScreenY = dScreenY;

        // Move the items.

        ControlItemList *selected = m_ruler->getSelectedItems();
        std::vector<QPointF>::const_iterator startPointIter =
                m_startPointList.cbegin();
        // For each selected item...
        for (QSharedPointer<ControlItem> controlItem : *selected) {

            // Downcast required to call EventControlItem::reconfigure().
            QSharedPointer<EventControlItem> item =
                    qSharedPointerDynamicCast<EventControlItem>(controlItem);
            if (!item)
                continue;

            const QPointF &startPoint = *startPointIter;

            // Compute x

            float x = startPoint.x() + deltaX;

            RG_DEBUG << "handleMouseMove" << x << startPoint.x() << deltaX;

            // If shift is not pressed, snap the x.
            if (!(e->modifiers & Qt::ShiftModifier)) {

                const timeT et = m_rulerScale->getTimeForX(x / xscale);
                const timeT etSnap = m_snapGrid->snapTime(et);
                x = m_rulerScale->getXForTime(etSnap) * xscale;

                RG_DEBUG << "handleMouseMove snap" << et << etSnap << x;
            }

            // Limit x
            const float xmin = m_ruler->getXMin() * xscale;
            const float xmax = (m_ruler->getXMax() - 1) * xscale;
            x = std::max(x, xmin);
            x = std::min(x, xmax);

            // Compute y
            float y = startPoint.y() + deltaY;
            y = std::max(y, 0.0f);
            y = std::min(y, 1.0f);

            // Move the item.
            item->reconfigure(x, y);

            ++startPointIter;
        }

        return FOLLOW_HORIZONTAL;

    }

    m_ruler->update();

    return NO_FOLLOW;
}

void
ControlMover::handleMouseRelease(const ControlMouseEvent *e)
{
    // If this is the end of a drag...
    if (m_overItem) {
        // Update the segment to reflect changes
        m_ruler->updateSegment();

        // Reset the cursor to its hover state.
        m_ruler->setCursor(m_overCursor);
    }

    // May have moved off the item during a drag so use setCursor to correct
    // its state.
    setCursor(e);

    m_ruler->update();
}

void ControlMover::setCursor(const ControlMouseEvent *e)
{
    bool isOverItem = false;

    // Check whether any of the items we are over is active.
    for (QSharedPointer<const ControlItem> controlItem : e->itemList) {
        if (controlItem->active()) {
            isOverItem = true;
            break;
        }
    }

    // No change?  Bail.
    if (m_overItem == isOverItem)
        return;

    if (isOverItem)
        m_ruler->setCursor(m_overCursor);
    else
        m_ruler->setCursor(m_notOverCursor);

    m_overItem = isOverItem;
}

void ControlMover::ready()
{
    m_ruler->setCursor(m_notOverCursor);
    m_overItem = false;
}


}
