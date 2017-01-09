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

#define RG_MODULE_STRING "[BaseTool]"

#include "BaseTool.h"

#include "misc/Debug.h"
#include <QCursor>
#include <QObject>
#include <QToolTip>
#include <QString>
#include <QMenu>


namespace Rosegarden
{

BaseTool::BaseTool(const QString& menuName, QObject* parent)
        : QObject(parent),
        m_menuName(menuName),
        m_menu(0)
{}

BaseTool::~BaseTool()
{
    RG_DEBUG << "BaseTool::~BaseTool()";
}

void BaseTool::ready()
{}

void BaseTool::stow()
{}

void BaseTool::showMenu()
{
    if (!hasMenu())
        return;

    if (!m_menu)
        createMenu();

    if (m_menu)
        m_menu->exec(QCursor::pos());
    else
        RG_DEBUG << "BaseTool::showMenu() : no menu to show";
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


