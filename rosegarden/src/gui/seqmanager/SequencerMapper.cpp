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


#include "SequencerMapper.h"

#include "misc/Debug.h"
#include "base/Exception.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/Track.h"
#include "sound/MappedComposition.h"
#include "sound/MappedEvent.h"
#include "sound/SequencerDataBlock.h"
#include <qfileinfo.h>
#include <qstring.h>


namespace Rosegarden
{

SequencerMapper::SequencerMapper(const QString filename)
        : m_fd( -1),
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
        throw Exception("file not found");
    }

    m_fd = ::open(m_filename.latin1(), O_RDWR);

    if (m_fd < 0) {
        RG_DEBUG << "SequencerMapper::map() : Couldn't open " << m_filename
        << endl;
        throw Exception("Couldn't open " + std::string(m_filename.data()));
    }

    m_mmappedSize = sizeof(SequencerDataBlock);
    m_mmappedBuffer = (long*)::mmap(0, m_mmappedSize, PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*) - 1) {

        RG_DEBUG << QString("mmap failed : (%1) %2\n").
        arg(errno).arg(strerror(errno));

        throw Exception("mmap failed");
    }

    RG_DEBUG << "SequencerMapper::map() : "
    << (void*)m_mmappedBuffer << "," << m_mmappedSize << endl;

    m_sequencerDataBlock = new (m_mmappedBuffer)
                           SequencerDataBlock(false);
}

void
SequencerMapper::unmap()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}

}
