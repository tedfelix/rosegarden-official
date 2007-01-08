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


#include "SegmentRepeatToCopyCommand.h"

#include "base/Event.h"
#include <klocale.h>
#include "base/Composition.h"
#include "base/Segment.h"


namespace Rosegarden
{

SegmentRepeatToCopyCommand::SegmentRepeatToCopyCommand(
    Segment *segment):
        KNamedCommand(i18n("Turn Repeats into Copies")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_detached(false)
{}

SegmentRepeatToCopyCommand::~SegmentRepeatToCopyCommand()
{
    if (m_detached) {
        std::vector<Segment*>::iterator it =
            m_newSegments.begin();

        for (; it != m_newSegments.end(); it++)
            delete (*it);
    }
}

void
SegmentRepeatToCopyCommand::execute()
{
    if (m_newSegments.size() == 0) {
        timeT newStartTime = m_segment->getEndMarkerTime();
        timeT newDuration =
            m_segment->getEndMarkerTime() - m_segment->getStartTime();
        Segment *newSegment;
        timeT repeatEndTime = m_segment->getRepeatEndTime();

        while (newStartTime + newDuration < repeatEndTime) {
            // Create new segment, transpose and turn off repeat
            //
            newSegment = new Segment(*m_segment);
            newSegment->setStartTime(newStartTime);
            newSegment->setRepeating(false);

            // Insert and store
            m_composition->addSegment(newSegment);
            m_newSegments.push_back(newSegment);

            // Move onto next
            newStartTime += newDuration;
        }

        // fill remaining partial segment
    } else {
        std::vector<Segment*>::iterator it =
            m_newSegments.begin();

        for (; it != m_newSegments.end(); it++)
            m_composition->addSegment(*it);
    }
    m_segment->setRepeating(false);
    m_detached = false;
}

void
SegmentRepeatToCopyCommand::unexecute()
{
    std::vector<Segment*>::iterator it =
        m_newSegments.begin();

    for (; it != m_newSegments.end(); it++)
        m_composition->detachSegment(*it);

    m_detached = true;
    m_segment->setRepeating(true);
}

}
