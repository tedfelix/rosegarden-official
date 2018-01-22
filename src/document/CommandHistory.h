/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMMANDHISTORY_H
#define RG_COMMANDHISTORY_H

#include <QObject>
#include <QString>

#include <stack>
#include <set>
#include <map>

class QAction;
class QMenu;
class QToolBar;
class QTimer;

namespace Rosegarden 
{

class Command;
class MacroCommand;
class ActionFileClient;

/**
 * The CommandHistory class stores a list of executed
 * commands and maintains Undo and Redo actions synchronised with
 * those commands.
 *
 * CommandHistory allows you to associate more than one Undo
 * and Redo menu or toolbar with the same command history, and it
 * keeps them all up-to-date at once.  This makes it effective in
 * systems where multiple views may be editing the same data.
 */
class CommandHistory : public QObject
{
    Q_OBJECT

public:
    virtual ~CommandHistory();

    static CommandHistory *getInstance();

    void clear();

    // These are done by the .rc file these days.
    //void registerMenu(QMenu *menu);
    //void registerToolbar(QToolBar *toolbar);

    /// Add a command to the command history.
    /**
     * The command will normally be executed before being added; but
     * if a compound operation is in use (see startCompoundOperation
     * below), the execute status of the compound operation will
     * determine whether the command is executed or not.
     */
    void addCommand(Command *command);
    
    /// Add a command to the command history.
    /**
     * If execute is true, the command will be executed before being
     * added.  Otherwise it will be assumed to have been already
     * executed -- a command should not be added to the history unless
     * its work has actually been done somehow!
     *
     * If a compound operation is in use (see startCompoundOperation
     * below), the execute value passed to this method will override
     * the execute status of the compound operation.  In this way it's
     * possible to have a compound operation mixing both to-execute
     * and pre-executed commands.
     *
     * If bundle is true, the command will be a candidate for bundling
     * with any adjacent bundleable commands that have the same name,
     * into a single compound command.  This is useful for small
     * commands that may be executed repeatedly altering the same data
     * (e.g. type text, set a parameter) whose number and extent is
     * not known in advance.  The bundle parameter will be ignored if
     * a compound operation is already in use.
     */
    void addCommand(Command *command, bool execute, bool bundle = false);
    
    /// Return the maximum number of items in the undo history.
    int getUndoLimit() const { return m_undoLimit; }

    /// Set the maximum number of items in the undo history.
    void setUndoLimit(int limit);

    /// Return the maximum number of items in the redo history.
    int getRedoLimit() const { return m_redoLimit; }

    /// Set the maximum number of items in the redo history.
    void setRedoLimit(int limit);
    
    /// Return the maximum number of items visible in undo and redo menus.
    int getMenuLimit() const { return m_menuLimit; }

    /// Set the maximum number of items in the menus.
    void setMenuLimit(int limit);

    /// Return the time after which a bundle will be closed if nothing is added.
    int getBundleTimeout() const { return m_bundleTimeout; }

    /// Set the time after which a bundle will be closed if nothing is added.
    void setBundleTimeout(int msec);

    /// Start recording commands to batch up into a single compound command.
    void startCompoundOperation(QString name, bool execute);

    /// Finish recording commands and store the compound command.
    void endCompoundOperation();

public slots:
    /**
     * Checkpoint function that should be called when the document is
     * saved.  If the undo/redo stack later returns to the point at
     * which the document was saved, the documentRestored signal will
     * be emitted.
     */
    virtual void documentSaved();

    /**
     * Add a command to the history that has already been executed,
     * without executing it again.  Equivalent to addCommand(command, false).
     */
    void addExecutedCommand(Command *);

    /**
     * Add a command to the history and also execute it.  Equivalent
     * to addCommand(command, true).
     */
    void addCommandAndExecute(Command *);

    void undo();
    void redo();

protected slots:
    void undoActivated(QAction *);
    void redoActivated(QAction *);
    void bundleTimerTimeout();
    
signals:
    /**
     * Emitted just before commandExecuted() so that linked segments can
     * update their siblings.
     */
    void updateLinkedSegments(Command *);

    /**
     * Emitted whenever a command has just been executed or
     * unexecuted, whether by addCommand, undo, or redo.  Note in
     * particular that this is emitted by both undo and redo.
     */
    void commandExecuted();

    /**
     * Emitted whenever a command has just been executed, whether by
     * addCommand or redo.  Note that this is not emitted by undo,
     * which emits commandUnexecuted(Command *) instead.
     */
    void commandExecuted(Command *);

    /**
     * Emitted whenever a command has just been unexecuted, whether by
     * addCommand or undo.
     */
    void commandUnexecuted(Command *);

    /**
     * Emitted when the undo/redo stack has reached the same state at
     * which the documentSaved slot was last called.
     */
    void documentRestored();


protected:
    CommandHistory();
    static CommandHistory *m_instance;

    // Actions and Menus

    /// Edit > Undo on all menus.
    QAction *m_undoAction;
    /// Edit > Redo on all menus.
    QAction *m_redoAction;
    /// RosegardenMainWindow toolbar undo.
    QAction *m_undoMenuAction;
    /// RosegardenMainWindow toolbar redo.
    QAction *m_redoMenuAction;

    QMenu *m_undoMenu;
    QMenu *m_redoMenu;

    std::map<QAction *, int> m_actionCounts;

    void updateActions();

    // Command Stacks
    typedef std::stack<Command *> CommandStack;
    CommandStack m_undoStack;
    CommandStack m_redoStack;
    void clipStack(CommandStack &stack, int limit);
    void clearStack(CommandStack &stack);
    void clipCommands();

    int m_undoLimit;
    int m_redoLimit;
    int m_menuLimit;
    int m_savedAt;

    // Compound
    MacroCommand *m_currentCompound;
    bool m_executeCompound;
    void addToCompound(Command *command, bool execute);

    // Bundle
    MacroCommand *m_currentBundle;
    QString m_currentBundleName;
    QTimer *m_bundleTimer;
    int m_bundleTimeout;
    void addToBundle(Command *command, bool execute);
    void closeBundle();
    
};

}

#endif
