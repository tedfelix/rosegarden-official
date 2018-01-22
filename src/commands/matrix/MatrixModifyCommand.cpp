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


#include "MatrixModifyCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "document/BasicCommand.h"


namespace Rosegarden
{

MatrixModifyCommand::MatrixModifyCommand(Segment &segment,
        Event *oldEvent,
        Event *newEvent,
        bool isMove,
        bool normalize):
        BasicCommand((isMove ? tr("Move Note") : tr("Modify Note")),
                     segment,
                     std::min(newEvent->getAbsoluteTime(),
                              oldEvent->getAbsoluteTime()),
                     std::max(oldEvent->getAbsoluteTime() +
                              oldEvent->getDuration(),
                              newEvent->getAbsoluteTime() +
                              newEvent->getDuration()),
                     true),
        m_normalize(normalize),
        m_oldEvent(oldEvent),
        m_newEvent(newEvent)
{}

void MatrixModifyCommand::modifySegment()
{
    std::string eventType = m_oldEvent->getType();

    if (eventType == Note::EventType) {

        timeT normalizeStart = std::min(m_newEvent->getAbsoluteTime(),
                                        m_oldEvent->getAbsoluteTime());

        timeT normalizeEnd = std::max(m_newEvent->getAbsoluteTime() +
                                      m_newEvent->getDuration(),
                                      m_oldEvent->getAbsoluteTime() +
                                      m_oldEvent->getDuration());

        Segment &segment(getSegment());
        segment.insert(m_newEvent);
        segment.eraseSingle(m_oldEvent);

        if (m_normalize) {
            segment.normalizeRests(normalizeStart, normalizeEnd);
        }
    }
}

}
