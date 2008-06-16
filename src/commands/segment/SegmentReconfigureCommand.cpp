/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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
#include "base/Track.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentReconfigureCommand::SegmentReconfigureCommand(QString name) :
        KNamedCommand(name)
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

        if (i->segment->getStartTime() != i->startTime) {
            i->segment->setStartTime(i->startTime);
        }

        if (i->segment->getEndMarkerTime() != i->endMarkerTime) {
            i->segment->setEndMarkerTime(i->endMarkerTime);
        }

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
