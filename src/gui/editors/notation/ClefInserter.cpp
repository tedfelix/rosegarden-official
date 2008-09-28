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


#include "ClefInserter.h"

#include <klocale.h>

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/ViewElement.h"
#include "commands/notation/ClefInsertionCommand.h"
#include "gui/general/EditTool.h"
#include "gui/general/LinedStaff.h"
#include "NotationElement.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"

#include <QAction>
#include <QIcon>
#include <QString>
#include <QMouseEvent>

namespace Rosegarden
{

ClefInserter::ClefInserter(NotationView* view)
        : NotationTool("ClefInserter", view),
        m_clef(Clef::Treble)
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
			connect( qa_notes, SIGNAL(triggered()), this, SLOT(slotNotesSelected())  );

    createMenu("clefinserter.rc");
}

void ClefInserter::slotNotesSelected()
{
    m_nParentView->slotLastNoteAction();
}

void ClefInserter::slotEraseSelected()
{
//     m_parentView->actionCollection()->action("erase")->activate();
	QAction* tac = this->findChild<QAction*>( "erase" );
	tac->setEnabled( true );
}

void ClefInserter::slotSelectSelected()
{
//     m_parentView->actionCollection()->action("select")->activate();
	QAction* tac = this->findChild<QAction*>( "select" );
	tac->setEnabled( true );
}

void ClefInserter::ready()
{
    m_nParentView->setCanvasCursor(Qt::crossCursor);
    m_nParentView->setHeightTracking(false);
}

void ClefInserter::setClef(std::string clefType)
{
    m_clef = clefType;
}

void ClefInserter::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement*)
{
    if (staffNo < 0)
        return ;
    Event *clef = 0, *key = 0;

    LinedStaff *staff = m_nParentView->getLinedStaff(staffNo);

    NotationElementList::iterator closestElement =
        staff->getClosestElementToCanvasCoords(e->x(), (int)e->y(),
                                               clef, key, false, -1);

    if (closestElement == staff->getViewElementList()->end())
        return ;

    timeT time = (*closestElement)->event()->getAbsoluteTime(); // not getViewAbsoluteTime()


    ClefInsertionCommand *command =
        new ClefInsertionCommand(staff->getSegment(), time, m_clef);

    m_nParentView->addCommandToHistory(command);

    Event *event = command->getLastInsertedEvent();
    if (event)
        m_nParentView->setSingleSelectedEvent(staffNo, event);
}

const QString ClefInserter::ToolName     = "clefinserter";

}
#include "ClefInserter.moc"
