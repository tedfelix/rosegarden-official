
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTESMENUADDINDICATIONCOMMAND_H_
#define _RG_NOTESMENUADDINDICATIONCOMMAND_H_

#include "document/BasicCommand.h"
#include <string>
#include <qstring.h>
#include "base/Event.h"




namespace Rosegarden
{

class EventSelection;
class Event;


class AddIndicationCommand : public BasicCommand
{
public:
    AddIndicationCommand(std::string indicationType,
                                  EventSelection &selection);
    virtual ~AddIndicationCommand();

    // tests whether the indication can be added without overlapping
    // another one of the same type
    bool canExecute();

    Event *getLastInsertedEvent() {
        return m_lastInsertedEvent;
    }
    virtual timeT getRelayoutEndTime() {
        return getStartTime() + m_indicationDuration;
    }

    static QString getGlobalName(std::string indicationType);

protected:
    virtual void modifySegment();

    std::string m_indicationType;
    timeT m_indicationDuration;
    Event *m_lastInsertedEvent;
};
    


}

#endif
