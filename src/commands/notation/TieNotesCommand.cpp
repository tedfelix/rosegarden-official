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

#include "TieNotesCommand.h"

#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "base/BaseProperties.h"
#include "document/CommandRegistry.h"


namespace Rosegarden
{


void
TieNotesCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand("tie_notes",
                       new SelectionCommandBuilder<TieNotesCommand>());
}

void
TieNotesCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    // Move part of this to SegmentNotationHelper?

    // For each event in the selection
    for (Event *event : m_selection->getSegmentEvents()) {

        Segment::iterator currNoteIter = segment.findSingle(event);
        Segment::iterator nextNoteIter;

        while ((nextNoteIter = helper.getNextAdjacentNote(currNoteIter, true, false)) !=
                segment.end()) {
            if (!m_selection->contains(*nextNoteIter))
                break;

            // Tie the current note to the next.
            (*currNoteIter)->set<Bool>(BaseProperties::TIED_FORWARD, true);
            (*currNoteIter)->unset(BaseProperties::TIE_IS_ABOVE);

            // Tie the next note to the current.
            (*nextNoteIter)->set<Bool>(BaseProperties::TIED_BACKWARD, true);

            currNoteIter = nextNoteIter;
        }
    }
}


}
