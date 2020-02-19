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
    m_active(false)
    //m_currentTime
{
    //RG_DEBUG << "iterator ctor";
}

// ++prefix
MEBIterator &
MEBIterator::operator++()
{
    int fill = m_mappedEventBuffer->size();
    if (m_index < fill)  ++m_index;
    return *this;
}

// postfix++
MEBIterator
MEBIterator::operator++(int)
{
    // This line is the main reason we need a copy ctor.
    MEBIterator r = *this;
    int fill = m_mappedEventBuffer->size();
    if (m_index < fill)  ++m_index;
    return r;
}

MEBIterator &
MEBIterator::operator+=(int offset)
{
    int fill = m_mappedEventBuffer->size();
    if (m_index + offset <= fill) {
        m_index += offset;
    } else {
        m_index = fill;
    }
    return *this;
}

MEBIterator &
MEBIterator::operator-=(int offset)
{
    if (m_index > offset)
        m_index -= offset;
    else
        m_index = 0;

    return *this;
}

bool
MEBIterator::operator==(const MEBIterator& it)
{
    return (m_index == it.m_index)  &&
           (m_mappedEventBuffer == it.m_mappedEventBuffer);
}

void
MEBIterator::reset()
{
    m_index = 0;
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

bool
MEBIterator::atEnd() const
{
    int size = m_mappedEventBuffer->size();
    return (m_index >= size);
}

void
MEBIterator::doInsert(MappedInserterBase &inserter, MappedEvent &evt)
{
    // Get time when note starts sounding, eg for finding the correct
    // controllers.  It can't simply be event time, because if we
    // jumped into the middle of a long note, we'd wrongly find the
    // controller values as they are at the time the note starts.
    if (evt.getEventTime() > m_currentTime)
        m_currentTime = evt.getEventTime();

    // Mapper does the actual insertion.
    getSegment()->doInsert(inserter, evt, m_currentTime, !isReady());
    setReady(true);
}

}

