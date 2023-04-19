/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTEXCLUDEFROMPRINTINGCOMMAND_H
#define RG_SEGMENTEXCLUDEFROMPRINTINGCOMMAND_H

#include "base/Selection.h"  // for SegmentSelection
#include "document/Command.h"  // for NamedCommand

#include <QCoreApplication>
#include <vector>


namespace Rosegarden
{


class Segment;

class SegmentExcludeFromPrintingCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentExcludeFromPrintingCommand)

public:
    SegmentExcludeFromPrintingCommand(
            SegmentSelection &segments,
			bool exclude);

    void execute() override;
    void unexecute() override;

private:

    /// Segments affected by the command.
    std::vector<Segment *> m_segments;

    bool m_newExcludeFromPrinting;

    /// One for each Segment affected by the command.
    std::vector<bool> m_oldExcludeFromPrinting;

};


}

#endif
