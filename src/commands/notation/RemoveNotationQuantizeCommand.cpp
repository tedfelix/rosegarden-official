/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RemoveNotationQuantizeCommand.h"

#include "base/Event.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include <QString>


namespace Rosegarden
{

void
RemoveNotationQuantizeCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("remove_quantization",
         new SelectionCommandBuilder<RemoveNotationQuantizeCommand>());
}

void
RemoveNotationQuantizeCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    std::vector<Event *> toInsert;
    std::vector<Event *> toErase;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        toInsert.push_back(new Event(**i,
                                     (*i)->getAbsoluteTime(),
                                     (*i)->getDuration(),
                                     (*i)->getSubOrdering(),
                                     (*i)->getAbsoluteTime(),
                                     (*i)->getDuration()));

        if ((*i)->isa(Note::EventType) &&
            (*i)->getNotationDuration() > (*i)->getDuration()) {
            toInsert.push_back(new Event(Note::EventRestType,
                                         (*i)->getAbsoluteTime() + (*i)->getDuration(),
                                         (*i)->getNotationDuration() - (*i)->getDuration(),
                                         Note::EventRestSubOrdering));
        }

        toErase.push_back(*i);
    }

    for (std::vector<Event *>::iterator i = toErase.begin(); i != toErase.end();
            ++i) {
        m_selection->getSegment().eraseSingle(*i);
    }

    for (std::vector<Event *>::iterator i = toInsert.begin(); i != toInsert.end();
            ++i) {
        m_selection->getSegment().insert(*i);
    }
}

}
