/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <Q3CanvasPixmap>
#include "MatrixEraser.h"
#include "misc/Debug.h"

#include <QDir>
#include "base/ViewElement.h"
#include "commands/matrix/MatrixEraseCommand.h"
#include "gui/general/EditTool.h"
#include "MatrixStaff.h"
#include "MatrixTool.h"
#include "MatrixView.h"
#include <QAction>
#include <QIcon>
#include <QString>
#include <QMouseEvent>


namespace Rosegarden
{

MatrixEraser::MatrixEraser(MatrixView* parent)
        : MatrixTool("MatrixEraser", parent),
        m_currentStaff(0)
{
    createAction("resize", SLOT(slotResizeSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("select", SLOT(slotSelectSelected()));
    createAction("move", SLOT(slotMoveSelected()));

    createMenu("matrixeraser.rc");
}

void MatrixEraser::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent*,
        ViewElement* el)
{
    MATRIX_DEBUG << "MatrixEraser::handleLeftButtonPress : el = "
    << el << endl;

    if (!el)
        return ; // nothing to erase

    m_currentStaff = m_mParentView->getStaff(staffNo);

    MatrixEraseCommand* command =
        new MatrixEraseCommand(m_currentStaff->getSegment(), el->event());

    m_mParentView->addCommandToHistory(command);

    m_mParentView->update();
}

void MatrixEraser::ready()
{
    m_mParentView->setCanvasCursor(Qt::pointingHandCursor);
    setBasicContextHelp();
}

void MatrixEraser::setBasicContextHelp()
{
    setContextHelp(tr("Click on a note to delete it"));
}

const QString MatrixEraser::ToolName    = "eraser";

}
