
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

/// A command with undo/redo functionality.
/**
 * Derivers provide their own version of modifySegment() which does the
 * actual work of the command.  This class takes care of undo/redo.
 *
 * The obvious way to implement undo is to store the entirety of the original
 * version of the Segment.  This class attempts to optimize this by storing
 * only the original Events in the range of time that has been modified
 * (m_savedEvents).
 *
 * "Brute force redo" means redo by copying a list of Events instead of calling
 * modifySegment() to perform the command again.  Brute force redo requires
 * more memory, but is more reliable.
 *
 * BasicCommand is an abstract subclass of NamedCommand that manages undo,
 * redo and notification of changes(?) within a contiguous region of a
 * single Rosegarden Segment, by brute force(?).  When a subclass
 * of BasicCommand executes, it stores a copy of the events that are
 * modified by the command, ready to be restored verbatim on undo.
 */
class BasicCommand : public NamedCommand
{
public:
    virtual ~BasicCommand() override;

    void execute() override;
    void unexecute() override;

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

    // Variant ctor to be used when the segment does not exist when
    // the command is created.  Implies brute force redo false.
    BasicCommand(const QString &name,
                 timeT start,
                 const QString& segmentMarking,
                 Composition* comp);

    /// Override this to do your command's actual work.
    virtual void modifySegment() = 0;

    /// Called once the Segment is guaranteed to exist.
    /**
     * ??? It appears as if no one overrides this.  Might be removable?
     */
    virtual void beginExecute();

    // ??? Why virtual?  Does someone override this?
    virtual Segment &getSegment();

    timeT getStartTime() { return m_startTime; }
    timeT getEndTime() { return m_endTime; }

    // ??? Not used at the moment.
    virtual timeT getRelayoutEndTime();

private:
    /// the composition
    /**
     * ??? Use the global instance instead of this.
     */
    Composition *m_comp;

    /// The Segment that this command is being run against.  This is a
    /// pointer rather than a reference because it is possible to
    /// create a command before the segment exists and set the segment
    /// later
    Segment *m_segment;
    /// if the segment is not set yet - get it from the segment marking
    void requireSegment();
    /// Copy Events in the ??? time range from m_segment to dest.
    void copyTo(Segment *dest, bool wholeSegment = false);
    /// Copy Events in the modification time range from source to m_segment.
    /**
     * Events in m_segment are removed in the time range before the copy.
     */
    void copyFrom(Segment *source, bool wholeSegment = false);

    /// Original start time for m_Segment.
    timeT m_originalStartTime;

    /// Command Start Time adjusted for notation.
    // ??? rename: m_commandStartTime
    timeT m_startTime;
    // ??? Always used to set m_startTime.  Make it return void and do that.
    //     rename: adjustCommandStartTime()
    timeT calculateStartTime(timeT given, Segment &segment);
    /// ??? What end time is this?  Command End Time?
    timeT m_endTime;
    // ??? Always used to set m_endTime.  Make it return void and do that.
    //     rename: setEndTime()
    timeT calculateEndTime(timeT given, Segment &segment);

    /// start and end of the range of events which are modified by modifySegment
    // ??? How does this differ from m_startTime and m_endTime?  Can it be
    //     narrower?  E.g. we've asked to perform a command on bar 1, but there
    //     is only an Event at beat 2?
    timeT m_modifiedEventsStart;
    timeT m_modifiedEventsEnd;
    /// Compute and store the range of Events modified by modifySegment.
    void calculateModifiedStartEnd();

    /// Events from m_segment prior to executing the command.
    /**
     * This is the undo buffer.
     *
     * ??? Use QSharedPointer or std::shared_ptr.
     * ??? Rename: m_undoSegment?
     */
    Segment *m_savedEvents;

    /// execute() will either use a list of Events or run segmentModify()
    /**
     * Brute-force means to copy Events from m_redoEvents to m_segment.  The
     * opposite is to perform the modification by calling segmentModify().
     */
    bool m_doBruteForceRedo;

    /// Events for redo, or for the "redoEvents" ctor.
    Segment *m_redoEvents;

    /// The segment marking for delayed access to segment
    QString m_segmentMarking;

};


}

#endif
