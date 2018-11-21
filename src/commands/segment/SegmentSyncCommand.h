
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

#ifndef RG_SEGMENTSYNCCOMMAND_H
#define RG_SEGMENTSYNCCOMMAND_H

#include "document/Command.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/CommandHistory.h"

#include <QCoreApplication>

namespace Rosegarden
{

class Segment;


class SegmentSyncCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentSyncCommand)

public:
    SegmentSyncCommand(Segment &segment,
        int newTranspose, int lowRange, int highRange, const Clef& clef);

    SegmentSyncCommand(SegmentSelection selection,
            int newTranspose, int lowRange, int highRange, const Clef& clef);

    SegmentSyncCommand(std::vector<Segment *> segments,
            int newTranspose, int lowRange, int highRange, const Clef& clef);

    SegmentSyncCommand(SegmentMultiSet& segments, TrackId track,
            int newTranspose, int lowRange, int highRange, const Clef& clef);

    ~SegmentSyncCommand() override;

protected:
    void processSegment(Segment &segment, int newTranspose, int lowRange, int highRange, const Clef& clef);
};

}

#endif
