// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "eventcommands.h"
#include "BaseProperties.h"

#include <klocale.h>
#include "rosestrings.h"
#include "rosedebug.h"

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::timeT;

EventInsertionCommand::EventInsertionCommand(Rosegarden::Segment &segment,
                                             timeT time,
                                             timeT endTime,
                                             Event *event) :
    BasicCommand(i18n("Insert Event"), segment, time, endTime),
    m_event(new Event(*event,
                      std::min(time, endTime),
                      (time < endTime) ? endTime - time : time - endTime))
{
    // nothing
}

EventInsertionCommand::~EventInsertionCommand()
{
    delete m_event;
    // don't want to delete m_lastInsertedEvent, it's just an alias
}

void EventInsertionCommand::modifySegment()
{
    m_lastInsertedEvent = new Event(*m_event);
    getSegment().insert(m_lastInsertedEvent);
}

