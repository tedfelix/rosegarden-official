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
    ControlMover(parent, "ControlPainter")
{
    m_overCursor = Qt::OpenHandCursor;
    m_notOverCursor = Qt::CrossCursor;
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
    if (e->itemList.size()) {
        ControllerEventsRuler *ruler = static_cast <ControllerEventsRuler*> (m_ruler);
        std::vector <ControlItem*>::const_iterator it = e->itemList.begin();
        ruler->clearSelectedItems();
        ruler->addToSelection(*it);
        ruler->eraseControllerEvent();

        m_ruler->setCursor(Qt::CrossCursor);
    }
    else {
        // Make new control event here
        // This tool should not be applied to a PropertyControlRuler but in case it is
        ControllerEventsRuler* ruler = dynamic_cast <ControllerEventsRuler*>(m_ruler);
        //if (ruler) ruler->insertControllerEvent(e->x,e->y);
        if (ruler) {
            ControlItem *item = ruler->addControlItem(e->x,e->y);
            ControlMouseEvent *newevent = new ControlMouseEvent(e);
            newevent->itemList.push_back(item);
            m_overItem = true;
            ControlMover::handleLeftButtonPress(newevent);
        }
    }
    
}

const QString ControlPainter::ToolName = "painter";
}

#include "ControlPainter.moc"
