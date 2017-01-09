/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddSlashesCommand.h"

#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "gui/editors/notation/NotationProperties.h"


namespace Rosegarden
{

void
AddSlashesCommand::registerCommand(CommandRegistry *r)
{
    static QString slashTitles[] = {
        tr("&None"), "&1", "&2", "&3", "&4", "&5"
    };

    for (int i = 0; i <= 5; ++i) {
        r->registerCommand
            (QString("slashes_%1").arg(i),
             new ArgumentAndSelectionCommandBuilder<AddSlashesCommand>());
    }
}

int
AddSlashesCommand::getArgument(QString actionName, CommandArgumentQuerier &)
{
    QString pfx("slashes_");
    if (actionName.startsWith(pfx)) {
        return actionName.right(actionName.length() - pfx.length()).toInt();
    }
    return 0;
}

void
AddSlashesCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if (m_number < 1) {
            (*i)->unset(NotationProperties::SLASHES);
        } else {
            (*i)->set<Int>(NotationProperties::SLASHES, m_number);
        }
    }
}

}
