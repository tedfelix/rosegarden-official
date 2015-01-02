/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TmpStatusMsg.h"

#include "gui/application/RosegardenApplication.h"

#include <QApplication>
#include <QEventLoop>
#include <QMainWindow>
#include <QStatusBar>
#include <QString>
#include <QLabel>
#include <QSettings>
#include <QUrl>


TmpStatusMsg::TmpStatusMsg(const QString& msg, QMainWindow* window, int id)
        : m_mainWindow(window),
        m_id(id)
{
//    m_mainWindow->statusBar()->changeItem(QString("  %1").arg(msg), m_id);
    m_mainWindow->statusBar()->showMessage( QString("  %1").arg(msg) );
    Rosegarden::rosegardenApplication->refreshGUI(50);
}

TmpStatusMsg::~TmpStatusMsg()
{
    m_mainWindow->statusBar()->clearMessage();
//    m_mainWindow->statusBar()->changeItem(m_defaultMsg, m_id);
    m_mainWindow->statusBar()->showMessage( m_defaultMsg );
    
    Rosegarden::rosegardenApplication->refreshGUI(50);
}


void TmpStatusMsg::setDefaultMsg(const QString& m)
{
    m_defaultMsg = m;
}

const QString& TmpStatusMsg::getDefaultMsg()
{
    return m_defaultMsg;
}

void TmpStatusMsg::setDefaultId(int id)
{
    m_defaultId = id;
}

int TmpStatusMsg::getDefaultId()
{
    return m_defaultId;
}


int TmpStatusMsg::m_defaultId = 1;
QString TmpStatusMsg::m_defaultMsg = "";

