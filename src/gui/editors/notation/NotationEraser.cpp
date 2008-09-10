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
#include "document/ConfigGroups.h"
#include "base/ViewElement.h"
#include "commands/notation/EraseEventCommand.h"
#include "gui/general/EditTool.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"
#include <kaction.h>
#include <QSettings>
#include <QIcon>
#include <QString>


namespace Rosegarden
{

NotationEraser::NotationEraser(NotationView* view)
        : NotationTool("NotationEraser", view),
        m_collapseRest(false)
{
    QSettings config;
    config.beginGroup( NotationViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( NotationViewConfigGroup );
    //  

    m_collapseRest = qStrToBool( config.value("collapse", "false" ) ) ;

    new KToggleAction(i18n("Collapse rests after erase"), 0, this,
                      SLOT(slotToggleRestCollapse()), actionCollection(),
                      "toggle_rest_collapse");

    QIcon icon
    (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                  makeToolbarPixmap("crotchet")));
    new KAction(i18n("Switch to Insert Tool"), icon, 0, this,
                SLOT(slotInsertSelected()), actionCollection(),
                "insert");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::
                    makeToolbarPixmap("select")));
    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    createMenu("notationeraser.rc");
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
