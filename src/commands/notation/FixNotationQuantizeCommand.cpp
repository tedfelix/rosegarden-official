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


#include "FixNotationQuantizeCommand.h"

#include "base/Event.h"
#include "base/Quantizer.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include <QString>


namespace Rosegarden
{

void
FixNotationQuantizeCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("fix_quantization",
         new SelectionCommandBuilder<FixNotationQuantizeCommand>());
}

void
FixNotationQuantizeCommand::modifySegment()
{
    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;
    Segment &segment(m_selection->getSegment());

    EventSelection::eventcontainer::iterator i;

    //!!! the Quantizer needs a fixQuantizedValues(EventSelection*)
    //method, but it hasn't got one yet so for the moment we're doing
    //this by hand.

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        timeT ud = (*i)->getDuration();
        timeT qt = (*i)->getNotationAbsoluteTime();
        timeT qd = (*i)->getNotationDuration();
        timeT maxDur = segment.getBarEndForTime(qt) - qt;
        timeT dur = (ud == qd ? Note::getNearestNote(qd).getDuration() : qd);

        if (dur > maxDur) {
            dur = maxDur;
        }

        toErase.push_back(*i);
        toInsert.push_back(new Event(**i, qt, dur));
    }

    for (size_t j = 0; j < toErase.size(); ++j) {
        Segment::iterator jtr(segment.findSingle(toErase[j]));
        if (jtr != segment.end())
            segment.erase(jtr);
    }

    for (size_t j = 0; j < toInsert.size(); ++j) {
        segment.insert(toInsert[j]);
    }

    segment.normalizeRests(segment.getStartTime(), segment.getEndTime());

    /*!!!
        Segment *segment(&m_selection->getSegment());
        m_quantizer->fixQuantizedValues
    	(segment,
    	 segment->findTime(m_selection->getStartTime()),
    	 segment->findTime(m_selection->getEndTime()));
    */

    //!!! normalizeRests?
}

}
