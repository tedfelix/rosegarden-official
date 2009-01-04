/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

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

#include "misc/Strings.h"

#include <QObject>
#include <QAction>

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
    QObject *obj = dynamic_cast<QObject *>(this);
    if (!obj) {
        std::cerr << "ERROR: ActionFileClient::createAction: ActionFileClient subclass is not a QObject" << std::endl;
        return 0;
    }
    QAction *action = new QAction(obj);
    action->setObjectName(actionName);
    if (connection != "") {
        QObject::connect(action, SIGNAL(triggered()),
                         obj, qStrToCharPtrUtf8(connection) );
    }
    return action;
}

QAction *
ActionFileClient::findAction(QString actionName)
{
    //!!! NB this is often called and the result dereferenced without checking for NULL -- fix these cases!

    QObject *obj = dynamic_cast<QObject *>(this);
    if (!obj) {
        std::cerr << "ERROR: ActionFileClient::findAction: ActionFileClient subclass is not a QObject" << std::endl;
        return 0;
    }
    return obj->findChild<QAction *>(actionName);
}

void
ActionFileClient::enterActionState(QString stateName)
{
    //&&& implement
#pragma warning("Implement enterActionState");
    std::cerr << "ERROR: enterActionState not implemented" << std::endl;
}

void
ActionFileClient::leaveActionState(QString stateName)
{
    //&&& implement
#pragma warning("Implement leaveActionState");
    std::cerr << "ERROR: leaveActionState not implemented" << std::endl;
}

bool
ActionFileClient::createGUI(QString rcFileName)
{
    QWidget *widg = dynamic_cast<QWidget *>(this);
    if (!widg) {
        std::cerr << "ERROR: ActionFileClient::createAction: ActionFileClient subclass is not a QWidget" << std::endl;
        return 0;
    }
    if (!m_actionFileParser) m_actionFileParser = new ActionFileParser(widg);
    if (!m_actionFileParser->load(rcFileName)) {
        std::cerr << "ActionFileClient::createGUI: ERROR: Failed to load action file" << std::endl;
        return false;
    }
    return true;
}

}

