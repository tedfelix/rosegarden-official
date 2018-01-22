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


#include "GraceCommand.h"

#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "document/BasicCommand.h"
#include "base/BaseProperties.h"
#include "document/CommandRegistry.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

/*!!!

void
GraceCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        (getGlobalName(), "group-grace", "", "grace",
         new SelectionCommandBuilder<GraceCommand>());
}

GraceCommand::GraceCommand(EventSelection &selection) :
        BasicCommand(getGlobalName(),
                     selection.getSegment(),
                     selection.getStartTime(),
                     getEffectiveEndTime(selection),
                     true),
        m_selection(&selection)
{}

timeT
GraceCommand::getEffectiveEndTime(EventSelection &
                                  selection)
{
    EventSelection::eventcontainer::iterator i =
        selection.getSegmentEvents().end();
    if (i == selection.getSegmentEvents().begin())
        return selection.getEndTime();
    --i;

    Segment::iterator si = selection.getSegment().findTime
                           ((*i)->getAbsoluteTime() + (*i)->getDuration());
    if (si == selection.getSegment().end())
        return selection.getEndTime();
    else
        return (*si)->getAbsoluteTime() + 1;
}

void
GraceCommand::modifySegment()
{
    Segment &s(getSegment());
    timeT startTime = getStartTime();
    timeT endOfLastGraceNote = startTime;
    int id = s.getNextId();

    // first turn the selected events into grace notes

    for (EventSelection::eventcontainer::iterator i =
                m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {
            (*i)->set<Bool>(IS_GRACE_NOTE, true);
            (*i)->set<Int>(BEAMED_GROUP_ID, id);
            (*i)->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_GRACE);
        }

        if ((*i)->getAbsoluteTime() + (*i)->getDuration() >
                endOfLastGraceNote) {
            endOfLastGraceNote =
                (*i)->getAbsoluteTime() + (*i)->getDuration();
        }
    }

    // then indicate that the following chord has grace notes

    Segment::iterator i0, i1;
    s.getTimeSlice(endOfLastGraceNote, i0, i1);

    while (i0 != i1 && i0 != s.end()) {
        if (!(*i0)->isa(Note::EventType)) {
            ++i0;
            continue;
        }
        (*i0)->set
        <Bool>(HAS_GRACE_NOTES, true);
        ++i0;
    }
}

*/

}
