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


#include "SetTriggerSegmentBasePitchCommand.h"

#include "base/Composition.h"
#include "base/TriggerSegment.h"


namespace Rosegarden
{

SetTriggerSegmentBasePitchCommand::SetTriggerSegmentBasePitchCommand(Composition *composition,
        TriggerSegmentId id,
        int newPitch) :
        NamedCommand(tr("Set Base Pitch")),
        m_composition(composition),
        m_id(id),
        m_newPitch(newPitch),
        m_oldPitch( -1)
{
    // nothing
}

SetTriggerSegmentBasePitchCommand::~SetTriggerSegmentBasePitchCommand()
{
    // nothing
}

void
SetTriggerSegmentBasePitchCommand::execute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    if (m_oldPitch == -1) {
        m_oldPitch = rec->getBasePitch();
    }
    rec->setBasePitch(m_newPitch);
}

void
SetTriggerSegmentBasePitchCommand::unexecute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    rec->setBasePitch(m_oldPitch);
}

}
