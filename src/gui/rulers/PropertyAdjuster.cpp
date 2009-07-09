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
#include "ControlRuler.h"
#include "ControlTool.h"
#include "ControlMouseEvent.h"
#include "misc/Debug.h"

#include <QCursor>

namespace Rosegarden
{

PropertyAdjuster::PropertyAdjuster(ControlRuler *parent) :
    ControlTool("", "PropertyAdjuster", parent)
{
//    createAction("select", SLOT(slotSelectSelected()));
//    createAction("draw", SLOT(slotDrawSelected()));
//    createAction("erase", SLOT(slotEraseSelected()));
//    createAction("resize", SLOT(slotResizeSelected()));
//
//    createMenu();
}

void
PropertyAdjuster::handleLeftButtonPress(const ControlMouseEvent *e)
{
}

void
PropertyAdjuster::handleMouseMove(const ControlMouseEvent *e)
{
    bool isOverItem = false;
    if (!e->itemList.empty()) {
        isOverItem = true;
    }

    if (!m_overItem) {
        if (isOverItem) {
            m_ruler->setCursor(Qt::OpenHandCursor);
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
PropertyAdjuster::handleMouseRelease(const ControlMouseEvent *e)
{
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

#include "PropertyAdjuster.moc"
