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


#ifndef _SEGMENTCOMMANDS_H_
#define _SEGMENTCOMMANDS_H_

#include "basiccommand.h"
#include "Segment.h"
#include "Composition.h"
#include "NotationTypes.h"


/*!!! shouldn't need a command just for selection -- you don't undo selection

class SegmentSelectionCommand : public KCommand
{
public:
    virtual ~SegmentSelectionCommand()

protected:
    SegmentSelectionCommand(const QString &name, SegmentSelection &selection,
                            bool bruteForceRedoRequired = false);
}
*/

class SegmentEraseCommand : public KCommand,
			    public SegmentCommand
{
public:
    SegmentEraseCommand(Rosegarden::Segment *segment);
    virtual ~SegmentEraseCommand();

    virtual void execute();
    virtual void unexecute();
    
private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
};

/*!!! to be implemented
class SegmentInsertCommand : public KCommand,
			     public SegmentCommand
{
};

//!!! combined change-start-time-and-track command?
class SegmentMoveCommand : public KCommand,
			   public SegmentCommand
{
};
*/


class AddTimeSignatureCommand : public KCommand,
				public TimeAndTempoChangeCommand
{
public:
    AddTimeSignatureCommand(Rosegarden::Composition *composition,
			    Rosegarden::timeT time,
			    Rosegarden::TimeSignature timeSig) :
	KCommand(name()),
	m_composition(composition),
	m_time(time),
	m_timeSignature(timeSig) { }
    virtual ~AddTimeSignatureCommand() { }

    static QString name() {
	return "Add &Time Signature Change...";
    }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::timeT m_time;
    Rosegarden::TimeSignature m_timeSignature;
    int m_timeSigIndex; // for undo
};    


#endif  // _SEGMENTCOMMANDS_H_
