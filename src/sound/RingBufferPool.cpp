/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RingBufferPool.h"


namespace Rosegarden
{


//#define DEBUG_RING_BUFFER_POOL 1

RingBufferPool::RingBufferPool(size_t bufferSize) :
    m_bufferSize(bufferSize),
    m_available(0)
{
    pthread_mutex_t initialisingMutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&m_lock, &initialisingMutex, sizeof(pthread_mutex_t));
}

RingBufferPool::~RingBufferPool()
{
    size_t allocatedCount = 0;
    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
        if (i->second)
            ++allocatedCount;
    }

    if (allocatedCount > 0) {
        std::cerr << "WARNING: RingBufferPool::~RingBufferPool: deleting pool with " << allocatedCount << " allocated buffers" << std::endl;
    }

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
        delete i->first;
    }

    m_buffers.clear();

    pthread_mutex_destroy(&m_lock);
}

void
RingBufferPool::setBufferSize(size_t n)
{
    if (m_bufferSize == n)
        return ;

    pthread_mutex_lock(&m_lock);

#ifdef DEBUG_RING_BUFFER_POOL

    std::cerr << "RingBufferPool::setBufferSize: from " << m_bufferSize
              << " to " << n << std::endl;
    int c = 0;
#endif

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
        if (!i->second) {
            delete i->first;
            i->first = new RingBuffer<sample_t>(n);
#ifdef DEBUG_RING_BUFFER_POOL

            std::cerr << "Resized buffer " << c++ << std::endl;
#endif

        } else {
#ifdef DEBUG_RING_BUFFER_POOL
            std::cerr << "Buffer " << c++ << " is already in use, resizing in place" << std::endl;
#endif

            i->first->resize(n);
        }
    }

    m_bufferSize = n;
    pthread_mutex_unlock(&m_lock);
}

void
RingBufferPool::setPoolSize(size_t n)
{
    pthread_mutex_lock(&m_lock);

#ifdef DEBUG_RING_BUFFER_POOL

    std::cerr << "RingBufferPool::setPoolSize: from " << m_buffers.size()
              << " to " << n << std::endl;
#endif

    size_t allocatedCount = 0, count = 0;

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
        if (i->second)
            ++allocatedCount;
        ++count;
    }

    if (count > n) {
        for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ) {
            if (!i->second) {
                delete i->first;
                m_buffers.erase(i);
                if (--count == n)
                    break;
            } else {
                ++i;
            }
        }
    }

    while (count < n) {
        m_buffers.push_back(AllocPair(new RingBuffer<sample_t>(m_bufferSize),
                                      false));
        ++count;
    }

    m_available = std::max(allocatedCount, n) - allocatedCount;

#ifdef DEBUG_RING_BUFFER_POOL

    std::cerr << "RingBufferPool::setPoolSize: have " << m_buffers.size()
              << " buffers (" << allocatedCount << " allocated, " << m_available << " available)" << std::endl;
#endif

    pthread_mutex_unlock(&m_lock);
}

bool
RingBufferPool::getBuffers(size_t n, RingBuffer<sample_t> **buffers)
{
    pthread_mutex_lock(&m_lock);

    size_t count = 0;

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
        if (!i->second && ++count == n)
            break;
    }

    if (count < n) {
#ifdef DEBUG_RING_BUFFER_POOL
        std::cerr << "RingBufferPool::getBuffers(" << n << "): not available (in pool of " << m_buffers.size() << "), resizing" << std::endl;
#endif

        AllocList newBuffers;

        while (count < n) {
            for (size_t i = 0; i < m_buffers.size(); ++i) {
                newBuffers.push_back(m_buffers[i]);
            }
            for (size_t i = 0; i < m_buffers.size(); ++i) {
                newBuffers.push_back(AllocPair(new RingBuffer<sample_t>(m_bufferSize),
                                               false));
            }
            count += m_buffers.size();
            m_available += m_buffers.size();
        }

        m_buffers = newBuffers;
    }

    count = 0;

#ifdef DEBUG_RING_BUFFER_POOL

    std::cerr << "RingBufferPool::getBuffers(" << n << "): available" << std::endl;
#endif

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
        if (!i->second) {
            i->second = true;
            i->first->reset();
            i->first->mlock();
            buffers[count] = i->first;
            --m_available;
            if (++count == n)
                break;
        }
    }

#ifdef DEBUG_RING_BUFFER_POOL
    std::cerr << "RingBufferPool::getBuffers: " << m_available << " remain in pool of " << m_buffers.size() << std::endl;
#endif

    pthread_mutex_unlock(&m_lock);
    return true;
}

void
RingBufferPool::returnBuffer(RingBuffer<sample_t> *buffer)
{
    pthread_mutex_lock(&m_lock);

#ifdef DEBUG_RING_BUFFER_POOL

    std::cerr << "RingBufferPool::returnBuffer" << std::endl;
#endif

    buffer->munlock();

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
        if (i->first == buffer) {
            i->second = false;
            ++m_available;
            if (buffer->getSize() != m_bufferSize) {
                delete buffer;
                i->first = new RingBuffer<sample_t>(m_bufferSize);
            }
        }
    }

#ifdef DEBUG_RING_BUFFER_POOL
    std::cerr << "RingBufferPool::returnBuffer: " << m_available << " remain in pool of " << m_buffers.size() << std::endl;
#endif

    pthread_mutex_unlock(&m_lock);
}


}
