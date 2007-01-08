/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "MatrixTool.h"

#include "gui/general/EditTool.h"
#include "MatrixView.h"
#include <kaction.h>
#include <qstring.h>


namespace Rosegarden
{

MatrixTool::MatrixTool(const QString& menuName, MatrixView* parent)
        : EditTool(menuName, parent),
        m_mParentView(parent)
{}

void
MatrixTool::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

void
MatrixTool::slotMoveSelected()
{
    m_parentView->actionCollection()->action("move")->activate();
}

void
MatrixTool::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void
MatrixTool::slotResizeSelected()
{
    m_parentView->actionCollection()->action("resize")->activate();
}

void
MatrixTool::slotDrawSelected()
{
    m_parentView->actionCollection()->action("draw")->activate();
}

const SnapGrid &
MatrixTool::getSnapGrid() const
{
    return m_mParentView->getSnapGrid();
}

}
#include "MatrixTool.moc"
