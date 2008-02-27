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


#include "SegmentQuickCopyCommand.h"

#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentQuickCopyCommand::SegmentQuickCopyCommand(Segment *segment):
        KNamedCommand(getGlobalName()),
        m_composition(segment->getComposition()),
        m_segmentToCopy(segment),
        m_segment(0),
        m_detached(false)
{}

SegmentQuickCopyCommand::~SegmentQuickCopyCommand()
{
    if (m_detached) {
        delete m_segment;
    }
}

void
SegmentQuickCopyCommand::execute()
{
    if (!m_segment) {
        m_segment = new Segment(*m_segmentToCopy);
        m_segment->setLabel(qstrtostr(i18n("%1 (copied)").arg
                                      (strtoqstr(m_segment->getLabel()))));
    }
    m_composition->addSegment(m_segment);
    m_detached = false;
}

void
SegmentQuickCopyCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}

}
