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


#include "CollapseRestsCommand.h"

#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "document/BasicCommand.h"
#include "document/CommandRegistry.h"
#include <QString>


namespace Rosegarden
{

void
CollapseRestsCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("collapse_rests_aggressively",
         new SelectionCommandBuilder<CollapseRestsCommand>());
}

CollapseRestsCommand::CollapseRestsCommand
(EventSelection &selection) :
        BasicCommand(getGlobalName(),
                     selection.getSegment(),
                     selection.getStartTime(),
                     selection.getEndTime())
{
    // nothing else
}

void CollapseRestsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.collapseRestsAggressively(getStartTime(), getEndTime());
}

}
