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


#include "MatrixEraseCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentMatrixHelper.h"
#include "document/BasicCommand.h"


namespace Rosegarden
{

MatrixEraseCommand::MatrixEraseCommand(Segment &segment,
                                       Event *event) :
        BasicCommand(tr("Erase Note"),
                     segment,
                     event->getAbsoluteTime(),
                     event->getAbsoluteTime() + event->getDuration(),
                     true),
        m_event(event),
        m_relayoutEndTime(getEndTime())
{
    // nothing
}

timeT MatrixEraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

void MatrixEraseCommand::modifySegment()
{
    SegmentMatrixHelper helper(getSegment());

    std::string eventType = m_event->getType();

    if (eventType == Note::EventType) {

        helper.deleteNote(m_event, false);

    }
}

}
