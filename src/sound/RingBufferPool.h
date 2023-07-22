/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RingBuffer.h"

#include <vector>
#include <utility>

#include <pthread.h>


namespace Rosegarden
{


class RingBufferPool
{
public:
    typedef float sample_t;

    explicit RingBufferPool(size_t bufferSize);
    virtual ~RingBufferPool();

    /**
     * Set the default size for buffers.  Buffers currently allocated
     * will not be resized until they are returned.
     */
    void setBufferSize(size_t n);

    size_t getBufferSize() const
    {
        return m_bufferSize;
    }

    /**
     * Discard or create buffers as necessary so as to have n buffers
     * in the pool.  This will not discard any buffers that are
     * currently allocated, so if more than n are allocated, more than
     * n will remain.
     */
    void setPoolSize(size_t n);

    size_t getPoolSize() const
    {
        return m_buffers.size();
    }

    /**
     * Return true if n buffers available, false otherwise.
     */
    bool getBuffers(size_t n, RingBuffer<sample_t> **buffers);

    /**
     * Return a buffer to the pool.
     */
    void returnBuffer(RingBuffer<sample_t> *buffer);

protected:
    // Want to avoid memory allocation if possible when marking a buffer
    // unallocated or allocated, so we use a single container for all

    typedef std::pair<RingBuffer<sample_t> *, bool> AllocPair;
    typedef std::vector<AllocPair> AllocList;
    AllocList m_buffers;

    size_t m_bufferSize;
    size_t m_available;

    pthread_mutex_t m_lock;
};


}
