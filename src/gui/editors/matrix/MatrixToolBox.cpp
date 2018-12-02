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

#define RG_MODULE_STRING "[MatrixToolBox]"

#include "MatrixToolBox.h"
#include "MatrixTool.h"
#include "MatrixWidget.h"
#include "MatrixPainter.h"
#include "MatrixEraser.h"
#include "MatrixSelector.h"
#include "MatrixMover.h"
#include "MatrixResizer.h"
#include "MatrixScene.h"
#include "MatrixVelocity.h"
#include "misc/Debug.h"

#include <QString>
#include <QMessageBox>

namespace Rosegarden
{

MatrixToolBox::MatrixToolBox(MatrixWidget *parent) :
    BaseToolBox(parent),
    m_widget(parent),
    m_scene(nullptr)
{
}

MatrixToolBox::~MatrixToolBox()
{
    RG_DEBUG << "MatrixToolBox::~MatrixToolBox()";
}

BaseTool *
MatrixToolBox::createTool(QString toolName)
{
    MatrixTool *tool = nullptr;

    QString toolNamelc = toolName.toLower();

    if (toolNamelc == MatrixPainter::ToolName())

        tool = new MatrixPainter(m_widget);

    else if (toolNamelc == MatrixEraser::ToolName())

        tool = new MatrixEraser(m_widget);

    else if (toolNamelc == MatrixSelector::ToolName())

        tool = new MatrixSelector(m_widget);

    else if (toolNamelc == MatrixMover::ToolName())

        tool = new MatrixMover(m_widget);

    else if (toolNamelc == MatrixResizer::ToolName())

        tool = new MatrixResizer(m_widget);

    else if (toolNamelc == MatrixVelocity::ToolName())

        tool = new MatrixVelocity(m_widget);
    
    else {
        QMessageBox::critical(nullptr, tr("Rosegarden"), QString("MatrixToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return nullptr;
    }

    m_tools.insert(toolName, tool);

    if (m_scene) {
        tool->setScene(m_scene);
        connect(m_scene, &MatrixScene::eventRemoved,
                tool, &MatrixTool::handleEventRemoved);
    }

    return tool;
}

void
MatrixToolBox::setScene(MatrixScene *scene)
{
    m_scene = scene;

    for (QHash<QString, BaseTool *>::iterator i = m_tools.begin();
         i != m_tools.end(); ++i) {
        MatrixTool *nt = dynamic_cast<MatrixTool *>(*i);
        if (nt) {
            nt->setScene(scene);
            connect(scene, &MatrixScene::eventRemoved,
                    nt, &MatrixTool::handleEventRemoved);
        }
    }
}

}
