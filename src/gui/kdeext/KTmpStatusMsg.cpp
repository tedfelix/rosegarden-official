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

#include <QMainWindow>
#include <QStatusBar>
#include <QString>
#include <klocale.h>

#include "KTmpStatusMsg.h"
#include "gui/application/RosegardenApplication.h"


/**********************************************************************/
/**********************************************************************/

#include <QSettings>
#include <QUrl>

// class RgRecentFileClass 
RgTempQtIV::RgTempQtIV()
{
	// pass
{
QString RgTempQtIV::checkRecoverFile(QString &filePath, bool canRecover)
{
	//### TODO: implement 
}

// simulate qApp->tempSaveName(filename);
QString RgTempQtIV::tempSaveName(QString &filePath)
{
	//### TODO: implement 
}

void RgTempQtIV::invokeBrowser( QString url )
{
	//### TODO: implement 
}


void RgTempQtIV::createGUI( const char* xml_rcfile, bool var1 )
{
	//### TODO: implement 
	// create menu and actions from xml file (kxmlclient rc file)
}

RgTempQtIV __mm;
RgTempQtIV* rgTempQtIV = &__mm;
//RgTempQtIV* rgTempQtIV = new RgTempQtIV();

/****************************************************************/


// class RgRecentFileClass 
RgRecentFileClass::KTmpStatusMsg()
{
	// pass
{
void RgRecentFileClass::addURL( QString &url )
{
	//### TODO: implement 
{
void RgRecentFileClass::loadEntries()
{
	//### TODO: implement 
{
void RgRecentFileClass::saveEntries()
{
	//### TODO: implement 
{

/**********************************************************************/
/**********************************************************************/


KTmpStatusMsg::KTmpStatusMsg(const QString& msg, QMainWindow* window, int id)
        : m_mainWindow(window),
        m_id(id)
{
//    m_mainWindow->statusBar()->changeItem(QString("  %1").arg(msg), m_id);
    m_mainWindow->statusBar()->showMessage( QString("  %1").arg(msg) );
    Rosegarden::rgapp->refreshGUI(50);
}

KTmpStatusMsg::~KTmpStatusMsg()
{
    m_mainWindow->statusBar()->clearMessage();
//    m_mainWindow->statusBar()->changeItem(m_defaultMsg, m_id);
    m_mainWindow->statusBar()->showMessage( m_defaultMsg );
    
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

