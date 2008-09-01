// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <QApplication>
#include <QEventLoop>

#include <kmainwindow.h>
#include <kstatusbar.h>
#include <klocale.h>

#include "KTmpStatusMsg.h"
#include "gui/application/RosegardenApplication.h"

KTmpStatusMsg::KTmpStatusMsg(const QString& msg, KMainWindow* window, int id)
        : m_mainWindow(window),
        m_id(id)
{
    m_mainWindow->statusBar()->changeItem(QString("  %1").arg(msg), m_id);
    Rosegarden::rgapp->refreshGUI(50);
}

KTmpStatusMsg::~KTmpStatusMsg()
{
    m_mainWindow->statusBar()->clear();
    m_mainWindow->statusBar()->changeItem(m_defaultMsg, m_id);
    Rosegarden::rgapp->refreshGUI(50);
}


void KTmpStatusMsg::setDefaultMsg(const QString& m)
{
    m_defaultMsg = m;
}

const QString& KTmpStatusMsg::getDefaultMsg()
{
    return m_defaultMsg;
}

void KTmpStatusMsg::setDefaultId(int id)
{
    m_defaultId = id;
}

int KTmpStatusMsg::getDefaultId()
{
    return m_defaultId;
}


int KTmpStatusMsg::m_defaultId = 1;
QString KTmpStatusMsg::m_defaultMsg = "";
