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

#include "MappedEventBuffer.h"

#include "document/RosegardenDocument.h"
#include "misc/Debug.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInserterBase.h"

// #define DEBUG_MAPPED_EVENT_BUFFER 1

namespace Rosegarden
{

MappedEventBuffer::MappedEventBuffer(RosegardenDocument *doc) :
    m_size(0),
    m_fill(0),
    m_buffer(0),
    m_isMetronome(false),
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
        resizeBuffer(size);

        SEQMAN_DEBUG << "SegmentMapper::init : size = " << size
                     << endl;

        initSpecial();
        dump();
    } else {
        SEQMAN_DEBUG << "SegmentMapper::init : mmap size = 0 - skipping mmapping for now\n";
    }
}

bool
MappedEventBuffer::refresh()
{
    bool res = false;

    int newFill = calculateSize();
    int oldSize = getBufferSize();

#ifdef DEBUG_MAPPED_EVENT_BUFFER    
    SEQMAN_DEBUG << "SegmentMapper::refresh() - " << this
                 << " - old size = " << oldSize
                 << " - old fill = " << getBufferFill()
                 << " - new fill = " << newFill
                 << endl;
#endif    
    if (newFill > oldSize) {
        res = true;
        resizeBuffer(newFill);
    }

    dump();

    return res;
}

int
MappedEventBuffer::getBufferSize() const
{
    return m_size.fetchAndAddRelaxed(0);
}

int
MappedEventBuffer::getBufferFill() const
{
    return m_fill.fetchAndAddRelaxed(0);
}

void
MappedEventBuffer::resizeBuffer(int newSize)
{
    if (newSize <= getBufferSize()) return;

    MappedEvent *oldBuffer = m_buffer;
    MappedEvent *newBuffer = new MappedEvent[newSize];

    if (oldBuffer) {
        for (int i = 0; i < m_fill; ++i) {
            newBuffer[i] = m_buffer[i];
        }
    }
    {
        QWriteLocker locker(&m_lock);
        m_buffer = newBuffer;
        m_size.fetchAndStoreRelease(newSize);
    }

#ifdef DEBUG_MAPPED_EVENT_BUFFER
    SEQUENCER_DEBUG << "MappedEventBuffer::resizeBuffer: Resized to " << newSize << " events" << endl;
#endif
    delete[] oldBuffer;
}

void
MappedEventBuffer::setBufferFill(int newFill)
{
    m_fill.fetchAndStoreRelaxed(newFill);
}

void
MappedEventBuffer::
mapAnEvent(MappedEvent *e)
{
    if (getBufferFill() >= getBufferSize()) {
        // We need a bigger buffer.  We scale by 1.5, a compromise
        // between allocating too often and wasting too much space.
        // We also add 1 in case the space didn't increase due to
        // rounding.
        int newSize = 1 + float(getBufferSize()) * 1.5;
        resizeBuffer(newSize);
    }
        
    getBuffer()[getBufferFill()] = e;
    // Some mappers need this to be done now because they may resize
    // the buffer later, which will only copy the filled part.
    setBufferFill(getBufferFill() + 1);
}

void
MappedEventBuffer::
doInsert(MappedInserterBase &inserter, MappedEvent &evt,
         RealTime /*start*/, bool /*firstOutput*/)
{ inserter.insertCopy(evt); }

void
MappedEventBuffer::
addOwner(void)
{
    m_refCount++;
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
    m_refCount--;
    if (!m_refCount) { delete this; }
}

    /*** MappedEventBuffer::iterator ***/

MappedEventBuffer::iterator::iterator(MappedEventBuffer *s) :
    m_s(s), m_index(0), m_ready(false)
{
    s->addOwner();
}

MappedEventBuffer::iterator &
MappedEventBuffer::iterator::operator=(const iterator& it)
{
    if (&it == this) return *this;

    m_s     = it.m_s;
    m_index = it.m_index;
    m_ready = it.m_ready;
    return *this;
}

MappedEventBuffer::iterator &
MappedEventBuffer::iterator::operator++()
{
    int fill = m_s->getBufferFill();
    if (m_index < fill) ++m_index;
    return *this;
}

MappedEventBuffer::iterator
MappedEventBuffer::iterator::operator++(int)
{
    iterator r = *this;
    int fill = m_s->getBufferFill();
    if (m_index < fill) ++m_index;
    return r;
}

MappedEventBuffer::iterator &
MappedEventBuffer::iterator::operator+=(int offset)
{
    int fill = m_s->getBufferFill();
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
    if (m_index > offset) m_index -= offset;
    else m_index = 0;
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
    if (e) return *e;
    else return MappedEvent();
}

MappedEvent *
MappedEventBuffer::iterator::peek() const
{
    QReadLocker locker(&m_s->m_lock);
    if (m_index >= m_s->getBufferFill()) {
        return 0;
    }
    return &m_s->m_buffer[m_index];
}

bool
MappedEventBuffer::iterator::atEnd() const
{
    int fill = m_s->getBufferFill();
    return (m_index >= fill);
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
        { m_currentTime = evt.getEventTime(); }

    // Mapper does the actual insertion.
    getSegment()->doInsert(inserter, evt, m_currentTime, !getReady());
    setReady(true);
}

}
