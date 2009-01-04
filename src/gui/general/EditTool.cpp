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


#include "EditTool.h"
#include "misc/Debug.h"
#include "base/Event.h"
#include "BaseTool.h"
#include "base/ViewElement.h"
#include "EditView.h"
#include "RosegardenCanvasView.h"
#include "misc/Strings.h"

#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QString>

//#include <kxmlguiclient.h>


namespace Rosegarden
{

EditTool::EditTool(const QString& menuName, EditView* view)
        : BaseTool(menuName, view),
        m_parentView(view)
{}

void EditTool::handleMousePress(timeT time,
                                int height,
                                int staffNo,
                                QMouseEvent* e,
                                ViewElement* el)
{
    RG_DEBUG << "EditTool::handleMousePress : mouse button = "
    << e->button() << endl;

    switch (e->button()) {

    case Qt::LeftButton:
        if (e->type() == QEvent::MouseButtonDblClick) {
            RG_DEBUG << "EditTool::handleMousePress: it's a double-click"
            << endl;
            handleMouseDoubleClick(time, height, staffNo, e, el);
        } else {
            RG_DEBUG << "EditTool::handleMousePress: it's a single-click"
            << endl;
            handleLeftButtonPress(time, height, staffNo, e, el);
        }
        break;

    case Qt::RightButton:
        handleRightButtonPress(time, height, staffNo, e, el);
        break;

    case Qt::MidButton:
        handleMidButtonPress(time, height, staffNo, e, el);
        break;

    default:
        RG_DEBUG << "EditTool::handleMousePress : no button mouse press\n";
        break;
    }
}

void EditTool::handleMidButtonPress(timeT,
                                    int, int,
                                    QMouseEvent*,
                                    ViewElement*)
{}

void EditTool::handleRightButtonPress(timeT,
                                      int, int,
                                      QMouseEvent*,
                                      ViewElement*)
{
    showMenu();
}

void EditTool::handleMouseDoubleClick(timeT,
                                      int, int,
                                      QMouseEvent*,
                                      ViewElement*)
{
    // nothing
}

int EditTool::handleMouseMove(timeT, int, QMouseEvent*)
{
    return RosegardenCanvasView::NoFollow;
}

void EditTool::handleMouseRelease(timeT, int, QMouseEvent*)
{}

void EditTool::createMenu(QString rcFileName)
{
    setRCFileName(rcFileName);
    createMenu();
}

void EditTool::createMenu()
{
    RG_DEBUG << "BaseTool::createMenu() " << m_rcFileName << " - " << m_menuName << endl;

    if (!createGUI(m_rcFileName)) {
        std::cerr << "EditTool::createMenu(" << m_rcFileName << "): menu creation failed" << std::endl;
        return;
    }

    QMenu *menu = findChild<QMenu *>(m_menuName);
    if (!menu) {
        std::cerr << "BaseTool::createMenu(" << m_rcFileName
                  << "): menu name "
                  << m_menuName << " not created by RC file\n";
        return;
    }

    m_menu = menu;
}

bool EditTool::hasMenu()
{
    return !m_rcFileName.isEmpty();
}

}
