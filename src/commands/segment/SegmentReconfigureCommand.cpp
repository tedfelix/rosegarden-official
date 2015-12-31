/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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


SegmentReconfigureCommand::SegmentReconfigureCommand(
        QString name,
        Composition *composition) :
    NamedCommand(name),
    m_composition(composition),
    m_oldEndTime(m_composition->getEndMarker())
{
    setUpdateLinks(false);
}

SegmentReconfigureCommand::~SegmentReconfigureCommand()
{}

void
SegmentReconfigureCommand::addSegment(Segment *segment,
                                      timeT newStartTime,
                                      timeT newEndMarkerTime,
                                      TrackId newTrack)
{
    Change change;
    change.segment = segment;
    change.newStartTime = newStartTime;
    change.newEndMarkerTime = newEndMarkerTime;
    change.newTrack = newTrack;
    m_changeSet.push_back(change);
}

#if 0
// unused
void
SegmentReconfigureCommand::addSegments(const ChangeSet &changes)
{
    for (ChangeSet::const_iterator i = changes.begin(); i != changes.end(); ++i) {
        m_changeSet.push_back(*i);
    }
}
#endif

void
SegmentReconfigureCommand::execute()
{
    timeT latestEndTime = swap();

    if (m_composition->autoExpandEnabled()) {
        // If the composition needs expanding, do so...
        if (latestEndTime > m_composition->getEndMarker())
            m_composition->setEndMarker(
                    m_composition->getBarEndForTime(latestEndTime));
    }
}

void
SegmentReconfigureCommand::unexecute()
{
    swap();

    // Restore the original composition end in case it was changed
    m_composition->setEndMarker(m_oldEndTime);
}

timeT
SegmentReconfigureCommand::swap()
{
    if (!m_composition)
        return 0;

    timeT latestEndTime = 0;
    
    for (ChangeSet::iterator i = m_changeSet.begin();
            i != m_changeSet.end(); ++i) {

        // set the segment's new values from the change set, but save the
        // previous values in the change set for use in the
        // next iteration of the execute/unexecute cycle.

        // #1083496: look up both of the "old" values before we set
        // anything, as setting the start time is likely to change the
        // end marker time.

        timeT prevStartTime = i->segment->getStartTime();
        timeT prevEndMarkerTime = i->segment->getEndMarkerTime(false);

        if (i->segment->getStartTime() != i->newStartTime) {
            i->segment->setStartTime(i->newStartTime);
        }

        if (i->segment->getEndMarkerTime() != i->newEndMarkerTime) {
            i->segment->setEndMarkerTime(i->newEndMarkerTime);
        }

        // Keep track of the latest end time.
        if (i->newEndMarkerTime > latestEndTime) {
            latestEndTime = i->newEndMarkerTime;
        }

        i->newStartTime = prevStartTime;
        i->newEndMarkerTime = prevEndMarkerTime;

        TrackId currentTrack = i->segment->getTrack();

        if (currentTrack != i->newTrack) {
            i->segment->setTrack(i->newTrack);
            i->newTrack = currentTrack;
        }

        // If segment left from the current segment is repeating then we need to reconfigure
        // it
        Segment* curr_segment = i->segment;
        Composition::iterator segment_iterator = m_composition->findSegment(curr_segment);

        // Check that we don't have most upper left segment in the composition
        // AND
        // composition has more than one segment
        // ??? Check for more than 1 segment is not needed.  If there is only
        //     one segment, we'll be on begin() and the first condition will
        //     be false.
        if ( segment_iterator != m_composition->begin() &&
             segment_iterator != m_composition->end() &&
             m_composition->getNbSegments() > 1 ) {
            // move to previous segment
            --segment_iterator;
            Segment* prevSegment = *segment_iterator;

            // Segments need to be on the same track
            if (curr_segment->getTrack() == prevSegment->getTrack()) {
                if (prevSegment->isRepeating() == true)
                    // Trigger update notifications by setting to true again.
                    prevSegment->setRepeating(true);
            }
        }

    }

    return latestEndTime;
}


}
