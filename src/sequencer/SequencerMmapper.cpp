
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <kstddirs.h>

#include <qfile.h>

#include "SequencerMmapper.h"
#include "misc/Debug.h"

#include "base/RealTime.h"
#include "base/Exception.h"
#include "sound/MappedEvent.h"
#include "sound/MappedComposition.h"

namespace Rosegarden
{

// Seems not to be properly defined under some gcc 2.95 setups
#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

SequencerMmapper::SequencerMmapper():
        m_fileName(createFileName()),
        m_fd( -1),
        m_mmappedBuffer(0),
        m_mmappedSize(sizeof(SequencerDataBlock))
{
    // just in case
    QFile::remove
        (m_fileName);

    m_fd = ::open(m_fileName.latin1(),
                  O_RDWR | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR);

    if (m_fd < 0) {
        SEQUENCER_DEBUG << "SequencerMmapper : Couldn't open " << m_fileName
        << endl;
        throw Exception("Couldn't open " + std::string(m_fileName.data()));
    }

    setFileSize(m_mmappedSize);

    //
    // mmap() file for writing
    //
    m_mmappedBuffer = ::mmap(0, m_mmappedSize,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*) - 1) {
        SEQUENCER_DEBUG <<
        QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Exception("mmap failed");
    }

    SEQUENCER_DEBUG << "SequencerMmapper : mmap size : " << m_mmappedSize
    << " at " << (void*)m_mmappedBuffer << endl;

    // initialise
    init();
}

SequencerMmapper::~SequencerMmapper()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
    QFile::remove
        (m_fileName);
}

void
SequencerMmapper::init()
{
    SEQUENCER_DEBUG << "SequencerMmapper::init()\n";

    m_sequencerDataBlock = new (m_mmappedBuffer)
                           SequencerDataBlock(true);

    ::msync(m_mmappedBuffer, m_mmappedSize, MS_ASYNC);
}

void
SequencerMmapper::setFileSize(size_t size)
{
    SEQUENCER_DEBUG << "SequencerMmapper : setting size of "
    << m_fileName << " to " << size << endl;
    // rewind
    ::lseek(m_fd, 0, SEEK_SET);

    // enlarge the file
    // (seek() to wanted size, then write a byte)
    //
    if (::lseek(m_fd, size - 1, SEEK_SET) == -1) {
        std::cerr << "WARNING: SequencerMmapper : Couldn't lseek in " << m_fileName
        << " to " << size << std::endl;
        throw Exception("lseek failed");
    }

    if (::write(m_fd, "\0", 1) != 1) {
        std::cerr << "WARNING: SequencerMmapper : Couldn't write byte in  "
        << m_fileName << std::endl;
        throw Exception("write failed");
    }
}

QString
SequencerMmapper::createFileName()
{
    return KGlobal::dirs()->resourceDirs("tmp").last() +
           "/rosegarden_sequencer_timing_block";
}

}

