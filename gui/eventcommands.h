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

#ifndef _EVENT_COMMANDS_H_
#define _EVENT_COMMANDS_H_

#include "basiccommand.h"
#include "Segment.h"

class EventInsertionCommand : public BasicCommand
{
public:
    EventInsertionCommand(Rosegarden::Segment &segment,
                          Rosegarden::Event *event);

    virtual ~EventInsertionCommand();

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }
    
protected:
    virtual void modifySegment();

    Rosegarden::Event *m_event;
    Rosegarden::Event *m_lastInsertedEvent; // an alias for another event
};


#endif // _EVENT_COMMANDS_H_
