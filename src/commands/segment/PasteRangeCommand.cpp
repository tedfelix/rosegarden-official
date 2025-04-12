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


#include "PasteRangeCommand.h"

#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include "commands/edit/PasteSegmentsCommand.h"
#include "commands/segment/InsertRangeCommand.h"
//#include "OpenOrCloseRangeCommand.h"
#include "PasteConductorDataCommand.h"


namespace Rosegarden
{

PasteRangeCommand::PasteRangeCommand(Composition *composition,
                                     Clipboard *clipboard,
                                     timeT pasteTime) :
        MacroCommand(tr("Paste Range"))
{
    timeT clipBeginTime = clipboard->getBaseTime();

    // Compute t1, the end of the pasted range in the composition.

    timeT t1 = pasteTime;

    if (clipboard->hasNominalRange()) {

        // Use the clipboard's time range to compute t1.
        clipboard->getNominalRange(clipBeginTime, t1);
        t1 = pasteTime + (t1 - clipBeginTime);

    } else {

        timeT duration = 0;

        // For each segment in the clipboard, find the longest paste range
        // duration required.
        for (Clipboard::iterator i = clipboard->begin();
                i != clipboard->end(); ++i) {
            timeT durationHere = (*i)->getEndMarkerTime() - clipBeginTime;
            if (i == clipboard->begin() || durationHere > duration) {
                duration = durationHere;
            }
        }

        if (duration <= 0)
            return ;

        t1 = pasteTime + duration;
    }

    InsertRangeCommand::addInsertionCommands(this, composition,
                                             pasteTime, t1 - pasteTime);

    addCommand(new PasteSegmentsCommand
               (composition, clipboard, pasteTime,
                composition->getTrackByPosition(0)->getId(),
                true));
    addCommand(new PasteConductorDataCommand(composition, clipboard, pasteTime));
}

}
