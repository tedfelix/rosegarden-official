/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

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

#include "base/TimeT.h"

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
 *
 * ??? This should be a member of RosegardenDocument.  Then it can
 *     be destroyed when the current document is destroyed, and it
 *     can be ignored when a temporary document is created.  As a
 *     global Singleton this is too confusing.
 */
class CommandHistory : public QObject
{
    Q_OBJECT

public:
    ~CommandHistory() override;

    static CommandHistory *getInstance();

    void clear();

    // These are done by the .rc file these days.
    //void registerMenu(QMenu *menu);
    //void registerToolbar(QToolBar *toolbar);

    /// Add a command to the command history.
    /**
     * The command will be executed before being added
     */
    void addCommand(Command *command);

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

    /// Enable/Disable undo (during playback).
    void enableUndo(bool enable);

    /// set pointer position
    void setPointerPosition(timeT pos);

    /// set pointer position
    void setPointerPositionForRedo(timeT pos);

    /// get pointer position
    timeT getPointerPosition() const;

public slots:
    /**
     * Checkpoint function that should be called when the document is
     * saved.  If the undo/redo stack later returns to the point at
     * which the document was saved, the documentRestored signal will
     * be emitted.
     */
    virtual void documentSaved();

    void undo();
    void redo();

protected slots:
    void undoActivated(QAction *);
    void redoActivated(QAction *);

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
     *
     * ??? Appears to be unused.
     */
    //void commandExecuted2(Command *);

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

    /**
     * Emitted just before a command is executed
     */
    void aboutToExecuteCommand();

    /**
     * Emitted just after a command undo
     */
    void commandUndone();

    /**
     * Emitted just after a command redo
     */
    void commandRedone();

    /**
     * Emitted when a command is initially executed (not on redo)
     */
    void commandExecutedInitially();

protected:
    CommandHistory();
    static CommandHistory *m_instance;

    // Actions and Menus

    /// Edit > Undo on all menus.
    QAction *m_undoAction;
    /// Edit > Redo on all menus.
    QAction *m_redoAction;

    QMenu *m_undoMenu;
    QMenu *m_redoMenu;

    std::map<QAction *, int> m_actionCounts;

    void updateActions();

    // Command Stacks
    struct CommandInfo
    {
        Command *command;
        timeT pointerPositionBefore;  // for undo
        timeT pointerPositionAfter;   // for redo
    };
    typedef std::stack<CommandInfo> CommandStack;
    CommandStack m_undoStack;
    CommandStack m_redoStack;
    void clipStack(CommandStack &stack, int limit);
    void clearStack(CommandStack &stack);
    void clipCommands();

    int m_undoLimit;
    int m_redoLimit;
    int m_menuLimit;
    int m_savedAt;

    /// Enable/Disable undo (during playback).
    bool m_enableUndo;

    // pointer position
    timeT m_pointerPosition;

};

}

#endif
