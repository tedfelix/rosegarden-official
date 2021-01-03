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


#include "SetTriggerSegmentDefaultTimeAdjustCommand.h"

#include "base/Composition.h"
#include "base/TriggerSegment.h"
#include <QObject>


namespace Rosegarden
{

SetTriggerSegmentDefaultTimeAdjustCommand::SetTriggerSegmentDefaultTimeAdjustCommand(Composition *composition,
        TriggerSegmentId id,
        std::string newDefaultTimeAdjust) :
        NamedCommand(tr("Set Default Time Adjust")),
        m_composition(composition),
        m_id(id),
        m_newDefaultTimeAdjust(newDefaultTimeAdjust),
        m_oldDefaultTimeAdjust("")
{
    // nothing
}

SetTriggerSegmentDefaultTimeAdjustCommand::~SetTriggerSegmentDefaultTimeAdjustCommand()
{
    // nothing
}

void
SetTriggerSegmentDefaultTimeAdjustCommand::execute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    if (m_oldDefaultTimeAdjust == "") {
        m_oldDefaultTimeAdjust = rec->getDefaultTimeAdjust();
    }
    rec->setDefaultTimeAdjust(m_newDefaultTimeAdjust);
}

void
SetTriggerSegmentDefaultTimeAdjustCommand::unexecute()
{
    TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec)
        return ;
    rec->setDefaultTimeAdjust(m_oldDefaultTimeAdjust);
}

}
