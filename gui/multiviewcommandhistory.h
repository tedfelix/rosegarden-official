/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef MULTI_VIEW_COMMAND_HISTORY_H
#define MULTI_VIEW_COMMAND_HISTORY_H

#include <qobject.h>
#include <qstring.h>
#include <stack>
#include <set>

class Command;
class KActionCollection;
class KToolBar;

/**
 * The MultiViewCommandHistory class is much like KCommandHistory in
 * that it stores a list of executed commands and maintains Undo and
 * Redo actions synchronised with those commands.
 *
 * The difference is that MultiViewCommandHistory allows you to
 * associate more than one Undo and Redo action with the same command
 * history, and it keeps them all up-to-date at once.  This makes it
 * effective in systems where multiple views may be editing the same
 * data at once.
 */

class MultiViewCommandHistory : public QObject
{
    Q_OBJECT
public:

    MultiViewCommandHistory();
    virtual ~MultiViewCommandHistory();

    void clear();
    
    void attachView(KActionCollection *collection);
    void detachView(KActionCollection *collection);

    void addCommand(Command *command, bool execute = true);
    
    /// @return the maximum number of items in the undo history
    int undoLimit() { return m_undoLimit; }

    /// Set the maximum number of items in the undo history
    void setUndoLimit(int limit);

    /// @return the maximum number of items in the redo history
    int redoLimit() { return m_redoLimit; }

    /// Set the maximum number of items in the redo history
    void setRedoLimit(int limit);
    
public slots:
    /**
     * Remember when you saved the document.
     * Call this right after saving the document. As soon as
     * the history reaches the current index again (via some
     * undo/redo operations) it will emit @ref documentRestored
     * If you implemented undo/redo properly the document is
     * the same you saved before.
     */
    virtual void documentSaved();

protected slots:
    void slotUndo();
    void slotRedo();
    void slotUndoAboutToShow();
    void slotUndoActivated(int);
    void slotRedoAboutToShow();
    void slotRedoActivated(int);

signals:
    /**
     * This is emitted every time a command is executed
     * (whether by addCommand, undo or redo).
     * You can use this to update the GUI, for instance.
     */
    void commandExecuted(Command *);

    /**
     * This is emitted every time a command is executed
     * (whether by addCommand, undo or redo).
     *
     * It should be connected to the update() slot of widgets
     * which need to repaint after a command
     */
    void commandExecuted();

    /**
     * This is emitted every time we reach the index where you
     * saved the document for the last time. See @ref documentSaved
     */
    void documentRestored();

private:
    //--------------- Data members ---------------------------------

    typedef std::set<KActionCollection *> ViewSet;
    ViewSet m_views;

    typedef std::stack<Command *> CommandStack;
    CommandStack m_undoStack;
    CommandStack m_redoStack;

    int m_undoLimit;
    int m_redoLimit;
    int m_savedAt;

    void updateButtons();
    void updateButton(const QString &text, const QString &name,
		      CommandStack &stack);
    void updateMenu(const QString &text, const QString &name,
		    CommandStack &stack);
    void clipCommands();

    void clipStack(CommandStack &stack, int limit);
    void clearStack(CommandStack &stack);
};


#endif
