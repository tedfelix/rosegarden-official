/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ControlToolBox.h"
#include "ControlTool.h"
#include "ControlRuler.h"
#include "PropertyAdjuster.h"
#include "ControlSelector.h"
#include "ControlPainter.h"
#include "ControlEraser.h"
#include "ControlMover.h"

#include <QString>
#include <QMessageBox>

namespace Rosegarden
{

ControlToolBox::ControlToolBox(ControlRuler *parent) :
    BaseToolBox(parent),
    m_ruler(parent)
{
}

BaseTool *
ControlToolBox::createTool(QString toolName)
{
    ControlTool *tool = 0;

    QString toolNamelc = toolName.toLower();

    if (toolNamelc == PropertyAdjuster::ToolName)
        tool = new PropertyAdjuster(m_ruler);
    else if (toolNamelc == ControlPainter::ToolName)
        tool = new ControlPainter(m_ruler);
    else if (toolNamelc == ControlEraser::ToolName)
        tool = new ControlEraser(m_ruler);
    else if (toolNamelc == ControlSelector::ToolName)
        tool = new ControlSelector(m_ruler);
    else if (toolNamelc == ControlMover::ToolName)
        tool = new ControlMover(m_ruler);
    else
    {
        QMessageBox::critical(0, tr("Rosegarden"), QString("ControlToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

//    if (m_scene) {
//        tool->setScene(m_scene);
//        connect(m_scene, SIGNAL(eventRemoved(Event *)),
//                tool, SLOT(handleEventRemoved(Event *)));
//    }

    return tool;
}

}
#include "ControlToolBox.moc"
