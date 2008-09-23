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


#include "SegmentTool.h"

#include "misc/Debug.h"
#include "CompositionView.h"
#include "document/RosegardenGUIDoc.h"
#include "document/MultiViewCommandHistory.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/general/BaseTool.h"
#include "SegmentToolBox.h"
#include "document/Command.h"
#include <kmainwindow.h>
#include <QPoint>
#include <qpopupmenu.h>
#include <QMouseEvent>


namespace Rosegarden
{

SegmentTool::SegmentTool(CompositionView* canvas, RosegardenGUIDoc *doc)
        : BaseTool("segment_tool_menu", dynamic_cast<KMainWindow*>(doc->parent())->factory(), canvas),
        m_canvas(canvas),
        m_doc(doc),
        m_changeMade(false)
{}

SegmentTool::~SegmentTool()
{}


void SegmentTool::ready()
{
    m_canvas->viewport()->setCursor(Qt::arrowCursor);
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

    RosegardenGUIApp *app =
        dynamic_cast<RosegardenGUIApp*>(m_doc->parent());

    if (app) {
        m_menu = static_cast<QPopupMenu*>
                 //(app->factory()->container("segment_tool_menu", app));
                 (m_parentFactory->container("segment_tool_menu", app));

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
    m_doc->getCommandHistory()->addCommand(command);
}

SegmentToolBox* SegmentTool::getToolBox()
{
    return m_canvas->getToolBox();
}

}
