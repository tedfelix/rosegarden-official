/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ActionFileParser.h"

#include <iostream>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QToolBar>
#include <QFileInfo>
#include <QMainWindow>
#include <QMenuBar>

#include "IconLoader.h"
#include "ResourceFinder.h"
#include "misc/Strings.h"

#include "document/CommandHistory.h"

using std::cerr;
using std::endl;

namespace Rosegarden
{
   
ActionFileParser::ActionFileParser(QWidget *actionOwner) :
    m_actionOwner(actionOwner)
{
}

ActionFileParser::~ActionFileParser()
{
}

QString
ActionFileParser::findRcFile(QString name)
{
    return ResourceFinder().getResourcePath("ui", name);
}

bool
ActionFileParser::load(QString actionRcFile)
{
    QString location = findRcFile(actionRcFile);
    if (location == "") {
        std::cerr << "ActionFileParser::load: Failed to find RC file \""
                  << actionRcFile << "\"" << std::endl;
        return false;
    }

    m_currentFile = location;

    QFile f(location);
    QXmlInputSource is(&f);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    return reader.parse(is);
}

bool
ActionFileParser::startDocument()
{
    return true;
}

bool
ActionFileParser::startElement(const QString& namespaceURI,
                               const QString& localName,
                               const QString& qName,
                               const QXmlAttributes& atts)
{
    QString name = qName.toLower();

    if (name == "menubar") {
        
        m_inMenuBar = true;
        
    } else if (name == "menu") {

        QString menuName = atts.value("name");
        if (menuName == "") {
            cerr << "WARNING: ActionFileParser::startElement(" << m_currentFile << "): No menu name provided in menu element" << endl;
        }
        
        if (!m_currentMenus.empty()) {
            addMenuToMenu(m_currentMenus.last(), menuName);
        }

        m_currentMenus.push_back(menuName);

    } else if (name == "toolbar") {

        QString toolbarName = atts.value("name");
        if (toolbarName == "") {
            cerr << "WARNING: ActionFileParser::startElement(" << m_currentFile << "): No toolbar name provided in toolbar element" << endl;
        }
        m_currentToolbar = toolbarName;
        
    } else if (name == "text") {

        // used to provide label for menu or title for toolbar, but
        // text comes from characters()

        if (!m_currentMenus.empty() || m_currentToolbar != "") {
            m_inText = true;
            m_currentText = "";
        }

    } else if (name == "action") {

        QString actionName = atts.value("name");
        if (actionName == "") {
            cerr << "WARNING: ActionFileParser::startElement(" << m_currentFile << "): No action name provided in action element" << endl;
        }

        if (m_currentMenus.empty() && m_currentToolbar == "" && m_currentState == "") {
            cerr << "WARNING: ActionFileParser::startElement(" << m_currentFile << "): Action \"" << actionName << "\" appears outside (valid) menu, toolbar or state element" << endl;
        }

        QString text = atts.value("text");
        QString icon = atts.value("icon");
        QString shortcut = atts.value("shortcut");
        QString group = atts.value("group");
        QString checked = atts.value("checked");

        //!!! return values
        if (text != "") setActionText(actionName, text);
        if (icon != "") setActionIcon(actionName, icon);
        if (shortcut != "") setActionShortcut(actionName, shortcut);
        if (group != "") setActionGroup(actionName, group);
        if (checked != "") setActionChecked(actionName,
                                            checked.toLower() == "true");
        
        //!!! can appear in menu, toolbar, state/enable, state/disable

        if (!m_currentMenus.empty()) addActionToMenu(m_currentMenus.last(), actionName);
        if (m_currentToolbar != "") addActionToToolbar(m_currentToolbar, actionName);

        if (m_currentState != "") {
            //!!!
        }

    } else if (name == "separator") {

        if (!m_currentMenus.empty()) addSeparatorToMenu(m_currentMenus.last());
        if (m_currentToolbar != "") addSeparatorToToolbar(m_currentToolbar);

    } else if (name == "state") {

        QString stateName = atts.value("name");
        if (stateName == "") {
            cerr << "WARNING: ActionFileParser::startElement(" << m_currentFile << "): No state name provided in state element" << endl;
        }
        m_currentState = stateName;

    } else if (name == "enable") {

        //!!! in state

    } else if (name == "disable") {

        //!!! in state
    }

    return true;
}

bool
ActionFileParser::endElement(const QString& namespaceURI,
                             const QString& localName,
                             const QString& qName)
{
    QString name = qName.toLower();

    if (name == "menubar") {
        
        m_inMenuBar = false;
        
    } else if (name == "menu") {

        m_currentMenus.pop_back();

    } else if (name == "toolbar") {

        m_currentToolbar = "";

    } else if (name == "text") {

        if (m_inText) {
            if (!m_currentMenus.empty()) {
                setMenuText(m_currentMenus.last(), m_currentText);
            }
            if (m_currentToolbar != "") {
                setToolbarText(m_currentToolbar, m_currentText);
            }
            m_inText = false;
        }

    } else if (name == "state") {
        
        m_currentState = "";
    }
        
    return true;
}

bool
ActionFileParser::characters(const QString &ch)
{
    if (m_inText) m_currentText += ch;
    return true;
}

bool
ActionFileParser::endDocument()
{
    return true;
}

bool
ActionFileParser::error(const QXmlParseException &exception)
{
    QString errorString =
	QString("ERROR: ActionFileParser: %1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    std::cerr << errorString.toLocal8Bit().data() << std::endl;
    return QXmlDefaultHandler::error(exception);
}

bool
ActionFileParser::fatalError(const QXmlParseException &exception)
{
    QString errorString =
	QString("FATAL ERROR: ActionFileParser: %1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    std::cerr << errorString.toLocal8Bit().data() << std::endl;
    return QXmlDefaultHandler::fatalError(exception);
}

QAction *
ActionFileParser::findAction(QString actionName)
{
    if (!m_actionOwner) return 0;
    //!!! we could create an action, if it does not yet exist, that
    //!!! pops up a dialog or something explaining that the action
    //!!! needs to have been created before the rc file is read
    return m_actionOwner->findChild<QAction *>(actionName);
}

QAction *
ActionFileParser::findStandardAction(QString actionName)
{
    CommandHistory *history = CommandHistory::getInstance();
    if (!history) return 0;
    return history->findChild<QAction *>(actionName);
}

QActionGroup *
ActionFileParser::findGroup(QString groupName)
{
    QActionGroup *group = m_actionOwner->findChild<QActionGroup *>(groupName);
    if (!group) {
        group = new QActionGroup(m_actionOwner);
        group->setObjectName(groupName);
    }
    return group;
}

QMenu *
ActionFileParser::findMenu(QString menuName)
{
    QMenu *menu = m_actionOwner->findChild<QMenu *>(menuName);
    if (!menu) {
        menu = new QMenu(m_actionOwner);
        menu->setObjectName(menuName);
        QMainWindow *mw = dynamic_cast<QMainWindow *>(m_actionOwner);
        if (mw) {
            mw->menuBar()->addMenu(menu);
        }
    }
    return menu;
}

QToolBar *
ActionFileParser::findToolbar(QString toolbarName)
{
    QToolBar *toolbar = m_actionOwner->findChild<QToolBar *>(toolbarName);
    if (!toolbar) {
        toolbar = new QToolBar(toolbarName, m_actionOwner);
        toolbar->setObjectName(toolbarName);
    }
    return toolbar;
}

QString
ActionFileParser::translate(QString actionName,
                            QString text,
                            QString purpose)
{
    return text; //!!! implement!
}                                       

bool
ActionFileParser::setActionText(QString actionName, QString text)
{
    if (actionName == "" || text == "") return false;
    QAction *action = findAction(actionName);
    if (!action) return false;
    action->setText(translate(actionName, text, "text"));
    return true;
}

bool
ActionFileParser::setActionIcon(QString actionName, QString icon)
{
    if (actionName == "" || icon == "") return false;
    QAction *action = findAction(actionName);
    if (!action) return false;
    action->setIcon(IconLoader().load(icon));
    return true;
}

bool
ActionFileParser::setActionShortcut(QString actionName, QString shortcut)
{
    if (actionName == "" || shortcut == "") return false;
    QAction *action = findAction(actionName);
    if (!action) return false;
    action->setShortcut(translate(actionName, shortcut, "shortcut"));
    return true;
}

bool
ActionFileParser::setActionGroup(QString actionName, QString groupName)
{
    if (actionName == "" || groupName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) return false;
    QActionGroup *group = findGroup(groupName);
    action->setActionGroup(group);
    return true;
}

bool
ActionFileParser::setActionChecked(QString actionName, bool checked)
{
    if (actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) return false;
    action->setCheckable(true);
    action->setChecked(checked);
    return true;
}

bool
ActionFileParser::setMenuText(QString name, QString text)
{
    if (name == "" || text == "") return false;
    QMenu *menu = findMenu(name);
    if (!menu) return false;
    menu->setTitle(translate(name, text, "menu title"));
    return true;
}

bool
ActionFileParser::addMenuToMenu(QString parent, QString child)
{
    if (parent == "" || child == "") return false;
    QMenu *parentMenu = findMenu(parent);
    QMenu *childMenu = findMenu(child);
    if (!parentMenu || !childMenu) return false;
    parentMenu->addMenu(childMenu);
    return true;
}

bool
ActionFileParser::addActionToMenu(QString menuName, QString actionName)
{
    if (menuName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    QMenu *menu = findMenu(menuName);
    if (!menu) return false;
    menu->addAction(action);
    return true;
}

bool
ActionFileParser::addSeparatorToMenu(QString menuName)
{
    if (menuName == "") return false;
    QMenu *menu = findMenu(menuName);
    if (!menu) return false;
    menu->addSeparator();
    return true;
}

bool
ActionFileParser::setToolbarText(QString name, QString text)
{
    //!!! This doesn't appear to be possible (no QToolBar::setTitle
    //!!! method), but I don't think that will be a big problem in
    //!!! practice because we can set a proper title from the ctor (in
    //!!! findToolbar).  Review
    return true;
}

bool
ActionFileParser::addActionToToolbar(QString toolbarName, QString actionName)
{
    if (toolbarName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    QToolBar *toolbar = findToolbar(toolbarName);
    if (!toolbar) return false;
    toolbar->addAction(action);
    return true;
}

bool
ActionFileParser::addSeparatorToToolbar(QString toolbarName)
{
    if (toolbarName == "") return false;
    QToolBar *toolbar = findToolbar(toolbarName);
    if (!toolbar) return false;
    toolbar->addSeparator();
    return true;
}

bool
ActionFileParser::addState(QString name)
{
    if (name == "") return false;
    //!!! implement
    return true;
}

bool
ActionFileParser::enableActionInState(QString stateName, QString actionName)
{
    if (stateName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) return false;
    //!!! implement
    return true;
}

bool
ActionFileParser::disableActionInState(QString stateName, QString actionName)
{
    if (stateName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) return false;
    //!!! implement
    return true;
}

}

