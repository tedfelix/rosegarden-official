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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include "mmappedcontrolblock.h"
#include "rosedebug.h"

ControlBlockMmapper::ControlBlockMmapper(QString fileName)
    : m_fileName(fileName),
      m_fd(-1),
      m_mmappedBuffer(0),
      m_mmappedSize(sizeof(ControlBlock)),
      m_controlBlock(0)
{
    m_fd = ::open(m_fileName.latin1(), O_RDWR);

    if (m_fd < 0) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't open " << m_fileName
                     << endl;
        throw Rosegarden::Exception(std::string("Couldn't open ") 
                + m_fileName.latin1());
    }

    //
    // mmap() file for reading
    //
    m_mmappedBuffer = (char*)::mmap(0, m_mmappedSize,
                                    PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {

        SEQUENCER_DEBUG << QString("mmap failed : (%1) %2\n").
            arg(errno).arg(strerror(errno));

        throw Rosegarden::Exception("mmap failed");
    }

    SEQMAN_DEBUG << "ControlBlockMmapper : mmap size : " << m_mmappedSize
                 << " at " << (void*)m_mmappedBuffer << endl;

    // Create new control block on file
    m_controlBlock = new (m_mmappedBuffer) ControlBlock;
}

ControlBlockMmapper::~ControlBlockMmapper()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}


void ControlBlockMmapper::refresh()
{
    SEQMAN_DEBUG << "ControlBlockMmapper::refresh() : Nothing to do - "
                 << "control block is of constant size\n";
}

