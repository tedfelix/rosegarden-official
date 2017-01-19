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

/// Base class for users of the .rc file.
/**
 * Inheriting from this class gives the deriver access to the menus and
 * toolbars in the .rc file.
 *
 * ??? Derivation seems a bit extreme in this case.  Consider using
 *     containment instead.  Pass the QObject pointer in via the ctor.
 *     That will then eliminate the need for multiple inheritance.
 *     One issue is whether anyone actually overrides any of these
 *     virtual functions.  If no one does, then containment really is
 *     preferable.
 */
class ActionFileClient
{
public:
    /**
     * Find an action of the given name.  This function will always
     * return a valid Action pointer; if the action does not exist, a
     * DecoyAction will be returned.  Usage such as
     * findAction("action_name")->setChecked(true); is acceptable.
     *
     * ??? This function is public because it is used by:
     *     NotationTool::findActionInParentView()
     *     MatrixTool::findActionInParentView()
     */
    virtual QAction *findAction(QString actionName);

protected:
    ActionFileClient();
    virtual ~ActionFileClient();

    /// Create a child QAction, set its object name, and connect its triggered().
    QAction *createAction(QString actionName, QString connection);
    /// Create a child QAction, set its object name, and connect its triggered().
    QAction *createAction(QString actionName, QObject *target, QString connection);

    /// Read the .rc file and create the QMenu and QToolBar objects.
    /**
     * The QAction child objects must be present before calling this function.
     * See createAction().
     */
    bool createGUI(QString rcname);

    /**
     * Find a group of the given name.  If it does not exist,
     * this will currently return a null pointer -- beware the
     * inconsistency with the other methods here!
     */
    virtual QActionGroup *findGroup(QString groupName);

    /**
     * Find a menu of the given object name.  If it does not exist,
     * this will currently return a null pointer -- beware the
     * inconsistency with the other methods here!
     */
    virtual QMenu *findMenu(QString menuName);

    /// Creates the toolbar if it doesn't exist.
    virtual QToolBar *findToolbar(QString toolbarName);

    /// Enable/disable and show/hide actions based on the new state.
    virtual void enterActionState(QString stateName);
    /// Enable/disable and show/hide actions based on leaving the state.
    virtual void leaveActionState(QString stateName);

    /// Enable auto-repeat for a toolbar button.
    void enableAutoRepeat(
            const QString &toolBarName,
            const QString &actionName);

private:
    // ActionCommandRegistry calls createAction().
    friend class ActionCommandRegistry;

    ActionFileParser *m_actionFileParser;
};

}

#endif

