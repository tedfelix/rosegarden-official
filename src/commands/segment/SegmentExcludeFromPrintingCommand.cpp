/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SegmentExcludeFromPrintingCommand]"

#include "SegmentExcludeFromPrintingCommand.h"


namespace Rosegarden
{


SegmentExcludeFromPrintingCommand::SegmentExcludeFromPrintingCommand(
        SegmentSelection &segments,
        bool exclude):
    NamedCommand(tr("Change Exclude From Printing")),
    m_newExcludeFromPrinting(exclude)
{
    // Copy Segment pointers from incoming std::set to our std::vector.
    for (Segment *segment : segments) {
        m_segments.push_back(segment);
    }
}

void
SegmentExcludeFromPrintingCommand::execute()
{
    // For each Segment
    for (size_t i = 0; i < m_segments.size(); ++i) {
        // Remember the old for unexecute().
        m_oldExcludeFromPrinting.push_back(m_segments[i]->getExcludeFromPrinting());
        // Switch to the new.
        m_segments[i]->setExcludeFromPrinting(m_newExcludeFromPrinting);
    }
}

void
SegmentExcludeFromPrintingCommand::unexecute()
{
    // For each Segment
    for (size_t i = 0; i < m_segments.size(); ++i) {
        // Restore the old.
        m_segments[i]->setExcludeFromPrinting(m_oldExcludeFromPrinting[i]);
    }
}


}
