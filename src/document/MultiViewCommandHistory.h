
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MULTIVIEWCOMMANDHISTORY_H_
#define _RG_MULTIVIEWCOMMANDHISTORY_H_

#include <set>
#include <stack>
#include <qobject.h>


class QString;
class KCommand;
class KActionCollection;


namespace Rosegarden
{



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

    void addCommand(KCommand *command, bool execute = true);
    
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
    void commandExecuted(KCommand *);

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

    typedef std::stack<KCommand *> CommandStack;
    CommandStack m_undoStack;
    CommandStack m_redoStack;

    int m_undoLimit;
    int m_redoLimit;
    int m_savedAt;

    void updateButtons();
    void updateButton(bool undo, const QString &name, CommandStack &stack);
    void updateMenu(bool undo, const QString &name, CommandStack &stack);
    void clipCommands();

    void clipStack(CommandStack &stack, int limit);
    void clearStack(CommandStack &stack);
};



}

#endif
