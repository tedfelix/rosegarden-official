/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CollapseNotesCommand.h"

#include "base/Event.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <QString>


namespace Rosegarden
{

void
CollapseNotesCommand::modifySegment()
{
    Segment &s(getSegment());
    SegmentNotationHelper helper(s);
    timeT endTime = getEndTime();

    // Because the selection tracks the segment as a SegmentObserver,
    // anything we do to the segment will also affect the selection.

    // And because collapsing a note may delete events before or after
    // it, we can't really iterate through the selection (or segment)
    // while we do it.

    // Instead we have to maintain a copy of all the candidate events,
    // and check that each one is still in the segment when we come to
    // collapse it.

    QList<Event *> candidates;

    foreach (Event *e, m_selection->getSegmentEvents()) {
        candidates.push_back(e);
    }

    foreach (Event *e, candidates) {
        if (s.findSingle(e) != s.end()) {
            Segment::iterator i = helper.collapseNoteAggressively(e, endTime);
            if (i != s.end()) {
                m_selection->addEvent(*i);
            }
        }
    }
}

}
