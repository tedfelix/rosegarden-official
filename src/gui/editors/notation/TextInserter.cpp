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

#include "TextInserter.h"

#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "commands/notation/EraseEventCommand.h"
#include "commands/notation/TextInsertionCommand.h"
#include "gui/dialogs/TextEventDialog.h"
#include "NotationTool.h"
#include "NotationWidget.h"
#include "NotationElement.h"
#include "NotationStaff.h"
#include "NotationScene.h"
#include "NotationMouseEvent.h"
#include "document/CommandHistory.h"

namespace Rosegarden
{

TextInserter::TextInserter(NotationWidget *widget) :
    NotationTool("textinserter.rc", "TextInserter", widget),
    m_text("", Text::Dynamic)
{
    createAction("select", SLOT(slotSelectSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("notes", SLOT(slotNotesSelected()));
}

void
TextInserter::slotNotesSelected()
{
    invokeInParentView("draw");
}

void
TextInserter::slotEraseSelected()
{
    invokeInParentView("erase");
}

void
TextInserter::slotSelectSelected()
{
    invokeInParentView("select");
}

void
TextInserter::ready()
{
    m_widget->setCanvasCursor(Qt::CrossCursor);
//!!!    m_nParentView->setHeightTracking(false);
}

void
TextInserter::handleLeftButtonPress(const NotationMouseEvent *e)
{
    if (!e->staff || !e->element) return;

    Text defaultText(m_text);
    timeT insertionTime;
    Event *eraseEvent = 0;

    insertionTime = e->element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    if (e->exact && e->element->event()->isa(Text::EventType)) {
        // edit an existing text, if that's what we clicked on
        try {
            defaultText = Text(*e->element->event());
        } catch (Exception e) {
        }
        eraseEvent = e->element->event();
    }

    TextEventDialog *dialog = new TextEventDialog
        (m_widget, m_scene->getNotePixmapFactory(), defaultText);

    if (dialog->exec() == QDialog::Accepted) {

        m_text = dialog->getText();

        TextInsertionCommand *command =
            new TextInsertionCommand
            (e->staff->getSegment(), insertionTime, m_text);

        if (eraseEvent) {
            MacroCommand *macroCommand = new MacroCommand(command->getName());
            macroCommand->addCommand(new EraseEventCommand
                                     (e->staff->getSegment(),
                                      eraseEvent, false));
            macroCommand->addCommand(command);
            CommandHistory::getInstance()->addCommand(macroCommand);
        } else {
            CommandHistory::getInstance()->addCommand(command);
        }

        Event *event = command->getLastInsertedEvent();
        if (event) {
            m_scene->setSingleSelectedEvent(&e->staff->getSegment(), event, false);
        }
    }

    delete dialog;
}

const QString TextInserter::ToolName = "textinserter";

}

#include "TextInserter.moc"


