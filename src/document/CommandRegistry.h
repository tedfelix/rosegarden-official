/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef RG_COMMANDREGISTRY_H
#define RG_COMMANDREGISTRY_H

#include <QObject>
#include <QString>
#include <QKeySequence>
#include <qnamespace.h>

#include "document/Command.h"

#include <map>
#include <vector>

#include <iostream>

#include "base/Exception.h"

namespace Rosegarden {

class EventSelection;
class CommandArgumentQuerier;

class CommandCancelled : public Exception
{
public:
    CommandCancelled() : Exception("") { }
};

class CommandFailed : public Exception
{
public:
    CommandFailed(std::string message) : Exception(message) { }
};

class AbstractCommandBuilder
{
public:
    virtual ~AbstractCommandBuilder();

    // may throw CommandCancelled
    virtual Command *build(QString actionName,
                           EventSelection &s,
                           CommandArgumentQuerier &querier) = 0;

    virtual EventSelection *getSubsequentSelection(Command *) { return 0; }
};

template <typename CommandType>
class SelectionCommandBuilder : public AbstractCommandBuilder
{
public:
    // may throw CommandCancelled
    virtual Command *build(QString /* actionName */,
                           EventSelection &s,
                           CommandArgumentQuerier &/* querier */) {
        return new CommandType(s);
    }

    virtual EventSelection *getSubsequentSelection(Command *c) {
        CommandType *command = dynamic_cast<CommandType *>(c);
        if (command) return command->getSubsequentSelection();
        return 0;
    }
};

template <typename CommandType>
class ArgumentAndSelectionCommandBuilder : public AbstractCommandBuilder
{
public:
    // may throw CommandCancelled
    virtual Command *build(QString actionName,
                           EventSelection &s,
                           CommandArgumentQuerier &querier) {
        return new CommandType(CommandType::getArgument(actionName, querier), s);
    }

    virtual EventSelection *getSubsequentSelection(Command *c) {
        CommandType *command = dynamic_cast<CommandType *>(c);
        if (command) return command->getSubsequentSelection();
        return 0;
    }
};


// ***************************************************************************


class CommandRegistry : public QObject
{
    Q_OBJECT

public:
    virtual ~CommandRegistry();

    void registerCommand(QString actionName,
                         AbstractCommandBuilder *builder) {
        addAction(actionName);
        m_builders[actionName] = builder;
    }

public slots:
    void slotInvokeCommand();

protected:
    CommandRegistry();

    typedef std::map<QString, AbstractCommandBuilder *> ActionBuilderMap;
    ActionBuilderMap m_builders;
	
    virtual void addAction(QString actionName) = 0;
    virtual void invokeCommand(QString actionName) = 0;

private:
    CommandRegistry(const CommandRegistry &);
    CommandRegistry &operator=(const CommandRegistry &);
    
};

class CommandArgumentQuerier
{
public:
    virtual QString getText(QString message, bool *ok) = 0;

protected:
    CommandArgumentQuerier() { }
    virtual ~CommandArgumentQuerier();
};

}

#endif


