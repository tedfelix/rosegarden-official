/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ActionFileClient.h"
#include "ActionFileParser.h"
#include "DecoyAction.h"

#include "misc/Strings.h"
#include "misc/Debug.h"

#include <QObject>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QToolBar>

namespace Rosegarden 
{

ActionFileClient::ActionFileClient() :
    m_actionFileParser(0)
{
}

ActionFileClient::~ActionFileClient()
{
    delete m_actionFileParser;
}

QAction *
ActionFileClient::createAction(QString actionName, QString connection)
{
    //RG_DEBUG << "ActionFileClient::createAction(" << actionName << ", " << connection << ")";
    QObject *obj = dynamic_cast<QObject *>(this);
    if (!obj) {
        RG_WARNING << "ERROR: ActionFileClient::createAction: ActionFileClient subclass is not a QObject";
        return 0;
    }
    QAction *action = new QAction(obj);
    action->setObjectName(actionName);
    QByteArray bc = connection.toUtf8();
    if (connection != "") {
        QObject::connect(action, SIGNAL(triggered()), obj, bc.data());
    }
    return action;
}

QAction *
ActionFileClient::createAction(QString actionName, QObject *target, QString connection)
{
    QObject *obj = dynamic_cast<QObject *>(this);
    if (!obj) {
        RG_WARNING << "ERROR: ActionFileClient::createAction: ActionFileClient subclass is not a QObject";
        return 0;
    }
    QAction *action = new QAction(obj);
    action->setObjectName(actionName);
    QByteArray bc = connection.toUtf8();
    if (connection != "") {
        QObject::connect(action, SIGNAL(triggered()), target, bc.data());
    }
    return action;
}

QAction *
ActionFileClient::findAction(QString actionName)
{
    QObject *obj = dynamic_cast<QObject *>(this);
    if (!obj) {
        RG_WARNING << "ERROR: ActionFileClient::findAction: ActionFileClient subclass is not a QObject";
        return DecoyAction::getInstance();
    }
    QAction *a = obj->findChild<QAction *>(actionName);
    if (!a) {
        RG_WARNING << "WARNING: ActionFileClient(\"" << obj->objectName()
                  << "\")::findAction: No such action as \"" << actionName << "\"";
        return DecoyAction::getInstance();
    }
    return a;
}

void
ActionFileClient::enterActionState(QString stateName)
{
    // inelegant, parser should just be a parser... we can't easily
    // put implementations of these in here because this object cannot
    // be a QObject and so cannot receive destroyed() signal from
    // objects... proper structure needed
    if (m_actionFileParser) m_actionFileParser->enterActionState(stateName);
}

void
ActionFileClient::leaveActionState(QString stateName)
{
    // inelegant, parser should just be a parser... we can't easily
    // put implementations of these in here because this object cannot
    // be a QObject and so cannot receive destroyed() signal from
    // objects... proper structure needed
    if (m_actionFileParser) m_actionFileParser->leaveActionState(stateName);
}

QActionGroup *
ActionFileClient::findGroup(QString groupName)
{
    QObject *obj = dynamic_cast<QObject *>(this);
    if (!obj) {
        RG_WARNING << "ERROR: ActionFileClient::findGroup: ActionFileClient subclass is not a QObject";
        return 0;
    }
    QWidget *widget = dynamic_cast<QWidget *>(this);
    QActionGroup *g = 0;
    if (widget) {
        g = obj->findChild<QActionGroup *>(groupName);
        if (!g) {
            RG_WARNING << "WARNING: ActionFileClient(\"" << obj->objectName()
                      << "\")::findGroup: No such action-group as \"" << groupName << "\"";
        }
    }
    return g;
}

QMenu *
ActionFileClient::findMenu(QString menuName)
{
    QObject *obj = dynamic_cast<QObject *>(this);
    if (!obj) {
        RG_WARNING << "ERROR: ActionFileClient::findMenu: ActionFileClient subclass is not a QObject";
        return 0;
    }
    QWidget *widget = dynamic_cast<QWidget *>(this);
    QMenu *m = 0;
    if (widget) {
        m = obj->findChild<QMenu *>(menuName);
        if (!m) {
            RG_WARNING << "WARNING: ActionFileClient(\"" << obj->objectName()
                      << "\")::findMenu: No such menu as \"" << menuName << "\"";
        }
    } else {
        ActionFileMenuWrapper *w = obj->findChild<ActionFileMenuWrapper *>(menuName);
        if (w) m = w->getMenu();
        else {
            RG_WARNING << "WARNING: ActionFileClient(\"" << obj->objectName()
                      << "\")::findMenu: No such menu (wrapper) as \"" << menuName << "\"";
        }
    }            
    return m;
}

QToolBar *
ActionFileClient::findToolbar(QString toolbarName)
{
    QWidget *w = dynamic_cast<QWidget *>(this);
    if (!w) {
        RG_WARNING << "ERROR: ActionFileClient::findToolbar: ActionFileClient subclass is not a QWidget";
        return 0;
    }
    QToolBar *t = w->findChild<QToolBar *>(toolbarName);
    if (!t) {
        RG_WARNING << "WARNING: ActionFileClient(\"" << w->objectName()
                  << "\")::findToolbar: No such toolbar as \"" << toolbarName << "\", creating one";
        t = new QToolBar(toolbarName, w);
        t->setObjectName(toolbarName);
        return t;
    }
    return t;
}

bool
ActionFileClient::createGUI(QString rcFileName)
{
    QObject *obj = dynamic_cast<QObject *>(this);
    if (!obj) {
        RG_WARNING << "ERROR: ActionFileClient::createGUI: ActionFileClient subclass is not a QObject";
        return 0;
    }
    if (!m_actionFileParser) m_actionFileParser = new ActionFileParser(obj);
    if (!m_actionFileParser->load(rcFileName)) {
        RG_WARNING << "ActionFileClient::createGUI: ERROR: Failed to load action file";
        return false;
    }
    return true;
}

}

