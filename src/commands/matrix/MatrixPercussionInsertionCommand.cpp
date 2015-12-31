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


#include "MatrixPercussionInsertionCommand.h"

#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentMatrixHelper.h"
#include "document/BasicCommand.h"
#include "base/BaseProperties.h"
#include "misc/Debug.h"


namespace Rosegarden
{
using namespace BaseProperties;


MatrixPercussionInsertionCommand::MatrixPercussionInsertionCommand(Segment &segment,
        timeT time,
        Event *event) :
        BasicCommand(tr("Insert Percussion Note"), segment,
                     getEffectiveStartTime(segment, time, *event),
                     getEndTime(segment, time, *event)),
        m_event(0),
        m_time(time)
{
    timeT endTime = getEndTime(segment, time, *event);
    m_event = new Event(*event, time, endTime - time);
}

MatrixPercussionInsertionCommand::~MatrixPercussionInsertionCommand()
{
    delete m_event;
    // don't want to delete m_lastInsertedEvent, it's just an alias
}

void MatrixPercussionInsertionCommand::modifySegment()
{
    MATRIX_DEBUG << "MatrixPercussionInsertionCommand::modifySegment()\n";

    if (!m_event->has(VELOCITY)) {
        m_event->set
        <Int>(VELOCITY, 100);
    }

    Segment &s = getSegment();

    Segment::iterator i = s.findTime(m_time);

    int pitch = 0;
    if (m_event->has(PITCH)) {
        pitch = m_event->get
                <Int>(PITCH);
    }

    while (i != s.begin()) {

        --i;

        if ((*i)->getAbsoluteTime() < m_time &&
                (*i)->isa(Note::EventType)) {

            if ((*i)->has(PITCH) &&
                    (*i)->get
                    <Int>(PITCH) == pitch) {

                if ((*i)->getAbsoluteTime() + (*i)->getDuration() > m_time) {
                    Event *newPrevious = new Event
                                         (**i, (*i)->getAbsoluteTime(), m_time - (*i)->getAbsoluteTime());
                    s.erase(i);
                    i = s.insert(newPrevious);
                } else {
                    break;
                }
            }
        }
    }

    SegmentMatrixHelper helper(s);
    m_lastInsertedEvent = new Event(*m_event);
    helper.insertNote(m_lastInsertedEvent);
}

timeT
MatrixPercussionInsertionCommand::getEffectiveStartTime(Segment &segment,
        timeT time,
        Event &event)
{
    timeT startTime = time;

    int pitch = 0;
    if (event.has(PITCH)) {
        pitch = event.get<Int>(PITCH);
    }

    Segment::iterator i = segment.findTime(time);
    while (i != segment.begin()) {
        --i;

        if ((*i)->has(PITCH) &&
                (*i)->get
                <Int>(PITCH) == pitch) {

            if ((*i)->getAbsoluteTime() < time &&
                    (*i)->isa(Note::EventType)) {
                if ((*i)->getAbsoluteTime() + (*i)->getDuration() > time) {
                    startTime = (*i)->getAbsoluteTime();
                } else {
                    break;
                }
            }
        }
    }

    return startTime;
}

timeT
MatrixPercussionInsertionCommand::getEndTime(Segment &segment,
        timeT time,
        Event &event)
{
    timeT endTime =
        time + Note(Note::Semibreve,
                    0).getDuration();
    timeT barEndTime = segment.getBarEndForTime(time);
    timeT segmentEndTime = segment.getEndMarkerTime();

    if (barEndTime > endTime)
        endTime = barEndTime;
    if (endTime > segmentEndTime)
        endTime = segmentEndTime;

    int pitch = 0;
    if (event.has(PITCH)) {
        pitch = event.get<Int>(PITCH);
    }

    for (Segment::iterator i = segment.findTime(time);
            segment.isBeforeEndMarker(i); ++i) {

        if ((*i)->has(PITCH) &&
                (*i)->get
                <Int>(PITCH) == pitch) {

            if ((*i)->getAbsoluteTime() > time &&
                    (*i)->isa(Note::EventType)) {
                endTime = (*i)->getAbsoluteTime();
            }
        }
    }

    Composition *comp = segment.getComposition();
    std::pair<timeT, timeT> barRange =
        comp->getBarRangeForTime(time);
    timeT barDuration = barRange.second - barRange.first;


    if (endTime > time + barDuration) {
        endTime = time + barDuration;
    }

    return endTime;
}

}
