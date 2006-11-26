/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "SegmentRecordCommand.h"

#include <klocale.h>
#include "base/Composition.h"
#include "base/Segment.h"


namespace Rosegarden
{

SegmentRecordCommand::SegmentRecordCommand(Segment *s) :
        KNamedCommand(i18n("Record")),
        m_composition(s->getComposition()),
        m_segment(s),
        m_detached(false)
{}

SegmentRecordCommand::~SegmentRecordCommand()
{
    if (m_detached) {
        delete m_segment;
    }
}

void
SegmentRecordCommand::execute()
{
    if (!m_segment->getComposition()) {
        m_composition->addSegment(m_segment);
    }

    m_detached = false;
}

void
SegmentRecordCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}

}
