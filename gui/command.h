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

#ifndef _COMMAND_H_
#define _COMMAND_H_


#include <set>
#include <qstring.h>

#include "Segment.h"

/**
 * A set of simple abstract base-classes for commands.
 *
 * Although you can execute any Command in Rosegarden's command
 * history, the various GUI views will only be able to refresh
 * correctly if all commands are subclassed from a more specific 
 * base.  (Please don't attempt to do the refresh yourself -- it's
 * much easier to support multiple views using this technique!)
 */


/**
 * Base class for commands.  
 *
 * We don't use KCommand because the KDE-2.2 and KDE-3.0 KCommand
 * interfaces are slightly different and it doesn't provide enough
 * to us to be worth the bother.  The CompoundCommand and
 * MultiViewCommandHistory classes are more interesting, and the
 * KDE versions of those are largely useless for our purposes.
 */

class Command
{
protected:
    /**
     * Creates a command
     * @param name the name of this command, translated, since it will appear
     * in the menus.
     */
    Command(const QString &name) : m_name(name) {}

public:
    virtual ~Command() {}

    /**
     * The main method: execute this command.
     * Implement here what this command is about, and remember to
     * record any information that will be helpful for @ref unexecute.
     */
    virtual void execute() = 0;

    /**
     * Unexecute (undo) this command.
     * Implement here the steps to take for undoing the command.
     * If you implement unexecute correctly, the application is in the same
     * state after unexecute as it was before execute. This means the next
     * call to execute will do the same thing as it did the first time.
     */
    virtual void unexecute() = 0;

    /**
     * @return the name of this command
     * Doesn't have to be internationalised; the command history object
     * should take care of that when it creates the associated menus and
     * buttons.
     */
    QString name() const { return m_name; }

    /**
     * Update the name of this command.
     * Rarely necessary.
     */
    void setName(const QString &name) { m_name = name; }

private:
    QString m_name;
};
 


/**
 * SegmentCommand is a base for commands that affect the "envelope"
 * of one or more entire segments, and that may or may not also
 * affect their contents -- for example, resizing, splitting, or
 * quantizing segments.
 */

class SegmentCommand : public Command
{
public:
    typedef std::set<Rosegarden::Segment *> SegmentSet;
    
    /** 
     * Obtain the set of affected Segments.  Note that Segments
     * in this set may have been removed from the Composition or
     * even destroyed by the time this command has completed.
     */
    virtual void getSegments(SegmentSet &) = 0;

protected:
    SegmentCommand(const QString &name) : Command(name) { }
};


/**
 * TimeAndTempoChangeCommand is a base for commands that affect
 * only non-Segment data in a Composition, such as time signature
 * and tempo data.
 */

class TimeAndTempoChangeCommand : public Command
{
protected:
    TimeAndTempoChangeCommand(const QString &name) : Command(name) { }
};


/**
 * IntraSegmentCommand is a base for commands that affect a
 * (section of) a single Segment.  If that section is contiguous,
 * then you may wish to derive from BasicCommand or
 * BasicSelectionCommand (see basiccommand.h), which are more
 * specialised subclasses of IntraSegmentCommand.
 */

class IntraSegmentCommand : public Command
{
public:
    /**
     * Return the segment in which the changes happened.
     */
    virtual Rosegarden::Segment &getSegment() = 0;

protected:
    IntraSegmentCommand(const QString &name) : Command(name) { }
};


/**
 * Somewhat like a KMacroCommand, but commands can only be added by a
 * subclass (i.e. it's not intended for user-created macros) and it
 * has accessors for the contained commands.  See MacroCommand below
 * for the user-created macro version.
 * 
 * Not derived from KMacroCommand for implementation reasons.
 *
 * It's not always a good idea to use CompoundCommand or MacroCommand:
 * the main problem is that it may be too difficult for any extant
 * views to work out how to refresh their contents efficiently
 * because the basic CompoundCommand and MacroCommand offer no
 * helpful information about the changes that have happened except
 * as a set of discrete contained commands.  This means views may well
 * respond to commands by refreshing multiple times.  In general
 * therefore it's likely to be more efficient (if less convenient)
 * to create your own class which does all of the operations at once.
 */

class CompoundCommand : public Command
{
public:
    virtual ~CompoundCommand();

    /**
     * Return the number of contained commands
     */
    virtual int getCommandCount() const;

    /**
     * Return a contained command by index.  The index is zero-based
     * and interpreted in the order in which the contained commands
     * were last executed or unexecuted -- so if the last thing that
     * happened was an unexecute, you'll get them in reverse order
     * (which is usually what you want).
     */
    virtual Command *getCommand(int n);

    virtual void execute();
    virtual void unexecute();

protected:
    CompoundCommand(const QString &name) :
	Command(name),
	m_lastWasUnexecute(false) { }

    /**
     * Add a command.  Must have been allocated via new;
     * the CompoundCommand assumes ownership.
     */
    void addCommand(Command *command);

private:
    std::vector<Command *> m_commands;
    bool m_lastWasUnexecute;
};


/**
 * The true macro version.  See CompoundCommand.
 */

class MacroCommand : public CompoundCommand
{
public:
    MacroCommand(const QString &name) : CompoundCommand(name) { }
    using CompoundCommand::addCommand;
};

 

#endif
