/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SetTriggerSegmentDefaultRetuneCommand.h"

#include "base/Composition.h"
#include "base/TriggerSegment.h"
#include <QObject>


namespace Rosegarden
{

SetTriggerSegmentDefaultRetuneCommand::SetTriggerSegmentDefaultRetuneCommand(Composition *composition,
        TriggerSegmentId id,
        bool newDefaultRetune) :
        NamedCommand(tr("Set Default Retune")),
        m_composition(composition),
        m_id(id),
        m_newDefaultRetune(newDefaultRetune),
        m_oldDefaultRetune(false),
        m_haveOldDefaultRetune(false)
{
    // nothing
}

SetTriggerSegmentDefaultRetuneCommand::~SetTriggerSegmentDefaultRetuneCommand()
{
    // nothing
}

void
SetTriggerSegmentDefaultRetuneCommand::execute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    if (!m_haveOldDefaultRetune) {
        m_oldDefaultRetune = rec->getDefaultRetune();
    }
    rec->setDefaultRetune(m_newDefaultRetune);
}

void
SetTriggerSegmentDefaultRetuneCommand::unexecute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    rec->setDefaultRetune(m_oldDefaultRetune);
}

}
