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

#include "NotationToolBox.h"

#include "gui/general/BaseTool.h"
#include "NotationWidget.h"
#include "NotationTool.h"
#include "NoteRestInserter.h"
#include "ClefInserter.h"
#include "TextInserter.h"
#include "GuitarChordInserter.h"
#include "SymbolInserter.h"
#include "NotationEraser.h"
#include "NotationSelector.h"
#include "NotationScene.h"

#include <QString>
#include <QMessageBox>

namespace Rosegarden
{

NotationToolBox::NotationToolBox(NotationWidget *parent) :
    BaseToolBox(parent),
    m_widget(parent),
    m_scene(0)
{
}

BaseTool *
NotationToolBox::createTool(QString toolName)
{
    NotationTool *tool = 0;

    QString toolNamelc = toolName.toLower();
    
    if (toolNamelc == ClefInserter::ToolName())

        tool = new ClefInserter(m_widget);

    else if (toolNamelc == SymbolInserter::ToolName())

        tool = new SymbolInserter(m_widget);

    else if (toolNamelc == TextInserter::ToolName())

        tool = new TextInserter(m_widget);

    else if (toolNamelc == GuitarChordInserter::ToolName())

        tool = new GuitarChordInserter(m_widget);

    else if (toolNamelc == NotationEraser::ToolName())

        tool = new NotationEraser(m_widget);

    else if (toolNamelc == NotationSelector::ToolName())

        tool = new NotationSelector(m_widget);

    else if (toolNamelc == NotationSelectorNoTies::ToolName())

        tool = new NotationSelectorNoTies(m_widget);

    else if (toolNamelc == NoteRestInserter::ToolName())

        tool = new NoteRestInserter(m_widget);

    else {
        QMessageBox::critical(0, tr("Rosegarden"), QString("NotationToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    if (m_scene) {
        tool->setScene(m_scene);
        // This would be great, if NotationTool had such a slot.  All
        // of its derivers do.  This needs to be reworked and carefully
        // tested to make sure it is doing what it should be doing.
        //connect(m_scene, SIGNAL(eventRemoved(Event *)),
        //        tool, SLOT(handleEventRemoved(Event *)));
    }

    return tool;
}

void
NotationToolBox::setScene(NotationScene *scene)
{
    m_scene = scene;

    for (QHash<QString, BaseTool *>::iterator i = m_tools.begin();
         i != m_tools.end(); ++i) {
        NotationTool *nt = dynamic_cast<NotationTool *>(*i);
        if (nt) {
            nt->setScene(scene);
            // This would be great, if NotationTool had such a slot.  All
            // of its derivers do.  This needs to be reworked and carefully
            // tested to make sure it is doing what it should be doing.
            //connect(scene, SIGNAL(eventRemoved(Event *)),
            //        nt, SLOT(handleEventRemoved(Event *)));
        }
    }
}

}


