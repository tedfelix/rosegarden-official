/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ActionFileParser]"

#define RG_NO_DEBUG_PRINT 1

#include "ActionFileParser.h"

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
#include "misc/Debug.h"
#include "base/Profiler.h"
#include "document/CommandHistory.h"


namespace Rosegarden
{
   
ActionFileParser::ActionFileParser(QObject *actionOwner) :
    m_actionOwner(actionOwner),
    m_inMenuBar(false),
    m_inText(false),
    m_inEnable(false),
    m_inDisable(false),
    m_inVisible(false),
    m_inInvisible(false),
    m_lastToolbarPosition(Top)
{
}

ActionFileParser::~ActionFileParser()
{
}

QString
ActionFileParser::findRcFile(QString name)
{
    return ResourceFinder().getResourcePath("rc", name);
}

bool
ActionFileParser::load(QString actionRcFile)
{
    //Profiler p("ActionFileParser::load");

    QString location = findRcFile(actionRcFile);
    if (location == "") {
        RG_WARNING << "load(): Failed to find RC file \"" << actionRcFile << "\"";
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
ActionFileParser::startElement(const QString& /* namespaceURI */,
                               const QString& /* localName */,
                               const QString& qName,
                               const QXmlAttributes& atts)
{
    QString name = qName.toLower();

    if (name == "menubar") {
        
        m_inMenuBar = true;
        
    } else if (name == "menu") {

        QString menuName = atts.value("name");
        if (menuName == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): No menu name provided in menu element";
        }
        
        if (m_inEnable) {
            enableMenuInState(m_currentState, menuName);
        } else if (m_inDisable) {
            disableMenuInState(m_currentState, menuName);
        } else if (!m_currentMenus.empty()) {
            addMenuToMenu(m_currentMenus.last(), menuName);
        } else {
            addMenuToMenubar(menuName);
        }

        m_currentMenus.push_back(menuName);

    } else if (name == "toolbar") {

        QString newline = atts.value("newline");
        if (newline == "true") addToolbarBreak(m_lastToolbarPosition);

        Position position = Default;
        QString posstr = atts.value("position");
        if (posstr == "top") position = Top;
        if (posstr == "bottom") position = Bottom;
        if (posstr == "left") position = Left;
        if (posstr == "right") position = Right;

        QString toolbarName = atts.value("name");
        if (toolbarName == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): No toolbar name provided in toolbar element";
        }
        (void)findToolbar(toolbarName, position); // creates it if necessary
        m_currentToolbar = toolbarName;
        m_lastToolbarPosition = position;

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
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): No action name provided in action element";
        }

        if (m_currentMenus.empty() && m_currentToolbar == "" &&
            (m_currentState == "" || (!m_inEnable && !m_inDisable
            && !m_inVisible && !m_inInvisible))) {
            RG_WARNING << "WARNING: startElement("
                << m_currentFile << "): Action \"" << actionName
                << "\" appears outside (valid) menu, toolbar or state "
                << "enable/disable/visible/invisible element";
        }

        QString text = atts.value("text");
        QString icon = atts.value("icon");
        QString shortcut = atts.value("shortcut");
        QString shortcutContext = atts.value("shortcut-context");
        QString tooltip = atts.value("tooltip");
        QString group = atts.value("group");
        QString checked = atts.value("checked");

        //!!! return values
        if (text != "") setActionText(actionName, text);
        if (icon != "") setActionIcon(actionName, icon);
        if (shortcut != "") setActionShortcut(actionName, shortcut,
            shortcutContext.toLower() == "application");
        if (tooltip != "") setActionToolTip(actionName, tooltip);
        if (group != "") setActionGroup(actionName, group);
        if (checked != "") setActionChecked(actionName,
            checked.toLower() == "true");
        
        // this can appear in menu, toolbar, state/enable, state/disable,
        // state/visible, state/invisible

        if (m_inEnable) {
            enableActionInState(m_currentState, actionName);
        } else if (m_inDisable) {
            disableActionInState(m_currentState, actionName);
        } else if (m_inVisible) {
            toVisibleActionInState(m_currentState, actionName);
        } else if (m_inInvisible) {
            toInvisibleActionInState(m_currentState, actionName);
        } else if (!m_currentMenus.empty()) {
            addActionToMenu(m_currentMenus.last(), actionName);
        } else if (m_currentToolbar != "") {
            addActionToToolbar(m_currentToolbar, actionName);
        }

    } else if (name == "separator") {

        if (!m_currentMenus.empty()) addSeparatorToMenu(m_currentMenus.last());
        if (m_currentToolbar != "") addSeparatorToToolbar(m_currentToolbar);

    } else if (name == "state") {

        QString stateName = atts.value("name");
        if (stateName == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): No state name provided in state element";
        }
        m_currentState = stateName;

    } else if (name == "enable") {

        if (m_currentState == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): Enable element appears outside state element";
        } else {
            m_inEnable = true;
        }

    } else if (name == "disable") {

        if (m_currentState == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): Disable element appears outside state element";
        } else {
            m_inDisable = true;
        }
    } else if (name == "visible") {

        if (m_currentState == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): Visible element appears outside state element";
        } else {
            m_inVisible = true;
        }
    } else if (name == "invisible") {

        if (m_currentState == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): Invisible element appears outside state element";
        } else {
            m_inInvisible = true;
        }
    }

    return true;
}

bool
ActionFileParser::endElement(const QString& /* namespaceURI */,
                             const QString& /* localName */,
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

    } else if (name == "enable") {
        
        m_inEnable = false;

    } else if (name == "disable") {
        
        m_inDisable = false;

    } else if (name == "visible") {
        
        m_inVisible = false;

    } else if (name == "invisible") {
        
        m_inInvisible = false;
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
            QString("ERROR: %1 at line %2, column %3 in file %4")
                .arg(exception.message())
                .arg(exception.lineNumber())
                .arg(exception.columnNumber())
                .arg(m_currentFile);

    RG_WARNING << errorString.toLocal8Bit().data();

    return QXmlDefaultHandler::error(exception);
}

bool
ActionFileParser::fatalError(const QXmlParseException &exception)
{
    QString errorString =
            QString("FATAL ERROR: %1 at line %2, column %3 in file %4")
                .arg(exception.message())
                .arg(exception.lineNumber())
                .arg(exception.columnNumber())
                .arg(m_currentFile);

    RG_WARNING << errorString.toLocal8Bit().data();

    return QXmlDefaultHandler::fatalError(exception);
}

QAction *
ActionFileParser::findAction(QString actionName)
{
    if (!m_actionOwner) return nullptr;
    //!!! we could create an action, if it does not yet exist, that
    //!!! pops up a dialog or something explaining that the action
    //!!! needs to have been created before the rc file is read
    return m_actionOwner->findChild<QAction *>(actionName);
}

QAction *
ActionFileParser::findStandardAction(QString actionName)
{
    CommandHistory *history = CommandHistory::getInstance();
    if (!history) return nullptr;
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
    QMenu *menu = nullptr;
    QWidget *widget = dynamic_cast<QWidget *>(m_actionOwner);
    if (widget) {
        menu = widget->findChild<QMenu *>(menuName);
        if (!menu) {
            menu = new QMenu(widget);
            menu->setObjectName(menuName);
        }
    } else {
        ActionFileMenuWrapper *ref =
            m_actionOwner->findChild<ActionFileMenuWrapper *>(menuName);
        if (ref) {
            menu = ref->getMenu();
        } else {
            menu = new QMenu(nullptr);
            menu->setObjectName(menuName);
            new ActionFileMenuWrapper(menu, m_actionOwner);
        }
    }
    menu->setMouseTracking(true);
    return menu;
}

QToolBar *
ActionFileParser::findToolbar(QString toolbarName, Position position)
{
    QWidget *widget = dynamic_cast<QWidget *>(m_actionOwner);
    if (!widget) {
        RG_WARNING << "findToolbar(\"" << toolbarName << "\"): Action owner is not a QWidget, cannot have toolbars";
        return nullptr;
    }
    QToolBar *toolbar = widget->findChild<QToolBar *>(toolbarName);
    if (!toolbar) {
        QMainWindow *mw = dynamic_cast<QMainWindow *>(widget);
        if (mw) {
            Qt::ToolBarArea area = Qt::TopToolBarArea;
            switch (position) {
                case Top: case Default: break;
                case Left: area = Qt::LeftToolBarArea; break;
                case Right: area = Qt::RightToolBarArea; break;
                case Bottom: area = Qt::BottomToolBarArea; break;
            }
            toolbar = new QToolBar(QObject::tr(toolbarName.toStdString().c_str()), mw);
            mw->addToolBar(area, toolbar);
        } else {
            toolbar = new QToolBar(QObject::tr(toolbarName.toStdString().c_str()), widget);
        }
        toolbar->setObjectName(toolbarName);
    }
    toolbar->setMouseTracking(true);
    return toolbar;
}

QString
ActionFileParser::translate(QString text,
                            QString disambiguation)
{
    // These translations are extracted from data/ui/*.rc files via
    // scripts/extract*.pl and pulled into the QObject translation context in
    // one great lump.
    
    if (!disambiguation.isEmpty()) return QObject::tr(text.toStdString().c_str(), disambiguation.toStdString().c_str());
    else return QObject::tr(text.toStdString().c_str());
}                                       

bool
ActionFileParser::setActionText(QString actionName, QString text)
{
    if (actionName == "" || text == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;

    // action names do not have a disambiguation in order to avoid making the
    // translators go fish 1000+ translations out of the obsolete strings pile
    action->setText(translate(text));
    return true;
}

bool
ActionFileParser::setActionIcon(QString actionName, QString icon)
{
    if (actionName == "" || icon == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    action->setIcon(IconLoader::load(icon));
    return true;
}

bool
ActionFileParser::setActionShortcut(QString actionName, QString shortcut, bool isApplicationContext)
{
    if (actionName == "" || shortcut == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;

    /*
     * Enable one or multiple shortcuts.  Only the first shortcut, which is
     * considered as the primary one, will be shown in the menus.
     */
    QStringList shortcuts = shortcut.split(", ");
    QList<QKeySequence> shortcutList;
    for (int i = 0; i < shortcuts.size(); i++) {

        // Keyboard shortcuts require the disambiguation "keyboard shortcut"
        // and this must match scripts/extract_menu_tr_strings.pl
        shortcutList.append(translate(shortcuts.at(i), "keyboard shortcut"));
    }
    action->setShortcuts(shortcutList);

    /*
     * Check if the shortcut should be available globally.
     */
    if (isApplicationContext) {
        action->setShortcutContext(Qt::ApplicationShortcut);
    }

    return true;
}

bool
ActionFileParser::setActionToolTip(QString actionName, QString tooltip)
{
    if (actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    action->setToolTip(tooltip);
    m_tooltipSet.insert(action);
    return true;
}

bool
ActionFileParser::setActionGroup(QString actionName, QString groupName)
{
    if (actionName == "" || groupName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
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
    if (!action) action = findStandardAction(actionName);
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

    // menu titles should not have disambiguation in order to avoid rendering a
    // very large number of pre-translated texts obsolete
    menu->setTitle(translate(text));
    return true;
}

bool
ActionFileParser::addMenuToMenu(QString parent, QString child)
{
    RG_DEBUG << "ActionFileParser::addMenuToMenu:" << parent << "," << child;
    if (parent == "" || child == "") return false;
    QMenu *parentMenu = findMenu(parent);
    QMenu *childMenu = findMenu(child);
    if (!parentMenu || !childMenu) return false;
    parentMenu->addMenu(childMenu);
    QMainWindow *mw = dynamic_cast<QMainWindow *>(m_actionOwner);
    if (!mw) return false;
    parentMenu->setMouseTracking(true);
    childMenu->setMouseTracking(true);
    return true;
}

bool
ActionFileParser::addMenuToMenubar(QString menuName)
{
    RG_DEBUG << "ActionFileParser::addMenuToMenubar:" << menuName;
    if (menuName == "") return false;
    QMenu *menu = findMenu(menuName);
    if (!menu) return false;
    QMainWindow *mw = dynamic_cast<QMainWindow *>(m_actionOwner);
    if (!mw) return false;
    mw->menuBar()->addMenu(menu);
    menu->setMouseTracking(true);
    mw->menuBar()->setMouseTracking(true);
    return true;
}

bool
ActionFileParser::addActionToMenu(QString menuName, QString actionName)
{
    RG_DEBUG << "ActionFileParser::addActionToMenu:" << menuName << "," << actionName;
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
    RG_DEBUG << "ActionFileParser::addSeparatorToMenu:" << menuName;
    if (menuName == "") return false;
    QMenu *menu = findMenu(menuName);
    if (!menu) return false;
    menu->addSeparator();
    return true;
}

bool
ActionFileParser::setToolbarText(QString /* name */, QString /* text */)
{
    //!!! This doesn't appear to be possible (no QToolBar::setTitle
    //!!! method), but I don't think that will be a big problem in
    //!!! practice because we can set a proper title from the ctor (in
    //!!! findToolbar).  Review
    return true;
}

// This function cribbed from gui/kernel/qaction.cpp in Qt 4.5
static QString
strippedText(QString s)
{
    s.remove(QString::fromLatin1("..."));
    int i = 0;
    while (i < s.size()) {
        ++i;
        if (s.at(i-1) != QLatin1Char('&'))
            continue;
        if (i < s.size() && s.at(i) == QLatin1Char('&'))
            ++i;
        s.remove(i-1,1);
    }
    return s.trimmed();
};

bool
ActionFileParser::addActionToToolbar(QString toolbarName, QString actionName)
{
    RG_DEBUG << "ActionFileParser::addActionToToolbar:" << toolbarName << "," << actionName;
    if (toolbarName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    QToolBar *toolbar = findToolbar(toolbarName, Default);
    if (!toolbar) return false;
    toolbar->addAction(action);

    // duration toolbar tooltips:
    // no text() toolTip() in English like "Double Whole Note (5)"
    if (!action->toolTip().isEmpty() && action->text().isEmpty()) {
        QString m(action->toolTip());
        QString tip = QObject::tr("%1").arg(QObject::tr(m.toStdString().c_str()));
        action->setToolTip(tip);
        //RG_DEBUG << "setting tip: " << tip;
        // transport toolbar tooltips:
        // text() translated, toolTip() in English
    } else if (strippedText(action->text()) != action->toolTip()) {
        QString m(action->toolTip());
        QString tip = QObject::tr("%1").arg(QObject::tr(m.toStdString().c_str()));
        action->setToolTip(tip);
        //RG_DEBUG << "setting tip: " << tip;
    } else if (action->shortcut() != QKeySequence()) {
        // Avoid setting an automatic tooltip if an explicit one has
        // been provided via setActionToolTip earlier.  We need to
        // remember this ourselves, as action->toolTip() will return a
        // Qt-generated automatic default if we haven't set it
        if (m_tooltipSet.find(action) == m_tooltipSet.end()) {
            QString tooltip = QObject::tr("%1 (%2)")
                .arg(strippedText(action->text()))
                .arg(action->shortcut().toString());
            action->setToolTip(tooltip);
        }
    }
    return true;
}

bool
ActionFileParser::addSeparatorToToolbar(QString toolbarName)
{
    RG_DEBUG << "ActionFileParser::addSeparatorToToolbar:" << toolbarName;
    if (toolbarName == "") return false;
    QToolBar *toolbar = findToolbar(toolbarName, Default);
    if (!toolbar) return false;
    toolbar->addSeparator();
    return true;
}

void
ActionFileParser::addToolbarBreak(Position position)
{
    QMainWindow *mw = dynamic_cast<QMainWindow *>(m_actionOwner);
    if (!mw) return;
    Qt::ToolBarArea area = Qt::TopToolBarArea;
    switch (position) {
        case Top: case Default: break;
        case Left: area = Qt::LeftToolBarArea; break;
        case Right: area = Qt::RightToolBarArea; break;
        case Bottom: area = Qt::BottomToolBarArea; break;
    }
    mw->addToolBarBreak(area);
}

bool
ActionFileParser::enableActionInState(QString stateName, QString actionName)
{
    if (stateName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    m_stateEnableMap[stateName].insert(action);
    connect(action, &QObject::destroyed, this, &ActionFileParser::slotObjectDestroyed);
    return true;
}

bool
ActionFileParser::disableActionInState(QString stateName, QString actionName)
{
    if (stateName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    m_stateDisableMap[stateName].insert(action);
    connect(action, &QObject::destroyed, this, &ActionFileParser::slotObjectDestroyed);
    return true;
}

bool
ActionFileParser::enableMenuInState(QString stateName, QString menuName)
{
    if (stateName == "" || menuName == "") return false;
    QMenu *menu = findMenu(menuName);
    if (!menu) return false;
    QList<QAction *> actions = menu->actions();
    for (int i = 0; i < actions.size(); ++i) {
        QAction *a = actions[i];
        if (!a) continue;
        m_stateEnableMap[stateName].insert(a);
        connect(a, &QObject::destroyed, this, &ActionFileParser::slotObjectDestroyed);
    }
    return true;
}

bool
ActionFileParser::disableMenuInState(QString stateName, QString menuName)
{
    if (stateName == "" || menuName == "") return false;
    QMenu *menu = findMenu(menuName);
    if (!menu) return false;
    QList<QAction *> actions = menu->actions();
    for (int i = 0; i < actions.size(); ++i) {
        QAction *a = actions[i];
        if (!a) continue;
        m_stateDisableMap[stateName].insert(a);
        connect(a, &QObject::destroyed, this, &ActionFileParser::slotObjectDestroyed);
    }
    return true;
}

bool
ActionFileParser::toVisibleActionInState(QString stateName, QString actionName)
{
    if (stateName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    m_stateVisibleMap[stateName].insert(action);
    connect(action, &QObject::destroyed, this, &ActionFileParser::slotObjectDestroyed);
    return true;
}

bool
ActionFileParser::toInvisibleActionInState(QString stateName, QString actionName)
{
    if (stateName == "" || actionName == "") return false;
    QAction *action = findAction(actionName);
    if (!action) action = findStandardAction(actionName);
    if (!action) return false;
    m_stateInvisibleMap[stateName].insert(action);
    connect(action, &QObject::destroyed, this, &ActionFileParser::slotObjectDestroyed);
    return true;
}

void
ActionFileParser::setEnabled(QAction *a, bool e)
{
    if (a) a->setEnabled(e);
}

void
ActionFileParser::setVisible(QAction *a, bool e)
{
    if (a) a->setVisible(e);
}

void
ActionFileParser::enterActionState(QString stateName)
{
    Profiler p("ActionFileParser::enterActionState");
    for (ActionSet::iterator i = m_stateInvisibleMap[stateName].begin();
         i != m_stateInvisibleMap[stateName].end(); ++i) {
        setVisible(*i, false);
    }
    for (ActionSet::iterator i = m_stateDisableMap[stateName].begin();
         i != m_stateDisableMap[stateName].end(); ++i) {
        setEnabled(*i, false);
    }
    for (ActionSet::iterator i = m_stateVisibleMap[stateName].begin();
         i != m_stateVisibleMap[stateName].end(); ++i) {
        setVisible(*i, true);
    }
    for (ActionSet::iterator i = m_stateEnableMap[stateName].begin();
         i != m_stateEnableMap[stateName].end(); ++i) {
        setEnabled(*i, true);
    }
}

void
ActionFileParser::leaveActionState(QString stateName)
{
    Profiler p("ActionFileParser::leaveActionState");
    for (ActionSet::iterator i = m_stateEnableMap[stateName].begin();
         i != m_stateEnableMap[stateName].end(); ++i) {
        setEnabled(*i, false);
    }
    for (ActionSet::iterator i = m_stateVisibleMap[stateName].begin();
         i != m_stateVisibleMap[stateName].end(); ++i) {
        setVisible(*i, false);
    }
    for (ActionSet::iterator i = m_stateInvisibleMap[stateName].begin();
         i != m_stateInvisibleMap[stateName].end(); ++i) {
        setVisible(*i, true);
    }
    for (ActionSet::iterator i = m_stateDisableMap[stateName].begin();
         i != m_stateDisableMap[stateName].end(); ++i) {
        setEnabled(*i, true);
    }
}

void
ActionFileParser::slotObjectDestroyed()
{
    QObject *o = sender();
    QAction *a = dynamic_cast<QAction *>(o);
    if (!a) return;

    for (StateMap::iterator i = m_stateEnableMap.begin();
         i != m_stateEnableMap.end(); ++i) {
        i->erase(a);
    }

    for (StateMap::iterator i = m_stateDisableMap.begin();
         i != m_stateDisableMap.end(); ++i) {
        i->erase(a);

    }
    for (StateMap::iterator i = m_stateVisibleMap.begin();
         i != m_stateVisibleMap.end(); ++i) {
        i->erase(a);

    }
    for (StateMap::iterator i = m_stateInvisibleMap.begin();
         i != m_stateInvisibleMap.end(); ++i) {
        i->erase(a);
    }
}

ActionFileMenuWrapper::ActionFileMenuWrapper(QMenu *menu, QObject *parent) :
    QObject(parent),
    m_menu(menu) 
{
    setObjectName(menu->objectName());
}

ActionFileMenuWrapper::~ActionFileMenuWrapper()
{
    delete m_menu;
}

QMenu *
ActionFileMenuWrapper::getMenu()
{
    return m_menu;
}

}
