/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ControlEraser.h"

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

ControlEraser::ControlEraser(ControlRuler *parent) :
    ControlTool("", "ControlEraser", parent)
{
//    createAction("select", SLOT(slotSelectSelected()));
//    createAction("draw", SLOT(slotDrawSelected()));
//    createAction("erase", SLOT(slotEraseSelected()));
//    createAction("resize", SLOT(slotResizeSelected()));
//
//    createMenu();
}

void
ControlEraser::handleLeftButtonPress(const ControlMouseEvent *e)
{
    if (!e->itemList.size()) return;

    ControllerEventsRuler *ruler = static_cast <ControllerEventsRuler*> (m_ruler);
    ControlItemVector::const_iterator it;

    // If any of the covered items is selected, delete entire selection
    for (it = e->itemList.begin(); it != e->itemList.end(); ++it) {
        if ((*it)->isSelected()) {
            ruler->eraseControllerEvent();
            break;
        }
    }

    if (it == e->itemList.end()) {
        it = e->itemList.begin();
        ruler->clearSelectedItems();
        ruler->addToSelection(*it);
        ruler->eraseControllerEvent();
    }
}

FollowMode
ControlEraser::handleMouseMove(const ControlMouseEvent *e)
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
    
    return NO_FOLLOW;
}

void
ControlEraser::handleMouseRelease(const ControlMouseEvent *e)
{
    if (m_overItem) {
        // This is the end of a drag event, reset the cursor to the state that it started
        m_ruler->setCursor(Qt::PointingHandCursor);
    }

    // May have moved off the item during a drag so use setCursor to correct its state
    setCursor(e);
}

void ControlEraser::setCursor(const ControlMouseEvent *e)
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
            m_ruler->setCursor(Qt::ArrowCursor);
            m_overItem = false;
        }
    }
}

void ControlEraser::ready()
{
    m_ruler->setCursor(Qt::CrossCursor);
    m_overItem = false;
//    connect(this, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
//            m_widget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));

//    m_widget->setCanvasCursor(Qt::sizeAllCursor);
//    setBasicContextHelp();
}

void ControlEraser::stow()
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

QString ControlEraser::ToolName() { return "eraser"; }

}

