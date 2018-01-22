/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MatrixEraser.h"

#include "MatrixMouseEvent.h"
#include "MatrixViewSegment.h"
#include "MatrixElement.h"
#include "MatrixWidget.h"

#include "base/ViewElement.h"
#include "commands/matrix/MatrixEraseCommand.h"
#include "document/CommandHistory.h"
#include "misc/Debug.h"

#include <Qt>

namespace Rosegarden
{

MatrixEraser::MatrixEraser(MatrixWidget *parent) :
    MatrixTool("matrixeraser.rc", "MatrixEraser", parent)
{
    createAction("resize", SLOT(slotResizeSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("select", SLOT(slotSelectSelected()));
    createAction("move", SLOT(slotMoveSelected()));

    createMenu();
}

void MatrixEraser::handleLeftButtonPress(const MatrixMouseEvent *e)
{
    if (!e->element || !e->viewSegment) return; // nothing to erase

    MatrixEraseCommand* command =
        new MatrixEraseCommand(e->viewSegment->getSegment(),
                               e->element->event());

    CommandHistory::getInstance()->addCommand(command);
}

void MatrixEraser::ready()
{
    if (m_widget) m_widget->setCanvasCursor(Qt::PointingHandCursor);
    setBasicContextHelp();
}

void MatrixEraser::setBasicContextHelp()
{
    setContextHelp(tr("Click on a note to delete it"));
}

QString MatrixEraser::ToolName() { return "eraser"; }

}


