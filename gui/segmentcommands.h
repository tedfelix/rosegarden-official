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
#include "rosegardenguidoc.h"


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

class SegmentInsertCommand : public KCommand,
                             public SegmentCommand
{
public:
    SegmentInsertCommand(RosegardenGUIDoc *doc,
                         Rosegarden::TrackId track,
                         Rosegarden::timeT startTime,
                         Rosegarden::timeT duration);
    virtual ~SegmentInsertCommand();

    virtual void execute();
    virtual void unexecute();

    Rosegarden::Segment* getSegment() const { return m_segment; }

private:
    RosegardenGUIDoc    *m_document;
    Rosegarden::Segment *m_segment;
    int                  m_track;
    Rosegarden::timeT    m_startTime;
    Rosegarden::timeT    m_duration;

};

class SegmentMoveCommand : public KCommand,
                           public SegmentCommand
{
public:
    SegmentMoveCommand(Rosegarden::Segment *segment);
    virtual ~SegmentMoveCommand();


    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
};


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
