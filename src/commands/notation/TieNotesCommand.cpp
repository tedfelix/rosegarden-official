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


#include "TieNotesCommand.h"

#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "base/BaseProperties.h"
#include <qstring.h>


namespace Rosegarden
{

using namespace BaseProperties;

void
TieNotesCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    //!!! move part of this to SegmentNotationHelper?

    for (EventSelection::eventcontainer::iterator i =
                m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        //	bool tiedForward;
        //	if ((*i)->get<Bool>(TIED_FORWARD, tiedForward) && tiedForward) {
        //	    continue;
        //	}

        Segment::iterator si = segment.findSingle(*i);
        Segment::iterator sj;
        while ((sj = helper.getNextAdjacentNote(si, true, false)) !=
                segment.end()) {
            if (!m_selection->contains(*sj))
                break;
            (*si)->set
            <Bool>(TIED_FORWARD, true);
            (*sj)->set
            <Bool>(TIED_BACKWARD, true);
            si = sj;
        }
    }
}

}
