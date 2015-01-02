/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "IncrementDisplacementsCommand.h"

#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "base/BaseProperties.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

void
IncrementDisplacementsCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("fine_position_left",
         new ArgumentAndSelectionCommandBuilder<IncrementDisplacementsCommand>());
    r->registerCommand
        ("fine_position_right",
         new ArgumentAndSelectionCommandBuilder<IncrementDisplacementsCommand>());
    r->registerCommand
        ("fine_position_up",
         new ArgumentAndSelectionCommandBuilder<IncrementDisplacementsCommand>());
    r->registerCommand
        ("fine_position_down",
         new ArgumentAndSelectionCommandBuilder<IncrementDisplacementsCommand>());
}

QPoint
IncrementDisplacementsCommand::getArgument(QString actionName, CommandArgumentQuerier &)
{
    if (actionName == "fine_position_left") {
        return QPoint(-500,    0);
    }
    if (actionName == "fine_position_right") {
        return QPoint( 500,    0);
    }
    if (actionName == "fine_position_up") {
        return QPoint(   0, -500);
    }
    if (actionName == "fine_position_down") {
        return QPoint(   0,  500);
    }

    return QPoint(0, 0);
}

void
IncrementDisplacementsCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        long prevX = 0, prevY = 0;
        (*i)->get<Int>(DISPLACED_X, prevX);
        (*i)->get<Int>(DISPLACED_Y, prevY);
        (*i)->setMaybe<Int>(DISPLACED_X, prevX + long(m_dx));
        (*i)->setMaybe<Int>(DISPLACED_Y, prevY + long(m_dy));
    }
}



}
