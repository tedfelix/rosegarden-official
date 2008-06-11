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

#ifndef _RG_SEGMENTSPLITTWICECOMMAND_H_
#define _RG_SEGMENTSPLITTWICECOMMAND_H_

#include <string>
#include <kcommand.h>
#include "base/Event.h"

#include "DeleteRangeCommand.h"




namespace Rosegarden
{

class Segment;
class Composition;

// Following command replaces two SegmentSplitCommand successive calls
// when cutting a range to fix bug #1961378 (crash when cutting a range))


class SegmentSplitTwiceCommand : public KNamedCommand
{
public:
    SegmentSplitTwiceCommand(Segment *segment,
                        timeT splitTime1, timeT splitTime2,
                        DeleteRangeCommand::RejoinCommand *rejoins = 0);
    virtual ~SegmentSplitTwiceCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Segment *m_segment;
    Segment *m_newSegmentA;
    Segment *m_newSegmentB;
    Segment *m_newSegmentC;
    timeT m_splitTime1;
    timeT m_splitTime2;
    bool m_detached;
    Composition *m_composition;
    DeleteRangeCommand::RejoinCommand *m_rejoins;
};


}

#endif
