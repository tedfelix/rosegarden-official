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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <kstddirs.h>

#include <qfile.h>

#include "sequencermapper.h"
#include "rosedebug.h"

#include "RealTime.h"
#include "Exception.h"
#include "MappedEvent.h"

// Seems not to be properly defined under some gcc 2.95 setups
#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

SequencerMmapper::SequencerMmapper():
    m_fileName(createFileName()),
    m_fd(-1),
    m_mmappedBuffer(0),
    m_mmappedSize(sizeof(Rosegarden::RealTime) + sizeof(bool) +
		  sizeof(Rosegarden::MappedEvent))
{
    // just in case
    QFile::remove(m_fileName);

    m_fd = ::open(m_fileName.latin1(),
		  O_RDWR | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR);

    if (m_fd < 0) {
        SEQUENCER_DEBUG << "SequencerMmapper : Couldn't open " << m_fileName
                     << endl;
        throw Rosegarden::Exception("Couldn't open " + std::string(m_fileName.data()));
    }

    setFileSize(m_mmappedSize);

    //
    // mmap() file for writing
    //
    m_mmappedBuffer = ::mmap(0, m_mmappedSize,
                             PROT_READ|PROT_WRITE,
                             MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {
        SEQUENCER_DEBUG <<
	    QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Rosegarden::Exception("mmap failed");
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
    QFile::remove(m_fileName);
}

void
SequencerMmapper::updatePositionPointer(Rosegarden::RealTime time)
{
    Rosegarden::RealTime *ptr = (Rosegarden::RealTime *)m_mmappedBuffer;
    *ptr = time;
}

void
SequencerMmapper::updateVisual(Rosegarden::MappedEvent *ev)
{
    char *buf = (char *)m_mmappedBuffer;
    buf += sizeof(Rosegarden::RealTime);

    bool *haveEventPtr = (bool *)buf;
    buf += sizeof(bool);

    Rosegarden::MappedEvent *eventPtr = (Rosegarden::MappedEvent *)buf;

    if (ev) {
	*haveEventPtr = true;
	*eventPtr = *ev;
    } else {
	*haveEventPtr = false;
    }
}


void
SequencerMmapper::init()
{
    SEQUENCER_DEBUG << "SequencerMmapper::initControlBlock()\n";

    updatePositionPointer(Rosegarden::RealTime::zeroTime);
    updateVisual(0);
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
        throw Rosegarden::Exception("lseek failed");
    }

    if (::write(m_fd, "\0", 1) != 1) {
        std::cerr << "WARNING: SequencerMmapper : Couldn't write byte in  "
                  << m_fileName << std::endl;
        throw Rosegarden::Exception("write failed");
    }
}

QString
SequencerMmapper::createFileName()
{
    return KGlobal::dirs()->resourceDirs("tmp").first() +
	"/rosegarden_sequencer_timing_block";
}

