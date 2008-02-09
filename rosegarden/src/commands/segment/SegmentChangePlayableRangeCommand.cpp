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


#include "SegmentChangePlayableRangeCommand.h"

#include "base/Segment.h"
#include "gui/editors/notation/NotationStrings.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentChangePlayableRangeCommand::SegmentChangePlayableRangeCommand(int low, int high, Segment *segment) :
        KNamedCommand(getGlobalName(low, high)),
        m_lowestPlayableNote(low),
        m_highestPlayableNote(high),
        m_segment(segment)
{
    // nothing
}

SegmentChangePlayableRangeCommand::~SegmentChangePlayableRangeCommand()
{
    // nothing
}

void
SegmentChangePlayableRangeCommand::execute()
{
    m_oldLowestPlayableNote = m_segment->getLowestPlayable();
    m_oldHighestPlayableNote = m_segment->getHighestPlayable();
	m_segment->setLowestPlayable(m_lowestPlayableNote);
	m_segment->setHighestPlayable(m_highestPlayableNote);
}

void
SegmentChangePlayableRangeCommand::unexecute()
{
   	m_segment->setLowestPlayable(m_oldLowestPlayableNote);
   	m_segment->setHighestPlayable(m_oldHighetPlayableNote);
}

QString
SegmentChangePlayableRangeCommand::getGlobalName(int low, int high)
{
    if (!unit) {
        return "Undo change playable range";
    } else {
        return QString("Change playable range to %1-%2").arg(low, high);
    }
}

}
