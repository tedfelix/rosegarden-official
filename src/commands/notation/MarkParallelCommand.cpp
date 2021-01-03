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


/*
 * MarkParallelCommand.cpp
 *
 *  Created on: Mar 25, 2015
 *      Author: lambache
 */


#include "MarkParallelCommand.h"

#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "gui/editors/notation/NotationProperties.h"
#include "base/BaseProperties.h"
#include "document/BasicCommand.h"

#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

MarkParallelCommand::MarkParallelCommand(Segment &segment, timeT begin, timeT end) : BasicCommand(tr("Mark Parallel"), segment, begin, end)
{
    // nothing
}

//void
//MarkParallelCommand::registerCommand(CommandRegistry *r)
//{
//    r->registerCommand
//        ("mark_parallel",
//         new SelectionCommandBuilder<MarkParallelCommand>());
//}

void
MarkParallelCommand::modifySegment()
{
}

}

