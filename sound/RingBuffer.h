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

#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <sys/types.h>

namespace Rosegarden
{

typedef struct
{
  char *buf;
  size_t len;
}
ringbuffer_data_t ;

class RingBuffer
{
public:
    RingBuffer(int size);
    ~RingBuffer();

    int lock();
    void reset();

    void writeAdvance(size_t cnt);
    void readAdvance(size_t cnt);
    
    // information methods
    size_t writeSpace();
    size_t readSpace();

    size_t read(char *dest, size_t cnt);
    size_t write(char *src, size_t cnt);

    void getReadVector(ringbuffer_data_t *vec);
    void getWriteVector(ringbuffer_data_t *vec);

protected:

  char                *m_buffer;
  volatile size_t      m_writePtr;
  volatile size_t      m_readPtr;
  size_t               m_size;
  size_t               m_sizeMask; 
  bool                 m_mlocked;

};

}

#endif // _RINGBUFFER_H_
