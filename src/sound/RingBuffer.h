/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_RINGBUFFER_H
#define RG_RINGBUFFER_H

#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>

#include "Scavenger.h"

//#define DEBUG_RINGBUFFER 1
//#define DEBUG_RINGBUFFER_CREATE_DESTROY 1

#ifdef DEBUG_RINGBUFFER
#define DEBUG_RINGBUFFER_CREATE_DESTROY 1
#endif

#ifdef DEBUG_RINGBUFFER_CREATE_DESTROY
#include <iostream>
static int extant_ringbuffers = 0;
#endif

#ifdef sun
#define MLOCK_TAKES_CHAR_PTR 1
#endif

namespace Rosegarden {

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
     * Create a ring buffer with room to write n samples.
     *
     * Note that the internal storage size will actually be n+1
     * samples, as one element is unavailable for administrative
     * reasons.  Since the ring buffer performs best if its size is a
     * power of two, this means n should ideally be some power of two
     * minus one.
     */
    RingBuffer(size_t n);

    virtual ~RingBuffer();

    /**
     * Return the total capacity of the ring buffer in samples.
     * (This is the argument n passed to the constructor.)
     */
    size_t getSize() const;

    /**
     * Resize the ring buffer.  This also empties it.  Actually swaps
     * in a new, larger buffer; the old buffer is scavenged after a
     * seemly delay.  Should be called from the write thread.
     */
    void resize(size_t newSize);

    /**
     * Return a new ring buffer (allocated with "new" -- called must
     * delete when no longer needed) of the given size, containing the
     * same data as this one.  If another thread reads from or writes
     * to this buffer during the call, the results may be incomplete
     * or inconsistent.  If this buffer's data will not fit in the new
     * size, the contents are undefined.
     */
    RingBuffer<T, N> *resized(int newSize, int R = 0) const;

    /**
     * Lock the ring buffer into physical memory.  Returns true
     * for success.
     */
    bool mlock();

    /**
     * Unlock the ring buffer from physical memory.  Returns true for
     * success.
     */
    bool munlock();

    /**
     * Reset read and write pointers, thus emptying the buffer.
     * Should be called from the write thread.
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
     * Read n samples from the buffer, if available, for reader R,
     * without advancing the read pointer -- i.e. a subsequent read()
     * or skip() will be necessary to empty the buffer.  If fewer than
     * n are available, the remainder will be zeroed out.  Returns the
     * number of samples actually read.
     */
    size_t peek(T *destination, size_t n, int R = 0) const;

    /**
     * Read one sample from the buffer, if available, without
     * advancing the read pointer -- i.e. a subsequent read() or
     * skip() will be necessary to empty the buffer.  Returns zero if
     * no sample was available.
     */
    T peek(int R = 0) const;

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

    static Scavenger<ScavengerArrayWrapper<T> > m_scavenger;

private:
    RingBuffer(const RingBuffer &); // not provided
    RingBuffer &operator=(const RingBuffer &); // not provided
};

template <typename T, int N>
Scavenger<ScavengerArrayWrapper<T> > RingBuffer<T, N>::m_scavenger;

template <typename T, int N>
RingBuffer<T, N>::RingBuffer(size_t n) :
    m_buffer(new T[n + 1]),
    m_writer(0),
    m_size(n + 1),
    m_mlocked(false)
{
#ifdef DEBUG_RINGBUFFER_CREATE_DESTROY
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::RingBuffer(" << n << ") [now have " << (++extant_ringbuffers) << "]" << std::endl;
#endif

    for (int i = 0; i < N; ++i) m_readers[i] = 0;

    m_scavenger.scavenge();
}

template <typename T, int N>
RingBuffer<T, N>::~RingBuffer()
{
#ifdef DEBUG_RINGBUFFER_CREATE_DESTROY
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::~RingBuffer [now have " << (--extant_ringbuffers) << "]" << std::endl;
#endif

    if (m_mlocked) {
#ifdef MLOCK_TAKES_CHAR_PTR
        ::munlock((char *)m_buffer, m_size * sizeof(T));
#else
        ::munlock((void *)m_buffer, m_size * sizeof(T));
#endif
    }
    delete[] m_buffer;

    m_scavenger.scavenge();
}

template <typename T, int N>
size_t
RingBuffer<T, N>::getSize() const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::getSize(): " << m_size-1 << std::endl;
#endif

    return m_size - 1;
}

template <typename T, int N>
void
RingBuffer<T, N>::resize(size_t newSize)
{
#ifdef DEBUG_RINGBUFFER_CREATE_DESTROY
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::resize(" << newSize << ")" << std::endl;
#endif

    m_scavenger.scavenge();

    if (m_mlocked) {
#ifdef MLOCK_TAKES_CHAR_PTR
        ::munlock((char *)m_buffer, m_size * sizeof(T));
#else
        ::munlock((void *)m_buffer, m_size * sizeof(T));
#endif
    }

    m_scavenger.claim(new ScavengerArrayWrapper<T>(m_buffer));

    reset();
    m_buffer = new T[newSize + 1];
    m_size = newSize + 1;

    if (m_mlocked) {
#ifdef MLOCK_TAKES_CHAR_PTR
        if (::mlock((char *)m_buffer, m_size * sizeof(T))) {
#else
        if (::mlock((void *)m_buffer, m_size * sizeof(T))) {
#endif
            m_mlocked = false;
        }
    }
}

template <typename T, int N>
RingBuffer<T, N> *
RingBuffer<T, N>::resized(int newSize, int R) const
{
    RingBuffer<T, N> *newBuffer = new RingBuffer<T, N>(newSize);

    int w = m_writer;
    int r = m_readers[R];

    while (r != w) {
        T value = m_buffer[r];
        newBuffer->write(&value, 1);
        if (++r == (int)m_size) r = 0;
    }

    return newBuffer;
}

template <typename T, int N>
bool
RingBuffer<T, N>::mlock()
{
#ifdef MLOCK_TAKES_CHAR_PTR
    if (::mlock((char *)m_buffer, m_size * sizeof(T))) return false;
#else
    if (::mlock((void *)m_buffer, m_size * sizeof(T))) return false;
#endif
    m_mlocked = true;
    return true;
}

template <typename T, int N>
bool
RingBuffer<T, N>::munlock()
{
#ifdef MLOCK_TAKES_CHAR_PTR
    if (::munlock((char *)m_buffer, m_size * sizeof(T))) return false;
#else
    if (::munlock((void *)m_buffer, m_size * sizeof(T))) return false;
#endif
    m_mlocked = false;
    return true;
}

template <typename T, int N>
void
RingBuffer<T, N>::reset()
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::reset" << std::endl;
#endif

    m_writer = 0;
    for (int i = 0; i < N; ++i) m_readers[i] = 0;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::getReadSpace(int R) const
{
    size_t writer = m_writer;
    size_t reader = m_readers[R];
    size_t space = 0;

    if (writer > reader) space = writer - reader;
    else space = ((writer + m_size) - reader) % m_size;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::getReadSpace(" << R << "): " << space << std::endl;
#endif

    return space;
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

#ifdef DEBUG_RINGBUFFER
    size_t rs(getReadSpace()), rp(m_readers[0]);

    std::cerr << "RingBuffer: write space " << space << ", read space "
              << rs << ", total " << (space + rs) << ", m_size " << m_size << std::endl;
    std::cerr << "RingBuffer: reader " << rp << ", writer " << m_writer << std::endl;
#endif

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::getWriteSpace(): " << space << std::endl;
#endif

    return space;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::read(T *destination, size_t n, int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::read(dest, " << n << ", " << R << ")" << std::endl;
#endif

    size_t available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only " << available << " samples available"
                  << std::endl;
#endif
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

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::read: read " << n << ", reader now " << m_readers[R] << std::endl;
#endif

    return n;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::readAdding(T *destination, size_t n, int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::readAdding(dest, " << n << ", " << R << ")" << std::endl;
#endif

    size_t available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only " << available << " samples available"
                  << std::endl;
#endif
        n = available;
    }
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
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::readOne(" << R << ")" << std::endl;
#endif

    if (m_writer == m_readers[R]) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: No sample available"
                  << std::endl;
#endif
        T t;
        memset(&t, 0, sizeof(T));
        return t;
    }
    T value = m_buffer[m_readers[R]];
    if (++m_readers[R] == m_size) m_readers[R] = 0;
    return value;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::peek(T *destination, size_t n, int R) const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::peek(dest, " << n << ", " << R << ")" << std::endl;
#endif

    size_t available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: Only " << available << " samples available"
		  << std::endl;
#endif
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

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::peek: read " << n << std::endl;
#endif

    return n;
}

template <typename T, int N>
T
RingBuffer<T, N>::peek(int R) const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::peek(" << R << ")" << std::endl;
#endif

    if (m_writer == m_readers[R]) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: No sample available"
                  << std::endl;
#endif
        T t;
        memset(&t, 0, sizeof(T));
        return t;
    }
    T value = m_buffer[m_readers[R]];
    return value;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::skip(size_t n, int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::skip(" << n << ", " << R << ")" << std::endl;
#endif

    size_t available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only " << available << " samples available"
                  << std::endl;
#endif
        n = available;
    }
    if (n == 0) return n;
    m_readers[R] = (m_readers[R] + n) % m_size;
    return n;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::write(const T *source, size_t n)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::write(" << n << ")" << std::endl;
#endif

    size_t available = getWriteSpace();
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only room for " << available << " samples"
                  << std::endl;
#endif
        n = available;
    }
    if (n == 0) return n;

    size_t here = m_size - m_writer;
    if (here >= n) {
        memcpy(m_buffer + m_writer, source, n * sizeof(T));
    } else {
        memcpy(m_buffer + m_writer, source, here * sizeof(T));
        memcpy(m_buffer, source + here, (n - here) * sizeof(T));
    }

    m_writer = (m_writer + n) % m_size;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::write: wrote " << n << ", writer now " << m_writer << std::endl;
#endif

    return n;
}

template <typename T, int N>
size_t
RingBuffer<T, N>::zero(size_t n)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::zero(" << n << ")" << std::endl;
#endif

    size_t available = getWriteSpace();
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only room for " << available << " samples"
                  << std::endl;
#endif
        n = available;
    }
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

#endif // RG_RINGBUFFER_H
