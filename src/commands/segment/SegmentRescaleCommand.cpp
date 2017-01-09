/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentRescaleCommand.h"

#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <QString>


namespace Rosegarden
{

SegmentRescaleCommand::SegmentRescaleCommand(Segment *s,
                                             int multiplier,
                                             int divisor) :
    NamedCommand(getGlobalName()),
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
    NamedCommand(getGlobalName()),
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

    // If we didn't get a start time, assume use the segmetn start
    if (!m_startTimeGiven) {
        m_startTime = startTime;
    }

    if (!m_newSegment) {

        m_newSegment = new Segment();
        m_newSegment->setTrack(m_segment->getTrack());
        std::string label = m_segment->getLabel();
        m_newSegment->setLabel(appendLabel(
                label, qstrtostr(tr("(rescaled)"))));
        m_newSegment->setColourIndex(m_segment->getColourIndex());

        // Now entire contents (even hidden contents are resized
        // in segment.
        for (Segment::iterator i = m_segment->begin();
                  i != m_segment->end(); ++i) {
        // Changed condition from m_segment->isBeforeEndMarker(i).

              //&& Need to place rests in new segment otherwise rests at
              // beginning and end of segment are lost
              //
              // if ((*i)->isa(Note::EventRestType))
              //    continue;

            timeT dt = (*i)->getAbsoluteTime() - startTime;
            timeT duration = (*i)->getDuration();

            //!!! use doubles for this calculation where necessary

            m_newSegment->insert
            (new Event(**i,
                       m_startTime + rescale(dt),
                       rescale(duration)));
        }
    }

    m_segment->getComposition()->addSegment(m_newSegment);
    m_segment->getComposition()->detachSegment(m_segment);
    m_newSegment->normalizeRests(m_newSegment->getStartTime(),
                                 m_newSegment->getEndTime());

    m_newSegment->setEndMarkerTime
    (m_startTime + rescale(m_segment->getEndMarkerTime() - 
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
