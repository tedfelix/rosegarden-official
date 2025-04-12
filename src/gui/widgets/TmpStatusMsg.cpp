/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TmpStatusMsg.h"

#include "gui/application/RosegardenApplication.h"

#include <QMainWindow>
#include <QStatusBar>
#include <QString>


TmpStatusMsg::TmpStatusMsg(const QString& msg, QMainWindow* window)
        : m_mainWindow(window)
{
    m_mainWindow->statusBar()->showMessage( QString("  %1").arg(msg) );
}

TmpStatusMsg::~TmpStatusMsg()
{
    m_mainWindow->statusBar()->clearMessage();
}
