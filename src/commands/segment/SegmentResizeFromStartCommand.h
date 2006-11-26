
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

#ifndef _RG_SEGMENTRESIZEFROMSTARTCOMMAND_H_
#define _RG_SEGMENTRESIZEFROMSTARTCOMMAND_H_

#include "document/BasicCommand.h"
#include "base/Event.h"




namespace Rosegarden
{

class Segment;


/**
 * SegmentResizeFromStartCommand moves the start time of a segment
 * leaving the events in it at the same absolute times as before, so
 * padding with rests or deleting events as appropriate.  (Distinct
 * from Segment::setStartTime, as used by SegmentReconfigureCommand,
 * which moves all the events in the segment.)
 * Not for use on audio segments (see AudioSegmentResizeFromStartCommand).
 */
class SegmentResizeFromStartCommand : public BasicCommand
{
public:
    SegmentResizeFromStartCommand(Segment *segment,
                                  timeT newStartTime);
    virtual ~SegmentResizeFromStartCommand();

protected:
    virtual void modifySegment();

private:
    Segment *m_segment;
    timeT m_oldStartTime;
    timeT m_newStartTime;
};



}

#endif
