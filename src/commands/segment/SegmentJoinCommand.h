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

#ifndef RG_SEGMENTJOINCOMMAND_H
#define RG_SEGMENTJOINCOMMAND_H

#include "document/Command.h"
#include "base/Selection.h"

#include <QString>

#include <vector>


namespace Rosegarden
{


class Segment;

class SegmentJoinCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentJoinCommand);

public:
    explicit SegmentJoinCommand(SegmentSelection &segments);
    ~SegmentJoinCommand() override;

    static QString getGlobalName() { return tr("&Join"); }

    void execute() override;
    void unexecute() override;

    typedef std::vector<Segment *> SegmentVec;
    static Segment *makeSegment(SegmentVec oldSegments);

private:
    SegmentVec m_oldSegments;
    Segment *m_newSegment;

    bool m_oldIsDetached;
};


}

#endif
