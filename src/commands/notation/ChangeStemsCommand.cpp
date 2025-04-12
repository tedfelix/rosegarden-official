/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ChangeStemsCommand.h"

#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "gui/editors/notation/NotationProperties.h"
#include "document/CommandRegistry.h"

#include <QString>


namespace Rosegarden
{


void
ChangeStemsCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("stems_up",
         new ArgumentAndSelectionCommandBuilder<ChangeStemsCommand>());
    r->registerCommand
        ("stems_down",
         new ArgumentAndSelectionCommandBuilder<ChangeStemsCommand>());
}

bool
ChangeStemsCommand::getArgument(const QString& actionName, CommandArgumentQuerier &)
{
    if (actionName == "stems_up") return true;
    else return false;
}

void
ChangeStemsCommand::modifySegment()
{
    EventContainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {
            (*i)->set<Bool>(NotationProperties::STEM_UP, m_up);
        }
    }
}


}
