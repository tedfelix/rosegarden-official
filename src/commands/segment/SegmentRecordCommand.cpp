/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentRecordCommand.h"

#include "base/Composition.h"
#include "base/Segment.h"
#include <QObject>


namespace Rosegarden
{

SegmentRecordCommand::SegmentRecordCommand(Segment *segment) :
        NamedCommand(tr("Record")),
        m_composition(segment->getComposition()),
        m_segment(segment),
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
