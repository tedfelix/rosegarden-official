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

#include <kcommand.h>
#include <set>

#include "Segment.h"

/**
 * A set of simple abstract base-classes for commands, to sit
 * between KCommand and your command's concrete implementation.
 *
 * Although you can execute any KCommand in Rosegarden's command
 * history, the various GUI views will only be able to refresh
 * correctly if all commands are subclassed from the appropriate one of
 * these bases.  (Please don't attempt to do the refresh yourself --
 * it's much easier to support multiple views using this technique!)
 */


/**
 * SegmentCommand is a base for commands that affect the "envelope"
 * of one or more entire segments, and that may or may not also
 * affect their contents -- for example, resizing or splitting
 * segments.
 */

class SegmentCommand : public KCommand
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
    SegmentCommand(const QString &name) : KCommand(name) { }
};


/**
 * TimeAndTempoChangeCommand is a base for commands that affect
 * only non-Segment data in a Composition, such as time signature
 * and tempo data.
 */

class TimeAndTempoChangeCommand : public KCommand
{
protected:
    TimeAndTempoChangeCommand(const QString &name) : KCommand(name) { }
};


/**
 * IntraSegmentCommand is a base for commands that affect a
 * (section of) a single Segment.  If that section is contiguous,
 * then you may wish to derive from BasicCommand or
 * BasicSelectionCommand (see basiccommand.h), which are more
 * specialised subclasses of IntraSegmentCommand.
 */

class IntraSegmentCommand : public KCommand
{
public:
    /**
     * Return the segment in which the changes happened.
     */
    virtual Rosegarden::Segment &getSegment() = 0;

protected:
    IntraSegmentCommand(const QString &name) : KCommand(name) { }
};


/**
 * Somewhat like a KMacroCommand, but commands can only be added by a
 * subclass (i.e. it's not intended for user-created macros) and it
 * has accessors for the contained commands.
 * 
 * Not derived from KMacroCommand for implementation reasons.
 */

class CompoundCommand : public KCommand
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
    virtual KCommand *getCommand(int n);

    virtual void execute();
    virtual void unexecute();

protected:
    CompoundCommand(const QString &name) :
	KCommand(name),
	m_lastWasUnexecute(false) { }

    /**
     * Add a command.  Must have been allocated via new;
     * the CompoundCommand assumes ownership.
     */
    void addCommand(KCommand *command);

private:
    std::vector<KCommand *> m_commands;
    bool m_lastWasUnexecute;
};


#endif
