/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
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
#include "SegmentSplitTwiceCommand.h"


namespace Rosegarden
{

DeleteRangeCommand::DeleteRangeCommand(Composition *composition,
                                       timeT t0, timeT t1) :
        MacroCommand(tr("Delete Range"))
{
    // First add commands to split the segments up.  Make a note of
    // segments that will need rejoining with their neighbours
    // afterwards.

    RejoinCommand *rejoinCommand = new RejoinCommand();

    // Audio segments first
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

            if ((*i)->getType() == Segment::Audio) {

                if ((*i)->getStartTime() >= t || (*i)->getEndMarkerTime() <= t) {
                    continue;
                }

                addCommand(new AudioSegmentSplitCommand(*i, t));
            }
        }
    }


    // then non audio segments
    for (Composition::iterator i = composition->begin();
            i != composition->end(); ++i) {

        if ((*i)->getType() != Segment::Audio) {

            // How many time to split the segment ?
            timeT t;
            int count = 0;
            if (t0 > (*i)->getStartTime() && t0 < (*i)->getEndMarkerTime()) {
                count++;
                t = t0;
            }
            if (t1 > (*i)->getStartTime() && t1 < (*i)->getEndMarkerTime()) {
                count++;
                t = t1;
            }

            // Split the segment
            switch(count) {
                case 0 : // Do nothing
                    break;

                case 1 : // Split segment once.
                    addCommand(new SegmentSplitCommand(*i, t, true));
                    break;

                case 2 : // Split segment twice.
                        // The first and last from the three resulting segments
                        // will be stored into the rejoinCommand to be rejoined later.
                    addCommand(new SegmentSplitTwiceCommand(*i, t0, t1, rejoinCommand));
                    break;
            }
        }
    }


    // Then commands to do the rest of the work

    addCommand(new EraseSegmentsStartingInRangeCommand(composition, t0, t1));

    addCommand(new OpenOrCloseRangeCommand(composition, t0, t1, false));

    addCommand(rejoinCommand);

}

DeleteRangeCommand::~DeleteRangeCommand()
{
}


DeleteRangeCommand::RejoinCommand::~RejoinCommand()
{
    for (std::vector<SegmentJoinCommand *>::iterator i = m_rejoins.begin();
            i != m_rejoins.end(); ++i) {
        delete *i;
    }
}

void
DeleteRangeCommand::RejoinCommand::execute()
{
    for (std::vector<SegmentJoinCommand *>::iterator i = m_rejoins.begin();
            i != m_rejoins.end(); ++i) {
        (*i)->execute();
    }
}

void
DeleteRangeCommand::RejoinCommand::unexecute()
{
    for (std::vector<SegmentJoinCommand *>::iterator i = m_rejoins.begin();
            i != m_rejoins.end(); ++i) {
        (*i)->unexecute();
    }
}

void
DeleteRangeCommand::RejoinCommand::addSegmentsPair(Segment *s1, Segment *s2)
{
    SegmentSelection selection;
    selection.insert(s1);
    selection.insert(s2);
    SegmentJoinCommand *joinCommand = new SegmentJoinCommand(selection);
    m_rejoins.push_back(joinCommand);
}

}
