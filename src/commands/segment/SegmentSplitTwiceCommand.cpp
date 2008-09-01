/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "SegmentSplitTwiceCommand.h"
#include "SegmentSplitCommand.h"
#include "DeleteRangeCommand.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <QString>


namespace Rosegarden
{

SegmentSplitTwiceCommand::SegmentSplitTwiceCommand(Segment *segment,
            timeT splitTime1, timeT splitTime2,
            DeleteRangeCommand::RejoinCommand *rejoins) :
        NamedCommand(i18n("Split Twice Segment")),
        m_segment(segment),
        m_newSegmentA(0),
        m_newSegmentB(0),
        m_newSegmentC(0),
        m_splitTime1(splitTime1),
        m_splitTime2(splitTime2),
        m_detached(true),
        m_composition(segment->getComposition()),
        m_rejoins(rejoins)
{

    // This command should not be called if the range doesn't cut the
    // segment twice
    assert(splitTime1 > segment->getStartTime());
    assert(splitTime1 < segment->getEndMarkerTime());
    assert(splitTime2 < segment->getEndMarkerTime());
    assert(splitTime2 > segment->getStartTime());
    assert(splitTime1 < splitTime2);


}

SegmentSplitTwiceCommand::~SegmentSplitTwiceCommand()
{
    if (m_detached) {
        delete m_newSegmentA;
        delete m_newSegmentB;
        delete m_newSegmentC;
    }
}

void
SegmentSplitTwiceCommand::execute()
{
    if (m_newSegmentA) {
        m_composition->addSegment(m_newSegmentA);
        m_composition->addSegment(m_newSegmentB);
        m_composition->addSegment(m_newSegmentC);

        m_composition->detachSegment(m_segment);

        m_detached = false; // i.e. new segments are not detached
        return;
    }

    SegmentSplitCommand split1
        = SegmentSplitCommand(m_segment, m_splitTime1, true);
    split1.execute();
    m_newSegmentA = split1.getSegmentA();
    m_newSegmentB = split1.getSegmentB();

    SegmentSplitCommand split2
        = SegmentSplitCommand(m_newSegmentB, m_splitTime2, true);
    split2.execute();
    m_newSegmentB = split2.getSegmentA();
    m_newSegmentC = split2.getSegmentB();

    if (m_rejoins) {
        // If in SegmentDeleteRange context prepare the junction
        // of external segments
        m_rejoins->addSegmentsPair(m_newSegmentA, m_newSegmentC);
    }

    m_detached = false; // i.e. new segments are not detached
}

void
SegmentSplitTwiceCommand::unexecute()
{
    m_composition->addSegment(m_segment);

    m_composition->detachSegment(m_newSegmentA);
    m_composition->detachSegment(m_newSegmentB);
    m_composition->detachSegment(m_newSegmentC);

    m_detached = true; // i.e. new segments are detached
}

}
