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


#include "MatrixToolBox.h"

#include "gui/general/EditToolBox.h"
#include "gui/general/EditTool.h"
#include "MatrixView.h"
#include "MatrixPainter.h"
#include "MatrixEraser.h"
#include "MatrixSelector.h"
#include "MatrixMover.h"
#include "MatrixResizer.h"

#include <qstring.h>
#include <kmessagebox.h>

namespace Rosegarden
{

MatrixToolBox::MatrixToolBox(MatrixView* parent)
        : EditToolBox(parent),
        m_mParentView(parent)
{}

EditTool* MatrixToolBox::createTool(const QString& toolName)
{
    MatrixTool* tool = 0;

    QString toolNamelc = toolName.lower();

    if (toolNamelc == MatrixPainter::ToolName)

        tool = new MatrixPainter(m_mParentView);

    else if (toolNamelc == MatrixEraser::ToolName)

        tool = new MatrixEraser(m_mParentView);

    else if (toolNamelc == MatrixSelector::ToolName)

        tool = new MatrixSelector(m_mParentView);

    else if (toolNamelc == MatrixMover::ToolName)

        tool = new MatrixMover(m_mParentView);

    else if (toolNamelc == MatrixResizer::ToolName)

        tool = new MatrixResizer(m_mParentView);

    else {
        KMessageBox::error(0, QString("MatrixToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    return tool;
    
}

}
#include "MatrixToolBox.moc"
