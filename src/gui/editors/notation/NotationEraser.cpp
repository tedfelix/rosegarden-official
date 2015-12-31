/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NotationEraser.h"

#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "commands/notation/EraseEventCommand.h"
#include "NotationTool.h"
#include "NotationWidget.h"
#include "NotationStaff.h"
#include "NotationElement.h"
#include "NotationMouseEvent.h"
#include "document/CommandHistory.h"

#include <QSettings>
#include <QAction>

namespace Rosegarden
{

NotationEraser::NotationEraser(NotationWidget *widget) :
    NotationTool("notationeraser.rc", "NotationEraser", widget),
    m_collapseRest(false)
{
    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    m_collapseRest = qStrToBool(settings.value("collapse", "false"));

    QAction *a = createAction("toggle_rest_collapse", SLOT(slotToggleRestCollapse()));
    a->setCheckable(true);
    a->setChecked(m_collapseRest);

    createAction("select", SLOT(slotSelectSelected()));
    createAction("insert", SLOT(slotInsertSelected()));

    settings.endGroup();
}

void
NotationEraser::ready()
{
    m_widget->setCanvasCursor(Qt::PointingHandCursor);
//!!!    m_nParentView->setHeightTracking(false);
}

void
NotationEraser::handleLeftButtonPress(const NotationMouseEvent *e)
{
    if (!e->element || !e->staff) return;

    EraseEventCommand *command =
        new EraseEventCommand(e->staff->getSegment(),
                              e->element->event(),
                              m_collapseRest);

    CommandHistory::getInstance()->addCommand(command);
}

void
NotationEraser::slotToggleRestCollapse()
{
    m_collapseRest = !m_collapseRest;
}

void
NotationEraser::slotInsertSelected()
{
    invokeInParentView("draw");
}

void
NotationEraser::slotSelectSelected()
{
    invokeInParentView("select");
}

const QString NotationEraser::ToolName = "notationeraser";

}

