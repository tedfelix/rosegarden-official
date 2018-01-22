
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

#ifndef RG_ERASEEVENTCOMMAND_H
#define RG_ERASEEVENTCOMMAND_H

#include "document/BasicCommand.h"
#include <string>
#include "base/Event.h"




namespace Rosegarden
{

class Segment;
class Event;


class EraseEventCommand : public BasicCommand
{
public:
    EraseEventCommand(Segment &segment,
                      Event *event,
                      bool collapseRest);
    virtual ~EraseEventCommand();

    virtual timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

    bool m_collapseRest;

    Event *m_event; // only used on 1st execute (cf bruteForceRedo)
    timeT m_relayoutEndTime;
    std::string makeName(std::string);
};



// Group menu commands



}

#endif
