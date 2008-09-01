/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PasteRangeCommand.h"

#include <klocale.h>
#include "AudioSegmentSplitCommand.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include "commands/edit/PasteSegmentsCommand.h"
#include "OpenOrCloseRangeCommand.h"
#include "PasteConductorDataCommand.h"
#include "SegmentSplitCommand.h"


namespace Rosegarden
{

PasteRangeCommand::PasteRangeCommand(Composition *composition,
                                     Clipboard *clipboard,
                                     timeT t0) :
        MacroCommand(i18n("Paste Range"))
{
    timeT clipBeginTime = clipboard->getBaseTime();

    timeT t1 = t0;

    if (clipboard->hasNominalRange()) {

        clipboard->getNominalRange(clipBeginTime, t1);
        t1 = t0 + (t1 - clipBeginTime);

    } else {

        timeT duration = 0;

        for (Clipboard::iterator i = clipboard->begin();
                i != clipboard->end(); ++i) {
            timeT durationHere = (*i)->getEndMarkerTime() - clipBeginTime;
            if (i == clipboard->begin() || durationHere > duration) {
                duration = durationHere;
            }
        }

        if (duration <= 0)
            return ;
        t1 = t0 + duration;
    }

    // Need to split segments before opening, at t0

    for (Composition::iterator i = composition->begin();
            i != composition->end(); ++i) {

        if ((*i)->getStartTime() >= t0 || (*i)->getEndMarkerTime() <= t0) {
            continue;
        }

        if ((*i)->getType() == Segment::Audio) {
            addCommand(new AudioSegmentSplitCommand(*i, t0));
        } else {
            addCommand(new SegmentSplitCommand(*i, t0));
        }
    }

    addCommand(new OpenOrCloseRangeCommand(composition, t0, t1, true));
    addCommand(new PasteSegmentsCommand
               (composition, clipboard, t0,
                composition->getTrackByPosition(0)->getId(),
                true));
    addCommand(new PasteConductorDataCommand(composition, clipboard, t0));
}

}
