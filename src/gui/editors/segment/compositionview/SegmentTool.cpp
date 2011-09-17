/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentTool.h"

#include "misc/Debug.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/general/BaseTool.h"
#include "SegmentToolBox.h"
#include "document/Command.h"

#include <QMainWindow>
#include <QPoint>
#include <QMenu>
#include <QMouseEvent>


namespace Rosegarden
{

SegmentTool::SegmentTool(CompositionView* canvas, RosegardenDocument *doc)
        : BaseTool("segment_tool_menu", canvas),
        m_canvas(canvas),
        m_doc(doc),
        m_changeMade(false)
{}

SegmentTool::~SegmentTool()
{}


void SegmentTool::ready()
{
    m_canvas->viewport()->setCursor(Qt::ArrowCursor);
}

void
SegmentTool::handleRightButtonPress(QMouseEvent *e)
{
    if (m_currentIndex) // mouse button is pressed for some tool
        return ;

    RG_DEBUG << "SegmentTool::handleRightButtonPress()\n";

    setCurrentIndex(m_canvas->getFirstItemAt(e->pos()));

    if (m_currentIndex) {
        if (!m_canvas->getModel()->isSelected(m_currentIndex)) {

            m_canvas->getModel()->clearSelected();
            m_canvas->getModel()->setSelected(m_currentIndex);
            m_canvas->getModel()->signalSelection();
        }
    }

    showMenu();
    setCurrentIndex(CompositionItem());
}

void
SegmentTool::createMenu()
{
    RG_DEBUG << "SegmentTool::createMenu()\n";

    RosegardenMainWindow *mainWindow =
        dynamic_cast<RosegardenMainWindow*>(m_doc->parent());

    if (mainWindow) {

        m_menu = mainWindow->findChild<QMenu*>( "segment_tool_menu" );

        if (!m_menu) {
            RG_DEBUG << "SegmentTool::createMenu() failed\n";
        }
    } else {
        RG_DEBUG << "SegmentTool::createMenu() failed: !app\n";
    }
}

void
SegmentTool::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
}

void SegmentTool::setCurrentIndex(CompositionItem item) 
{
    if (item != m_currentIndex) 
    {
        delete m_currentIndex;
        m_currentIndex = item; 
    }
}

SegmentToolBox* SegmentTool::getToolBox()
{
    return m_canvas->getToolBox();
}

}
