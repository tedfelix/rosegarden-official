// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
  Guillaume Laurent   <glaurent@telegraph-road.org>,
  Chris Cannam        <cannam@all-day-breakfast.com>,
  Richard Bown        <bownie@bownie.com>

  The moral right of the authors to claim authorship of this work
  has been asserted.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include <qpopupmenu.h>
#include <qcursor.h>

#include "edittool.h"
#include "editview.h"

#include "rosestrings.h"
#include "rosedebug.h"

EditToolBox::EditToolBox(EditView *parent)
    : QObject(parent),
      m_parentView(parent),
      m_tools(17, false)
{
    //m_tools.setAutoDelete(true);
}


EditTool* EditToolBox::getTool(const QString& toolName)
{
    EditTool* tool = m_tools[toolName];

    if (!tool) tool = createTool(toolName);
    
    return tool;
}

//////////////////////////////////////////////////////////////////////
//                         EditTool
//////////////////////////////////////////////////////////////////////

EditTool::EditTool(const QString& menuName, EditView* view)
    : QObject(view),
      m_menuName(menuName),
      m_parentView(view),
      m_menu(0)
{
}

EditTool::~EditTool()
{
    RG_DEBUG << "EditTool::~EditTool()\n";

    //     delete m_menu;
    //     m_parentView->factory()->removeClient(this);
    //    m_instance = 0;
}

void EditTool::ready()
{
}

void EditTool::stow()
{
}

void EditTool::createMenu(const QString& rcFileName)
{
    setXMLFile(rcFileName);
    m_parentView->factory()->addClient(this);

    QWidget* tmp =  m_parentView->factory()->container(m_menuName, this);

    m_menu = dynamic_cast<QPopupMenu*>(tmp);
}

void EditTool::handleMousePress(Rosegarden::timeT time,
                                int height,
                                int staffNo,
                                QMouseEvent* e,
                                Rosegarden::ViewElement* el)
{
    RG_DEBUG << "EditTool::handleMousePress : mouse button = "
                         << e->button() << endl;

    switch (e->button()) {

    case Qt::LeftButton:
        handleLeftButtonPress(time, height, staffNo, e, el);
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

void EditTool::handleMidButtonPress(Rosegarden::timeT,
                                    int, int,
                                    QMouseEvent*,
                                    Rosegarden::ViewElement*)
{
}

void EditTool::handleRightButtonPress(Rosegarden::timeT,
                                      int, int,
                                      QMouseEvent*,
                                      Rosegarden::ViewElement*)
{
    showMenu();
}

void EditTool::handleMouseDblClick(Rosegarden::timeT,
                                   int, int,
                                   QMouseEvent*,
                                   Rosegarden::ViewElement*)
{
    // nothing
}


bool EditTool::handleMouseMove(Rosegarden::timeT, int, QMouseEvent*)
{
    return false;
}

void EditTool::handleMouseRelease(Rosegarden::timeT, int, QMouseEvent*)
{
}

void EditTool::showMenu()
{
    if (m_menu)
        m_menu->exec(QCursor::pos());
    else
        RG_DEBUG << "EditTool::showMenu() : no menu to show\n";
}

void EditTool::setParentView(EditView* view)
{
    m_parentView = view;
}

