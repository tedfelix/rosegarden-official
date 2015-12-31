
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIXPERCUSSIONINSERTIONCOMMAND_H
#define RG_MATRIXPERCUSSIONINSERTIONCOMMAND_H

#include "document/BasicCommand.h"
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Event;


class MatrixPercussionInsertionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MatrixPercussionInsertionCommand)

public:
    MatrixPercussionInsertionCommand(Segment &segment,
                                     timeT time,
                                     Event *event);

    virtual ~MatrixPercussionInsertionCommand();

    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }
    
protected:
    virtual void modifySegment();

    timeT getEffectiveStartTime(Segment &segment,
                                            timeT startTime,
                                            Event &event);
    timeT getEndTime(Segment &segment,
                                 timeT endTime,
                                 Event &event);

    Event *m_event;
    timeT m_time;
    Event *m_lastInsertedEvent; // an alias for another event
};

//------------------------------


}

#endif
