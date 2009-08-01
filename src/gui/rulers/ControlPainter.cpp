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

#include "ControlPainter.h"

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

ControlPainter::ControlPainter(ControlRuler *parent) :
    ControlTool("", "ControlPainter", parent)
{
//    createAction("select", SLOT(slotSelectSelected()));
//    createAction("draw", SLOT(slotDrawSelected()));
//    createAction("erase", SLOT(slotEraseSelected()));
//    createAction("resize", SLOT(slotResizeSelected()));
//
//    createMenu();
}

void
ControlPainter::handleLeftButtonPress(const ControlMouseEvent *e)
{
    if (m_overItem) {
        m_ruler->setCursor(Qt::ClosedHandCursor);
        m_mouseLastY = e->y;
    }
    else {
        // Make new control event here
        // This tool should not be applied to a PropertyControlRuler but in case it is
        ControllerEventsRuler* ruler = dynamic_cast <ControllerEventsRuler*>(m_ruler);
        //if (ruler) ruler->insertControllerEvent(e->x,e->y);
        if (ruler) ruler->addControlItem(e->x,e->y);
    }
}

void
ControlPainter::handleMouseMove(const ControlMouseEvent *e)
{
    if (e->buttons == Qt::NoButton) {
        // No button pressed, set cursor style
        setCursor(e);
    }

    if ((e->buttons & Qt::LeftButton) && m_overItem) {
        // A property drag action is in progress
//        float delta = (e->value-m_mouseLastY);
//        m_mouseLastY = e->value;
//        ControlItemList *selected = m_ruler->getSelectedItems();
//        for (ControlItemList::iterator it = selected->begin(); it != selected->end(); ++it) {
//            (*it)->setValue((*it)->getValue()+delta);
//        }
    }
}

void
ControlPainter::handleMouseRelease(const ControlMouseEvent *e)
{
    if (m_overItem) {
        // This is the end of a drag event, reset the cursor to the state that it started
        m_ruler->setCursor(Qt::PointingHandCursor);
    }

    // May have moved off the item during a drag so use setCursor to correct its state
    setCursor(e);
}

void ControlPainter::setCursor(const ControlMouseEvent *e)
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

void ControlPainter::ready()
{
    m_ruler->setCursor(Qt::CrossCursor);
    m_overItem = false;
//    connect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
//            m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));

//    m_widget->setCanvasCursor(Qt::sizeAllCursor);
//    setBasicContextHelp();
}

void ControlPainter::stow()
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

const QString ControlPainter::ToolName = "painter";

}

#include "ControlPainter.moc"
