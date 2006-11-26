/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
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
#include <kaction.h>
#include <kcommand.h>
#include <qiconset.h>
#include <qregexp.h>
#include <qstring.h>


namespace Rosegarden
{

using namespace BaseProperties;

RestInserter::RestInserter(NotationView* view)
    : NoteInserter("RestInserter", view)
{
    QIconSet icon;
    
    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                      makeToolbarPixmap("dotted-rest-crotchet")));
    new KToggleAction(i18n("Dotted rest"), icon, 0, this,
                      SLOT(slotToggleDot()), actionCollection(),
                      "toggle_dot");
    
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                                 makeToolbarPixmap("select")));
    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                      makeToolbarPixmap("crotchet")));
    new KAction(i18n("Switch to Inserting Notes"), icon, 0, this,
                SLOT(slotNotesSelected()), actionCollection(),
                "notes");

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

    KCommand *activeCommand = insertionCommand;

    if (m_nParentView->isInTripletMode()) {
        Segment::iterator i(segment.findTime(time));
        if (i != segment.end() &&
            !(*i)->has(BEAMED_GROUP_TUPLET_BASE)) {

            KMacroCommand *command = new KMacroCommand(insertionCommand->name());
            command->addCommand(new TupletCommand
                                (segment, time, note.getDuration()));
            command->addCommand(insertionCommand);
            activeCommand = command;
        }
    }

    m_nParentView->addCommandToHistory(activeCommand);

    return insertionCommand->getLastInsertedEvent();
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
    actionName.replace(QRegExp(" "), "_");
    m_parentView->actionCollection()->action(actionName)->activate();
}

const QString RestInserter::ToolName     = "restinserter";

}
#include "RestInserter.moc"
