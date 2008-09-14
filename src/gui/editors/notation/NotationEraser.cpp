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


#include "NotationEraser.h"
#include <QApplication>

#include <klocale.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/ViewElement.h"
#include "commands/notation/EraseEventCommand.h"
#include "gui/general/EditTool.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"
#include <QAction>
#include <QSettings>
#include <QIcon>
#include <QString>


namespace Rosegarden
{

NotationEraser::NotationEraser(NotationView* view)
        : NotationTool("NotationEraser", view),
        m_collapseRest(false)
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_collapseRest = qStrToBool( settings.value("collapse", "false" ) ) ;

    QAction* qa_toggle_rest_collapse = new QAction( 0, i18n("Collapse rests after erase"), dynamic_cast<QObject*>(this) );
	connect( qa_toggle_rest_collapse, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleRestCollapse()) );
	qa_toggle_rest_collapse->setObjectName( "toggle_rest_collapse" );	//### FIX: deallocate QAction ptr
	qa_toggle_rest_collapse->setCheckable( true );	//
	qa_toggle_rest_collapse->setAutoRepeat( false );	//
	//qa_toggle_rest_collapse->setActionGroup( 0 );	// QActionGroup*
	qa_toggle_rest_collapse->setChecked( false );	//
	// ;

    QIcon icon
    (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                  makeToolbarPixmap("crotchet")));
    QAction *qa_insert = new QAction( "Switch to Insert Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_insert->setIcon(icon); 
			connect( qa_insert, SIGNAL(triggered()), this, SLOT(slotInsertSelected())  );

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::
                    makeToolbarPixmap("select")));
    QAction *qa_select = new QAction( "Switch to Select Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_select->setIcon(icon); 
			connect( qa_select, SIGNAL(triggered()), this, SLOT(slotSelectSelected())  );

    createMenu("notationeraser.rc");

    settings.endGroup();
}

void NotationEraser::ready()
{
    m_nParentView->setCanvasCursor(Qt::pointingHandCursor);
    m_nParentView->setHeightTracking(false);
}

void NotationEraser::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent*,
        ViewElement* element)
{
    if (!element || staffNo < 0)
        return ;

    EraseEventCommand *command =
        new EraseEventCommand(m_nParentView->getStaff(staffNo)->getSegment(),
                              element->event(),
                              m_collapseRest);

    m_nParentView->addCommandToHistory(command);
}

void NotationEraser::slotToggleRestCollapse()
{
    m_collapseRest = !m_collapseRest;
}

void NotationEraser::slotInsertSelected()
{
    m_nParentView->slotLastNoteAction();
}

void NotationEraser::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

const QString NotationEraser::ToolName   = "notationeraser";

}
#include "NotationEraser.moc"
