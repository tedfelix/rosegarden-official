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

#ifndef RG_ACTIONFILEPARSER_H
#define RG_ACTIONFILEPARSER_H

#include <QXmlDefaultHandler>
#include <QMap>
#include <QObject>
#include <set>

class QAction;
class QActionGroup;
class QMenu;
class QToolBar;

namespace Rosegarden
{


/// Convert a .rc file to QMenu and QToolBar objects.
/**
 * This class is used by ActionFileClient to load the menus and toolbars
 * from a .rc file and translate them into QMenu and QToolBar objects that
 * are then added as children to the ActionFileClient deriver.
 */
class ActionFileParser : public QObject, public QXmlDefaultHandler
{
    Q_OBJECT

public:
    ActionFileParser(QObject *actionOwner);
    virtual ~ActionFileParser();
    
    bool load(QString actionRcFile);

    /// Enable/disable and show/hide actions based on the new state.
    void enterActionState(QString stateName);
    /// Enable/disable and show/hide actions based on leaving the state.
    void leaveActionState(QString stateName);

private slots:
    void slotObjectDestroyed();

private:
    enum Position { Top, Bottom, Left, Right, Default };

    bool setActionText(QString actionName, QString text);
    bool setActionIcon(QString actionName, QString icon);
    bool setActionShortcut(QString actionName, QString shortcut, bool isApplicationContext);
    bool setActionToolTip(QString actionName, QString tooltip);
    bool setActionGroup(QString actionName, QString group);
    bool setActionChecked(QString actionName, bool);

    bool setMenuText(QString name, QString text);
    bool addMenuToMenu(QString parent, QString child);
    bool addMenuToMenubar(QString menuName);
    bool addActionToMenu(QString menuName, QString actionName);
    bool addSeparatorToMenu(QString menuName);
    
    bool setToolbarText(QString name, QString text);
    bool addActionToToolbar(QString toolbarName, QString actionName);
    bool addSeparatorToToolbar(QString toolbarName);
    void addToolbarBreak(Position);

    bool enableActionInState(QString stateName, QString actionName);
    bool disableActionInState(QString stateName, QString actionName);

    /**
     * Set the action as visible for state.
     */
    bool toVisibleActionInState(QString stateName, QString actionName);

    /**
     * Set the action as invisible for state.
     */
    bool toInvisibleActionInState(QString stateName, QString actionName);

    bool enableMenuInState(QString stateName, QString menuName);
    bool disableMenuInState(QString stateName, QString menuName);

    /** Translate a string with QObject::tr() and an optional disambiguation
     *  \param text            The text to translate
     *  \param disambiguation  Context disambiguation, if required
     *  \return A translated QString
     */
    QString translate(QString text, QString disambiguation = "");

    QString findRcFile(QString name);

    QAction *findAction(QString name);
    QAction *findStandardAction(QString name);
    QActionGroup *findGroup(QString name);
    QMenu *findMenu(QString name);
    QToolBar *findToolbar(QString name, Position position);

    typedef std::set<QAction *> ActionSet;
    typedef QMap<QString, ActionSet> StateMap;
    // Map of enable(d) items when entering action state.
    StateMap m_stateEnableMap;
    // Map of disable(d) items when entering action state.
    StateMap m_stateDisableMap;
    // Map of visible items when entering action state.
    StateMap m_stateVisibleMap;
    // Map of invisible items when entering action state.
    StateMap m_stateInvisibleMap;

    /**
     * Null safe setter for QAction->enable(bool).
     */
    void setEnabled(QAction *, bool);

    /**
     * Null safe setter for QAction->setVisible(bool).
     */
    void setVisible(QAction *, bool);

    ActionSet m_tooltipSet;

    QObject *m_actionOwner;
    bool m_inMenuBar;
    bool m_inText;
    bool m_inEnable;
    bool m_inDisable;
    bool m_inVisible;  // Are we inside a State/visible tag?
    bool m_inInvisible;  // Are we inside a State/invisible tag?
    QStringList m_currentMenus;
    QString m_currentToolbar;
    QString m_currentState;
    QString m_currentText;
    QString m_currentFile;
    Position m_lastToolbarPosition;

    // QXmlDefaultHandler methods

    virtual bool startDocument();

    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts);

    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName);

    virtual bool characters(const QString& ch);

    virtual bool endDocument();

    bool error(const QXmlParseException &exception);
    bool fatalError(const QXmlParseException &exception);
};

/**
 * A QMenu needs a QWidget as its parent, but the action file client
 * will not necessarily be a QWidget -- for example, it might be a
 * tool object.  In this case, we need to make a menu that has no
 * parent, but we need to wrap it in something that is parented by the
 * action file client so that we can find it later and it shares the
 * scope of the client.  This is that wrapper.
 */
class ActionFileMenuWrapper : public QObject
{
    Q_OBJECT

public:
    ActionFileMenuWrapper(QMenu *menu, QObject *parent);
    virtual ~ActionFileMenuWrapper();

    QMenu *getMenu();

private:
    QMenu *m_menu;
};


}

#endif
