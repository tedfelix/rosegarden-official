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


#include "SegmentSyncClefCommand.h"

#include "base/Selection.h"
#include "commands/notation/ClefInsertionCommand.h"

namespace Rosegarden
{
SegmentSyncClefCommand::SegmentSyncClefCommand(Segment &segment, const Clef& clef) :
        KMacroCommand(i18n("Sync segment clef"))
{
    processSegment(segment, clef);
}

void 
SegmentSyncClefCommand::processSegment(Segment &segment, const Clef& clef)
{
    KMacroCommand * macroCommand = this;

    // TODO delete it somewhere.
    EventSelection * wholeSegment = new EventSelection(segment, segment.getStartTime(), segment.getEndMarkerTime());

    EventSelection::eventcontainer::iterator i;

    for (i = wholeSegment->getSegmentEvents().begin();
        i != wholeSegment->getSegmentEvents().end(); ++i) {
        if ((*i)->isa(Rosegarden::Clef::EventType)) {
            macroCommand->addCommand
                    (new ClefInsertionCommand
                     (segment,
                       (*i)->getAbsoluteTime(),
                       clef, false, false));
        }
    }
        
}


SegmentSyncClefCommand::~SegmentSyncClefCommand()
{}


}
