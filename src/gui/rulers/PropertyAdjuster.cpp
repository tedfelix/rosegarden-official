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

#include "PropertyAdjuster.h"

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
#include "ControlRuler.h"
#include "ControllerEventsRuler.h"
#include "ControlTool.h"
#include "ControlMouseEvent.h"
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

ControlTool::FollowMode
PropertyAdjuster::handleMouseMove(const ControlMouseEvent *e)
{
    if (e->buttons == Qt::NoButton) {
        // No button pressed, set cursor style
        setCursor(e);
    }

    if ((e->buttons & Qt::LeftButton) && m_overItem) {
        // A property drag action is in progress
        float delta = (e->y-m_mouseLastY);
        m_mouseLastY = e->y;
        ControlItemList *selected = m_ruler->getSelectedItems();
        for (ControlItemList::iterator it = selected->begin(); it != selected->end(); ++it) {
            (*it)->setValue((*it)->y()+delta);
        }
        m_ruler->update();
    }

    return NoFollow;
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
}

void PropertyAdjuster::setCursor(const ControlMouseEvent *e)
{
    bool isOverItem = false;
    std::vector<ControlItem*>::const_iterator it = e->itemList.begin();
    for (; it != e->itemList.end(); ++it) {
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

void PropertyAdjuster::ready()
{
//    connect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
//            m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));

//    m_widget->setCanvasCursor(Qt::sizeAllCursor);
//    setBasicContextHelp();
}

void PropertyAdjuster::stow()
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

const QString PropertyAdjuster::ToolName = "adjuster";

}

