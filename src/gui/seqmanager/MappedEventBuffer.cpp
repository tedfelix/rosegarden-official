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

#include "MappedEventBuffer.h"

#include "document/RosegardenDocument.h"
#include "misc/Debug.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInserterBase.h"

// #define DEBUG_MAPPED_EVENT_BUFFER 1

namespace Rosegarden
{

MappedEventBuffer::MappedEventBuffer(RosegardenDocument *doc) :
    m_buffer(0),
    m_capacity(0),
    m_size(0),
    m_doc(doc),
    m_start(RealTime::zeroTime),
    m_end(RealTime::beforeMaxTime),
    m_refCount(0)
{
}

MappedEventBuffer::~MappedEventBuffer()
{
    // Safe even if NULL.
    delete[] m_buffer;
}

void
MappedEventBuffer::init()
{
    int size = calculateSize();

    if (size > 0) {
        reserve(size);

        SEQMAN_DEBUG << "SegmentMapper::init : size = " << size
                     << endl;

        initSpecial();
        fillBuffer();
    } else {
        SEQMAN_DEBUG << "SegmentMapper::init : mmap size = 0 - skipping mmapping for now\n";
    }
}

bool
MappedEventBuffer::refresh()
{
    bool resized = false;

    int newFill = calculateSize();
    int oldSize = capacity();

#ifdef DEBUG_MAPPED_EVENT_BUFFER    
    SEQMAN_DEBUG << "SegmentMapper::refresh() - " << this
                 << " - old size = " << oldSize
                 << " - old fill = " << size()
                 << " - new fill = " << newFill
                 << endl;
#endif

    // If we need to expand the buffer to hold the events
    if (newFill > oldSize) {
        resized = true;
        reserve(newFill);
    }

    // Ask the deriver to fill the buffer from the document
    fillBuffer();

    return resized;
}

int
MappedEventBuffer::capacity() const
{
    return m_capacity.fetchAndAddRelaxed(0);
}

int
MappedEventBuffer::size() const
{
    return m_size.fetchAndAddRelaxed(0);
}

void
MappedEventBuffer::reserve(int newSize)
{
    if (newSize <= capacity())  return;

    MappedEvent *oldBuffer = m_buffer;
    MappedEvent *newBuffer = new MappedEvent[newSize];

    if (oldBuffer) {
        for (int i = 0; i < m_size.fetchAndAddRelaxed(0); ++i) {
            newBuffer[i] = m_buffer[i];
        }
    }

    {
        QWriteLocker locker(&m_lock);
        m_buffer = newBuffer;
        m_capacity.fetchAndStoreRelease(newSize);
    }

#ifdef DEBUG_MAPPED_EVENT_BUFFER
    SEQUENCER_DEBUG << "MappedEventBuffer::reserve: Resized to " << newSize << " events" << endl;
#endif

    delete[] oldBuffer;
}

void
MappedEventBuffer::resize(int newFill)
{
    m_size.fetchAndStoreRelaxed(newFill);
}

void
MappedEventBuffer::
mapAnEvent(MappedEvent *e)
{
    if (size() >= capacity()) {
        // We need a bigger buffer.  We scale by 1.5, a compromise
        // between allocating too often and wasting too much space.
        // We also add 1 in case the space didn't increase due to
        // rounding.
        int newSize = 1 + float(capacity()) * 1.5;
        reserve(newSize);
    }

    getBuffer()[size()] = e;
    // Some mappers need this to be done now because they may resize
    // the buffer later, which will only copy the filled part.
    resize(size() + 1);
}

void
MappedEventBuffer::
doInsert(MappedInserterBase &inserter, MappedEvent &evt,
         RealTime /*start*/, bool /*firstOutput*/)
{
    inserter.insertCopy(evt);
}

// The default doesn't have to do anything to get ready.
void
MappedEventBuffer::
makeReady(MappedInserterBase &/*inserter*/, RealTime /*time*/) {}

void
MappedEventBuffer::
addOwner(void)
{
    ++m_refCount;

#ifdef DEBUG_MAPPED_EVENT_BUFFER
    SEQUENCER_DEBUG << "MappedEventBuffer::addOwner"
                    << (void*)this
                    << "giving refcount"
                    << (int) m_refCount
                    << endl;
#endif
}

void
MappedEventBuffer::
removeOwner(void)
{
#ifdef DEBUG_MAPPED_EVENT_BUFFER
    SEQUENCER_DEBUG << "MappedEventBuffer::removeOwner"
                    << (void*)this
                    << "which had refcount"
                    << (int) m_refCount
                    << endl;
#endif

    --m_refCount;
    if (!m_refCount)  delete this;
}

    /*** MappedEventBuffer::iterator ***/

MappedEventBuffer::iterator::iterator(MappedEventBuffer *s) :
    m_s(s),
    m_index(0),
    m_ready(false),
    m_active(false)
    //m_currentTime
{
    s->addOwner();
}

// UNTESTED
MappedEventBuffer::iterator::iterator(const iterator &i) :
    m_s(i.m_s),
    m_index(i.m_index),
    m_ready(i.m_ready),
    m_active(i.m_active),
    m_currentTime(i.m_currentTime)
{
    // Add a reference count for this
    m_s->addOwner();
}

// UNTESTED
MappedEventBuffer::iterator &
MappedEventBuffer::iterator::operator=(const iterator& rhs)
{
    // Handle self-assignment gracefully.
    // With this particular class, the issue is that the call to
    // removeOwner() might cause the MappedEventBuffer to be deleted.
    // Then the following call to addOwner() would be on a deleted
    // object.
    if (&rhs == this)
        return *this;

    // Adjust reference count
    m_s->removeOwner();

    // Full bitwise copy
    m_s = rhs.m_s;
    m_index = rhs.m_index;
    m_ready = rhs.m_ready;
    m_active = rhs.m_active;
    m_currentTime = rhs.m_currentTime;

    // Adjust reference count
    m_s->addOwner();

    return *this;
}

// ++prefix
MappedEventBuffer::iterator &
MappedEventBuffer::iterator::operator++()
{
    int fill = m_s->size();
    if (m_index < fill)  ++m_index;
    return *this;
}

// postfix++
MappedEventBuffer::iterator
MappedEventBuffer::iterator::operator++(int)
{
    // This line is the main reason we need a copy ctor.
    iterator r = *this;
    int fill = m_s->size();
    if (m_index < fill)  ++m_index;
    return r;
}

MappedEventBuffer::iterator &
MappedEventBuffer::iterator::operator+=(int offset)
{
    int fill = m_s->size();
    if (m_index + offset <= fill) {
        m_index += offset;
    } else {
        m_index = fill;
    }
    return *this;
}

MappedEventBuffer::iterator &
MappedEventBuffer::iterator::operator-=(int offset)
{
    if (m_index > offset)
        m_index -= offset;
    else
        m_index = 0;

    return *this;
}

bool MappedEventBuffer::iterator::operator==(const iterator& it)
{
    return (m_index == it.m_index) && (m_s == it.m_s);
}

void MappedEventBuffer::iterator::reset()
{
    m_index = 0;
}

MappedEvent
MappedEventBuffer::iterator::operator*()
{
    const MappedEvent *e = peek();

    if (e)
        return *e;
    else
        return MappedEvent();
}

MappedEvent *
MappedEventBuffer::iterator::peek() const
{
    // The lock formerly here has moved out to callers.

    // If we're at the end, return NULL
    if (m_index >= m_s->size())
        return 0;

    // Otherwise return a pointer into the buffer.
    return &m_s->m_buffer[m_index];
}

bool
MappedEventBuffer::iterator::atEnd() const
{
    int size = m_s->size();
    return (m_index >= size);
}

void
MappedEventBuffer::iterator::
doInsert(MappedInserterBase &inserter, MappedEvent &evt)
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
