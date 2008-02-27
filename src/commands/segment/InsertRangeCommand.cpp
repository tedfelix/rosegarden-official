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


#include "InsertRangeCommand.h"

#include <klocale.h>
#include "AudioSegmentSplitCommand.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include "OpenOrCloseRangeCommand.h"
#include "SegmentSplitCommand.h"


namespace Rosegarden
{

InsertRangeCommand::InsertRangeCommand(Composition *composition,
				       timeT t0, timeT duration) :
    KMacroCommand(i18n("Insert Range"))
{
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

    addCommand(new OpenOrCloseRangeCommand(composition, t0, t0 + duration, true));
}

}
