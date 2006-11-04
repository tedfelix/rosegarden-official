/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "TempoSegmentMmapper.h"

#include "base/Event.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenGUIDoc.h"
#include "SegmentMmapper.h"
#include "sound/MappedEvent.h"
#include "SpecialSegmentMmapper.h"
#include <qstring.h>


namespace Rosegarden
{

void TempoSegmentMmapper::dump()
{
    RealTime eventTime;

    Composition& comp = m_doc->getComposition();
    MappedEvent* bufPos = m_mmappedEventBuffer;

    for (int i = 0; i < comp.getTempoChangeCount(); ++i) {

        std::pair<timeT, tempoT> tempoChange = comp.getTempoChange(i);

        eventTime = comp.getElapsedRealTime(tempoChange.first);
        MappedEvent* mappedEvent = new (bufPos) MappedEvent();
        mappedEvent->setType(MappedEvent::Tempo);
        mappedEvent->setEventTime(eventTime);

        // Nasty hack -- we use the instrument ID to pass through the
        // raw tempo value, as it has the appropriate range (unlike
        // e.g. tempo1 + tempo2).  These events are not actually used
        // on the sequencer side yet, so this may change to something
        // nicer at some point.
        mappedEvent->setInstrument(tempoChange.second);

        ++bufPos;
    }

    // Store the number of events at the start of the shared memory region
    *(size_t *)m_mmappedRegion = (bufPos - m_mmappedEventBuffer);
}

size_t TempoSegmentMmapper::computeMmappedSize()
{
    return m_doc->getComposition().getTempoChangeCount() * sizeof(MappedEvent);
}

}
