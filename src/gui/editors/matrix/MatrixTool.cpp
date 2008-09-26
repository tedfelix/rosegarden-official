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


#include "MatrixTool.h"

#include "gui/general/EditTool.h"
#include "MatrixView.h"
#include <QAction>
#include <QString>


namespace Rosegarden
{

MatrixTool::MatrixTool(const QString& menuName, MatrixView* parent)
        : EditTool(menuName, parent),
        m_mParentView(parent)
{}

void
MatrixTool::slotSelectSelected()
{
//	m_parentView->actionCollection()->action("select")->activate();
// 	QList<QAction*> al = m_parentView->findChildren("select");
//	m_parentView->qa_select->activate();
	
	//@@@ fix: method to find QActions could be too slow
	QAction* ac = m_parentView->findChild<QAction*>("select");
	ac->setEnabled(true);
}

void
MatrixTool::slotMoveSelected()
{
//     m_parentView->actionCollection()->action("move")->activate();
	QAction* ac = m_parentView->findChild<QAction*>("move");
	ac->setEnabled(true);
}

void
MatrixTool::slotEraseSelected()
{
//     m_parentView->actionCollection()->action("erase")->activate();
	QAction* ac = m_parentView->findChild<QAction*>("erase");
	ac->setEnabled(true);
}

void
MatrixTool::slotResizeSelected()
{
//     m_parentView->actionCollection()->action("resize")->activate();
	QAction* ac = m_parentView->findChild<QAction*>("resize");
	ac->setEnabled(true);
}

void
MatrixTool::slotDrawSelected()
{
//     m_parentView->actionCollection()->action("draw")->activate();
	QAction* ac = m_parentView->findChild<QAction*>("draw");
	ac->setEnabled(true);
}

const SnapGrid &
MatrixTool::getSnapGrid() const
{
    return m_mParentView->getSnapGrid();
}

}
#include "MatrixTool.moc"
