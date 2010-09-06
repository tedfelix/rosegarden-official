/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentReconfigureCommand.h"

#include "base/Segment.h"
#include "base/Composition.h"
#include "base/Track.h"
#include <QString>


namespace Rosegarden
{

SegmentReconfigureCommand::SegmentReconfigureCommand(QString name) :
        NamedCommand(name),
        m_shouldExpand(false),
        m_compEndTime(0),
        m_comp(0)
        
{}

SegmentReconfigureCommand::~SegmentReconfigureCommand()
{}

void
SegmentReconfigureCommand::addSegment(Segment *segment,
                                      timeT startTime,
                                      timeT endTime,
                                      TrackId track)
{
    SegmentRec record;
    record.segment = segment;
    record.startTime = startTime;
    record.endTime = endTime;
    record.track = track;
    m_records.push_back(record);
    trackCompositionEnd(record);
}

void
SegmentReconfigureCommand::addSegments(const SegmentRecSet &records)
{
    for (SegmentRecSet::const_iterator i = records.begin(); i != records.end(); ++i) {
        m_records.push_back(*i);
        trackCompositionEnd(*i);
    }
}

void
SegmentReconfigureCommand::execute()
{
    swap();
}

void
SegmentReconfigureCommand::unexecute()
{
    swap();
}

void
SegmentReconfigureCommand::swap()
{
    
    for (SegmentRecSet::iterator i = m_records.begin();
            i != m_records.end(); ++i) {

        // set the segment's values from the record, but set the
        // previous values back in to the record for use in the
        // next iteration of the execute/unexecute cycle.

        // #1083496: look up both of the "old" values before we set
        // anything, as setting the start time is likely to change the
        // end marker time.

        timeT prevStartTime = i->segment->getStartTime();
        timeT prevEndTime = i->segment->getEndTime();

        // Set start and end time without regard to composition
        // Start and end time.  This allows segments to soft truncate
        // along start and end of composition.
        i->segment->setStartTime(i->startTime);
        i->segment->setEndTime(i->endTime);

        i->startTime = prevStartTime;
        i->endTime = prevEndTime;

        TrackId currentTrack = i->segment->getTrack();

        if (currentTrack != i->track) {
            i->segment->setTrack(i->track);
            i->track = currentTrack;
        }
    }
    
    // Swap Composition End Times
    if (m_shouldExpand && m_comp) {
        timeT prevEndTime = m_comp->getEndMarker();
        m_comp->setEndMarker(m_compEndTime);
        m_compEndTime = prevEndTime;
    }
}

void
SegmentReconfigureCommand::trackCompositionEnd(SegmentRec record) {
    Composition *comp = record.segment->getComposition();
    
    if (!comp) {
        // Segment not part of composition
        return;
    }
    
    timeT compEndTime = comp->getEndMarker();
    if (record.endTime > compEndTime) {
        m_shouldExpand = true;
        m_comp = comp;
    }
    
    if (m_shouldExpand && record.endTime > m_compEndTime) {
        m_compEndTime = record.endTime;
    }
}
}
