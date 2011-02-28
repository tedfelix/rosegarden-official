
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SEGMENTRECONFIGURECOMMAND_H_
#define _RG_SEGMENTRECONFIGURECOMMAND_H_

#include "base/Track.h"
#include "document/Command.h"
#include <QString>
#include <vector>
#include "base/Event.h"




namespace Rosegarden
{

class Segment;


/**
 * SegmentReconfigureCommand is a general-purpose command for
 * moving, resizing or changing the track of one or more segments
 */
class SegmentReconfigureCommand : public NamedCommand
{
public:
    SegmentReconfigureCommand(QString name);
    virtual ~SegmentReconfigureCommand();

    struct SegmentRec {
        Segment *segment;
        timeT startTime;
        timeT endMarkerTime;
        TrackId track;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;

    void addSegment(Segment *segment,
                    timeT startTime,
                    timeT endMarkerTime,
                    TrackId track);

    void addSegments(const SegmentRecSet &records);

    void execute();
    void unexecute();

private:
    void swap();
    
    SegmentRecSet m_records; 
};

}

#endif
