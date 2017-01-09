
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTRESIZEFROMSTARTCOMMAND_H
#define RG_SEGMENTRESIZEFROMSTARTCOMMAND_H

#include "document/BasicCommand.h"
#include "base/Event.h"

#include <QCoreApplication>


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
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentResizeFromStartCommand)

public:
    SegmentResizeFromStartCommand(Segment *segment,
                                  timeT newStartTime);
    virtual ~SegmentResizeFromStartCommand();

    static QString getGlobalName() { return tr("Resize Segment"); }

protected:
    virtual void modifySegment();

private:
    Segment *m_segment;
    timeT m_oldStartTime;
    timeT m_newStartTime;
};



}

#endif
