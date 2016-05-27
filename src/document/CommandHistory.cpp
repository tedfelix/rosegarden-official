/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CommandHistory.h"

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

CommandHistory *CommandHistory::m_instance = 0;

CommandHistory::CommandHistory() :
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
    // All Edit > Undo menu items share this QAction object.
    m_undoAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    m_undoAction->setObjectName("edit_undo");
    m_undoAction->setShortcut(tr("Ctrl+Z"));
    m_undoAction->setStatusTip(tr("Undo the last editing operation"));
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(undo()));

    // Undo button for the main window toolbar.
    m_undoMenuAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    m_undoMenuAction->setObjectName("edit_toolbar_undo");
    connect(m_undoMenuAction, SIGNAL(triggered()), this, SLOT(undo()));
    
    m_undoMenu = new QMenu(tr("&Undo"));
    m_undoMenuAction->setMenu(m_undoMenu);
    connect(m_undoMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(undoActivated(QAction*)));

    // All Edit > Redo menu items share this QAction object.
    m_redoAction = new QAction(QIcon(":/icons/redo.png"), tr("Re&do"), this);
    m_redoAction->setObjectName("edit_redo");
    m_redoAction->setShortcut(tr("Ctrl+Shift+Z"));
    m_redoAction->setStatusTip(tr("Redo the last operation that was undone"));
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(redo()));
    
    // Redo button for the main window toolbar.
    m_redoMenuAction = new QAction(QIcon(":/icons/redo.png"), tr("Re&do"), this);
    m_redoMenuAction->setObjectName("edit_toolbar_redo");
    connect(m_redoMenuAction, SIGNAL(triggered()), this, SLOT(redo()));

    m_redoMenu = new QMenu(tr("Re&do"));
    m_redoMenuAction->setMenu(m_redoMenu);
    connect(m_redoMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(redoActivated(QAction*)));
}

CommandHistory::~CommandHistory()
{
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);

    delete m_undoMenu;
    delete m_redoMenu;
}

CommandHistory *
CommandHistory::getInstance()
{
    if (!m_instance) m_instance = new CommandHistory();
    return m_instance;
}

void
CommandHistory::clear()
{
#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::clear()" << std::endl;
#endif
    closeBundle();
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);
    updateActions();
}

#if 0
void
CommandHistory::registerMenu(QMenu *menu)
{
    menu->addAction(m_undoAction);
    menu->addAction(m_redoAction);
}

void
CommandHistory::registerToolbar(QToolBar *toolbar)
{
    toolbar->addAction(m_undoMenuAction);
    toolbar->addAction(m_redoMenuAction);
}
#endif

void
CommandHistory::addCommand(Command *command)
{
    if (!command) return;

    if (m_currentCompound) {
        addToCompound(command, m_executeCompound);
        return;
    }

    addCommand(command, true);
}

void
CommandHistory::addCommand(Command *command, bool execute, bool bundle)
{
    if (!command) return;

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::addCommand: " << command->getName().toLocal8Bit().data() << " at " << command << ": execute = " << execute << ", bundle = " << bundle << " (m_currentCompound = " << m_currentCompound << ", m_currentBundle = " << m_currentBundle << ")" << std::endl;
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
        std::cerr << "CommandHistory::clearing redo stack" << std::endl;
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
    emit updateLinkedSegments(command);
    emit commandExecuted();
    emit commandExecuted(command);

    updateActions();
}

void
CommandHistory::addToBundle(Command *command, bool execute)
{
    if (m_currentBundle) {
        if (!command || (command->getName() != m_currentBundleName)) {
#ifdef DEBUG_COMMAND_HISTORY
            std::cerr << "CommandHistory::addToBundle: "
                      << command->getName().toStdString()
                      << ": closing current bundle" << std::endl;
#endif
            closeBundle();
        }
    }

    if (!command) return;

    if (!m_currentBundle) {

#ifdef DEBUG_COMMAND_HISTORY
        std::cerr << "CommandHistory::addToBundle: "
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
    std::cerr << "CommandHistory::addToBundle: "
              << command->getName().toStdString()
              << ": adding to bundle" << std::endl;
#endif

    if (execute) command->execute();
    m_currentBundle->addCommand(command);

    // Emit even if we aren't executing the command, because
    // someone must have executed it for this to make any sense
    emit updateLinkedSegments(command);
    emit commandExecuted();
    emit commandExecuted(command);

    updateActions();

    delete m_bundleTimer;
    m_bundleTimer = new QTimer(this);
    connect(m_bundleTimer, SIGNAL(timeout()), this, SLOT(bundleTimerTimeout()));
    m_bundleTimer->start(m_bundleTimeout);
}

void
CommandHistory::closeBundle()
{
#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::closeBundle" << std::endl;
#endif

    m_currentBundle = 0;
    m_currentBundleName = "";
}

void
CommandHistory::bundleTimerTimeout()
{
#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::bundleTimerTimeout: bundle is " << m_currentBundle << std::endl;
#endif

    closeBundle();
}

void
CommandHistory::addToCompound(Command *command, bool execute)
{
#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::addToCompound: " << command->getName().toLocal8Bit().data() << std::endl;
#endif
    if (!m_currentCompound) {
        std::cerr << "CommandHistory::addToCompound: ERROR: no compound operation in value()!" << std::endl;
        return;
    }

    if (execute) command->execute();
    m_currentCompound->addCommand(command);
}

void
CommandHistory::startCompoundOperation(QString name, bool execute)
{
    if (m_currentCompound) {
        std::cerr << "CommandHistory::startCompoundOperation: ERROR: compound operation already in value()!" << std::endl;
        std::cerr << "(name is " << m_currentCompound->getName().toLocal8Bit().data() << ")" << std::endl;
        return;
    }
 
    closeBundle();
   
    m_currentCompound = new MacroCommand(name);
    m_executeCompound = execute;
}

void
CommandHistory::endCompoundOperation()
{
    if (!m_currentCompound) {
        std::cerr << "CommandHistory::endCompoundOperation: ERROR: no compound operation in value()!" << std::endl;
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
CommandHistory::addExecutedCommand(Command *command)
{
    addCommand(command, false);
}

void
CommandHistory::addCommandAndExecute(Command *command)
{
    addCommand(command, true);
}

void
CommandHistory::undo()
{
    if (m_undoStack.empty()) return;

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::undo()" << std::endl;
#endif

    closeBundle();

    Command *command = m_undoStack.top();
    command->unexecute();
    emit updateLinkedSegments(command);
    emit commandExecuted();
    emit commandUnexecuted(command);

    m_redoStack.push(command);
    m_undoStack.pop();

    clipCommands();
    updateActions();

    if ((int)m_undoStack.size() == m_savedAt) emit documentRestored();
}

void
CommandHistory::redo()
{
    if (m_redoStack.empty()) return;

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::redo()" << std::endl;
#endif

    closeBundle();

    Command *command = m_redoStack.top();
    command->execute();
    emit updateLinkedSegments(command);
    emit commandExecuted();
    emit commandExecuted(command);

    m_undoStack.push(command);
    m_redoStack.pop();
    // no need to clip

    updateActions();

    if ((int)m_undoStack.size() == m_savedAt) emit documentRestored();
}

void
CommandHistory::setUndoLimit(int limit)
{
    if (limit > 0 && limit != m_undoLimit) {
        m_undoLimit = limit;
        clipCommands();
    }
}

void
CommandHistory::setRedoLimit(int limit)
{
    if (limit > 0 && limit != m_redoLimit) {
        m_redoLimit = limit;
        clipCommands();
    }
}

void
CommandHistory::setMenuLimit(int limit)
{
    m_menuLimit = limit;
    updateActions();
}

void
CommandHistory::setBundleTimeout(int ms)
{
    m_bundleTimeout = ms;
}

void
CommandHistory::documentSaved()
{
    closeBundle();
    m_savedAt = (int)m_undoStack.size();
}

void
CommandHistory::clipCommands()
{
    if ((int)m_undoStack.size() > m_undoLimit) {
        m_savedAt -= (int(m_undoStack.size()) - m_undoLimit);
    }

    clipStack(m_undoStack, m_undoLimit);
    clipStack(m_redoStack, m_redoLimit);
}

void
CommandHistory::clipStack(CommandStack &stack, int limit)
{
    int i;

    if ((int)stack.size() > limit) {

        CommandStack tempStack;

        for (i = 0; i < limit; ++i) {
#ifdef DEBUG_COMMAND_HISTORY
            Command *command = stack.top();
            std::cerr << "CommandHistory::clipStack: Saving recent command: " << command->getName().toLocal8Bit().data() << " at " << command << std::endl;
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
CommandHistory::clearStack(CommandStack &stack)
{
    while (!stack.empty()) {
        Command *command = stack.top();
        // Not safe to call getName() on a command about to be deleted
#ifdef DEBUG_COMMAND_HISTORY
        std::cerr << "CommandHistory::clearStack: About to delete command " << command << std::endl;
#endif
        delete command;
        stack.pop();
    }
}

void
CommandHistory::undoActivated(QAction *action)
{
    int pos = m_actionCounts[action];
    for (int i = 0; i <= pos; ++i) {
        undo();
    }
}

void
CommandHistory::redoActivated(QAction *action)
{
    int pos = m_actionCounts[action];
    for (int i = 0; i <= pos; ++i) {
        redo();
    }
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

void
CommandHistory::updateActions()
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
            action->setToolTip(strippedText(text));

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
            action->setToolTip(strippedText(text));
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
