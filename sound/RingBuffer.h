// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <sys/types.h>
#include <sys/mman.h>

namespace Rosegarden
{

/**
 * RingBuffer implements a lock-free ring buffer for one writer and N
 * readers, that is to be used to store a sample type T.
 *
 * For efficiency, RingBuffer frequently initialises samples by
 * writing zeroes into their memory space, so T should normally be a
 * simple type that can safely be set to zero using memset.
 */

template <typename T, int N = 1>
class RingBuffer
{
public:
    /**
     * Create a ring buffer with room for n samples.
     */
    RingBuffer(size_t n);

    virtual ~RingBuffer();

    /**
     * Return the total capacity of the ring buffer in samples.
     * (This is the argument n passed to the constructor.)
     */
    size_t getSize() const;

    /**
     * Lock the ring buffer into physical memory.  Returns true
     * for success.
     */
    bool mlock();

    /**
     * Reset read and write pointers, thus emptying the buffer.
     */
    void reset();

    /**
     * Return the amount of data available for reading by reader R, in
     * samples.
     */
    size_t getReadSpace(int R = 0) const;

    /**
     * Return the amount of space available for writing, in samples.
     */
    size_t getWriteSpace() const;

    /**
     * Read n samples from the buffer, for reader R.  If fewer than n
     * are available, the remainder will be zeroed out.  Returns the
     * number of samples actually read.
     */
    size_t read(T *destination, size_t n, int R = 0);

    /**
     * Read n samples from the buffer, for reader R, adding them to
     * the destination.  If fewer than n are available, the remainder
     * will be left alone.  Returns the number of samples actually
     * read.
     */
    size_t readAdding(T *destination, size_t n, int R = 0);

    /**
     * Read one sample from the buffer, for reader R.  If no sample is
     * available, this will silently return zero.  Calling this
     * repeatedly is obviously slower than calling read once, but it
     * may be good enough if you don't want to allocate a buffer to
     * read into.
     */
    T readOne(int R = 0);

    /**
     * Pretend to read n samples from the buffer, for reader R,
     * without actually returning them (i.e. discard the next n
     * samples).  Returns the number of samples actually available for
     * discarding.
     */
    size_t skip(size_t n, int R = 0);

    /**
     * Write n samples to the buffer.  If insufficient space is
     * available, not all samples may actually be written.  Returns
     * the number of samples actually written.
     */
    size_t write(const T *source, size_t n);

    /**
     * Write n zero-value samples to the buffer.  If insufficient
     * space is available, not all zeros may actually be written.
     * Returns the number of zeroes actually written.
     */
    size_t zero(size_t n);

protected:
    T               *m_buffer;
    volatile size_t  m_writer;
    volatile size_t  m_readers[N];
    size_t           m_size;
    bool             m_mlocked;
};

template <typename T, int N>
RingBuffer<T, N>::RingBuffer(size_t n) :
    m_buffer(new T[n]),
    m_writer(0),
    m_size(n + 1),
    m_mlocked(false)
{
    for (int i = 0; i < N; ++i) m_readers[i] = 0;
}

template <typename T, int N>
RingBuffer<T, N>::~RingBuffer()
{
    if (m_mlocked) {
	munlock((void *)m_buffer, m_size);
    }
    delete[] m_buffer;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::getSize() const
{
    return m_size - 1;
}

template <typename T, int N>
bool
RingBuffer<T, N>::mlock()
{
    if (::mlock((void *)m_buffer, m_size)) return false;
    m_mlocked = true;
    return true;
}

template <typename T, int N>
void
RingBuffer<T, N>::reset()
{
    for (int i = 0; i < N; ++i) m_readers[i] = 0;
    m_writer = 0;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::getReadSpace(int R) const
{
    size_t writer = m_writer;
    size_t reader = m_readers[R];

    if (writer > reader) return writer - reader;
    else return ((writer + m_size) - reader) % m_size;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::getWriteSpace() const
{
    size_t space = 0;
    for (int i = 0; i < N; ++i) {
	size_t here = (m_readers[i] + m_size - m_writer - 1) % m_size;
	if (i == 0 || here < space) space = here;
    }
    return space;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::read(T *destination, size_t n, int R)
{
    size_t available = getReadSpace(R);
    if (n > available) {
	memset(destination + available, 0, (n - available) * sizeof(T));
	n = available;
    }
    if (n == 0) return n;

    size_t here = m_size - m_readers[R];
    if (here >= n) {
	memcpy(destination, m_buffer + m_readers[R], n * sizeof(T));
    } else {
	memcpy(destination, m_buffer + m_readers[R], here * sizeof(T));
	memcpy(destination + here, m_buffer, (n - here) * sizeof(T));
    }

    m_readers[R] = (m_readers[R] + n) % m_size;
    return n;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::readAdding(T *destination, size_t n, int R)
{
    size_t available = getReadSpace(R);
    if (n > available) n = available;
    if (n == 0) return n;

    size_t here = m_size - m_readers[R];

    if (here >= n) {
	for (size_t i = 0; i < n; ++i) {
	    destination[i] += (m_buffer + m_readers[R])[i];
	}
    } else {
	for (size_t i = 0; i < here; ++i) {
	    destination[i] += (m_buffer + m_readers[R])[i];
	}
	for (size_t i = 0; i < (n - here); ++i) {
	    destination[i + here] += m_buffer[i];
	}
    }

    m_readers[R] = (m_readers[R] + n) % m_size;
    return n;
}

template <typename T, int N>
T
RingBuffer<T, N>::readOne(int R)
{
    if (m_writer == m_readers[R]) return 0;
    T value = m_buffer[m_readers[R]];
    if (++m_readers[R] == m_size) m_readers[R] = 0;
    return value;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::skip(size_t n, int R)
{
    size_t available = getReadSpace(R);
    if (n > available) n = available;
    if (n == 0) return n;
    m_readers[R] = (m_readers[R] + n) % m_size;
    return n;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::write(const T *source, size_t n)
{
    size_t available = getWriteSpace();
    if (n > available) n = available;
    if (n == 0) return n;

    size_t here = m_size - m_writer;
    if (here >= n) {
	memcpy(m_buffer + m_writer, source, n * sizeof(T));
    } else {
	memcpy(m_buffer + m_writer, source, here * sizeof(T));
	memcpy(m_buffer, source + here, (n - here) * sizeof(T));
    }

    m_writer = (m_writer + n) % m_size;
    return n;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::zero(size_t n)
{
    size_t available = getWriteSpace();
    if (n > available) n = available;
    if (n == 0) return n;

    size_t here = m_size - m_writer;
    if (here >= n) {
	memset(m_buffer + m_writer, 0, n * sizeof(T));
    } else {
	memset(m_buffer + m_writer, 0, here * sizeof(T));
	memset(m_buffer, 0, (n - here) * sizeof(T));
    }

    m_writer = (m_writer + n) % m_size;
    return n;
}


}

#endif // _RINGBUFFER_H_
