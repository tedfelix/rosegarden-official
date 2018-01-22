/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SetTriggerSegmentBaseVelocityCommand.h"

#include "base/Composition.h"
#include "base/TriggerSegment.h"


namespace Rosegarden
{

SetTriggerSegmentBaseVelocityCommand::SetTriggerSegmentBaseVelocityCommand(Composition *composition,
        TriggerSegmentId id,
        int newVelocity) :
        NamedCommand(tr("Set Base Velocity")),
        m_composition(composition),
        m_id(id),
        m_newVelocity(newVelocity),
        m_oldVelocity( -1)
{
    // nothing
}

SetTriggerSegmentBaseVelocityCommand::~SetTriggerSegmentBaseVelocityCommand()
{
    // nothing
}

void
SetTriggerSegmentBaseVelocityCommand::execute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    if (m_oldVelocity == -1) {
        m_oldVelocity = rec->getBaseVelocity();
    }
    rec->setBaseVelocity(m_newVelocity);
}

void
SetTriggerSegmentBaseVelocityCommand::unexecute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    rec->setBaseVelocity(m_oldVelocity);
}

}
