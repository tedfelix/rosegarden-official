
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
*/

#ifndef RG_BASICCOMMAND_H
#define RG_BASICCOMMAND_H

#include "base/Segment.h"
#include "document/Command.h"
#include "base/Event.h"
#include "misc/Debug.h"

class QString;

namespace Rosegarden
{

class EventSelection;
class CommandArgumentQuerier; // forward declaration useful for some subclasses

/**
 * BasicCommand is an abstract subclass of Command that manages undo,
 * redo and notification of changes within a contiguous region of a
 * single Rosegarden Segment, by brute force.  When a subclass
 * of BasicCommand executes, it stores a copy of the events that are
 * modified by the command, ready to be restored verbatim on undo.
 */

class BasicCommand : public NamedCommand
{
public:
    ~BasicCommand() override;

    void execute() override;
    void unexecute() override;

    virtual Segment &getSegment();

    timeT getStartTime() { return m_startTime; }
    timeT getEndTime() { return m_endTime; }
    virtual timeT getRelayoutEndTime();

    /// events selected after command; 0 if no change / no meaningful selection
    virtual EventSelection *getSubsequentSelection() { return nullptr; }

protected:
    /**
     * You should pass "bruteForceRedoRequired = true" if your
     * subclass's implementation of modifySegment uses discrete
     * event pointers or segment iterators to determine which
     * events to modify, in which case it won't work when
     * replayed for redo because the pointers may no longer be
     * valid.  In which case, BasicCommand will implement redo
     * much like undo, and will only call your modifySegment 
     * the very first time the command object is executed.
     *
     * It is always safe to pass bruteForceRedoRequired true,
     * it's just normally a waste of memory.
     */
    BasicCommand(const QString &name,
                 Segment &segment,
                 timeT start, timeT end,
                 bool bruteForceRedoRequired = false);

    // Variant ctor to be used when events to insert are known when
    // the command is cted.  Implies brute force redo.
    BasicCommand(const QString &name,
                 Segment &segment,
		 Segment *redoEvents);

    virtual void modifySegment() = 0;

    virtual void beginExecute();

private:
    /// Copy from m_segment to segment.
    void copyTo(Segment *segment);
    /// Copy from segment to m_segment replacing events in the time range.
    void copyFrom(Segment *segment);

    timeT calculateStartTime(timeT given, Segment &segment);
    timeT calculateEndTime(timeT given, Segment &segment);

    timeT m_startTime;
    timeT m_endTime;

    /// The Segment that this command is being run against.
    Segment &m_segment;
    /// Events from m_segment prior to executing the command.
    Segment m_savedEvents;

    /// Redo or execute() will be using a list of events (m_redoEvents).
    bool m_doBruteForceRedo;
    /// Events for redo, or for the "redoEvents" ctor.
    Segment *m_redoEvents;
};



}

#endif
