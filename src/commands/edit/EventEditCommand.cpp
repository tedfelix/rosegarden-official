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


#include "EventEditCommand.h"

#include "base/Event.h"
#include "base/Segment.h"
#include "document/BasicCommand.h"
#include <QString>


namespace Rosegarden
{

EventEditCommand::EventEditCommand(Segment &segment,
                                   Event *eventToModify,
                                   const Event &newEvent) :
        BasicCommand(getGlobalName(),
                     segment,
                     std::min(eventToModify->getAbsoluteTime(),
                              newEvent.getAbsoluteTime()),
                     std::max(eventToModify->getAbsoluteTime() +
                              eventToModify->getDuration(),
                              newEvent.getAbsoluteTime() +
                              newEvent.getDuration()),
                     true),  // bruteForceRedo
        m_oldEvent(eventToModify),
        m_newEvent(newEvent)
{
    // nothing else to see here
}

void
EventEditCommand::modifySegment()
{
    Segment &segment(getSegment());
    segment.eraseSingle(m_oldEvent);
    segment.insert(new Event(m_newEvent));
    segment.normalizeRests(getStartTime(), getEndTime());
}

}
