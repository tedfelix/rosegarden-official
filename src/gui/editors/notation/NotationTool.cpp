/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NotationTool.h"
#include "misc/Debug.h"

#include "NotationWidget.h"

#include "misc/Strings.h"

#include <QMenu>


namespace Rosegarden
{

NotationTool::NotationTool(QString rcFileName, QString menuName,
                           NotationWidget *widget) :
    BaseTool(menuName, widget),
    m_widget(widget),
    m_scene(nullptr),
    m_rcFileName(rcFileName)
{
}

NotationTool::NotationTool(NotationWidget *widget) :
    BaseTool("", widget),
    m_widget(widget)
{
}

NotationTool::~NotationTool()
{
    NOTATION_DEBUG << "NotationTool::~NotationTool()";
//    delete m_menu;
}

void
NotationTool::ready()
{
    m_widget->setCursor(Qt::ArrowCursor);
//!!!    m_widget->setHeightTracking(false);
}

void
NotationTool::stow()
{
}

void
NotationTool::handleLeftButtonPress(const NotationMouseEvent *) { }

void
NotationTool::handleMidButtonPress(const NotationMouseEvent *) { }

void
NotationTool::handleRightButtonPress(const NotationMouseEvent *) 
{
    showMenu();
}

void
NotationTool::handleMouseRelease(const NotationMouseEvent *) { }

void
NotationTool::handleMouseDoubleClick(const NotationMouseEvent *) { }

FollowMode
NotationTool::handleMouseMove(const NotationMouseEvent *)
{
    return NO_FOLLOW;
}

void
NotationTool::handleWheelTurned(int, const NotationMouseEvent *) { }

void
NotationTool::invokeInParentView(QString actionName)
{
    QAction *a = findActionInParentView(actionName);
    if (!a) {
        RG_WARNING << "NotationTool::invokeInParentView: No action \"" << actionName
                  << "\" found in parent view";
    } else {
        a->trigger();
    }
}

QAction *
NotationTool::findActionInParentView(QString actionName)
{
    if (!m_widget) return nullptr;
    QWidget *w = m_widget;
    ActionFileClient *c = nullptr;
    while (w && !(c = dynamic_cast<ActionFileClient *>(w))) {
        w = w->parentWidget();
    }
    if (!c) {
        RG_WARNING << "NotationTool::findActionInParentView: Can't find ActionFileClient in parent widget hierarchy";
        return nullptr;
    }
    QAction *a = c->findAction(actionName);
    return a;
}

void
NotationTool::createMenu()
{
    NOTATION_DEBUG << "NotationTool::createMenu() " << m_rcFileName << " - " << m_menuName;

    if (!createMenusAndToolbars(m_rcFileName)) {
        RG_WARNING << "NotationTool::createMenu(" << m_rcFileName << "): menu creation failed";
        m_menu = nullptr;
        return;
    }

    QMenu *menu = findMenu(m_menuName);
    if (!menu) {
        RG_WARNING << "NotationTool::createMenu(" << m_rcFileName
                   << "): menu name "
                   << m_menuName << "not created by RC file";
        return;
    }

    m_menu = menu;
}    

}


