
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

#ifndef _RG_BASICCOMMAND_H_
#define _RG_BASICCOMMAND_H_

#include "base/Segment.h"
#include <kcommand.h>
#include "base/Event.h"
#include "misc/Debug.h"

class QString;

namespace Rosegarden
{

class EventSelection;

/**
 * BasicCommand is an abstract subclass of Command that manages undo,
 * redo and notification of changes within a contiguous region of a
 * single Rosegarden Segment, by brute force.  When a subclass
 * of BasicCommand executes, it stores a copy of the events that are
 * modified by the command, ready to be restored verbatim on undo.
 */

class BasicCommand : public KNamedCommand
{
public:
    virtual ~BasicCommand();

    virtual void execute();
    virtual void unexecute();

    virtual Segment &getSegment();

    timeT getStartTime() { return m_startTime; }
    timeT getEndTime() { return m_endTime; }
    virtual timeT getRelayoutEndTime();

    /// events selected after command; 0 if no change / no meaningful selection
    virtual EventSelection *getSubsequentSelection() { return 0; }

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

    virtual void modifySegment() = 0;

    virtual void beginExecute();

private:
    void copyTo(Segment *);
    void copyFrom(Segment *);

    timeT calculateStartTime(timeT given,
                                         Segment &segment);
    timeT calculateEndTime(timeT given,
                                       Segment &segment);

    timeT m_startTime;
    timeT m_endTime;

    Segment &m_segment;
    Segment m_savedEvents;

    bool m_doBruteForceRedo;
    Segment *m_redoEvents;
};



}

#endif
