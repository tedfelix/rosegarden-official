/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MatrixPercussionInsertionCommand]"

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
        m_event(nullptr),
        m_time(time),
        m_lastInsertedEvent(0)
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
MatrixPercussionInsertionCommand::getEndTime(
        const Segment &segment,
        timeT time,
        const Event &event)
{
    timeT endTime = time + Note(Note::Semibreve, 0).getDuration();

    // Expand to the bar end.
    const timeT barEndTime = segment.getBarEndForTime(time);
    if (barEndTime > endTime)
        endTime = barEndTime;

    // Limit to the Segment end.
    const timeT segmentEndTime = segment.getEndMarkerTime();
    if (endTime > segmentEndTime)
        endTime = segmentEndTime;

    // No pitch?  We've done enough.  Bail.
    // This should never happen as there is no way to insert a non-pitched
    // Event in the percussion matrix.  This is more to avoid an exception
    // from get<Int>(PITCH) if things go wrong.
    if (!event.has(PITCH))
        return endTime;

    const int pitch = event.get<Int>(PITCH);

    // For each Event in the Segment from time to the end of the Segment
    for (Segment::iterator i = segment.findTimeConst(time);
         segment.isBeforeEndMarker(i);
         ++i) {

        // Not a note?  Skip.
        if (!(*i)->isa(Note::EventType))
            continue;

        // No pitch?  Skip.
        if (!(*i)->has(PITCH))
            continue;

        // Wrong pitch?  Skip.
        if ((*i)->get<Int>(PITCH) != pitch)
            continue;

        // Same time?  Skip.
        if ((*i)->getAbsoluteTime() == time)
            continue;

        // This Event is after the time of the Event to be added and
        // has the same pitch.  Use the time of this Event as the end
        // time for the Event to be added.  This will avoid unnecessary
        // rests in the notation.
        endTime = (*i)->getAbsoluteTime();

        break;
    }

    const Composition *comp = segment.getComposition();
    std::pair<timeT, timeT> barRange = comp->getBarRangeForTime(time);
    const timeT barDuration = barRange.second - barRange.first;

    // Limit to bar duration
    if (endTime > time + barDuration)
        endTime = time + barDuration;

    return endTime;
}


}
