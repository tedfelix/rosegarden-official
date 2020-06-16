
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

    ~MatrixPercussionInsertionCommand() override;

    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }
    
protected:
    void modifySegment() override;

    timeT getEffectiveStartTime(Segment &segment,
                                            timeT startTime,
                                            Event &event);

    /// Compute the end time for a percussion event.
    /**
     * We try to make percussion notes contiguous so that they will look
     * correct in notation.  Unfortunately, this wreaks some havoc on
     * the velocity ruler.  Perhaps the velocity ruler needs a percussion mode
     * where it would show each note as if it were a 64th?
     */
    timeT getEndTime(const Segment &segment,
                     timeT endTime,
                     const Event &event);

    Event *m_event;
    timeT m_time;
    Event *m_lastInsertedEvent; // an alias for another event
};


}

#endif
