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

#ifndef _BASIC_COMMAND_H_
#define _BASIC_COMMAND_H_

/**
 * Command that handles undo on a single Segment by brute-force,
 * saving the affected area of the Segment before the execute() and
 * then restoring it on unexecute().  Subclass this with your own
 * implementation.
 *
 * This class implements execute() and unexecute(); in order to create
 * a working subclass, you should normally put the code you would
 * otherwise have put in execute() into modifySegment().
 */

#include <qobject.h>

#include <kcommand.h>

#include "Segment.h"
#include "SegmentNotationHelper.h"

class EventSelection;


class BasicCommand : public QObject, public KCommand
{
    Q_OBJECT

public:
    virtual ~BasicCommand();

    virtual void execute();
    virtual void unexecute();

    Rosegarden::Segment &getSegment() { return m_segment; }
    const Rosegarden::Segment &getSegment() const { return m_segment; }

    Rosegarden::timeT getBeginTime() { return m_savedEvents.getStartIndex(); }
    Rosegarden::timeT getEndTime() { return m_endTime; }
    virtual Rosegarden::timeT getRelayoutEndTime() { return getEndTime(); }

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
		 Rosegarden::Segment &segment,
		 Rosegarden::timeT begin, Rosegarden::timeT end,
		 bool bruteForceRedoRequired = false);

    virtual void modifySegment(Rosegarden::SegmentNotationHelper &) = 0;

    virtual void beginExecute();

private:
    //--------------- Data members ---------------------------------

    void copyTo(Rosegarden::Segment *);
    void copyFrom(Rosegarden::Segment *);

    Rosegarden::Segment &m_segment;
    Rosegarden::Segment m_savedEvents;
    Rosegarden::timeT m_endTime;

    bool m_doBruteForceRedo;
    Rosegarden::Segment *m_redoEvents;
};


/**
 * Subclass of BasicCommand that manages the brute-force undo and redo
 * extends based on a given selection.
 */

class BasicSelectionCommand : public BasicCommand
{
public:
    virtual ~BasicSelectionCommand();

protected:
    BasicSelectionCommand(const QString &name, EventSelection &selection,
			  bool bruteForceRedoRequired = false);
};

#endif
