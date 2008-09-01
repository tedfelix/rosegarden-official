/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MultiViewCommandHistory.h"

#include "Command.h"

#include <QRegExp>
#include <QMenu>
#include <QToolBar>
#include <QString>
#include <QTimer>
#include <QAction>

#include <iostream>

//#define DEBUG_COMMAND_HISTORY 1

namespace Rosegarden
{

MultiViewCommandHistory *MultiViewCommandHistory::m_instance = 0;

MultiViewCommandHistory::MultiViewCommandHistory() :
    m_undoLimit(50),
    m_redoLimit(50),
    m_menuLimit(15),
    m_savedAt(0),
    m_currentCompound(0),
    m_executeCompound(false),
    m_currentBundle(0),
    m_bundleTimer(0),
    m_bundleTimeout(5000)
{
    m_undoAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    m_undoAction->setShortcut(tr("Ctrl+Z"));
    m_undoAction->setStatusTip(tr("Undo the last editing operation"));
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    
    m_undoMenuAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    connect(m_undoMenuAction, SIGNAL(triggered()), this, SLOT(undo()));
    
    m_undoMenu = new QMenu(tr("&Undo"));
    m_undoMenuAction->setMenu(m_undoMenu);
    connect(m_undoMenu, SIGNAL(triggered(QAction *)),
	    this, SLOT(undoActivated(QAction*)));

    m_redoAction = new QAction(QIcon(":/icons/redo.png"), tr("Re&do"), this);
    m_redoAction->setShortcut(tr("Ctrl+Shift+Z"));
    m_redoAction->setStatusTip(tr("Redo the last operation that was undone"));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(redo()));
    
    m_redoMenuAction = new QAction(QIcon(":/icons/redo.png"), tr("Re&do"), this);
    connect(m_redoMenuAction, SIGNAL(triggered()), this, SLOT(redo()));

    m_redoMenu = new QMenu(tr("Re&do"));
    m_redoMenuAction->setMenu(m_redoMenu);
    connect(m_redoMenu, SIGNAL(triggered(QAction *)),
	    this, SLOT(redoActivated(QAction*)));
}

MultiViewCommandHistory::~MultiViewCommandHistory()
{
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);

    delete m_undoMenu;
    delete m_redoMenu;
}

MultiViewCommandHistory *
MultiViewCommandHistory::getInstance()
{
    if (!m_instance) m_instance = new MultiViewCommandHistory();
    return m_instance;
}

void
MultiViewCommandHistory::clear()
{
#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "MultiViewCommandHistory::clear()" << std::endl;
#endif
    closeBundle();
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);
    updateActions();
}

void
MultiViewCommandHistory::registerMenu(QMenu *menu)
{
    menu->addAction(m_undoAction);
    menu->addAction(m_redoAction);
}

void
MultiViewCommandHistory::registerToolbar(QToolBar *toolbar)
{
    toolbar->addAction(m_undoMenuAction);
    toolbar->addAction(m_redoMenuAction);
}

void
MultiViewCommandHistory::addCommand(Command *command)
{
    if (!command) return;

    if (m_currentCompound) {
	addToCompound(command, m_executeCompound);
	return;
    }

    addCommand(command, true);
}

void
MultiViewCommandHistory::addCommand(Command *command, bool execute, bool bundle)
{
    if (!command) return;

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "MultiViewCommandHistory::addCommand: " << command->getName().toLocal8Bit().data() << " at " << command << ": execute = " << execute << ", bundle = " << bundle << " (m_currentCompound = " << m_currentCompound << ", m_currentBundle = " << m_currentBundle << ")" << std::endl;
#endif

    if (m_currentCompound) {
	addToCompound(command, execute);
	return;
    }

    if (bundle) {
	addToBundle(command, execute);
	return;
    } else if (m_currentBundle) {
	closeBundle();
    }

#ifdef DEBUG_COMMAND_HISTORY
    if (!m_redoStack.empty()) {
        std::cerr << "MultiViewCommandHistory::clearing redo stack" << std::endl;
    }
#endif

    // We can't redo after adding a command
    clearStack(m_redoStack);

    // can we reach savedAt?
    if ((int)m_undoStack.size() < m_savedAt) m_savedAt = -1; // nope

    m_undoStack.push(command);
    clipCommands();
    
    if (execute) {
	command->execute();
    }

    // Emit even if we aren't executing the command, because
    // someone must have executed it for this to make any sense
    emit commandExecuted();
    emit commandExecuted(command);

    updateActions();
}

void
MultiViewCommandHistory::addToBundle(Command *command, bool execute)
{
    if (m_currentBundle) {
	if (!command || (command->getName() != m_currentBundleName)) {
#ifdef DEBUG_COMMAND_HISTORY
            std::cerr << "MultiViewCommandHistory::addToBundle: "
                      << command->getName().toStdString()
                      << ": closing current bundle" << std::endl;
#endif
	    closeBundle();
	}
    }

    if (!command) return;

    if (!m_currentBundle) {

#ifdef DEBUG_COMMAND_HISTORY
        std::cerr << "MultiViewCommandHistory::addToBundle: "
                  << command->getName().toStdString()
                  << ": creating new bundle" << std::endl;
#endif

	// need to addCommand before setting m_currentBundle, as addCommand
	// with bundle false will reset m_currentBundle to 0
	MacroCommand *mc = new BundleCommand(command->getName());
	addCommand(mc, false);
	m_currentBundle = mc;
	m_currentBundleName = command->getName();
    }

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "MultiViewCommandHistory::addToBundle: "
              << command->getName().toStdString()
              << ": adding to bundle" << std::endl;
#endif

    if (execute) command->execute();
    m_currentBundle->addCommand(command);

    // Emit even if we aren't executing the command, because
    // someone must have executed it for this to make any sense
    emit commandExecuted();
    emit commandExecuted(command);

    updateActions();

    delete m_bundleTimer;
    m_bundleTimer = new QTimer(this);
    connect(m_bundleTimer, SIGNAL(timeout()), this, SLOT(bundleTimerTimeout()));
    m_bundleTimer->start(m_bundleTimeout);
}

void
MultiViewCommandHistory::closeBundle()
{
#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "MultiViewCommandHistory::closeBundle" << std::endl;
#endif

    m_currentBundle = 0;
    m_currentBundleName = "";
}

void
MultiViewCommandHistory::bundleTimerTimeout()
{
#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "MultiViewCommandHistory::bundleTimerTimeout: bundle is " << m_currentBundle << std::endl;
#endif

    closeBundle();
}

void
MultiViewCommandHistory::addToCompound(Command *command, bool execute)
{
#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "MultiViewCommandHistory::addToCompound: " << command->getName().toLocal8Bit().data() << std::endl;
#endif
    if (!m_currentCompound) {
	std::cerr << "MultiViewCommandHistory::addToCompound: ERROR: no compound operation in progress!" << std::endl;
        return;
    }

    if (execute) command->execute();
    m_currentCompound->addCommand(command);
}

void
MultiViewCommandHistory::startCompoundOperation(QString name, bool execute)
{
    if (m_currentCompound) {
	std::cerr << "MultiViewCommandHistory::startCompoundOperation: ERROR: compound operation already in progress!" << std::endl;
	std::cerr << "(name is " << m_currentCompound->getName().toLocal8Bit().data() << ")" << std::endl;
        return;
    }
 
    closeBundle();
   
    m_currentCompound = new MacroCommand(name);
    m_executeCompound = execute;
}

void
MultiViewCommandHistory::endCompoundOperation()
{
    if (!m_currentCompound) {
	std::cerr << "MultiViewCommandHistory::endCompoundOperation: ERROR: no compound operation in progress!" << std::endl;
        return;
    }

    MacroCommand *toAdd = m_currentCompound;
    m_currentCompound = 0;

    if (toAdd->haveCommands()) {

        // We don't execute the macro command here, because we have
        // been executing the individual commands as we went along if
        // m_executeCompound was true.
        addCommand(toAdd, false);
    }
}    

void
MultiViewCommandHistory::addExecutedCommand(Command *command)
{
    addCommand(command, false);
}

void
MultiViewCommandHistory::addCommandAndExecute(Command *command)
{
    addCommand(command, true);
}

void
MultiViewCommandHistory::undo()
{
    if (m_undoStack.empty()) return;

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "MultiViewCommandHistory::undo()" << std::endl;
#endif

    closeBundle();

    Command *command = m_undoStack.top();
    command->unexecute();
    emit commandExecuted();
    emit commandUnexecuted(command);

    m_redoStack.push(command);
    m_undoStack.pop();

    clipCommands();
    updateActions();

    if ((int)m_undoStack.size() == m_savedAt) emit documentRestored();
}

void
MultiViewCommandHistory::redo()
{
    if (m_redoStack.empty()) return;

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "MultiViewCommandHistory::redo()" << std::endl;
#endif

    closeBundle();

    Command *command = m_redoStack.top();
    command->execute();
    emit commandExecuted();
    emit commandExecuted(command);

    m_undoStack.push(command);
    m_redoStack.pop();
    // no need to clip

    updateActions();

    if ((int)m_undoStack.size() == m_savedAt) emit documentRestored();
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
MultiViewCommandHistory::setMenuLimit(int limit)
{
    m_menuLimit = limit;
    updateActions();
}

void
MultiViewCommandHistory::setBundleTimeout(int ms)
{
    m_bundleTimeout = ms;
}

void
MultiViewCommandHistory::documentSaved()
{
    closeBundle();
    m_savedAt = m_undoStack.size();
}

void
MultiViewCommandHistory::clipCommands()
{
    if ((int)m_undoStack.size() > m_undoLimit) {
	m_savedAt -= (m_undoStack.size() - m_undoLimit);
    }

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
#ifdef DEBUG_COMMAND_HISTORY
	    Command *command = stack.top();
	    std::cerr << "MultiViewCommandHistory::clipStack: Saving recent command: " << command->getName().toLocal8Bit().data() << " at " << command << std::endl;
#endif
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
	Command *command = stack.top();
	// Not safe to call getName() on a command about to be deleted
#ifdef DEBUG_COMMAND_HISTORY
	std::cerr << "MultiViewCommandHistory::clearStack: About to delete command " << command << std::endl;
#endif
	delete command;
	stack.pop();
    }
}

void
MultiViewCommandHistory::undoActivated(QAction *action)
{
    int pos = m_actionCounts[action];
    for (int i = 0; i <= pos; ++i) {
	undo();
    }
}

void
MultiViewCommandHistory::redoActivated(QAction *action)
{
    int pos = m_actionCounts[action];
    for (int i = 0; i <= pos; ++i) {
	redo();
    }
}

void
MultiViewCommandHistory::updateActions()
{
    m_actionCounts.clear();

    for (int undo = 0; undo <= 1; ++undo) {

	QAction *action(undo ? m_undoAction : m_redoAction);
	QAction *menuAction(undo ? m_undoMenuAction : m_redoMenuAction);
	QMenu *menu(undo ? m_undoMenu : m_redoMenu);
	CommandStack &stack(undo ? m_undoStack : m_redoStack);

	if (stack.empty()) {

	    QString text(undo ? tr("Nothing to undo") : tr("Nothing to redo"));

	    action->setEnabled(false);
	    action->setText(text);

	    menuAction->setEnabled(false);
	    menuAction->setText(text);

	} else {

	    action->setEnabled(true);
	    menuAction->setEnabled(true);

	    QString commandName = stack.top()->getName();
	    commandName.replace(QRegExp("&"), "");

	    QString text = (undo ? tr("&Undo %1") : tr("Re&do %1"))
		.arg(commandName);

	    action->setText(text);
	    menuAction->setText(text);
	}

	menu->clear();

	CommandStack tempStack;
	int j = 0;

	while (j < m_menuLimit && !stack.empty()) {

	    Command *command = stack.top();
	    tempStack.push(command);
	    stack.pop();

	    QString commandName = command->getName();
	    commandName.replace(QRegExp("&"), "");

	    QString text;
	    if (undo) text = tr("&Undo %1").arg(commandName);
	    else      text = tr("Re&do %1").arg(commandName);
	    
	    QAction *action = menu->addAction(text);
	    m_actionCounts[action] = j++;
	}

	while (!tempStack.empty()) {
	    stack.push(tempStack.top());
	    tempStack.pop();
	}
    }
}


}

