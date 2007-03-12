/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "SegmentRescaleCommand.h"

#include "misc/Strings.h"
#include "base/Event.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentRescaleCommand::SegmentRescaleCommand(Segment *s,
                                             int multiplier,
                                             int divisor) :
    KNamedCommand(getGlobalName()),
    m_segment(s),
    m_newSegment(0),
    m_startTimeGiven(false),
    m_startTime(s->getStartTime()),
    m_multiplier(multiplier),
    m_divisor(divisor),
    m_detached(false)
{
    // nothing
}

SegmentRescaleCommand::SegmentRescaleCommand(Segment *s,
                                             int multiplier,
                                             int divisor,
                                             timeT st) :
    KNamedCommand(getGlobalName()),
    m_segment(s),
    m_newSegment(0),
    m_startTimeGiven(true),
    m_startTime(st),
    m_multiplier(multiplier),
    m_divisor(divisor),
    m_detached(false)
{
    // nothing
}

SegmentRescaleCommand::~SegmentRescaleCommand()
{
    if (m_detached) {
        delete m_segment;
    } else {
        delete m_newSegment;
    }
}

timeT
SegmentRescaleCommand::rescale(timeT t)
{
    // avoid overflows by using doubles
    double d = t;
    d *= m_multiplier;
    d /= m_divisor;
    d += 0.5;
    return (timeT)d;
}

void
SegmentRescaleCommand::execute()
{
    timeT startTime = m_segment->getStartTime();

    if (m_startTimeGiven) startTime = m_startTime;

    if (!m_newSegment) {

        m_newSegment = new Segment();
        m_newSegment->setTrack(m_segment->getTrack());
        QString oldLabel = strtoqstr(m_segment->getLabel());
        if (oldLabel.endsWith(i18n("(rescaled)"))) {
            m_newSegment->setLabel(m_segment->getLabel());
        } else {
            m_newSegment->setLabel(qstrtostr(i18n("%1 (rescaled)").arg
                                             (oldLabel)));
        }
        m_newSegment->setColourIndex(m_segment->getColourIndex());

        for (Segment::iterator i = m_segment->begin();
                m_segment->isBeforeEndMarker(i); ++i) {

            if ((*i)->isa(Note::EventRestType))
                continue;

            timeT dt = (*i)->getAbsoluteTime() - startTime;
            timeT duration = (*i)->getDuration();

            //!!! use doubles for this calculation where necessary

            m_newSegment->insert
            (new Event(**i,
                       startTime + rescale(dt),
                       rescale(duration)));
        }
    }

    m_segment->getComposition()->addSegment(m_newSegment);
    m_segment->getComposition()->detachSegment(m_segment);
    m_newSegment->normalizeRests(m_newSegment->getStartTime(),
                                 m_newSegment->getEndTime());

    m_newSegment->setEndMarkerTime
    (startTime + rescale(m_segment->getEndMarkerTime() - 
                         m_segment->getStartTime()));

    m_detached = true;
}

void
SegmentRescaleCommand::unexecute()
{
    m_newSegment->getComposition()->addSegment(m_segment);
    m_newSegment->getComposition()->detachSegment(m_newSegment);
    m_detached = false;
}

}
