// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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

#include <kmessagebox.h>

#include "edittool.h"
#include "editview.h"

#include "rosestrings.h"
#include "rosedebug.h"

BaseToolBox::BaseToolBox(QWidget* parent)
    : QObject(parent),
      m_tools(17, // default size, from the Qt docs
              false) // but we want it to be case insensitive
{
    //m_tools.setAutoDelete(true);
}

BaseTool* BaseToolBox::getTool(const QString& toolName)
{
    BaseTool* tool = m_tools[toolName];

    if (!tool) tool = createTool(toolName);
    
    return tool;
}


EditToolBox::EditToolBox(EditView *parent)
    : BaseToolBox(parent),
      m_parentView(parent)
{

}

EditTool* EditToolBox::getTool(const QString& toolName)
{
    return dynamic_cast<EditTool*>(BaseToolBox::getTool(toolName));
}

EditTool* EditToolBox::createTool(const QString&)
{
    KMessageBox::error(0, "EditToolBox::createTool called - this should never happen");
    return 0;
}

//////////////////////////////////////////////////////////////////////
//                         EditTool
//////////////////////////////////////////////////////////////////////

BaseTool::BaseTool(const QString& menuName, KXMLGUIFactory* factory, QObject* parent)
    : QObject(parent),
      m_menuName(menuName),
      m_menu(0),
      m_parentFactory(factory)
{
}

BaseTool::~BaseTool()
{
    RG_DEBUG << "BaseTool::~BaseTool()\n";

    //     delete m_menu;
    //     m_parentView->factory()->removeClient(this);
    //    m_instance = 0;
}

void BaseTool::ready()
{
}

void BaseTool::stow()
{
}

void BaseTool::showMenu()
{
    if (!m_menu) createMenu();

    if (m_menu)
        m_menu->exec(QCursor::pos());
    else
        RG_DEBUG << "BaseTool::showMenu() : no menu to show\n";
}

//////////////////////////////////////

EditTool::EditTool(const QString& menuName, EditView* view)
    : BaseTool(menuName, view->factory(), view),
      m_parentView(view)
{
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

void EditTool::handleMouseDoubleClick(Rosegarden::timeT,
				      int, int,
				      QMouseEvent*,
				      Rosegarden::ViewElement*)
{
    // nothing
}


int EditTool::handleMouseMove(Rosegarden::timeT, int, QMouseEvent*)
{
    return NoFollow;
}

void EditTool::handleMouseRelease(Rosegarden::timeT, int, QMouseEvent*)
{
}

void EditTool::createMenu(QString rcFileName)
{
    setRCFileName(rcFileName);
    createMenu();
}

void EditTool::createMenu()
{
    RG_DEBUG << "BaseTool::createMenu() " << m_rcFileName << " - " << m_menuName << endl;

    setXMLFile(m_rcFileName);
    m_parentFactory->addClient(this);

    QWidget* tmp =  m_parentFactory->container(m_menuName, this);

    if (!tmp)
        RG_DEBUG << "BaseTool::createMenu(" << m_rcFileName
                 << ") : menu creation failed (name : "
                 << m_menuName << ")\n";

    m_menu = dynamic_cast<QPopupMenu*>(tmp);
}
