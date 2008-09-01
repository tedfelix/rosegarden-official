
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MmappedControlBlock.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include "misc/Debug.h"

namespace Rosegarden
{

MmappedControlBlock::MmappedControlBlock(QString fileName)
        : m_fileName(fileName),
        m_fd( -1),
        m_mmappedBuffer(0),
        m_mmappedSize(sizeof(ControlBlock)),
        m_controlBlock(0)
{
    m_fd = ::open(m_fileName.toLatin1().data(), O_RDWR);

    if (m_fd < 0) {
        SEQMAN_DEBUG << "MmappedControlBlock : Couldn't open " << m_fileName
        << endl;
        throw Exception(std::string("Couldn't open ")
                        + m_fileName.toLatin1().data());
    }

    //
    // mmap() file for reading
    //
    m_mmappedBuffer = (char*)::mmap(0, m_mmappedSize,
                                    PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*) - 1) {

        SEQUENCER_DEBUG << QString("mmap failed : (%1) %2\n").
        arg(errno).arg(strerror(errno));

        throw Exception("mmap failed");
    }

    SEQMAN_DEBUG << "MmappedControlBlock : mmap size : " << m_mmappedSize
    << " at " << (void*)m_mmappedBuffer << endl;

    // Create new control block on file
    m_controlBlock = new (m_mmappedBuffer) ControlBlock;
}

MmappedControlBlock::~MmappedControlBlock()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}

}
