/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RemoveFingeringMarksCommand.h"

#include "base/Selection.h"
#include "document/CommandRegistry.h"


namespace Rosegarden
{


void
RemoveFingeringMarksCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("remove_fingering_marks",
         new SelectionCommandBuilder<RemoveFingeringMarksCommand>());
}

void
RemoveFingeringMarksCommand::modifySegment()
{
    EventContainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        std::vector<Mark> marks = Marks::getMarks(**i);
        for (std::vector<Mark>::iterator j = marks.begin();
                j != marks.end(); ++j) {
            if (Marks::isFingeringMark(*j)) {
                Marks::removeMark(**i, *j);
            }
        }
    }
}


}
