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


#include "FretboardInserter.h"

#include <klocale.h>
#include "base/Event.h"
#include "base/Exception.h"
#include "base/Staff.h"
#include "base/ViewElement.h"
#include "commands/notation/EraseEventCommand.h"
#include "commands/notation/FretboardInsertionCommand.h"
#include "gui/editors/guitar/GuitarTabSelectorDialog.h"
#include "gui/general/EditTool.h"
#include "gui/general/LinedStaff.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"
#include <kaction.h>
#include <qdialog.h>
#include <qiconset.h>
#include <qstring.h>


namespace Rosegarden
{

FretboardInserter::FretboardInserter(NotationView* view)
        : NotationTool("FretboardInserter", view),
        m_guitarChord_ref (m_nParentView)
{
    QIconSet icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::
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
                SLOT(slotNoteSelected()), actionCollection(),
                "notes");

    m_guitarChord_ref.init();
    createMenu("fretboardinserter.rc");
}

void FretboardInserter::slotFretboardSelected()
{
    // Switch to last selected Fretboard
    // m_nParentView->slotLastFretboardAction();
}

void FretboardInserter::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void FretboardInserter::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

void FretboardInserter::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    std::cout << "FretboardInserter::handleLeftButtonPress" << std::endl;

    if (staffNo < 0) {
        return ;
    }

    Staff *staff = m_nParentView->getStaff(staffNo);

    if (element && element->event()->isa(Guitar::Fingering::EventType)) {
        handleSelectedFretboard (element, staff);
    } else {
        createNewFretboard (element, staff, e);
    }
}

bool FretboardInserter::processDialog( Staff* staff,
                                       timeT& insertionTime)
{
    bool result = false;

    if (m_guitarChord_ref.exec() == QDialog::Accepted) {
        Guitar::Fingering m_chord = m_guitarChord_ref.getArrangement();

        FretboardInsertionCommand *command =
            new FretboardInsertionCommand
            (staff->getSegment(), insertionTime, m_chord);

        m_nParentView->addCommandToHistory(command);
        result = true;
    }

    return result;
}

void FretboardInserter::handleSelectedFretboard (ViewElement* element, Staff *staff)
{
    std::cout << "FretboardInserter::handleSelectedFretboard" << std::endl;


    // Get time of where fretboard is inserted
    timeT insertionTime = element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    // edit an existing fretboard, if that's what we clicked on
    try {
        Guitar::Fingering existingFret (*element->event());

        m_guitarChord_ref.setArrangement( &existingFret );
        if ( processDialog( staff, insertionTime ) ) {
            // Erase old fretboard
            EraseEventCommand *command =
                new EraseEventCommand(staff->getSegment(),
                                      element->event(),
                                      false);

            m_nParentView->addCommandToHistory(command);
        }
    } catch (Exception e) {}
}

void FretboardInserter::createNewFretboard (ViewElement* element, Staff *staff, QMouseEvent* e)
{
    std::cout << "FretboardInserter::createNewFretboard" << std::endl;
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

}
