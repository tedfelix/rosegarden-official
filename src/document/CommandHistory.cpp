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


#include "CommandHistory.h"

#include "Command.h"
#include "gui/general/ActionData.h"

#include <QRegularExpression>
#include <QMenu>
#include <QToolBar>
#include <QString>
#include <QTimer>
#include <QAction>

#include <iostream>

//#define DEBUG_COMMAND_HISTORY 1

namespace Rosegarden
{

CommandHistory *CommandHistory::m_instance = nullptr;

CommandHistory::CommandHistory() :
    m_undoLimit(50),
    m_redoLimit(50),
    m_menuLimit(15),
    m_savedAt(0),
    m_enableUndo(true)
{
    // All Edit > Undo menu items share this QAction object.
    m_undoAction = new QAction(QIcon(":/icons/undo.png"), tr("&Undo"), this);
    m_undoAction->setObjectName("edit_undo");
    m_undoAction->setStatusTip(tr("Undo the last editing operation"));
    connect(m_undoAction, &QAction::triggered, this, &CommandHistory::undo);

    m_undoMenu = new QMenu(tr("&Undo"));
    m_undoAction->setMenu(m_undoMenu);
    connect(m_undoMenu, &QMenu::triggered,
            this, &CommandHistory::undoActivated);

    // All Edit > Redo menu items share this QAction object.
    m_redoAction = new QAction(QIcon(":/icons/redo.png"), tr("Re&do"), this);
    m_redoAction->setObjectName("edit_redo");
    m_redoAction->setStatusTip(tr("Redo the last operation that was undone"));
    connect(m_redoAction, &QAction::triggered, this, &CommandHistory::redo);

    m_redoMenu = new QMenu(tr("Re&do"));
    m_redoAction->setMenu(m_redoMenu);
    connect(m_redoMenu, &QMenu::triggered,
            this, &CommandHistory::redoActivated);
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
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);
    updateActions();
}

void
CommandHistory::addCommand(Command *command)
{
    if (!command) return;

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::addCommand: " << command->getName().toLocal8Bit().data() << " at " << command << std::endl;
#endif

#ifdef DEBUG_COMMAND_HISTORY
    if (!m_redoStack.empty()) {
        std::cerr << "CommandHistory::clearing redo stack" << std::endl;
    }
#endif

    // We can't redo after adding a command
    clearStack(m_redoStack);

    // can we reach savedAt?
    if ((int)m_undoStack.size() < m_savedAt) m_savedAt = -1; // nope

    emit aboutToExecuteCommand();

    CommandInfo commInfo;
    commInfo.command = command;
    commInfo.pointerPositionBefore = m_pointerPosition;
    m_undoStack.push(commInfo);
    clipCommands();

    // Execute the command
    command->execute();

    emit updateLinkedSegments(command);
    emit commandExecuted();
    //emit commandExecuted2(command);
    emit commandExecutedInitially();

    updateActions();
    // pointerPositionAfter can not be updated here as the cursor
    // position is changed after the addCommand call
}

void
CommandHistory::undo()
{
    if (m_undoStack.empty()) return;

#ifdef DEBUG_COMMAND_HISTORY
    std::cerr << "CommandHistory::undo()" << std::endl;
#endif

    CommandInfo commInfo = m_undoStack.top();
    commInfo.command->unexecute();
    emit updateLinkedSegments(commInfo.command);
    emit commandExecuted();
    emit commandUnexecuted(commInfo.command);
    m_pointerPosition = commInfo.pointerPositionBefore;
    emit commandUndone();

    m_redoStack.push(commInfo);
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

    CommandInfo commInfo = m_redoStack.top();
    commInfo.command->execute();
    emit updateLinkedSegments(commInfo.command);
    emit commandExecuted();
    //emit commandExecuted2(commInfo.command);
    m_pointerPosition = commInfo.pointerPositionAfter;
    emit commandRedone();

    m_undoStack.push(commInfo);
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
CommandHistory::documentSaved()
{
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
            CommandInfo commInfo = stack.top();
            std::cerr << "CommandHistory::clipStack: Saving recent command: " << commInfo.command->getName().toLocal8Bit().data() << " at " << command << std::endl;
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
        CommandInfo commInfo = stack.top();
        // Not safe to call getName() on a command about to be deleted
#ifdef DEBUG_COMMAND_HISTORY
        std::cerr << "CommandHistory::clearStack: About to delete command " << command << std::endl;
#endif
        delete commInfo.command;
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

    // for undo then redo
    for (int undo = 0; undo <= 1; ++undo) {

        QAction *action(undo ? m_undoAction : m_redoAction);
        QMenu *menu(undo ? m_undoMenu : m_redoMenu);
        CommandStack &stack(undo ? m_undoStack : m_redoStack);
        QString actionName(undo ? "edit_undo" : "edit_redo");

        if (stack.empty()) {

            QString text(undo ? tr("Nothing to undo") : tr("Nothing to redo"));

            action->setEnabled(false);
            action->setText(text);
            action->setToolTip(strippedText(text));
        } else {

            QString commandName = stack.top().command->getName();
            commandName.replace(QRegularExpression("&"), "");

            QString text = (undo ? tr("&Undo %1") : tr("Re&do %1"))
                .arg(commandName);
            ActionData* adata = ActionData::getInstance();
            QString key = "rosegardenmainwindow.rc:";
            key += actionName;
            std::set<QKeySequence> ksSet = adata->getShortcuts(key);
            QStringList kssl;
            foreach(auto ks, ksSet) {
                kssl.append(ks.toString(QKeySequence::NativeText));
            }
            QString scString = kssl.join(", ");

            if (kssl.size() > 0) {
                // add the shortcuts to the tooltip
                text = text + " (" + scString + ")";
            }

            action->setEnabled(m_enableUndo);
            action->setText(text);
            action->setToolTip(strippedText(text));
        }

        menu->clear();

        CommandStack tempStack;
        int j = 0;

        while (j < m_menuLimit && !stack.empty()) {

            CommandInfo commInfo = stack.top();
            tempStack.push(commInfo);
            stack.pop();

            QString commandName = commInfo.command->getName();
            commandName.replace(QRegularExpression("&"), "");

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

void
CommandHistory::enableUndo(bool enable)
{
    m_enableUndo = enable;
    updateActions();
}

void
CommandHistory::setPointerPosition(timeT pos)
{
    m_pointerPosition = pos;
}

void
CommandHistory::setPointerPositionForRedo(timeT pos)
{
    // This call happens after a command has been initially
    // executed. That means the relevant command is on the top of the
    // undo stack. The pointerPositionAfter value must be set for this
    // command.

    if ((int)m_undoStack.size() == 0) return;
    CommandInfo& top = m_undoStack.top();
    top.pointerPositionAfter = pos;
}

timeT
CommandHistory::getPointerPosition() const
{
    return m_pointerPosition;
}

}
