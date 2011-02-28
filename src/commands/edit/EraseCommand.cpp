/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "EraseCommand.h"

#include "misc/Debug.h"
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <QString>


namespace Rosegarden
{

EraseCommand::EraseCommand(EventSelection &selection) :
        BasicSelectionCommand(getGlobalName(), selection, true),
        m_selection(&selection),
        m_relayoutEndTime(getEndTime())
{
    // nothing else
}

void
EraseCommand::modifySegment()
{
    RG_DEBUG << "EraseCommand::modifySegment" << endl;

    std::vector<Event *> toErase;
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Clef::EventType) ||
                (*i)->isa(Key ::EventType)) {
            m_relayoutEndTime = getSegment().getEndTime();
        }

        // We used to do this by calling SegmentNotationHelper::deleteEvent
        // on each event in the selection, but it's probably easier to
        // cope with general selections by deleting everything in the
        // selection and then normalizing the rests.  The deleteEvent
        // mechanism is still the more sensitive way to do it for single
        // events, and it's what's used by EraseEventCommand and thus
        // the notation eraser tool.

        toErase.push_back(*i);
    }

    for (size_t j = 0; j < toErase.size(); ++j) {
        getSegment().eraseSingle(toErase[j]);
    }

    getSegment().normalizeRests(getStartTime(), getEndTime());
}

timeT
EraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

}
