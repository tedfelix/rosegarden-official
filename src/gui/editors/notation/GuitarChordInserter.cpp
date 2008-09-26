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


#include "GuitarChordInserter.h"

#include <klocale.h>

#include "base/Event.h"
#include "base/Exception.h"
#include "base/Staff.h"
#include "base/ViewElement.h"
#include "commands/notation/EraseEventCommand.h"
#include "commands/notation/GuitarChordInsertionCommand.h"
#include "gui/general/EditTool.h"
#include "gui/general/LinedStaff.h"
#include "gui/editors/guitar/GuitarChordSelectorDialog.h"
#include "misc/Debug.h"
#include "NotationElement.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"

#include <QAction>
#include <QDialog>
#include <QIcon>
#include <QString>
#include <QMouseEvent>

namespace Rosegarden
{

GuitarChordInserter::GuitarChordInserter(NotationView* view)
        : NotationTool("GuitarChordInserter", view),
        m_guitarChordSelector(0)
{
    QIcon icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::
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
			connect( qa_notes, SIGNAL(triggered()), this, SLOT(slotNoteSelected())  );

    m_guitarChordSelector = new GuitarChordSelectorDialog(m_nParentView);
    m_guitarChordSelector->init();
    createMenu("guitarchordinserter.rc");
}

void GuitarChordInserter::slotGuitarChordSelected()
{
    // Switch to last selected Guitar Chord
    // m_nParentView->slotLastGuitarChordAction();
}

void GuitarChordInserter::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void GuitarChordInserter::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

void GuitarChordInserter::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    NOTATION_DEBUG << "GuitarChordInserter::handleLeftButtonPress" << endl;

    if (staffNo < 0) {
        return ;
    }

    Staff *staff = m_nParentView->getStaff(staffNo);

    if (element && element->event()->isa(Guitar::Chord::EventType)) {
        handleSelectedGuitarChord (element, staff);
    } else {
        createNewGuitarChord (element, staff, e);
    }
}

bool GuitarChordInserter::processDialog( Staff* staff,
                                       timeT& insertionTime)
{
    bool result = false;

    if (m_guitarChordSelector->exec() == QDialog::Accepted) {
        Guitar::Chord chord = m_guitarChordSelector->getChord();

        GuitarChordInsertionCommand *command =
            new GuitarChordInsertionCommand
            (staff->getSegment(), insertionTime, chord);

        m_nParentView->addCommandToHistory(command);
        result = true;
    }

    return result;
}

void GuitarChordInserter::handleSelectedGuitarChord (ViewElement* element, Staff *staff)
{
    NOTATION_DEBUG << "GuitarChordInserter::handleSelectedGuitarChord" << endl;


    // Get time of where guitar chord is inserted
    timeT insertionTime = element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    // edit an existing guitar chord, if that's what we clicked on
    try {
        Guitar::Chord chord(*(element->event()));

        m_guitarChordSelector->setChord(chord);
        
        if ( processDialog( staff, insertionTime ) ) {
            // Erase old guitar chord
            EraseEventCommand *command =
                new EraseEventCommand(staff->getSegment(),
                                      element->event(),
                                      false);

            m_nParentView->addCommandToHistory(command);
        }
    } catch (Exception e) {}
}

void GuitarChordInserter::createNewGuitarChord (ViewElement* element, Staff *staff, QMouseEvent* e)
{
    NOTATION_DEBUG << "GuitarChordInserter::createNewGuitarChord" << endl;
    Event *clef = 0, *key = 0;

    LinedStaff *s = dynamic_cast<LinedStaff *>(staff);

    NotationElementList::iterator closestElement =
        s->getClosestElementToCanvasCoords(e->x(), (int)e->y(),
                                           clef, key, false, -1);

    if (closestElement == staff->getViewElementList()->end()) {
        return ;
    }

    timeT insertionTime = (*closestElement)->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    processDialog( staff, insertionTime );
}

const QString GuitarChordInserter::ToolName = "guitarchordinserter";

}
#include "GuitarChordInserter.moc"
