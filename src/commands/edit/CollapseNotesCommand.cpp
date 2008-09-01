/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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
    SegmentNotationHelper helper(getSegment());
    timeT endTime = getEndTime();

    // This is really nasty stuff.  We can't go in forward direction
    // using the j-iterator trick because collapseNoteAggressively may
    // erase the following iterator as well as the preceding one.  We
    // can't go backward naively, because collapseNoteAggressively
    // erases i from the EventSelection now that it's a
    // SegmentObserver.  We need the fancy hybrid j-iterator-backward
    // technique applied to selections instead of segments.

    EventSelection::eventcontainer::iterator i =
        m_selection->getSegmentEvents().end();
    EventSelection::eventcontainer::iterator j = i;
    EventSelection::eventcontainer::iterator beg =
        m_selection->getSegmentEvents().begin();
    bool thisOne = false;

    while (i != beg && (!thisOne || (*i != *beg))) {

        --j;

        if (thisOne) {
            helper.collapseNoteAggressively(*i, endTime);
        }

        // rather than "true" one could perform a test to see
        // whether j pointed to a candidate for collapsing:
        thisOne = true;

        i = j;
    }

    if (thisOne) {
        helper.collapseNoteAggressively(*i, endTime);
    }
}

}
