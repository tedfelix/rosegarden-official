/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COPYSEGMENTCOMMAND_H
#define RG_COPYSEGMENTCOMMAND_H

#include "base/TimeT.h"
#include "base/Track.h"
#include "document/Command.h"

namespace Rosegarden
{

/// Copy a segment to a new position (track and time)

class Segment;
class Composition;

class CopySegmentCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::CopySegmentCommand)

public:
    CopySegmentCommand(Composition *composition,
                       Segment* segment,
                       timeT startTime,
                       TrackId track,
                       bool CopyAsLink);

    ~CopySegmentCommand() override;

    static QString getGlobalName() { return tr("&CopySegment"); }

    void execute() override;
    void unexecute() override;

private:
    Composition *m_composition;
    Segment* m_segment;
    timeT m_startTime;
    TrackId m_track;
    bool m_copyAsLink;
    bool m_detached;
    timeT m_oldEndTime;
    Segment* m_addedSegment;
    bool m_originalSegmantisLinked;
};



}

#endif
