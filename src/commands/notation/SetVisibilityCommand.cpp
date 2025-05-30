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

#include "SetVisibilityCommand.h"

#include "base/Selection.h"
#include "base/BaseProperties.h"
#include "document/CommandRegistry.h"


namespace Rosegarden
{


void
SetVisibilityCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("make_visible",
         new ArgumentAndSelectionCommandBuilder<SetVisibilityCommand>());
    r->registerCommand
        ("make_invisible",
         new ArgumentAndSelectionCommandBuilder<SetVisibilityCommand>());
}

bool
SetVisibilityCommand::getArgument(const QString& actionName,
                                  CommandArgumentQuerier &)
{
    if (actionName == "make_visible") return true;
    else return false;
}

void
SetVisibilityCommand::modifySegment()
{
    EventContainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end(); ++i) {

        if (m_visible) {
            (*i)->unset(BaseProperties::INVISIBLE);
        } else {
            (*i)->set<Bool>(BaseProperties::INVISIBLE, true);
        }
    }
}


}
