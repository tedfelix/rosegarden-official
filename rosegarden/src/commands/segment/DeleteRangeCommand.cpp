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


#include "DeleteRangeCommand.h"

#include "AudioSegmentSplitCommand.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "EraseSegmentsStartingInRangeCommand.h"
#include "OpenOrCloseRangeCommand.h"
#include "SegmentJoinCommand.h"
#include "SegmentSplitCommand.h"


namespace Rosegarden
{

DeleteRangeCommand::DeleteRangeCommand(Composition *composition,
                                       timeT t0, timeT t1) :
        KMacroCommand(i18n("Delete Range"))
{
    // First add commands to split the segments up.  Make a note of
    // segments that will need rejoining with their neighbours
    // afterwards.

    std::vector<Segment *> rejoins;

    for (int e = 0; e < 2; ++e) {

        // Split all segments at the range end first, then the range
        // begin afterwards.  This is because the split commands create
        // new segments for the right part and leave the left parts in
        // the original segments, so that we can use the same segment
        // pointer to do the left split as we did for the right

        timeT t = t1;
        if (e == 1)
            t = t0;

        for (Composition::iterator i = composition->begin();
                i != composition->end(); ++i) {

            if ((*i)->getStartTime() >= t || (*i)->getEndMarkerTime() <= t) {
                continue;
            }

            if ((*i)->getType() == Segment::Audio) {
                addCommand(new AudioSegmentSplitCommand(*i, t));
            } else {
                addCommand(new SegmentSplitCommand(*i, t));

                if (t == t0 && (*i)->getEndMarkerTime() > t1) {
                    rejoins.push_back(*i);
                }
            }
        }
    }

    // Then commands to do the rest of the work

    addCommand(new EraseSegmentsStartingInRangeCommand(composition, t0, t1));

    addCommand(new OpenOrCloseRangeCommand(composition, t0, t1, false));

    for (std::vector<Segment *>::iterator i = rejoins.begin();
            i != rejoins.end(); ++i) {
        addCommand(new RejoinCommand(composition, *i,
                                     (*i)->getEndMarkerTime() + t0 - t1));
    }
}

DeleteRangeCommand::~DeleteRangeCommand()
{}

void
DeleteRangeCommand::RejoinCommand::execute()
{
    if (m_joinCommand) {
        m_joinCommand->execute();
        return ;
    }

    //!!! Need to remove the "(split)" names from the segment bits

    for (Composition::iterator i = m_composition->begin();
            i != m_composition->end(); ++i) {
        if ((*i) == m_segment)
            continue;
        if ((*i)->getTrack() != m_segment->getTrack())
            continue;
        if ((*i)->getEndMarkerTime() != m_endMarkerTime)
            continue;
        if ((*i)->getStartTime() <= m_segment->getStartTime())
            continue;
        SegmentSelection selection;
        selection.insert(m_segment);
        selection.insert(*i);
        m_joinCommand = new SegmentJoinCommand(selection);
        m_joinCommand->execute();
        break;
    }
}

}
