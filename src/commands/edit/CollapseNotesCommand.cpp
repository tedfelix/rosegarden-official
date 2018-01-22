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

    // We also can't test events to find out whether they're still in
    // the segment or not, because the event comparator will crash if
    // an event has actually been deleted.

    // So, we maintain a set of the events we have already seen
    // (checking in this set by pointer comparison only, for safety)
    // and traverse the selection requesting a collapse for each event
    // that is not already in our seen set.  Each time a collapse is
    // requested, we fly back to the start of the selection -- this is
    // partly so we are sure to see any new events that may appear
    // during collapsing, and partly so that our active iterator is
    // always valid even if an event is deleted from the selection.

    QSet<Event *> seen;

    EventSelection::eventcontainer::iterator i =
        m_selection->getSegmentEvents().begin();

    while (i != m_selection->getSegmentEvents().end()) {

        Event *e = *i;

        if (!seen.contains(e)) {

            seen.insert(e);

            Segment::iterator collapsed =
                helper.collapseNoteAggressively(e, endTime);
            if (collapsed != s.end()) {
                m_selection->addEvent(*collapsed);
            }

            i = m_selection->getSegmentEvents().begin();
            continue;
        }

        ++i;
    }
    helper.makeNotesViable(m_selection->getStartTime(), endTime);
    
}

}
