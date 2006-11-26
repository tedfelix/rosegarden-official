
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_DELETERANGECOMMAND_H_
#define _RG_DELETERANGECOMMAND_H_

#include <kcommand.h>
#include "base/Event.h"
#include <klocale.h>

#include "SegmentJoinCommand.h"


namespace Rosegarden
{

class Segment;
class Composition;


class DeleteRangeCommand : public KMacroCommand
{
public:
    DeleteRangeCommand(Composition *composition,
                       timeT begin,
                       timeT end);
    virtual ~DeleteRangeCommand();

private:
    class RejoinCommand : public KNamedCommand
    {
    public:
        // This command rejoins s on to a subsequent segment on the same
        // track that ends at endMarkerTime (presumably the original end
        // marker time of s, with the range duration subtracted).

        RejoinCommand(Composition *c,
                      Segment *s,
                      timeT endMarkerTime) :
            KNamedCommand(i18n("Rejoin Command")),
            m_composition(c), m_segment(s), m_endMarkerTime(endMarkerTime),
            m_joinCommand(0) { }

        ~RejoinCommand() { delete m_joinCommand; }

        void execute();
        void unexecute() { if (m_joinCommand) m_joinCommand->unexecute(); }

    private:
        Composition *m_composition;
        Segment *m_segment;
        timeT m_endMarkerTime;

        SegmentJoinCommand *m_joinCommand;
    };
};



}

#endif
