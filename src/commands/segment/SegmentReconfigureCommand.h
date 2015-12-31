
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

#ifndef RG_SEGMENTRECONFIGURECOMMAND_H
#define RG_SEGMENTRECONFIGURECOMMAND_H

#include "document/Command.h"
#include "base/Track.h"
#include "base/Event.h"

#include <QString>
#include <vector>

namespace Rosegarden
{


class Composition;
class Segment;

/// Move or resize segments, or change their tracks.
class SegmentReconfigureCommand : public NamedCommand
{
public:
    SegmentReconfigureCommand(QString name, Composition *composition);
    virtual ~SegmentReconfigureCommand();

    // rename: addChange()
    void addSegment(Segment *segment,
                    timeT newStartTime,
                    timeT newEndMarkerTime,
                    TrackId newTrack);

private:
    Composition *m_composition;
    timeT m_oldEndTime;

    struct Change {
        Segment *segment;
        timeT newStartTime;
        timeT newEndMarkerTime;
        TrackId newTrack;
    };
    typedef std::vector<Change> ChangeSet;

    // Before execute(), this contains the changes to be made.  After
    // execute(), this contains the original values for unexecute().
    ChangeSet m_changeSet;

    // Does the actual work of swapping the changes into the segments.
    timeT swap();
    
    // Command overrides
    void execute();
    void unexecute();

    // unused
//    void addSegments(const ChangeSet &changes);
};


}

#endif
