/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SegmentLinkToCopyCommand.h"

#include "base/Segment.h"
//#include "misc/AppendLabel.h"
//#include "misc/Strings.h"
//#include "base/Composition.h"
#include "base/SegmentLinker.h"

namespace Rosegarden
{

SegmentLinkToCopyCommand::SegmentLinkToCopyCommand(Segment *segment) :
    NamedCommand(getGlobalName()),
    m_composition(segment->getComposition()),
    m_Segment(segment),
    m_SegmentCopy(segment->clone())
{

}

SegmentLinkToCopyCommand::~SegmentLinkToCopyCommand()
{
    delete m_SegmentCopy;
}

void
SegmentLinkToCopyCommand::execute()
{
    SegmentLinker::unlinkSegment(m_Segment);
}

void
SegmentLinkToCopyCommand::unexecute()
{
    if (m_SegmentCopy->isLinked()) {
        m_SegmentCopy->getLinker()->addLinkedSegment(m_Segment);
    }
}

}