/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PropertyAdjuster.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "document/CommandHistory.h"
#include "ControlItem.h"
#include "ControlRuler.h"
#include "ControllerEventsRuler.h"
#include "ControlTool.h"
#include "ControlMouseEvent.h"
#include "PropertyControlRuler.h"
#include "misc/Debug.h"

#include <QCursor>

namespace Rosegarden
{

PropertyAdjuster::PropertyAdjuster(ControlRuler *parent) :
    ControlTool("", "PropertyAdjuster", parent)
{
//    if (dynamic_cast<ControllerEventsRuler *> (parent)) m_canSelect = true;
//    else m_canSelect = false;
    m_canSelect = true;
}

void
PropertyAdjuster::handleLeftButtonPress(const ControlMouseEvent *e)
{
    if (m_canSelect) {
        unsigned int items = e->itemList.size();
        if (m_ruler->getSelectedItems()->empty() && items > 0) {
            // If there aren't selected items, we select the last of
            // the mouse click items because it is over the others.
            m_ruler->addToSelection(e->itemList[items-1]);
        }
    }

    if (m_overItem) {
        m_ruler->setCursor(Qt::ClosedHandCursor);
        m_mouseLastY = e->y;
    }

    m_ruler->update();
}

FollowMode
PropertyAdjuster::handleMouseMove(const ControlMouseEvent *e)
{
    setBasicContextHelp();

    if (e->buttons == Qt::NoButton) {
        // No button pressed, set cursor style
        setCursor(e);
    }

    if ((e->buttons & Qt::LeftButton) && m_overItem) {
        // A property drag action is in progress
        float delta = (e->y-m_mouseLastY);
        m_mouseLastY = e->y;

        // Assuming here that PropertyAdjuster is only used for velocity
        int minVelocity = 127;
        int maxVelocity = 0;
        ControlItemList *selected = m_ruler->getSelectedItems();
        for (ControlItemList::iterator it = selected->begin();
                                        it != selected->end(); ++it) {
            float newY = (*it)->y() + delta;
            (*it)->setValue(newY);
            int velocity =
                dynamic_cast<PropertyControlRuler *>(m_ruler)->yToValue(newY);

            if (velocity > 127) velocity = 127;
            if (velocity < 0) velocity = 0;

            if (velocity > maxVelocity) maxVelocity = velocity;
            if (velocity < minVelocity) minVelocity = velocity;
        }
        m_ruler->update();

        if (minVelocity == maxVelocity) {
            setContextHelp(tr("Velocity: %1").arg(minVelocity));
        } else {
            setContextHelp(tr("Velocity: %1 to %2")
                                .arg(minVelocity).arg(maxVelocity));
        }
    }
    return NO_FOLLOW;
}

void
PropertyAdjuster::handleMouseRelease(const ControlMouseEvent *e)
{
    m_ruler->updateSegment();

    if (m_overItem) {
        // This is the end of a drag event, reset the cursor to the state that it started
        m_ruler->setCursor(Qt::PointingHandCursor);
    }

    // May have moved off the item during a drag so use setCursor to correct its state
    setCursor(e);

    setBasicContextHelp();
}

void
PropertyAdjuster::setCursor(const ControlMouseEvent *e)
{
    bool isOverItem = false;

    for (ControlItemVector::const_iterator it = e->itemList.begin();
         it != e->itemList.end();
         ++it) {
        if ((*it)->isSelected() || m_canSelect) {
            isOverItem = true;
            break;
        }
    }

    if (!m_overItem) {
        if (isOverItem) {
            m_ruler->setCursor(Qt::PointingHandCursor);
            m_overItem = true;
        }
    } else {
        if (!isOverItem) {
            m_ruler->unsetCursor();
            m_overItem = false;
        }
    }
}

void
PropertyAdjuster::ready()
{
//    connect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
//            m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));

//    m_widget->setCanvasCursor(Qt::sizeAllCursor);

    setBasicContextHelp();
}

void
PropertyAdjuster::stow()
{
//    disconnect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
//               m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));
}

void
PropertyAdjuster::setBasicContextHelp()
{
    setContextHelp("");
}

QString
PropertyAdjuster::ToolName()
{
    return "adjuster";
}

}

