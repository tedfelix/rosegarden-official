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

#define RG_MODULE_STRING "[SegmentTool]"

#include "SegmentTool.h"

#include "misc/Debug.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/general/BaseTool.h"
#include "SegmentToolBox.h"
#include "document/Command.h"

#include <QMenu>
#include <QMouseEvent>


namespace Rosegarden
{

SegmentTool::SegmentTool(CompositionView* canvas, RosegardenDocument *doc)
        : BaseTool("SegmentTool", canvas),
        m_canvas(canvas),
        m_doc(doc),
        m_changeMade(false)
{
    //RG_DEBUG << "SegmentTool::SegmentTool()";

    // Set up the actions for the right-click context menu.  Note that
    // all of these end up simply delegating to RosegardenMainWindow.
    createAction("edit_default", SLOT(slotEdit()));
    createAction("edit_matrix", SLOT(slotEditInMatrix()));
    createAction("edit_percussion_matrix", SLOT(slotEditInPercussionMatrix()));
    createAction("edit_notation", SLOT(slotEditAsNotation()));
    createAction("edit_event_list", SLOT(slotEditInEventList()));
    createAction("edit_pitch_tracker", SLOT(slotEditInPitchTracker()));
    // Can we get some of the following connectionless mojo for some
    // of these others too?
    //createAction("edit_undo", ...);  // handled by a higher authority?
    //createAction("edit_redo", ...);  // handled by a higher authority?
    createAction("edit_cut", SLOT(slotEditCut()));
    createAction("edit_copy", SLOT(slotEditCopy()));
    createAction("edit_paste", SLOT(slotEditPaste()));
    createAction("delete", SLOT(slotDeleteSelectedSegments()));
    createAction("join_segments", SLOT(slotJoinSegments()));
    createAction("quantize_selection", SLOT(slotQuantizeSelection()));
    createAction("repeat_quantize", SLOT(slotRepeatQuantizeSelection()));
    createAction("relabel_segment", SLOT(slotRelabelSegments()));
    createAction("transpose", SLOT(slotTransposeSegments()));
    createAction("select", SLOT(slotPointerSelected()));
    createAction("move", SLOT(slotMoveSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("resize", SLOT(slotResizeSelected()));
    createAction("split", SLOT(slotSplitSelected()));
}

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

    //RG_DEBUG << "SegmentTool::handleRightButtonPress()\n";

    setCurrentIndex(m_canvas->getModel()->getFirstItemAt(e->pos()));

    if (m_currentIndex) {
        if (!m_canvas->getModel()->isSelected(m_currentIndex)) {

            m_canvas->getModel()->clearSelected();
            m_canvas->getModel()->setSelected(m_currentIndex);
            m_canvas->getModel()->signalSelection();
        }
    }

    showMenu();
    setCurrentIndex(CompositionItemPtr());
}

void
SegmentTool::createMenu()
{
    // New version based on the one in MatrixTool.

    const QString rcFileName = "segmenttool.rc";

    //RG_DEBUG << "SegmentTool::createMenu() " << rcFileName << " - " << m_menuName << endl;

    if (!createGUI(rcFileName)) {
        std::cerr << "SegmentTool::createMenu(" << rcFileName
                  << "): menu creation failed\n";
        m_menu = 0;
        return;
    }

    QMenu *menu = findMenu(m_menuName);
    if (!menu) {
        std::cerr << "SegmentTool::createMenu(" << rcFileName
                  << "): menu name "
                  << m_menuName << " not created by RC file\n";
        return;
    }

    m_menu = menu;
}

void
SegmentTool::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
}

void SegmentTool::setCurrentIndex(CompositionItemPtr item)
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

void SegmentTool::slotEdit()
{
    RosegardenMainWindow::self()->slotEdit();
}
void SegmentTool::slotEditInMatrix()
{
    RosegardenMainWindow::self()->slotEditInMatrix();
}
void SegmentTool::slotEditInPercussionMatrix()
{
    RosegardenMainWindow::self()->slotEditInPercussionMatrix();
}
void SegmentTool::slotEditAsNotation()
{
    RosegardenMainWindow::self()->slotEditAsNotation();
}
void SegmentTool::slotEditInEventList()
{
    RosegardenMainWindow::self()->slotEditInEventList();
}
void SegmentTool::slotEditInPitchTracker()
{
    RosegardenMainWindow::self()->slotEditInPitchTracker();
}
void SegmentTool::slotEditCut()
{
    RosegardenMainWindow::self()->slotEditCut();
}
void SegmentTool::slotEditCopy()
{
    RosegardenMainWindow::self()->slotEditCopy();
}
void SegmentTool::slotEditPaste()
{
    RosegardenMainWindow::self()->slotEditPaste();
}
void SegmentTool::slotDeleteSelectedSegments()
{
    RosegardenMainWindow::self()->slotDeleteSelectedSegments();
}
void SegmentTool::slotJoinSegments()
{
    RosegardenMainWindow::self()->slotJoinSegments();
}
void SegmentTool::slotQuantizeSelection()
{
    RosegardenMainWindow::self()->slotQuantizeSelection();
}
void SegmentTool::slotRepeatQuantizeSelection()
{
    RosegardenMainWindow::self()->slotRepeatQuantizeSelection();
}
void SegmentTool::slotRelabelSegments()
{
    RosegardenMainWindow::self()->slotRelabelSegments();
}
void SegmentTool::slotTransposeSegments()
{
    RosegardenMainWindow::self()->slotTransposeSegments();
}
void SegmentTool::slotPointerSelected()
{
    RosegardenMainWindow::self()->slotPointerSelected();
}
void SegmentTool::slotMoveSelected()
{
    RosegardenMainWindow::self()->slotMoveSelected();
}
void SegmentTool::slotDrawSelected()
{
    RosegardenMainWindow::self()->slotDrawSelected();
}
void SegmentTool::slotEraseSelected()
{
    RosegardenMainWindow::self()->slotEraseSelected();
}
void SegmentTool::slotResizeSelected()
{
    RosegardenMainWindow::self()->slotResizeSelected();
}
void SegmentTool::slotSplitSelected()
{
    RosegardenMainWindow::self()->slotSplitSelected();
}


}

#include "SegmentTool.moc"
