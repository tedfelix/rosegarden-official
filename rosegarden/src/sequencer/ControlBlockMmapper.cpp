
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ControlBlockMmapper.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include "misc/Debug.h"

namespace Rosegarden
{

ControlBlockMmapper::ControlBlockMmapper(QString fileName)
        : m_fileName(fileName),
        m_fd( -1),
        m_mmappedBuffer(0),
        m_mmappedSize(sizeof(ControlBlock)),
        m_controlBlock(0)
{
    m_fd = ::open(m_fileName.latin1(), O_RDWR);

    if (m_fd < 0) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't open " << m_fileName
        << endl;
        throw Exception(std::string("Couldn't open ")
                        + m_fileName.latin1());
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

}
