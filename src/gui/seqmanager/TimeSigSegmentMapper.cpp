/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TimeSigSegmentMapper.h"

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

void
TimeSigSegmentMapper::dump()
{
    RealTime eventTime;

    Composition& comp = m_doc->getComposition();

    int index = 0;

    for (int i = 0; i < comp.getTimeSignatureCount(); ++i) {

        std::pair<timeT, TimeSignature> timeSigChange = comp.getTimeSignatureChange(i);

        eventTime = comp.getElapsedRealTime(timeSigChange.first);

        MappedEvent e;
        e.setType(MappedEvent::TimeSignature);
        e.setEventTime(eventTime);
        e.setData1(timeSigChange.second.getNumerator());
        e.setData2(timeSigChange.second.getDenominator());

        m_mapped->getBuffer()[index] = e;
        ++index;
    }

    m_mapped->setBufferFill(index);
}

int
TimeSigSegmentMapper::calculateSize()
{
    return m_doc->getComposition().getTimeSignatureCount();
}

}
