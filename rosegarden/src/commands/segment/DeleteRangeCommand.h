/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
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


    class RejoinCommand : public KNamedCommand
    {
    public:
        // This command rejoins pairs of subsequent segment on the same
        // track. Segments pairs are defined using the addSegmentsPair
        // method.

        RejoinCommand() :
            KNamedCommand(i18n("Rejoin Command"))
            { }

        virtual ~RejoinCommand();

        // Add a pair of segments that will be jointed later
        void addSegmentsPair(Segment *s1, Segment *s2);

        void execute();
        void unexecute();


    private:
        Segment *m_segment;
        timeT m_endMarkerTime;
        Composition *m_composition;

        std::vector<SegmentJoinCommand *> m_rejoins;
    };
};



}

#endif
