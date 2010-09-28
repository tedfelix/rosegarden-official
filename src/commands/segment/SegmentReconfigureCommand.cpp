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
        NamedCommand(name)
{}

SegmentReconfigureCommand::~SegmentReconfigureCommand()
{}

void
SegmentReconfigureCommand::addSegment(Segment *segment,
                                      timeT startTime,
                                      timeT endMarkerTime,
                                      TrackId track)
{
    SegmentRec record;
    record.segment = segment;
    record.startTime = startTime;
    record.endMarkerTime = endMarkerTime;
    record.track = track;
    m_records.push_back(record);
}

void
SegmentReconfigureCommand::addSegments(const SegmentRecSet &records)
{
    for (SegmentRecSet::const_iterator i = records.begin(); i != records.end(); ++i) {
        m_records.push_back(*i);
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
        timeT prevEndMarkerTime = i->segment->getEndMarkerTime();

        // Hold the preliminary and most likely end time for this segment
        timeT newEndMarkerTime = i->endMarkerTime;
        
        // Must determine several things about the new segment to see:
        //
        // 1. if it is in the composition
        // 2. if this was a 'move' or a 'resize' event that we are performing.
        // 3. if the 'moving' segment was being moved to the left.
        // 4. if the right hand of the segment truncated by the composition end marker.
        // 
        // So we will know how to gracefully handle restoring of the true duration
        // of the segment.
        //
        // NOTE: A resized segment that is placed at the end of the composition
        // will loss its resize info as it is moved back into the composition.
        // We would need to track a third state to handle the distiction between
        // a resize and when the end of composition truncates the segment.
        Composition *comp = i->segment->getComposition();
        
        if (comp) {
            timeT prevEndTime = i->segment->getEndTime();
            timeT compEndMarkerTime = comp->getEndMarker();
            
            bool isMove = (prevEndMarkerTime - prevStartTime) == (i->endMarkerTime - i->startTime);

            if ((compEndMarkerTime == prevEndMarkerTime) &&
                (compEndMarkerTime < prevEndTime) &&
                (prevStartTime > i->startTime) && (isMove)) {
                newEndMarkerTime = prevEndTime - (prevStartTime - i->startTime);
            }
        }

        // Set start and end time without regard to composition
        // Start and end time.  This allows segments to soft truncate
        // along end of composition.
        i->segment->setStartTime(i->startTime);
        i->segment->setEndMarkerTime(newEndMarkerTime);

        i->startTime = prevStartTime;
        i->endMarkerTime = prevEndMarkerTime;

        TrackId currentTrack = i->segment->getTrack();

        if (currentTrack != i->track) {
            i->segment->setTrack(i->track);
            i->track = currentTrack;
        }
    }
    
}

}
