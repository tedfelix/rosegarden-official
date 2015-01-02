/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ACTIONFILECLIENT_H
#define RG_ACTIONFILECLIENT_H

#include <QString>

class QAction;
class QActionGroup;
class QMenu;
class QToolBar;
class QObject;

namespace Rosegarden
{

class ActionFileParser;

class ActionFileClient // base class for users of the ActionFileParser
{
public:
    /**
     * Find an action of the given name.  This function will always
     * return a valid Action pointer; if the action does not exist, a
     * DecoyAction will be returned.  Usage such as
     * findAction("action_name")->setChecked(true); is acceptable.
     */
    virtual QAction *findAction(QString actionName);

    /**
     * Find an group of the given name.  If it does not exist,
     * this will currently return a null pointer -- beware the
     * inconsistency with the other methods here!
     */
    virtual QActionGroup *findGroup(QString groupName);
    virtual void enterActionState(QString stateName);
    virtual void leaveActionState(QString stateName);

    /**
     * Find a menu of the given object name.  If it does not exist,
     * this will currently return a null pointer -- beware the
     * inconsistency with the other methods here!
     */
    virtual QMenu *findMenu(QString menuName);

    /**
     * Find a toolbar of the given name, creating it if it does not
     * exist.
     */
    virtual QToolBar *findToolbar(QString toolbarName);

protected:
    ActionFileClient();
    virtual ~ActionFileClient();

    QAction *createAction(QString actionName, QString connection);
    QAction *createAction(QString actionName, QObject *target, QString connection);
    bool createGUI(QString rcname);
    friend class ActionCommandRegistry;

private:
    ActionFileParser *m_actionFileParser;
};

}

#endif

