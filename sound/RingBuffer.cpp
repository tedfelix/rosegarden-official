// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
Rosegarden-4
A sequencer and musical notation editor.

This program is Copyright 2000-2003
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

/*
 * This class based on Rohan Drape's ISO/POSIX C version of Paul Davis's
 * lock free ringbuffer C++ code.  This is safe for the case of one read
 * thread and one write thread. 
 *
 */

#include "RingBuffer.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>


namespace Rosegarden
{


// Create a new ringbuffer to hold at least `sz' bytes of data. The
// actual buffer size is rounded up to the next power of two.
//
RingBuffer::RingBuffer(int size)
{
    int power_of_two;

    for (power_of_two = 1; 1 << power_of_two < size; power_of_two++);
    

    m_size = 1 << power_of_two;
    m_sizeMask = m_size;
    m_sizeMask -= 1;
    m_writePtr = 0;
    m_readPtr = 0;
    m_buffer = new char[m_size];
    m_mlocked = false;
}


RingBuffer::~RingBuffer()
{
    if  (m_mlocked)
    {
        munlock(m_buffer, m_size);
    }

    delete [] m_buffer;
}

// Lock the data block of the buffer using the system call 'mlock'.
//
int 
RingBuffer::lock()
{
    if (mlock(m_buffer, m_size))
    {
        return -1;
    }

    m_mlocked = true;
    return 0;
}


// Reset the read and write pointers to zero. This is not thread safe.
//
void
RingBuffer::reset()
{
    m_readPtr = 0;
    m_writePtr = 0;
}


void 
RingBuffer::writeAdvance(size_t cnt)
{
    m_writePtr += cnt;
    m_writePtr &= m_sizeMask;
}

void 
RingBuffer::readAdvance(size_t cnt)
{
    m_readPtr += cnt;
    m_readPtr &= m_sizeMask;
}

// Return the number of bytes available for writing.  This is the
// number of bytes in front of the write pointer and behind the read
// pointer.
//
size_t 
RingBuffer::writeSpace()
{
    size_t w, r;

    w = m_writePtr;
    r = m_readPtr;

    if (w > r)
        return ((r - w + m_size) & m_sizeMask) - 1;
    else if ( w < r)
        return (r - w) - 1;
    else
        return m_size - 1;
}


// Return the number of bytes available for reading.  This is the
// number of bytes in front of the read pointer and behind the write
// pointer.
//
size_t
RingBuffer::readSpace()
{
    size_t w, r;

    w = m_writePtr;
    r = m_readPtr;

    if (w > r)
        return w - r;
    else
        return (w - r + m_size) & m_sizeMask;
}


// The copying data reader.  Copy at most `cnt' bytes from buffer to
// `dest'.  Returns the actual number of bytes copied.
//
size_t 
RingBuffer::read(char *dest, size_t cnt)
{
    size_t free_cnt;
    size_t cnt2;
    size_t to_read;
    size_t n1, n2;
  
    if ((free_cnt = readSpace()) == 0) {
      return 0;
    }

    to_read = cnt > free_cnt ? free_cnt : cnt;

    cnt2 = m_readPtr + to_read;

    if (cnt2 > m_size) {
      n1 = m_size - m_readPtr;
      n2 = cnt2 & m_sizeMask;
    } else {
      n1 = to_read;
      n2 = 0;
    }

    memcpy (dest, &(m_buffer[m_readPtr]), n1);
    m_readPtr += n1;
    m_readPtr &= m_sizeMask;
  
    if (n2) {
      memcpy (dest + n1, &(m_buffer[m_readPtr]), n2);
      m_readPtr += n2;
      m_readPtr &= m_sizeMask;
    }

    return to_read;

}

// The copying data writer.  Copy at most `cnt' bytes to m_buffer from
// `src'.  Returns the actual number of bytes copied.
//
size_t 
RingBuffer::write(char *src, size_t cnt)
{
    size_t free_cnt;
    size_t cnt2;
    size_t to_write;
    size_t n1, n2;

    if ((free_cnt = writeSpace()) == 0) {
      return 0;
    }

    to_write = cnt > free_cnt ? free_cnt : cnt;

    cnt2 = m_writePtr + to_write;

    if (cnt2 > m_size) {
      n1 = m_size - m_writePtr;
      n2 = cnt2 & m_sizeMask;
    } else {
      n1 = to_write;
      n2 = 0;
    }

    memcpy (&(m_buffer[m_writePtr]), src, n1);
    m_writePtr += n1;
    m_writePtr &= m_sizeMask;

    if (n2) {
      memcpy (&(m_buffer[m_writePtr]), src + n1, n2);
      m_writePtr += n2;
      m_writePtr &= m_sizeMask;
    }

    return to_write;

}

// The non-copying data reader.  `vec' is an array of two places.  Set
// the values at `vec' to hold the current readable data at `rb'.  If
// the readable data is in one segment the second segment has zero
// length. 
//
void
RingBuffer::getReadVector(ringbuffer_data_t *vec)
{
  size_t free_cnt;
  size_t cnt2;
  size_t w, r;

  w = m_writePtr;
  r = m_readPtr;

  if (w > r) {
    free_cnt = w - r;
  } else {
    free_cnt = (w - r + m_size) & m_sizeMask;
  }

  cnt2 = r + free_cnt;

  if (cnt2 > m_size) {

    /* Two part vector: the rest of the buffer after the current write
       ptr, plus some from the start of the buffer. */

    vec[0].buf = &(m_buffer[r]);
    vec[0].len = m_size - r;
    vec[1].buf = m_buffer;
    vec[1].len = cnt2 & m_sizeMask;

  } else {

    /* Single part vector: just the rest of the buffer */

    vec[0].buf = &(m_buffer[r]);
    vec[0].len = free_cnt;
    vec[1].len = 0;
  }
}

// The non-copying data writer.  `vec' is an array of two places.  Set
// the values at `vec' to hold the current writeable data at `rb'.  If
// the writeable data is in one segment the second segment has zero
// length.
//
void 
RingBuffer::getWriteVector(ringbuffer_data_t *vec)
{
  size_t free_cnt;
  size_t cnt2;
  size_t w, r;

  w = m_writePtr;
  r = m_readPtr;

  if (w > r) {
    free_cnt = ((r - w + m_size) & m_sizeMask) - 1;
  } else if (w < r) {
    free_cnt = (r - w) - 1;
  } else {
    free_cnt = m_size - 1;
  }

  cnt2 = w + free_cnt;

  if (cnt2 > m_size) {

    /* Two part vector: the rest of the buffer after the current write
       ptr, plus some from the start of the buffer. */

    vec[0].buf = &(m_buffer[w]);
    vec[0].len = m_size - w;
    vec[1].buf = m_buffer;
    vec[1].len = cnt2 & m_sizeMask;
  } else {
    vec[0].buf = &(m_buffer[w]);
    vec[0].len = free_cnt;
    vec[1].len = 0;
  }

}


}
