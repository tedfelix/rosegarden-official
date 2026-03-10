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

#include "BeamCommand.h"

#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/CommandRegistry.h"
#include "base/BaseProperties.h"


namespace Rosegarden
{


void
BeamCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("beam",
         new SelectionCommandBuilder<BeamCommand>());
}

void
BeamCommand::modifySegment()
{
    // If the selection contains notes from a tupled group, we want to
    // preserve the tuplet.  Find the tupled group ID if any, and use
    // it as the shared group ID so that non-tupled notes join the same
    // group rather than a new one that would overwrite the tuplet type.
    int id = -1;
    for (EventContainer::iterator i =
                m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {
        std::string t;
        if ((*i)->get<String>(BaseProperties::BEAMED_GROUP_TYPE, t)  &&
            t == BaseProperties::GROUP_TYPE_TUPLED) {
            long tupledId = -1;
            (*i)->get<Int>(BaseProperties::BEAMED_GROUP_ID, tupledId);
            id = static_cast<int>(tupledId);
            break;
        }
    }
    if (id < 0)
        id = getSegment().getNextId();

    for (EventContainer::iterator i =
                m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {
            std::string t;
            if ((*i)->get<String>(BaseProperties::BEAMED_GROUP_TYPE, t)  &&
                t == BaseProperties::GROUP_TYPE_TUPLED) {
                // Preserve the tuplet group: reassign to the shared ID
                // but keep GROUP_TYPE_TUPLED so the bracket still renders.
                (*i)->set<Int>(BaseProperties::BEAMED_GROUP_ID, id);
            } else {
                (*i)->set<Int>(BaseProperties::BEAMED_GROUP_ID, id);
                (*i)->set<String>(BaseProperties::BEAMED_GROUP_TYPE,
                                  BaseProperties::GROUP_TYPE_BEAMED);
            }
        }
    }
}


}
