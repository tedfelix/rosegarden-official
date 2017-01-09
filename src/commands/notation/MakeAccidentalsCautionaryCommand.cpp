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


#include "MakeAccidentalsCautionaryCommand.h"

#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "gui/editors/notation/NotationProperties.h"
#include "document/CommandRegistry.h"
#include <QString>


namespace Rosegarden
{

void
MakeAccidentalsCautionaryCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("show_cautionary",
         new ArgumentAndSelectionCommandBuilder<MakeAccidentalsCautionaryCommand>());
    r->registerCommand
        ("cancel_cautionary",
         new ArgumentAndSelectionCommandBuilder<MakeAccidentalsCautionaryCommand>());
}

bool
MakeAccidentalsCautionaryCommand::getArgument(QString actionName, CommandArgumentQuerier &)
{
    if (actionName == "show_cautionary") return true;
    else return false;
}

QString
MakeAccidentalsCautionaryCommand::getGlobalName(bool cautionary)
{
    if (cautionary)
        return tr("Use &Cautionary Accidentals");
    else
        return tr("Cancel C&autionary Accidentals");
}

void
MakeAccidentalsCautionaryCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {
            if (m_cautionary) {
                (*i)->set
                <Bool>(NotationProperties::USE_CAUTIONARY_ACCIDENTAL,
                       true);
            } else {
                (*i)->unset(NotationProperties::USE_CAUTIONARY_ACCIDENTAL);
            }
        }
    }
}

}
