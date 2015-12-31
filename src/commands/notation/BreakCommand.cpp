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


#include "BreakCommand.h"

#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "gui/editors/notation/NotationProperties.h"
#include "base/BaseProperties.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

void
BreakCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("break_group",
         new SelectionCommandBuilder<BreakCommand>());
}

void
BreakCommand::modifySegment()
{
    for (EventSelection::eventcontainer::iterator i =
                m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        (*i)->unset(NotationProperties::BEAMED);
        (*i)->unset(BEAMED_GROUP_ID);
        (*i)->unset(BEAMED_GROUP_TYPE);
        (*i)->clearNonPersistentProperties();
    }
}

}
