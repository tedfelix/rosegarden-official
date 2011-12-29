/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TempoSegmentMapper.h"

#include "base/Event.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenDocument.h"
#include "SegmentMapper.h"
#include "sound/MappedEvent.h"
#include "sound/MappedSegment.h"
#include "SpecialSegmentMapper.h"
#include <QString>


namespace Rosegarden
{

void TempoSegmentMapper::dump()
{
    RealTime eventTime;

    Composition& comp = m_doc->getComposition();

    int index = 0;

    for (int i = 0; i < comp.getTempoChangeCount(); ++i) {

        std::pair<timeT, tempoT> tempoChange = comp.getTempoChange(i);

        eventTime = comp.getElapsedRealTime(tempoChange.first);
        MappedEvent e;
        e.setType(MappedEvent::Tempo);
        e.setEventTime(eventTime);

        // Nasty hack -- we use the instrument ID to pass through the
        // raw tempo value, as it has the appropriate range (unlike
        // e.g. tempo1 + tempo2).  These events are not actually used
        // on the sequencer side yet, so this may change to something
        // nicer at some point.
        e.setInstrument(tempoChange.second);

        m_mapped->getBuffer()[index] = e;
        ++index;
    }

    m_mapped->setBufferFill(index);
}

int
TempoSegmentMapper::calculateSize()
{
    return m_doc->getComposition().getTempoChangeCount();
}

}
