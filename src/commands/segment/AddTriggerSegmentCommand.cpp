/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddTriggerSegmentCommand.h"

#include "base/Composition.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenDocument.h"


namespace Rosegarden
{

AddTriggerSegmentCommand::AddTriggerSegmentCommand(RosegardenDocument *doc,
        timeT duration,
        int basePitch,
        int baseVelocity) :
        NamedCommand(tr("Add Triggered Segment")),
        m_composition(&doc->getComposition()),
        m_duration(duration),
        m_basePitch(basePitch),
        m_baseVelocity(baseVelocity),
        m_id(0),
        m_segment(0),
        m_detached(false)
{
    // nothing else
}

AddTriggerSegmentCommand::~AddTriggerSegmentCommand()
{
    if (m_detached)
        delete m_segment;
}

TriggerSegmentId
AddTriggerSegmentCommand::getId() const
{
    return m_id;
}

void
AddTriggerSegmentCommand::execute()
{
    if (m_segment) {
        m_composition->addTriggerSegment(m_segment, m_id, m_basePitch, m_baseVelocity);
    } else {
        m_segment = new Segment();
        m_segment->setEndMarkerTime(m_duration);
        TriggerSegmentRec *rec = m_composition->addTriggerSegment
                                 (m_segment, m_basePitch, m_baseVelocity);
        if (rec)
            m_id = rec->getId();
    }
    m_detached = false;
}

void
AddTriggerSegmentCommand::unexecute()
{
    if (m_segment)
        m_composition->detachTriggerSegment(m_id);
    m_detached = true;
}

}
