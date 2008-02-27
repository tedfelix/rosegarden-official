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


#include "SegmentSingleRepeatToCopyCommand.h"

#include <klocale.h>
#include "base/Composition.h"
#include "base/Segment.h"


namespace Rosegarden
{

SegmentSingleRepeatToCopyCommand::SegmentSingleRepeatToCopyCommand(
    Segment *segment,
    timeT time):
        KNamedCommand(i18n("Turn Single Repeat into Copy")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_newSegment(0),
        m_time(time),
        m_detached(false)
{}

SegmentSingleRepeatToCopyCommand::~SegmentSingleRepeatToCopyCommand()
{
    if (m_detached)
        delete m_newSegment;
}

void
SegmentSingleRepeatToCopyCommand::execute()
{
    if (!m_newSegment) {
        m_newSegment = new Segment(*m_segment);
        m_newSegment->setStartTime(m_time);
        m_newSegment->setRepeating(true);
    }

    m_composition->addSegment(m_newSegment);
    m_detached = false;
}

void
SegmentSingleRepeatToCopyCommand::unexecute()
{
    m_composition->detachSegment(m_newSegment);
    m_detached = true;
}

}
