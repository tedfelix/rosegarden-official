// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.1
  A sequencer and musical notation editor.

  This program is Copyright 2000-2001
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

#include "rosedebug.h"

#include "edittool.h"

#include "editview.h"

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
    kdDebug(KDEBUG_AREA) << "EditTool::~EditTool()\n";

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

void EditTool::handleMousePress(int height,
                                Rosegarden::timeT time,
                                int staffNo,
                                QMouseEvent* e,
                                Rosegarden::ViewElement* el)
{
    kdDebug(KDEBUG_AREA) << "EditTool::handleMousePress : mouse button = "
                         << e->button() << endl;

    switch (e->button()) {

    case Qt::LeftButton:
        handleLeftButtonPress(height, time, staffNo, e, el);
        break;

    case Qt::RightButton:
        handleRightButtonPress(height, time, staffNo, e, el);
        break;

    case Qt::MidButton:
        handleMidButtonPress(height, time, staffNo, e, el);
        break;

    default:
        kdDebug(KDEBUG_AREA) << "EditTool::handleMousePress : no button mouse press\n";
        break;
    }
}

void EditTool::handleMidButtonPress(int, int,
                                    Rosegarden::timeT,
                                    QMouseEvent*,
                                    Rosegarden::ViewElement*)
{
}

void EditTool::handleRightButtonPress(int, int,
                                      Rosegarden::timeT,
                                      QMouseEvent*,
                                      Rosegarden::ViewElement*)
{
    showMenu();
}

void EditTool::handleMouseDblClick(int, int,
                                   Rosegarden::timeT,
                                   QMouseEvent*,
                                   Rosegarden::ViewElement*)
{
    // nothing
}


void EditTool::handleMouseMove(int, Rosegarden::timeT, QMouseEvent*)
{
}

void EditTool::handleMouseRelease(int, Rosegarden::timeT, QMouseEvent*)
{
}

void EditTool::showMenu()
{
    if (m_menu)
        m_menu->exec(QCursor::pos());
    else
        kdDebug(KDEBUG_AREA) << "EditTool::showMenu() : no menu to show\n";
}

void EditTool::setParentView(EditView* view)
{
    m_parentView = view;
}

