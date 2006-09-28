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


#include "ControlBlockMmapper.h"

#include <kstddirs.h>
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Exception.h"
#include "base/MidiProgram.h"
#include "base/Track.h"
#include "document/RosegardenGUIDoc.h"
#include "sequencer/mmappedcontrolblock.h"
#include "sound/ControlBlock.h"
#include <kglobal.h>
#include <qfile.h>
#include <qstring.h>


namespace Rosegarden
{

ControlBlockMmapper::ControlBlockMmapper(RosegardenGUIDoc* doc)
        : m_doc(doc),
        m_fileName(createFileName()),
        m_fd( -1),
        m_mmappedBuffer(0),
        m_mmappedSize(sizeof(ControlBlock)),
        m_controlBlock(0)
{
    // just in case
    QFile::remove
        (m_fileName);

    m_fd = ::open(m_fileName.latin1(), O_RDWR | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR);
    if (m_fd < 0) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't open " << m_fileName
        << endl;
        throw Exception("Couldn't open " + qstrtostr(m_fileName));
    }

    setFileSize(m_mmappedSize);

    //
    // mmap() file for writing
    //
    m_mmappedBuffer = ::mmap(0, m_mmappedSize,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*) - 1) {
        SEQMAN_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Exception("mmap failed");
    }

    SEQMAN_DEBUG << "ControlBlockMmapper : mmap size : " << m_mmappedSize
    << " at " << (void*)m_mmappedBuffer << endl;

    // Create new control block on file
    initControlBlock();
}

ControlBlockMmapper::~ControlBlockMmapper()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
    QFile::remove
        (m_fileName);
}

QString ControlBlockMmapper::createFileName()
{
    return KGlobal::dirs()->resourceDirs("tmp").last() + "/rosegarden_control_block";
}

void ControlBlockMmapper::updateTrackData(Track *t)
{
    m_controlBlock->updateTrackData(t);
}

void ControlBlockMmapper::setTrackDeleted(TrackId t)
{
    m_controlBlock->setTrackDeleted(t, true);
}

void ControlBlockMmapper::updateMidiFilters(MidiFilter thruFilter,
        MidiFilter recordFilter)
{
    m_controlBlock->setThruFilter(thruFilter);
    m_controlBlock->setRecordFilter(recordFilter);
}

void ControlBlockMmapper::updateMetronomeData(InstrumentId instId)
{
    m_controlBlock->setInstrumentForMetronome(instId);
}

void ControlBlockMmapper::updateMetronomeForPlayback()
{
    bool muted = !m_doc->getComposition().usePlayMetronome();
    SEQMAN_DEBUG << "ControlBlockMmapper::updateMetronomeForPlayback: muted=" << muted << endl;
    if (m_controlBlock->isMetronomeMuted() == muted)
        return ;
    m_controlBlock->setMetronomeMuted(muted);
}

void ControlBlockMmapper::updateMetronomeForRecord()
{
    bool muted = !m_doc->getComposition().useRecordMetronome();
    SEQMAN_DEBUG << "ControlBlockMmapper::updateMetronomeForRecord: muted=" << muted << endl;
    if (m_controlBlock->isMetronomeMuted() == muted)
        return ;
    m_controlBlock->setMetronomeMuted(muted);
}

bool ControlBlockMmapper::updateSoloData(bool solo,
        TrackId selectedTrack)
{
    bool changed = false;

    if (solo != m_controlBlock->isSolo()) {

        changed = true;

    } else if (solo &&
               (selectedTrack != m_controlBlock->getSelectedTrack())) {

        changed = true;
    }

    m_controlBlock->setSolo(solo);
    m_controlBlock->setSelectedTrack(selectedTrack);

    return changed;
}

void ControlBlockMmapper::setDocument(RosegardenGUIDoc* doc)
{
    SEQMAN_DEBUG << "ControlBlockMmapper::setDocument()\n";
    m_doc = doc;
    initControlBlock();
}

void ControlBlockMmapper::initControlBlock()
{
    SEQMAN_DEBUG << "ControlBlockMmapper::initControlBlock()\n";

    m_controlBlock = new (m_mmappedBuffer) ControlBlock(m_doc->getComposition().getMaxTrackId());

    Composition& comp = m_doc->getComposition();

    for (Composition::trackiterator i = comp.getTracks().begin(); i != comp.getTracks().end(); ++i) {
        Track* track = i->second;
        if (track == 0)
            continue;

        m_controlBlock->updateTrackData(track);
    }

    m_controlBlock->setMetronomeMuted(!comp.usePlayMetronome());

    m_controlBlock->setThruFilter(m_doc->getStudio().getMIDIThruFilter());
    m_controlBlock->setRecordFilter(m_doc->getStudio().getMIDIRecordFilter());

    ::msync(m_mmappedBuffer, m_mmappedSize, MS_ASYNC);
}

void ControlBlockMmapper::setFileSize(size_t size)
{
    SEQMAN_DEBUG << "ControlBlockMmapper : setting size of "
    << m_fileName << " to " << size << endl;
    // rewind
    ::lseek(m_fd, 0, SEEK_SET);

    //
    // enlarge the file
    // (seek() to wanted size, then write a byte)
    //
    if (::lseek(m_fd, size - 1, SEEK_SET) == -1) {
        std::cerr << "WARNING: ControlBlockMmapper : Couldn't lseek in " << m_fileName
        << " to " << size << std::endl;
        throw Exception("lseek failed");
    }

    if (::write(m_fd, "\0", 1) != 1) {
        std::cerr << "WARNING: ControlBlockMmapper : Couldn't write byte in  "
        << m_fileName << std::endl;
        throw Exception("write failed");
    }

}

void
ControlBlockMmapper::enableMIDIThruRouting(bool state)
{
    m_controlBlock->setMidiRoutingEnabled(state);
}

}
