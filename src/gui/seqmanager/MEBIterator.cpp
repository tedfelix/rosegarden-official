/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MEBIterator]"

#include "MEBIterator.h"

#include "sound/MappedEvent.h"

namespace Rosegarden
{

MEBIterator::MEBIterator(
        QSharedPointer<MappedEventBuffer> mappedEventBuffer) :
    m_mappedEventBuffer(mappedEventBuffer),
    m_index(0),
    m_ready(false),
    m_active(false),
    m_currentTime()
{
    if (bug1560Logging())
        RG_DEBUG << "ctor: Clearing the m_ready flag.";
}

// ++prefix
MEBIterator &
MEBIterator::operator++()
{
    if (m_index < m_mappedEventBuffer->size())
        ++m_index;

    return *this;
}

void
MEBIterator::moveTo(const RealTime &time)
{
    // Rather than briefly unlock and immediately relock each
    // iteration, we leave the lock on until we're done.
    QReadLocker locker(getLock());

    // For each event from the current iterator position
    while (1) {
        if (atEnd())
            break;

        // We use peek because it's safe even if we have not fully
        // filled the buffer yet.  That means we can get nullptr.
        const MappedEvent *event = peek();

        // We know nothing about the event yet.  Stop here.
        if (!event)
            break;

#if 1
        // If the event sounds past time, stop here.
        // This will cause re-firing of events in progress.
        if (event->getEventTime() + event->getDuration() >= time)
            break;
#else
        // If the event starts on or after time, stop here.
        // This will cause events in progress to be skipped.
        if (event->getEventTime() >= time)
            break;
#endif

        operator++();
    }

    // Since we moved, we need to send a channel setup again.
    m_ready = false;

    if (bug1560Logging())
        RG_DEBUG << "moveTo(): done...  m_ready is" << m_ready;
}

MappedEvent
MEBIterator::operator*()
{
    const MappedEvent *e = peek();

    if (e)
        return *e;
    else
        return MappedEvent();
}

MappedEvent *
MEBIterator::peek() const
{
    // The lock formerly here has moved out to callers.

    // If we're at the end, return nullptr
    if (m_index >= m_mappedEventBuffer->size())
        return nullptr;

    // Otherwise return a pointer into the buffer.
    return &m_mappedEventBuffer->m_buffer[m_index];
}

void
MEBIterator::doInsert(MappedInserterBase &inserter, MappedEvent &event)
{
    // Get time when note starts sounding, eg for finding the correct
    // controllers.  It can't simply be event time, because if we
    // jumped into the middle of a long note, we'd wrongly find the
    // controller values as they are at the time the note starts.
    if (event.getEventTime() > m_currentTime)
        m_currentTime = event.getEventTime();

    // Mapper does the actual insertion.
    m_mappedEventBuffer->doInsert(
            inserter,
            event,
            m_currentTime,  // start
            !m_ready);  // firstOutput

    m_ready = true;
}


}
