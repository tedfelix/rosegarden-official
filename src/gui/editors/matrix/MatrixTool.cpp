/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MatrixTool.h"
#include "misc/Debug.h"
#include "misc/Strings.h"

#include "MatrixWidget.h"
#include "MatrixScene.h"

#include <QAction>
#include <QString>
#include <QMenu>


namespace Rosegarden
{

MatrixTool::MatrixTool(QString rcFileName, QString menuName,
                           MatrixWidget *widget) :
    BaseTool(menuName, widget),
    m_widget(widget),
    m_scene(0),
    m_rcFileName(rcFileName)
{
}

MatrixTool::~MatrixTool()
{
    // We don't need (or want) to delete the menu; it's owned by the
    // ActionFileMenuWrapper parented to our QObject base class
    MATRIX_DEBUG << "MatrixTool::~MatrixTool()";
}

void
MatrixTool::handleLeftButtonPress(const MatrixMouseEvent *) { }

void
MatrixTool::handleMidButtonPress(const MatrixMouseEvent *) { }

void
MatrixTool::handleRightButtonPress(const MatrixMouseEvent *) 
{
    showMenu();
}

void
MatrixTool::handleMouseRelease(const MatrixMouseEvent *) { }

void
MatrixTool::handleMouseDoubleClick(const MatrixMouseEvent *) { }

MatrixTool::FollowMode
MatrixTool::handleMouseMove(const MatrixMouseEvent *)
{
    return NoFollow;
}

void
MatrixTool::handleEventRemoved(Event *)
{}

void
MatrixTool::slotSelectSelected()
{
    invokeInParentView("select");
}

void
MatrixTool::slotMoveSelected()
{
    invokeInParentView("move");
}

void
MatrixTool::slotEraseSelected()
{
    invokeInParentView("erase");
}

void
MatrixTool::slotResizeSelected()
{
    invokeInParentView("resize");
}

void
MatrixTool::slotDrawSelected()
{
    invokeInParentView("draw");
}

void 
MatrixTool::slotVelocityChangeSelected()
{
    invokeInParentView("velocity");
}

const SnapGrid *
MatrixTool::getSnapGrid() const
{
    if (!m_scene) return 0;
    return m_scene->getSnapGrid();
}

void
MatrixTool::invokeInParentView(QString actionName)
{
    QAction *a = findActionInParentView(actionName);
    if (!a) {
        RG_WARNING << "MatrixTool::invokeInParentView: No action \"" << actionName
                  << "\" found in parent view";
    } else {
        a->trigger();
    }
}

QAction *
MatrixTool::findActionInParentView(QString actionName)
{
    if (!m_widget) return 0;
    QWidget *w = m_widget;
    ActionFileClient *c = 0;
    while (w && !(c = dynamic_cast<ActionFileClient *>(w))) {
        w = w->parentWidget();
    }
    if (!c) {
        RG_WARNING << "MatrixTool::findActionInParentView: Can't find ActionFileClient in parent widget hierarchy";
        return 0;
    }
    QAction *a = c->findAction(actionName);
    return a;
}

void
MatrixTool::createMenu()
{
    MATRIX_DEBUG << "MatrixTool::createMenu() " << m_rcFileName << " - " << m_menuName;

    if (!createGUI(m_rcFileName)) {
        RG_WARNING << "MatrixTool::createMenu(" << m_rcFileName << "): menu creation failed";
        m_menu = 0;
        return;
    }

    QMenu *menu = findMenu(m_menuName);
    if (!menu) {
        RG_WARNING << "MatrixTool::createMenu(" << m_rcFileName
                  << "): menu name "
                  << m_menuName << " not created by RC file";
        return;
    }

    m_menu = menu;
}    

}
