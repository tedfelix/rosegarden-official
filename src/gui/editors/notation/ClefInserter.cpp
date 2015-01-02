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

#include "ClefInserter.h"

#include "commands/notation/ClefInsertionCommand.h"

#include "NotationTool.h"
#include "NotationWidget.h"
#include "NotationElement.h"
#include "NotationStaff.h"
#include "NotationScene.h"
#include "NotationMouseEvent.h"

#include "document/CommandHistory.h"

namespace Rosegarden
{

ClefInserter::ClefInserter(NotationWidget *widget) :
    NotationTool("clefinserter.rc", "ClefInserter", widget),
    m_clef(Clef::Treble)
{
    createAction("select", SLOT(slotSelectSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("notes", SLOT(slotNotesSelected()));
}

void
ClefInserter::slotNotesSelected()
{
    invokeInParentView("draw");
}

void
ClefInserter::slotEraseSelected()
{
    invokeInParentView("erase");
}

void
ClefInserter::slotSelectSelected()
{
    invokeInParentView("select");
}

void
ClefInserter::ready()
{
    m_widget->setCanvasCursor(Qt::CrossCursor);
//!!!    m_nParentView->setHeightTracking(false);
}

void
ClefInserter::slotSetClef(Clef clefType)
{
    m_clef = clefType;
}

void
ClefInserter::handleLeftButtonPress(const NotationMouseEvent *e)
{
    if (!e->staff || !e->element) return;

    timeT time = e->element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    ClefInsertionCommand *command =
        new ClefInsertionCommand(e->staff->getSegment(), time, m_clef);

    CommandHistory::getInstance()->addCommand(command);

    Event *event = command->getLastInsertedEvent();
    if (event) {
        m_scene->setSingleSelectedEvent(&e->staff->getSegment(), event, false);
    }
}

const QString ClefInserter::ToolName = "clefinserter";

}

#include "ClefInserter.moc"


