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


#include "SegmentToolBox.h"

#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "gui/general/BaseToolBox.h"
#include "SegmentTool.h"
#include "SegmentSelector.h"
#include "SegmentEraser.h"
#include "SegmentJoiner.h"
#include "SegmentMover.h"
#include "SegmentPencil.h"
#include "SegmentResizer.h"
#include "SegmentSplitter.h"
#include <QString>
#include <QMessageBox>

namespace Rosegarden
{

SegmentToolBox::SegmentToolBox(CompositionView* parent, RosegardenDocument* doc)
        : BaseToolBox(parent),
        m_canvas(parent),
        m_doc(doc)
{}

SegmentTool* SegmentToolBox::createTool(QString toolName)
{
    SegmentTool* tool = 0;

    QString toolNamelc = toolName.toLower();
    
    if (toolNamelc == SegmentPencil::ToolName)

        tool = new SegmentPencil(m_canvas, m_doc);

    else if (toolNamelc == SegmentEraser::ToolName)

        tool = new SegmentEraser(m_canvas, m_doc);

    else if (toolNamelc == SegmentMover::ToolName)

        tool = new SegmentMover(m_canvas, m_doc);

    else if (toolNamelc == SegmentResizer::ToolName)

        tool = new SegmentResizer(m_canvas, m_doc);

    else if (toolNamelc == SegmentSelector::ToolName)

        tool = new SegmentSelector(m_canvas, m_doc);

    else if (toolNamelc == SegmentSplitter::ToolName)

        tool = new SegmentSplitter(m_canvas, m_doc);

    else if (toolNamelc == SegmentJoiner::ToolName)

        tool = new SegmentJoiner(m_canvas, m_doc);

    else {
        QMessageBox::critical(0, tr("Rosegarden"), QString("SegmentToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    return tool;
}

SegmentTool* SegmentToolBox::getTool(QString toolName)
{
    return dynamic_cast<SegmentTool*>(BaseToolBox::getTool(toolName));
}

}
