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


#include "SegmentQuickCopyCommand.h"

#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include <QString>


namespace Rosegarden
{

SegmentQuickCopyCommand::SegmentQuickCopyCommand(Segment *segment):
        NamedCommand(getGlobalName()),
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
        m_segment = m_segmentToCopy->clone(false);
        std::string label = m_segmentToCopy->getLabel();
        // We rename the original segment to end in (copied) since it is
        // the one that is being moved by SegmentTool.  The copy is being
        // left behind in place of the original.
        m_segmentToCopy->setLabel(appendLabel(label, qstrtostr(tr("(copied)"))));
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
