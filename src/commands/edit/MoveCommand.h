/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MOVECOMMAND_H
#define RG_MOVECOMMAND_H

#include "document/BasicCommand.h"
#include <QString>
#include <QCoreApplication>
#include "base/Event.h"

namespace Rosegarden
{

class Segment;
class EventSelection;
class Event;


class MoveCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MoveCommand)
public:
    MoveCommand(Segment &segment,
                timeT delta,
                bool useNotationTimings,
                EventSelection &selection);
    
    static QString getGlobalName(timeT delta = 0);

    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    timeT m_delta;
    bool m_useNotationTimings;
    Event *m_lastInsertedEvent;
};


}

#endif
