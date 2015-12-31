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

#include "ControlMover.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
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

ControlMover::ControlMover(ControlRuler *parent, QString menuName) :
    ControlTool("", menuName, parent),
    m_overCursor(Qt::OpenHandCursor),
    m_notOverCursor(Qt::ArrowCursor)
{
}

void
ControlMover::handleLeftButtonPress(const ControlMouseEvent *e)
{
    if (m_overItem) {
        m_ruler->setCursor(Qt::BlankCursor);

        std::vector<ControlItem*>::const_iterator it = e->itemList.begin();
        if ((*it)->isSelected()) {
            if (e->modifiers & (Qt::ShiftModifier))
                m_ruler->removeFromSelection(*it);
        } else {
            if (!(e->modifiers & (Qt::ShiftModifier))) {
                // No add to selection modifiers so clear the current selection
                m_ruler->clearSelectedItems();
            }

            m_ruler->addToSelection(*it);
        }
        
        m_startPointList.clear();
        ControlItemList *selected = m_ruler->getSelectedItems();
        for (ControlItemList::iterator it = selected->begin(); it != selected->end(); ++it) {
            m_startPointList.push_back(QPointF((*it)->xStart(),(*it)->y()));
        }
    } else {
        if (!(e->modifiers & (Qt::ShiftModifier))) {
            // No add to selection modifiers so clear the current selection
            m_ruler->clearSelectedItems();
        }
    }

    m_mouseStartX = e->x;
    m_mouseStartY = e->y;
    m_lastDScreenX = 0.0f;
    m_lastDScreenY = 0.0f;
    
    m_ruler->update();
}

ControlTool::FollowMode
ControlMover::handleMouseMove(const ControlMouseEvent *e)
{
    if (e->buttons == Qt::NoButton) {
        // No button pressed, set cursor style
        setCursor(e);
    }

    if ((e->buttons & Qt::LeftButton) && m_overItem) {
        // A drag action is in progress        
        float deltaX = (e->x-m_mouseStartX);
        float deltaY = (e->y-m_mouseStartY);

        double xscale = m_ruler->getXScale();
        double yscale = m_ruler->getYScale();
        float dScreenX = deltaX / xscale;
        float dScreenY = deltaY / yscale;

        if (e->modifiers & Qt::ControlModifier) {
            // If the control key is held down, restrict movement to either horizontal or vertical
            //    depending on the direction the item has been moved
            
            // For small displacements from the starting position, use the direction of this movement
            //    rather than the actual displacement - makes dragging through the original position
            //    less likely to switch constraint axis
            if ((fabs(dScreenX) < CONTROL_SMALL_DISTANCE) && (fabs(dScreenY) < CONTROL_SMALL_DISTANCE)) {
                dScreenX = dScreenX-m_lastDScreenX;
                dScreenY = dScreenY-m_lastDScreenY;
            }
        
            if (fabs(dScreenX) > fabs(dScreenY)) {
                deltaY = 0;
            } else {
                deltaX = 0;
            }
        }
        
        m_lastDScreenX = dScreenX;
        m_lastDScreenY = dScreenY;
        
        EventControlItem *item;
        ControlItemList *selected = m_ruler->getSelectedItems();
        std::vector<QPointF>::iterator pIt = m_startPointList.begin();
        for (ControlItemList::iterator it = selected->begin(); it != selected->end(); ++it) {
            item = dynamic_cast <EventControlItem*> (*it);

            float x = pIt->x()+deltaX;
            float xmin = m_ruler->getXMin() * xscale;
            float xmax = (m_ruler->getXMax() - 1) * xscale;
            x = std::max(x,xmin);
            x = std::min(x,xmax);

            float y = pIt->y()+deltaY;
            y = std::max(y,0.0f);
            y = std::min(y,1.0f);
            if (item) item->reconfigure(x,y);
            ++pIt;
        }
        return FollowHorizontal;
    }

    m_ruler->update();
    
    return NoFollow;
}

void
ControlMover::handleMouseRelease(const ControlMouseEvent *e)
{
    if (m_overItem) {
        // This is the end of a drag event
        // Update the segment to reflect changes
        m_ruler->updateSegment();

        // Reset the cursor to the state that it started
        m_ruler->setCursor(m_overCursor);
    }

    // May have moved off the item during a drag so use setCursor to correct its state
    setCursor(e);

    m_ruler->update();
}

void ControlMover::setCursor(const ControlMouseEvent *e)
{
    bool isOverItem = false;

    if (e->itemList.size()) isOverItem = true;

    if (!m_overItem) {
        if (isOverItem) {
            m_ruler->setCursor(m_overCursor);
            m_overItem = true;
        }
    } else {
        if (!isOverItem) {
            m_ruler->setCursor(m_notOverCursor);
            m_overItem = false;
        }
    }
}

void ControlMover::ready()
{
    m_ruler->setCursor(m_notOverCursor);
    m_overItem = false;
}

void ControlMover::stow()
{
}

const QString ControlMover::ToolName = "mover";
}

