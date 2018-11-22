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

#include "SegmentQuickLinkCommand.h"

#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/SegmentLinker.h"

namespace Rosegarden
{

SegmentQuickLinkCommand::SegmentQuickLinkCommand(Segment *segment) :
        NamedCommand(getGlobalName()),
        m_composition(segment->getComposition()),
        m_originalSegment(segment),
        m_newLinkedSegment(nullptr),
        m_detached(false),
        m_originalSegmentLinked(false)
{

}

SegmentQuickLinkCommand::~SegmentQuickLinkCommand()
{
    if (m_detached) {
        delete m_newLinkedSegment;
    }
}

void
SegmentQuickLinkCommand::execute()
{
    if (!m_newLinkedSegment) {
        m_originalSegmentLinked = m_originalSegment->isLinked();
        m_newLinkedSegment = SegmentLinker::createLinkedSegment(m_originalSegment);
        
        std::string label = m_originalSegment->getLabel();
        m_newLinkedSegment->setLabel(appendLabel(label, qstrtostr(tr("(linked)"))));
    } else {
        if (!m_originalSegmentLinked) {
            m_newLinkedSegment->getLinker()->addLinkedSegment(m_originalSegment);
        }
    }

    m_composition->addSegment(m_newLinkedSegment);
    m_detached = false;
}

void
SegmentQuickLinkCommand::unexecute()
{
    if (!m_originalSegmentLinked) {
        SegmentLinker::unlinkSegment(m_originalSegment);
    }
    
    m_composition->detachSegment(m_newLinkedSegment);
    m_detached = true;
}

}