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

#include "RestInserter.h"
#include "misc/Debug.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/BaseProperties.h"
#include "base/Segment.h"
#include "commands/notation/NoteInsertionCommand.h"
#include "commands/notation/RestInsertionCommand.h"
#include "commands/notation/TupletCommand.h"
#include "misc/Strings.h"
#include "NotationStrings.h"
#include "NotationTool.h"
#include "NotationWidget.h"
#include "NoteInserter.h"
#include "NotePixmapFactory.h"
#include "document/Command.h"
#include "document/CommandHistory.h"

#include <QAction>
#include <QIcon>
#include <QRegExp>
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

RestInserter::RestInserter(NotationWidget *widget) :
    NoteInserter("restinserter.rc", "RestInserter", widget)
{
    createAction("toggle_dot", SLOT(slotToggleDot()));
    createAction("select", SLOT(slotSelectSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("notes", SLOT(slotNotesSelected()));
}

void
RestInserter::showPreview()
{
    // no preview available for now
}

Event *
RestInserter::doAddCommand(Segment &segment, timeT time, timeT endTime,
                           const Note &note, int, Accidental)
{
    if (time < segment.getStartTime() ||
        endTime > segment.getEndMarkerTime() ||
        time + note.getDuration() > segment.getEndMarkerTime()) {
        return 0;
    }

    NoteInsertionCommand *insertionCommand =
        new RestInsertionCommand(segment, time, endTime, note);

    Command *activeCommand = insertionCommand;

    if (m_widget->isInTripletMode()) {
        Segment::iterator i(segment.findTime(time));
        if (i != segment.end() &&
            !(*i)->has(BEAMED_GROUP_TUPLET_BASE)) {

            MacroCommand *command = new MacroCommand(insertionCommand->getName());
            command->addCommand(new TupletCommand
                                (segment, time, note.getDuration()));
            command->addCommand(insertionCommand);
            activeCommand = command;
        }
    }

    CommandHistory::getInstance()->addCommand(activeCommand);

    return insertionCommand->getLastInsertedEvent();
}

void RestInserter::slotSetDots(unsigned int dots)
{
    QAction *dotsAction = findActionInParentView("toggle_dot");
	
    if (dotsAction && m_noteDots != dots) {
        dotsAction->setChecked(dots > 0);
        slotToggleDot();
        m_noteDots = dots;
    }
}

void RestInserter::slotToggleDot()
{
    m_noteDots = (m_noteDots) ? 0 : 1;
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note, true));
    actionName.replace(QRegExp("-"), "_");
	
    QAction *action = findActionInParentView( actionName );
	
    if (!action) {
        std::cerr << "WARNING: No such action as " << qstrtostr(actionName) << std::endl;
    } else {
        action->setChecked(true);
    }
}

void RestInserter::slotNotesSelected()
{
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note));
    actionName.replace(QRegExp("-"), "_");
	
    QAction *action = findActionInParentView(actionName);
    action->setChecked(true);
}

const QString RestInserter::ToolName = "restinserter";

}

#include "RestInserter.moc"


