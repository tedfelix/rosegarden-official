/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
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
#include "document/BasicSelectionCommand.h"
#include "base/BaseProperties.h"
#include "document/CommandRegistry.h"
#include "misc/Strings.h"
#include <qstring.h>


namespace Rosegarden
{

using namespace BaseProperties;

void
AddTextMarkCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        (getGlobalName(), "text-mark", "", "add_text_mark",
         new ArgumentAndSelectionCommandBuilder<AddTextMarkCommand>());
}

std::string
AddTextMarkCommand::getArgument(QString actionName, CommandArgumentQuerier &querier)
{
    bool ok = false;
    QString str = querier.getText(i18n("Text:"), &ok);
    if (ok) return qstrtostr(str);
    else throw CommandCancelled();
}

void
AddTextMarkCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        long n = 0;
        (*i)->get<Int>(MARK_COUNT, n);
        (*i)->set<Int>(MARK_COUNT, n + 1);
        (*i)->set<String>(getMarkPropertyName(n), Marks::getTextMark(m_text));
    }
}

}
