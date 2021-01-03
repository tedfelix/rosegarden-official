/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
    Rosegarden
*/

#ifndef RG_COMMAND_H
#define RG_COMMAND_H

#include <QString>
#include <QCoreApplication> // for Q_DECLARE_TR_FUNCTIONS

#include <vector>
#include <rosegardenprivate_export.h>

namespace Rosegarden
{

class ROSEGARDENPRIVATE_EXPORT Command
{
public:
    Command() : m_updateLinks(true) { }
    virtual ~Command() { }

    virtual void execute() = 0;
    virtual void unexecute() = 0;
    virtual QString getName() const = 0;
    
    bool getUpdateLinks() const { return m_updateLinks; }
    void setUpdateLinks(bool update) { m_updateLinks = update; }

private:
    bool m_updateLinks;
};

class ROSEGARDENPRIVATE_EXPORT NamedCommand : public Command
{
public:
    NamedCommand(QString name) : m_name(name) { }
    ~NamedCommand() override { }

    QString getName() const override { return m_name; }
    virtual void setName(QString name) { m_name = name; }

protected:
    QString m_name;
};

class ROSEGARDENPRIVATE_EXPORT MacroCommand : public Command
{
public:
    MacroCommand(QString name);
    ~MacroCommand() override;

    virtual void addCommand(Command *command);
    virtual void deleteCommand(Command *command);
    virtual bool haveCommands() const;

    void execute() override;
    void unexecute() override;

    QString getName() const override;
    virtual void setName(QString name);
    
    virtual const std::vector<Command *>& getCommands() { return m_commands; }

protected:
    QString m_name;
    std::vector<Command *> m_commands;
};

/**
 * BundleCommand is a MacroCommand whose name includes a note of how
 * many commands it contains
 */
class ROSEGARDENPRIVATE_EXPORT BundleCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(BundleCommand)
public:
    BundleCommand(QString name);
    ~BundleCommand() override;

    QString getName() const override;
};

}

#endif
