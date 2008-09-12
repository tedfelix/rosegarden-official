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


#include <Q3CanvasPixmap>
#include "MatrixEraser.h"
#include "misc/Debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include "base/ViewElement.h"
#include "commands/matrix/MatrixEraseCommand.h"
#include "gui/general/EditTool.h"
#include "MatrixStaff.h"
#include "MatrixTool.h"
#include "MatrixView.h"
#include <QAction>
#include <kglobal.h>
#include <QIcon>
#include <QString>


namespace Rosegarden
{

MatrixEraser::MatrixEraser(MatrixView* parent)
        : MatrixTool("MatrixEraser", parent),
        m_currentStaff(0)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    Q3CanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIcon icon = QIcon(pixmap);

    QAction *qa_select = new QAction( "Switch to Select Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_select->setIcon(icon); 
			connect( qa_select, SIGNAL(triggered()), this, SLOT(slotSelectSelected())  );

    QAction *qa_draw = new QAction( "Switch to Draw Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_draw->setIconText("pencil"); 
			connect( qa_draw, SIGNAL(triggered()), this, SLOT(slotDrawSelected())  );

    QAction *qa_move = new QAction( "Switch to Move Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_move->setIconText("move"); 
			connect( qa_move, SIGNAL(triggered()), this, SLOT(slotMoveSelected())  );

    pixmap.load(pixmapDir + "/toolbar/resize.xpm");
    icon = QIcon(pixmap);
    QAction *qa_resize = new QAction( "Switch to Resize Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_resize->setIcon(icon); 
			connect( qa_resize, SIGNAL(triggered()), this, SLOT(slotResizeSelected())  );

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
    setContextHelp(i18n("Click on a note to delete it"));
}

const QString MatrixEraser::ToolName    = "eraser";

}
