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
#include "MappedEvent.h"
#include "MappedComposition.h"

const int SequencerMapper::m_recordingBufferSize = 1000; //!!! events -- dup with sequencer -- fix! rewrite this disgusting class!

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

    m_mmappedSize =
	sizeof(Rosegarden::RealTime) +
	sizeof(int) +
	sizeof(bool) +
	sizeof(Rosegarden::MappedEvent) +
	sizeof(int) +
	m_recordingBufferSize * sizeof(Rosegarden::MappedEvent);

    m_mmappedBuffer = (long*)::mmap(0, m_mmappedSize, PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {

        RG_DEBUG << QString("mmap failed : (%1) %2\n").
            arg(errno).arg(strerror(errno));

        throw Rosegarden::Exception("mmap failed");
    }

    RG_DEBUG << "SequencerMapper::map() : "
                    << (void*)m_mmappedBuffer << "," << m_mmappedSize << endl;

    // adding 1 at each stage will work because the pointer arithmetic
    // is in multiples of the size of the type of the pointer
    m_positionPtrPtr = (Rosegarden::RealTime *)(m_mmappedBuffer);
    m_eventIndexPtr = (int *)(m_positionPtrPtr + 1);
    m_haveEventPtr = (bool *)(m_eventIndexPtr + 1);
    m_eventPtr = (Rosegarden::MappedEvent *)(m_haveEventPtr + 1);
    m_recordEventIndexPtr = (int *)(m_eventPtr + 1);
    m_recordEventBuffer = (Rosegarden::MappedEvent *)(m_recordEventIndexPtr + 1);
}

void 
SequencerMapper::unmap()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}

Rosegarden::RealTime
SequencerMapper::getPositionPointer() const
{
    return *m_positionPtrPtr;
}

bool
SequencerMapper::getVisual(Rosegarden::MappedEvent &ev) const
{
    static int eventIndex = 0;

    if (!*m_haveEventPtr) {
	return false;
    } else {
	int thisEventIndex = *m_eventIndexPtr;
	if (thisEventIndex == eventIndex) return false;
	ev = *m_eventPtr;
	eventIndex = thisEventIndex;
	return true;
    }
}

int
SequencerMapper::getRecordedEvents(Rosegarden::MappedComposition &mC) const
{
    static int readIndex = 0;

    int currentIndex = *m_recordEventIndexPtr;
    int count = 0;

    while (readIndex != currentIndex) {
	mC.insert(new Rosegarden::MappedEvent
		  (m_recordEventBuffer[readIndex]));
	if (++readIndex == m_recordingBufferSize) readIndex = 0;
	++count;
    }

    return count;
}


