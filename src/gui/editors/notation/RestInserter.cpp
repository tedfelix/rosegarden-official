/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RestInserter.h"

#include <klocale.h>
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/BaseProperties.h"
#include "base/Segment.h"
#include "commands/notation/NoteInsertionCommand.h"
#include "commands/notation/RestInsertionCommand.h"
#include "commands/notation/TupletCommand.h"
#include "gui/general/EditTool.h"
#include "NotationStrings.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NoteInserter.h"
#include "NotePixmapFactory.h"
#include <QAction>
#include "document/Command.h"
#include <QIcon>
#include <QRegExp>
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

RestInserter::RestInserter(NotationView* view)
    : NoteInserter("RestInserter", view)
{
    QIcon icon;
    
    icon = QIcon
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                      makeToolbarPixmap("dotted-rest-crotchet")));
    new KToggleAction(i18n("Dotted rest"), icon, 0, this,
                      SLOT(slotToggleDot()), actionCollection(),
                      "toggle_dot");
    
    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                                 makeToolbarPixmap("select")));
    QAction *qa_select = new QAction( "Switch to Select Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_select->setIcon(icon); 
			connect( qa_select, SIGNAL(triggered()), this, SLOT(slotSelectSelected())  );

    QAction *qa_erase = new QAction( "Switch to Erase Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_erase->setIconText("eraser"); 
			connect( qa_erase, SIGNAL(triggered()), this, SLOT(slotEraseSelected())  );

    icon = QIcon
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                      makeToolbarPixmap("crotchet")));
    QAction *qa_notes = new QAction( "Switch to Inserting Notes", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_notes->setIcon(icon); 
			connect( qa_notes, SIGNAL(triggered()), this, SLOT(slotNotesSelected())  );

    createMenu("restinserter.rc");
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

    if (m_nParentView->isInTripletMode()) {
        Segment::iterator i(segment.findTime(time));
        if (i != segment.end() &&
            !(*i)->has(BEAMED_GROUP_TUPLET_BASE)) {

            MacroCommand *command = new MacroCommand(insertionCommand->objectName());
            command->addCommand(new TupletCommand
                                (segment, time, note.getDuration()));
            command->addCommand(insertionCommand);
            activeCommand = command;
        }
    }

    m_nParentView->addCommandToHistory(activeCommand);

    return insertionCommand->getLastInsertedEvent();
}

void RestInserter::slotSetDots(unsigned int dots)
{
    KToggleAction *dotsAction = dynamic_cast<KToggleAction *>
                                (actionCollection()->action("toggle_dot"));
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
    KAction *action = m_parentView->actionCollection()->action(actionName);
    if (!action) {
        std::cerr << "WARNING: No such action as " << actionName << std::endl;
    } else {
        action->activate();
    }
}

void RestInserter::slotNotesSelected()
{
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note));
    actionName.replace(QRegExp("-"), "_");
    m_parentView->actionCollection()->action(actionName)->activate();
}

const QString RestInserter::ToolName     = "restinserter";

}
#include "RestInserter.moc"
