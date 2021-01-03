/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "DeleteTriggerSegmentCommand.h"

#include "base/Composition.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenDocument.h"


namespace Rosegarden
{

DeleteTriggerSegmentCommand::DeleteTriggerSegmentCommand(RosegardenDocument *doc,
        TriggerSegmentId id) :
        NamedCommand(tr("Delete Triggered Segment")),
        m_composition(&doc->getComposition()),
        m_id(id),
        m_segment(nullptr),
        m_basePitch( -1),
        m_baseVelocity( -1),
        m_detached(true)
{
    // nothing else
}

DeleteTriggerSegmentCommand::~DeleteTriggerSegmentCommand()
{
    if (m_detached)
        delete m_segment;
}

void
DeleteTriggerSegmentCommand::execute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    m_segment = rec->getSegment();
    m_basePitch = rec->getBasePitch();
    m_baseVelocity = rec->getBaseVelocity();
    m_composition->detachTriggerSegment(m_id);
    m_detached = true;
}

void
DeleteTriggerSegmentCommand::unexecute()
{
    if (m_segment)
        m_composition->addTriggerSegment(m_segment, m_id, m_basePitch, m_baseVelocity);
    m_detached = false;
}

}
