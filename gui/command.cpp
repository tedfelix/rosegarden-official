
// -*- c-basic-offset: 4 -*-

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

#include "command.h"

CompoundCommand::~CompoundCommand()
{
    for (int i = 0; i < getCommandCount(); ++i) {
	delete getCommand(i);
    }
}

int
CompoundCommand::getCommandCount() const
{
    return m_commands.size();
}

KCommand *
CompoundCommand::getCommand(int n)
{
    // Strange one, this.  If views request the commands that
    // comprise a compound command that's just been executed so
    // they can refresh correctly, then they want the commands
    // in the order in which they were executed -- which means
    // "forward" for execute and "backward" for unexecute.
    // Hence this.

    if (m_lastWasUnexecute) {
	return m_commands[getCommandCount() - n - 1];
    } else {
	return m_commands[n];
    }
}

void
CompoundCommand::execute()
{
    m_lastWasUnexecute = false;
    for (int i = 0; i < getCommandCount(); ++i) {
	getCommand(i)->execute();
    }
}

void
CompoundCommand::unexecute()
{
    m_lastWasUnexecute = true;

    // m_lastWasUnexecute causes this to operate in reverse order,
    // if you see what I mean:

    for (int i = 0; i < getCommandCount(); ++i) {
	getCommand(i)->unexecute();
    }
}

void
CompoundCommand::addCommand(KCommand *command)
{
    m_commands.push_back(command);
}
