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

#include "AddTextMarkCommand.h"

#include "base/Selection.h"
#include "base/BaseProperties.h"
#include "document/CommandRegistry.h"
#include "misc/Strings.h"


namespace Rosegarden
{


void
AddTextMarkCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("add_text_mark",
         new ArgumentAndSelectionCommandBuilder<AddTextMarkCommand>());
}

std::string
AddTextMarkCommand::getArgument(QString /* actionName */, CommandArgumentQuerier &querier)
{
    bool ok = false;
    QString str = querier.getText(tr("Text:"), &ok);
    if (ok) return qstrtostr(str);
    else throw CommandCancelled();
}

void
AddTextMarkCommand::modifySegment()
{
    EventContainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        long n = 0;
        (*i)->get<Int>(BaseProperties::MARK_COUNT, n);
        (*i)->set<Int>(BaseProperties::MARK_COUNT, n + 1);
        (*i)->set<String>(BaseProperties::getMarkPropertyName(n),
                          Marks::getTextMark(m_text));
    }
}


}
