/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "multiviewcommandhistory.h"

#include "rosedebug.h"
#include <iostream>

#include <kaction.h>
#include <kstdaction.h>
#include <kcommand.h>
#include <klocale.h>
#include <kpopupmenu.h>

using std::cerr;
using std::endl;


MultiViewCommandHistory::MultiViewCommandHistory() :
    m_undoLimit(50),
    m_redoLimit(50)
{
    // nothing
}

MultiViewCommandHistory::~MultiViewCommandHistory()
{
    clearStack(m_undoStack);
    clearStack(m_redoStack);
}

void
MultiViewCommandHistory::clear()
{
    clearStack(m_undoStack);
    clearStack(m_redoStack);
}

void
MultiViewCommandHistory::attachView(KActionCollection *collection)
{
    if (m_views.find(collection) != m_views.end()) return;

    KToolBarPopupAction *undo = new KToolBarPopupAction
	(i18n("Und&o"),
	 "undo",
	 KStdAccel::key(KStdAccel::Undo),
	 this,
	 SLOT(undo()),
	 collection,
	 KStdAction::stdName(KStdAction::Undo));

    connect
	(undo->popupMenu(),
	 SIGNAL(aboutToShow()),
	 this,
	 SLOT(slotUndoAboutToShow()));

    connect
	(undo->popupMenu(),
	 SIGNAL(activated(int)),
	 this,
	 SLOT(slotUndoActivated(int)));

    KToolBarPopupAction *redo = new KToolBarPopupAction
	(i18n("Re&do"),
	 "redo",
	 KStdAccel::key(KStdAccel::Redo),
	 this,
	 SLOT(redo()),
	 collection,
	 KStdAction::stdName(KStdAction::Redo));
    
    connect
	(redo->popupMenu(),
	 SIGNAL(aboutToShow()),
	 this,
	 SLOT(slotRedoAboutToShow()));

    connect
	(redo->popupMenu(),
	 SIGNAL(activated(int)),
	 this,
	 SLOT(slotRedoActivated(int)));

    m_views.insert(collection);
    updateButtons();
}

void
MultiViewCommandHistory::detachView(KActionCollection *collection)
{
    ViewSet::iterator i = m_views.find(collection);
    if (i != m_views.end()) m_views.erase(collection);
}

void
MultiViewCommandHistory::addCommand(KCommand *command, bool execute)
{
    if (!command) return;

    // We can't redo after adding a command
    clearStack(m_redoStack);

    m_undoStack.push(command);
    clipCommands();
    
    if (execute) {
	command->execute();
	emit commandExecuted();
    }

    updateButtons();
}

void
MultiViewCommandHistory::undo()
{
    if (m_undoStack.empty()) return;

    m_undoStack.top()->unexecute();
    emit commandExecuted();

    m_redoStack.push(m_undoStack.top());
    m_undoStack.pop();
    clipCommands();
    updateButtons();
}

void
MultiViewCommandHistory::redo()
{
    if (m_redoStack.empty()) return;

    m_redoStack.top()->execute();
    emit commandExecuted();

    m_undoStack.push(m_redoStack.top());
    m_redoStack.pop();
    // no need to clip
    updateButtons();
}

void
MultiViewCommandHistory::setUndoLimit(int limit)
{
    if (limit > 0 && limit != m_undoLimit) {
        m_undoLimit = limit;
        clipCommands();
    }
}

void
MultiViewCommandHistory::setRedoLimit(int limit)
{
    if (limit > 0 && limit != m_redoLimit) {
        m_redoLimit = limit;
        clipCommands();
    }
}

void
MultiViewCommandHistory::documentSaved()
{
    //!!! not yet implemented
}

void
MultiViewCommandHistory::clipCommands()
{
    clipStack(m_undoStack, m_undoLimit);
    clipStack(m_redoStack, m_redoLimit);
}

void
MultiViewCommandHistory::clipStack(CommandStack &stack, int limit)
{
    int i;

    if ((int)stack.size() > limit) {

	CommandStack tempStack;
	for (i = 0; i < limit; ++i) {
	    tempStack.push(stack.top());
	    stack.pop();
	}
	clearStack(stack);
	for (i = 0; i < m_undoLimit; ++i) {
	    stack.push(tempStack.top());
	    tempStack.pop();
	}
    }
}

void
MultiViewCommandHistory::clearStack(CommandStack &stack)
{
    while (!stack.empty()) {
	delete stack.top();
	stack.pop();
    }
}

void
MultiViewCommandHistory::slotUndoActivated(int pos)
{
    kdDebug(KDEBUG_AREA) << "KCommandHistory::slotUndoActivated " << pos << endl;
    for (int i = 0 ; i <= pos; ++i) undo();
}

void
MultiViewCommandHistory::slotRedoActivated(int pos)
{
    kdDebug(KDEBUG_AREA) << "KCommandHistory::slotRedoActivated " << pos << endl;
    for (int i = 0 ; i <= pos; ++i) redo();
}

void
MultiViewCommandHistory::slotUndoAboutToShow()
{
    kdDebug(KDEBUG_AREA) << "MultiViewCommandHistory::slotUndoAboutToShow" << endl;
    updateMenu("Undo", KStdAction::stdName(KStdAction::Undo), m_undoStack);
}

void
MultiViewCommandHistory::slotRedoAboutToShow()
{
    kdDebug(KDEBUG_AREA) << "MultiViewCommandHistory::slotRedoAboutToShow" << endl;
    updateMenu("Redo", KStdAction::stdName(KStdAction::Redo), m_redoStack);
}

void
MultiViewCommandHistory::updateButtons()
{
    updateButton("Undo", KStdAction::stdName(KStdAction::Undo), m_undoStack);
    updateButton("Redo", KStdAction::stdName(KStdAction::Redo), m_redoStack);
}


void
MultiViewCommandHistory::updateButton(const QString &text,
				      const QString &name,
				      CommandStack &stack)
{
    for (ViewSet::iterator i = m_views.begin(); i != m_views.end(); ++i) {

	kdDebug(KDEBUG_AREA) << "name is \"" << name << "\"" << endl;

	KAction *action = (*i)->action(name);

	kdDebug(KDEBUG_AREA) << "action is " << action << endl;

	if (!action) continue;

	if (stack.empty()) {
	    action->setEnabled(false);
	    action->setText(i18n("Nothing to " + text));
	} else {
	    action->setEnabled(true);
	    action->setText(i18n(text + " " + stack.top()->name()));
	}
    }
}


void
MultiViewCommandHistory::updateMenu(const QString &text,
				    const QString &name,
				    CommandStack &stack)
{
    for (ViewSet::iterator i = m_views.begin(); i != m_views.end(); ++i) {

	kdDebug(KDEBUG_AREA) << "name is \"" << name << "\"" << endl;

	KAction *action = (*i)->action(name);

	kdDebug(KDEBUG_AREA) << "action is " << action << endl;

	if (!action) continue;

	KToolBarPopupAction *popupAction =
	    dynamic_cast<KToolBarPopupAction *>(action);
	if (!popupAction) continue;

	QPopupMenu *menu = popupAction->popupMenu();
	if (!menu) continue;
	menu->clear();

	CommandStack tempStack;

	while (!stack.empty()) {

	    KCommand *command = stack.top();
	    tempStack.push(command);
	    stack.pop();

	    menu->insertItem(i18n(text + " " + command->name()));
	}

	while (!tempStack.empty()) {
	    stack.push(tempStack.top());
	    tempStack.pop();
	}
    }
}

