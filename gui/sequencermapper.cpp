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

#include <qdir.h>

#include "sequencermapper.h"
#include "rosedebug.h"

#include "RealTime.h"
#include "Exception.h"

SequencerMapper::SequencerMapper(const QString filename)
    : m_fd(-1),
      m_mmappedSize(0),
      m_mmappedBuffer((void *)0),
      m_filename(filename)
{
    RG_DEBUG << "SequencerMapper::SequencerMapper - mmapping " << filename << endl;
    map();
}


SequencerMapper::~SequencerMapper()
{
    unmap();
}


void
SequencerMapper::map()
{
    QFileInfo fInfo(m_filename);
    if (!fInfo.exists()) {
        RG_DEBUG << "SequencerMapper::map() : file " << m_filename << " doesn't exist\n";
        throw Rosegarden::Exception("file not found");
    }

    m_fd = ::open(m_filename.latin1(), O_RDWR);

    if (m_fd < 0) {
	RG_DEBUG << "SequencerMapper::map() : Couldn't open " << m_filename
                     << endl;
        throw Rosegarden::Exception("Couldn't open " + std::string(m_filename.data()));
    }

    m_mmappedSize = sizeof(Rosegarden::RealTime);

    m_mmappedBuffer = (long*)::mmap(0, m_mmappedSize, PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {

        RG_DEBUG << QString("mmap failed : (%1) %2\n").
            arg(errno).arg(strerror(errno));

        throw Rosegarden::Exception("mmap failed");
    }

    RG_DEBUG << "SequencerMapper::map() : "
                    << (void*)m_mmappedBuffer << "," << m_mmappedSize << endl;
}

bool
SequencerMapper::remap()
{
/*
    QFileInfo fInfo(m_filename);
    size_t newSize = fInfo.size();

    SEQUENCER_DEBUG << "remap() from " << m_mmappedSize << " to "
                    << newSize << endl;

    if (m_mmappedSize == newSize) {

        SEQUENCER_DEBUG << "remap() : sizes are identical, remap not forced - "
                        << "nothing to do\n";
        return false;
    }

#ifdef linux
    m_mmappedBuffer = (MappedEvent*)::mremap(m_mmappedBuffer, m_mmappedSize, newSize, MREMAP_MAYMOVE);
#else
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    m_mmappedBuffer = (MappedEvent*)::mmap(0, newSize, PROT_READ, MAP_SHARED, m_fd, 0);
#endif

    if (m_mmappedBuffer == (void*)-1) {

            SEQUENCER_DEBUG << QString("mremap failed : (%1) %2\n").
                arg(errno).arg(strerror(errno));

            throw Rosegarden::Exception("mremap failed");
    }

    m_mmappedSize = newSize;
    m_nbMappedEvents = m_mmappedSize / sizeof(MappedEvent);

    return true;
*/
    return true;
}

void 
SequencerMapper::unmap()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}

