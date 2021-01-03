/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RemoveMarksCommand.h"

#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <QString>
#include "document/CommandRegistry.h"
#include "base/BaseProperties.h"


namespace Rosegarden
{

using namespace BaseProperties;

void
RemoveMarksCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("remove_marks",
         new SelectionCommandBuilder<RemoveMarksCommand>());
}

void
RemoveMarksCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        long n = 0;
        (*i)->get
        <Int>(MARK_COUNT, n);
        (*i)->unset(MARK_COUNT);

        for (int j = 0; j < n; ++j) {
            (*i)->unset(getMarkPropertyName(j));
        }
    }
}

}
