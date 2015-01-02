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

#include "GuitarChordInserter.h"

#include "base/Event.h"
#include "base/Exception.h"
#include "commands/notation/EraseEventCommand.h"
#include "commands/notation/GuitarChordInsertionCommand.h"
#include "document/CommandHistory.h"
#include "gui/editors/guitar/GuitarChordSelectorDialog.h"
#include "misc/Debug.h"
#include "NotationMouseEvent.h"
#include "NotationElement.h"
#include "NotationStaff.h"
#include "NotationWidget.h"
#include "NotePixmapFactory.h"

namespace Rosegarden
{

GuitarChordInserter::GuitarChordInserter(NotationWidget *widget) :
    NotationTool("guitarchordinserter.rc", "GuitarChordInserter", widget),
    m_guitarChordSelector(0)
{
    createAction("select", SLOT(slotSelectSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("notes", SLOT(slotNotesSelected()));

    m_guitarChordSelector = new GuitarChordSelectorDialog(m_widget);
    m_guitarChordSelector->init();
}

void
GuitarChordInserter::slotNotesSelected()
{
    invokeInParentView("draw");
}

void
GuitarChordInserter::slotGuitarChordSelected()
{
    // Switch to last selected Guitar Chord
    // m_nParentView->slotLastGuitarChordAction();
}

void
GuitarChordInserter::slotEraseSelected()
{
    invokeInParentView("erase");
}

void
GuitarChordInserter::slotSelectSelected()
{
    invokeInParentView("select");
}

void
GuitarChordInserter::ready()
{
    m_widget->setCanvasCursor(Qt::CrossCursor);
//!!!    m_nParentView->setHeightTracking(false);
}

void
GuitarChordInserter::handleLeftButtonPress(const NotationMouseEvent *e)
{
    if (!e->staff) return;

    if (e->element && e->exact &&
        e->element->event()->isa(Guitar::Chord::EventType)) {
        handleSelectedGuitarChord(e);
    } else {
        createNewGuitarChord(e);
    }
}

bool
GuitarChordInserter::processDialog(NotationStaff* staff,
                                   timeT& insertionTime)
{
    bool result = false;

    if (m_guitarChordSelector->exec() == QDialog::Accepted) {
        Guitar::Chord chord = m_guitarChordSelector->getChord();

        GuitarChordInsertionCommand *command =
            new GuitarChordInsertionCommand
            (staff->getSegment(), insertionTime, chord);

        CommandHistory::getInstance()->addCommand(command);
        result = true;
    }

    return result;
}

void
GuitarChordInserter::handleSelectedGuitarChord(const NotationMouseEvent *e)
{
    // Get time of where guitar chord is inserted
    timeT insertionTime = e->element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    // edit an existing guitar chord, if that's what we clicked on
    try {
        Guitar::Chord chord(*(e->element->event()));

        m_guitarChordSelector->setChord(chord);
        
        if (processDialog(e->staff, insertionTime)) {
            // Erase old guitar chord
            EraseEventCommand *command =
                new EraseEventCommand(e->staff->getSegment(),
                                      e->element->event(),
                                      false);

            CommandHistory::getInstance()->addCommand(command);
        }
    } catch (Exception e) {}
}

void GuitarChordInserter::createNewGuitarChord(const NotationMouseEvent *e)
{
    timeT insertionTime = e->element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()
    processDialog(e->staff, insertionTime);
}

const QString GuitarChordInserter::ToolName = "guitarchordinserter";

}

#include "GuitarChordInserter.moc"
