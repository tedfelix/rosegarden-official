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

#include "segmentcommands.h"


// --------- Erase Segment --------
//
SegmentEraseCommand::SegmentEraseCommand(Rosegarden::Segment *segment) :
    KCommand("Erase Segment"),
    m_composition(segment->getComposition()),
    m_segment(segment)
{
    // nothing else
}

SegmentEraseCommand::~SegmentEraseCommand()
{
    // This is the only place the Segment can safely be deleted, and
    // then only if it is not in the Composition (i.e. if we executed
    // more recently than we unexecuted)

    if (!m_segment->getComposition()) {
	delete m_segment;
    }
}


void
SegmentEraseCommand::getSegments(SegmentSet &segments)
{
    segments.insert(m_segment);
}


void
SegmentEraseCommand::execute()
{
    m_composition->detachSegment(m_segment);
}

void
SegmentEraseCommand::unexecute()
{
    m_composition->addSegment(m_segment);
}


// --------- Insert Segment --------
//
SegmentInsertCommand::SegmentInsertCommand(Rosegarden::Composition *c,
                                           Rosegarden::TrackId track,
                                           Rosegarden::timeT startTime,
                                           Rosegarden::timeT duration):
    KCommand("Insert Segment"),
    m_composition(c),
    m_segment(0),
    m_track(track),
    m_startTime(startTime),
    m_duration(duration)
{
}

SegmentInsertCommand::~SegmentInsertCommand()
{
    if (!m_segment->getComposition()) {
	delete m_segment;
    }
}


void
SegmentInsertCommand::getSegments(std::set<Rosegarden::Segment *> &segments)
{
    segments.insert(m_segment);
}


void
SegmentInsertCommand::execute()
{
    if (!m_segment)
    {
        // Create and insert Segment
        //
        m_segment = new Rosegarden::Segment();
        m_segment->setTrack(m_track);
        m_segment->setStartTime(m_startTime);
	m_composition->addSegment(m_segment);
        m_segment->setDuration(m_duration);

        // Add the SegmentItem to the canvas (does nothing
        // if the SegmentItem already exists)
        //
//!!!        m_document->addSegmentItem(m_segment);
    }
    else
    {
        m_composition->addSegment(m_segment);
//!!!        m_document->addSegmentItem(m_segment);
    }
    
}

void
SegmentInsertCommand::unexecute()
{
//!!!    m_document->deleteSegmentItem(m_segment);
    m_composition->detachSegment(m_segment);
}

// --------- Move Segment --------
//
SegmentMoveCommand::SegmentMoveCommand(Rosegarden::Segment *segment):
    KCommand("Move Segment"),
    m_composition(segment->getComposition()),
    m_segment(segment)
{
}

SegmentMoveCommand::~SegmentMoveCommand()
{
    if (!m_segment->getComposition()) {
	delete m_segment;
    }
}

void
SegmentMoveCommand::execute()
{
}

void
SegmentMoveCommand::unexecute()
{
}


// --------- Add Time Signature --------
// 
void
AddTimeSignatureCommand::execute()
{
    m_timeSigIndex = m_composition->addTimeSignature(m_time, m_timeSignature);
}

void
AddTimeSignatureCommand::unexecute()
{
    m_composition->removeTimeSignature(m_timeSigIndex);
}

