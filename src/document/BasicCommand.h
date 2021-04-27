
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

#include <memory>  // for shared_ptr

class QString;
#include <QSharedPointer>

namespace Rosegarden
{


class EventSelection;
class CommandArgumentQuerier; // forward declaration useful for some subclasses


/// A command with undo/redo functionality.
/**
 * Derivers provide their own version of modifySegment() which does the
 * actual work of the command.  This class takes care of undo/redo.
 *
 * This class stores the entirety of the original version of the Segment
 * in m_originalEvents.
 *
 * On undo (unexecute()), this class only copies back the events in the
 * range of time that was modified.  This is done primarily because the
 * UI refresh code is terribly inefficient and refreshes the entire UI for
 * each and every Event that gets added to a Segment.
 *
 * The times passed to the constructor are no longer used to determine
 * the range of events to copy. This is now determined by
 * calculateModifiedStartEnd(). The getStartTime() and getEndTime() methods
 * are used as a store for the times provided in the constructors. The
 * use of these methods is deprecated. It is only necessary for the
 * derivers to override modifySegment().
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
 *
 * TODO
 * - Remove the deprecated member functions and variables.
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

    // Brute force execute() ctor.
    /**
     * To be used when events to insert are known when the command is
     * constructed.  Implies brute force redo.
     */
    BasicCommand(const QString &name,
                 Segment &segment,
                 Segment *redoEvents);

    // Ctor to be used when the Segment does not exist when
    // the command is created.  Brute force redo is not supported.
    BasicCommand(const QString &name,
                 timeT start,
                 const QString &segmentMarking);

    /// Override this to do your command's actual work.
    virtual void modifySegment() = 0;

    /// Get a reference to the segment
    Segment &getSegment();

    /// Thes methods are deprecated
    timeT getStartTime() { return m_startTime; }
    timeT getEndTime() { return m_endTime; }

    // This method is not used. Classes should not override it
    virtual timeT getRelayoutEndTime();

private:

    /// The Segment that this command is being run against.  This is a
    /// pointer rather than a reference because it is possible to
    /// create a command before the segment exists and set the segment
    /// later
    Segment *m_segment;
    /// if the segment is not set yet - get it from the segment marking
    void requireSegment();
    /// Copy all Events within m_segment's time range from m_segment to dest.
    void copyTo(QSharedPointer<Segment> dest);
    /// Copy Events in the modification time range from source to m_segment.
    /**
     * Events in m_segment are removed in the time range before the copy.
     */
    void copyFrom(QSharedPointer<Segment> source, bool wholeSegment = false);

    /// Original start time for m_Segment.
    timeT m_originalStartTime;

    // m_startTime, m_endTime, calculateStartTime(), and
    // calculateEndTime() are no longer required for the
    // functionality.  These should be phased out on a
    // command-by-command basis.

    /// Command Start Time adjusted for notation. [DEPRECATED]
    timeT m_startTime;
    timeT m_endTime;
    /// Calculate start time for m_startTime.  [DEPRECATED]
    timeT calculateStartTime(timeT given, Segment &segment);
    /// Calculate end time for m_endTime.  [DEPRECATED]
    timeT calculateEndTime(timeT given, Segment &segment);

    /// Start time of Events which were modified by modifySegment().
    /**
     * Set by calculateModifiedStartEnd().
     */
    timeT m_modifiedEventsStart;
    /// End time of Events which were modified by modifySegment().
    /**
     * Set by calculateModifiedStartEnd().
     */
    timeT m_modifiedEventsEnd;

    /// Compare m_segment and m_originalEvents and find the start/end
    /// of the changes.
    /**
     * Sets m_modifiedEventsStart and m_modifiedEventsEnd.
     */
    void calculateModifiedStartEnd();

    /// All Events from m_segment prior to executing the command.
    /**
     * This is a complete backup of m_segment.
     *
     */
    QSharedPointer<Segment> m_originalEvents;

    /// execute() will either use a list of Events or run segmentModify()
    /**
     * Brute-force means to copy Events from m_redoEvents to m_segment.  The
     * opposite is to perform the modification by calling segmentModify().
     */
    bool m_doBruteForceRedo;

    /// Events for redo, or for the "redoEvents" ctor.
    QSharedPointer<Segment> m_redoEvents;

    /// The segment marking for delayed access to segment
    QString m_segmentMarking;

};


}

#endif
