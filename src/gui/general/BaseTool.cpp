/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "BaseTool.h"

#include "misc/Debug.h"
#include <kxmlguifactory.h>
#include <qcursor.h>
#include <qobject.h>
#include <qpopupmenu.h>
#include <qstring.h>


namespace Rosegarden
{

BaseTool::BaseTool(const QString& menuName, KXMLGUIFactory* factory, QObject* parent)
        : QObject(parent),
        m_menuName(menuName),
        m_menu(0),
        m_parentFactory(factory)
{}

BaseTool::~BaseTool()
{
    RG_DEBUG << "BaseTool::~BaseTool()\n";

    //     delete m_menu;
    //     m_parentView->factory()->removeClient(this);
    //    m_instance = 0;
}

void BaseTool::ready()
{}

void BaseTool::stow()
{}

void BaseTool::showMenu()
{
    if (!hasMenu())
        return ;

    if (!m_menu)
        createMenu();

    if (m_menu)
        m_menu->exec(QCursor::pos());
    else
        RG_DEBUG << "BaseTool::showMenu() : no menu to show\n";
}

QString BaseTool::getCurrentContextHelp() const
{
    return m_contextHelp;
}

void BaseTool::setContextHelp(const QString &help)
{
    m_contextHelp = help;
    emit showContextHelp(m_contextHelp);
}

}

#include "BaseTool.moc"

