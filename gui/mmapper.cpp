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
#include <kstddirs.h>

#include "mmapper.h"

#include "Segment.h"
#include "Track.h"
#include "Composition.h"
#include "ControlBlock.h"
#include "MappedEvent.h"
#include "SegmentPerformanceHelper.h"

#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"
#include "rgapplication.h"

// Seems not to be properly defined under some gcc 2.95 setups
#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

using Rosegarden::ControlBlock;
using Rosegarden::Track;
using Rosegarden::Segment;
using Rosegarden::Composition;
using Rosegarden::MappedEvent;
using Rosegarden::timeT;

ControlBlockMmapper::ControlBlockMmapper(RosegardenGUIDoc* doc)
    : m_doc(doc),
      m_fileName(createFileName()),
      m_needsRefresh(true),
      m_fd(-1),
      m_mmappedBuffer(0),
      m_mmappedSize(sizeof(ControlBlock)),
      m_controlBlock(0)
{
    // just in case
    QFile::remove(m_fileName);

    m_fd = ::open(m_fileName.latin1(), O_RDWR|O_CREAT|O_TRUNC,
                  S_IRUSR|S_IWUSR);
    if (m_fd < 0) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't open " << m_fileName
                     << endl;
        throw Rosegarden::Exception("Couldn't open " + qstrtostr(m_fileName));
    }

    setFileSize(m_mmappedSize);

    //
    // mmap() file for writing
    //
    m_mmappedBuffer = ::mmap(0, m_mmappedSize,
                             PROT_READ|PROT_WRITE,
                             MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {
        SEQMAN_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Rosegarden::Exception("mmap failed");
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
    QFile::remove(m_fileName);
}


QString ControlBlockMmapper::createFileName()
{
    return KGlobal::dirs()->resourceDirs("tmp").first() + "/rosegarden_control_block";
}

void ControlBlockMmapper::refresh()
{
    SEQMAN_DEBUG << "ControlBlockMmapper : refresh\n";

    if (m_needsRefresh) {
        ::msync(m_mmappedBuffer, m_mmappedSize, MS_ASYNC);

        rgapp->sequencerSend("remapControlBlock()");

        m_needsRefresh = false;
    }
}

void ControlBlockMmapper::updateTrackData(Track *t)
{
    m_controlBlock->updateTrackData(t);
    m_needsRefresh = true;
}

void ControlBlockMmapper::updateMetronomeData(Rosegarden::InstrumentId instId)
{
    m_controlBlock->setInstrumentForMetronome(instId);
    m_needsRefresh = true;
}

void ControlBlockMmapper::updateMetronomeForPlayback()
{
    bool muted = !m_doc->getComposition().usePlayMetronome();
    SEQMAN_DEBUG << "ControlBlockMmapper::updateMetronomeForPlayback: muted=" << muted << endl;
    if (m_controlBlock->isMetronomeMuted() == muted) return;
    m_controlBlock->setMetronomeMuted(muted);
    m_needsRefresh = true;
}

void ControlBlockMmapper::updateMetronomeForRecord()
{
    bool muted = !m_doc->getComposition().useRecordMetronome();
    SEQMAN_DEBUG << "ControlBlockMmapper::updateMetronomeForRecord: muted=" << muted << endl;
    if (m_controlBlock->isMetronomeMuted() == muted) return;
    m_controlBlock->setMetronomeMuted(muted);
    m_needsRefresh = true;
}

void ControlBlockMmapper::updateSoloData(bool solo,
                                         Rosegarden::TrackId selectedTrack)
{
    m_controlBlock->setSolo(solo);
    m_controlBlock->setSelectedTrack(selectedTrack);
    m_needsRefresh = true;
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

    m_controlBlock = new (m_mmappedBuffer) ControlBlock(m_doc->getComposition().getNbTracks());

    Composition& comp = m_doc->getComposition();
    
    for(Composition::trackiterator i = comp.getTracks().begin(); i != comp.getTracks().end(); ++i) {
        Track* track = i->second;
        if (track == 0) continue;
        
        m_controlBlock->updateTrackData(track);
    }

    m_controlBlock->setMetronomeMuted(!comp.usePlayMetronome());

    refresh();
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
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't lseek in " << m_fileName
                     << " to " << size << endl;
        throw Rosegarden::Exception("lseek failed");
    }
    
    if (::write(m_fd, "\0", 1) != 1) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't write byte in  "
                     << m_fileName << endl;
        throw Rosegarden::Exception("write failed");
    }
    
}

//----------------------------------------

SegmentMmapper::SegmentMmapper(RosegardenGUIDoc* doc,
                               Segment* segment, const QString& fileName)
    : m_doc(doc),
      m_segment(segment),
      m_fileName(fileName),
      m_fd(-1),
      m_mmappedSize(0),
      m_mmappedBuffer((MappedEvent*)0)
{
    SEQMAN_DEBUG << "SegmentMmapper : " << this
                 << " trying to mmap " << m_fileName
                 << endl;

    m_fd = ::open(m_fileName.latin1(), O_RDWR|O_CREAT|O_TRUNC,
                  S_IRUSR|S_IWUSR);
    if (m_fd < 0) {
        SEQMAN_DEBUG << "SegmentMmapper : Couldn't open " << m_fileName
                     << endl;
        throw Rosegarden::Exception("Couldn't open " + qstrtostr(m_fileName));
    }

    SEQMAN_DEBUG << "SegmentMmapper : mmap size = " << m_mmappedSize
                 << endl;
}

void SegmentMmapper::init()
{
    m_mmappedSize = computeMmappedSize();

    if (m_mmappedSize > 0) {
        setFileSize(m_mmappedSize);
        doMmap();
        dump();
    } else {
        SEQMAN_DEBUG << "SegmentMmapper::init : mmap size = 0 - skipping mmapping for now\n";
    }
}


size_t SegmentMmapper::computeMmappedSize()
{
    if (!m_segment) return 0;

    int repeatCount = getSegmentRepeatCount();

    return (repeatCount + 1) * m_segment->size() * sizeof(MappedEvent);
}


SegmentMmapper::~SegmentMmapper()
{
    SEQMAN_DEBUG << "~SegmentMmapper : " << this
                 << " unmapping " << (void*)m_mmappedBuffer
                 << " of size " << m_mmappedSize
                 << endl;

    if (m_mmappedBuffer && m_mmappedSize)
        ::munmap(m_mmappedBuffer, m_mmappedSize);

    ::close(m_fd);
    SEQMAN_DEBUG << "~SegmentMmapper : removing " << m_fileName << endl;

    QFile::remove(m_fileName);
}


bool SegmentMmapper::refresh()
{
    bool res = false;

    size_t newMmappedSize = computeMmappedSize();

    SEQMAN_DEBUG << "SegmentMmapper::refresh() - " << getFileName()
                 << " - m_mmappedBuffer = " << (void*)m_mmappedBuffer
                 << " - new size = " << newMmappedSize
                 << " - old size = " << m_mmappedSize
                 << endl;

    // always zero out
    memset(m_mmappedBuffer, 0, m_mmappedSize);

    if (newMmappedSize != m_mmappedSize) {

        res = true;

        if (newMmappedSize == 0) {

            // nothing to do, just msync and go
            ::msync(m_mmappedBuffer, m_mmappedSize, MS_ASYNC);
            m_mmappedSize = 0;
            return true;

        } else {

            setFileSize(newMmappedSize);
            remap(newMmappedSize);
        }
    }
    
    dump();

    return res;
}

void SegmentMmapper::setFileSize(size_t size)
{
    SEQMAN_DEBUG << "SegmentMmapper::setFileSize() : setting size of "
                 << m_fileName << " to " << size
                 << " - current size = " << m_mmappedSize << endl;

    if (size < m_mmappedSize) {

        ftruncate(m_fd, size);

    } else {

        // On linux, ftrunctate can enlarge a file, but this isn't specified by POSIX
        // so go the safe way

        if (size == 0) {
            SEQMAN_DEBUG << "SegmentMmapper : size == 0 : no resize to do\n";
            return;
        }
    
        // rewind
        ::lseek(m_fd, 0, SEEK_SET);

        //
        // enlarge the file
        // (seek() to wanted size, then write a byte)
        //
        if (::lseek(m_fd, size - 1, SEEK_SET) == -1) {
            SEQMAN_DEBUG << "SegmentMmapper : Couldn't lseek in " << m_fileName
                         << " to " << size << endl;
            throw Rosegarden::Exception("lseek failed");
        }
    
        if (::write(m_fd, "\0", 1) != 1) {
            SEQMAN_DEBUG << "SegmentMmapper : Couldn't write byte in  "
                         << m_fileName << endl;
            throw Rosegarden::Exception("write failed");
        }
    
    }
    
    
}

void SegmentMmapper::remap(size_t newsize)
{
    SEQMAN_DEBUG << "SegmentMmapper : remapping " << m_fileName
                 << " from size " << m_mmappedSize
                 << " to size " << newsize << endl;

    if (!m_mmappedBuffer) { // nothing to mremap, just mmap
        
        SEQMAN_DEBUG << "SegmentMmapper : nothing to remap - mmap instead\n";
        m_mmappedSize = newsize;
        doMmap();

    } else {

#ifdef linux
        m_mmappedBuffer = (MappedEvent*)::mremap(m_mmappedBuffer, m_mmappedSize,
                                          newsize, MREMAP_MAYMOVE);
#else
	::munmap(m_mmappedBuffer, m_mmappedSize);
	m_mmappedBuffer = (MappedEvent *)::mmap(0, newsize,
					 PROT_READ|PROT_WRITE,
					 MAP_SHARED, m_fd, 0);
#endif
    
        if (m_mmappedBuffer == (void*)-1) {
            SEQMAN_DEBUG << QString("mremap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
            throw Rosegarden::Exception("mremap failed");

        }

        m_mmappedSize = newsize;

    }
    
}

void SegmentMmapper::doMmap()
{
    //
    // mmap() file for writing
    //
    m_mmappedBuffer = (MappedEvent*)::mmap(0, m_mmappedSize,
                                           PROT_READ|PROT_WRITE,
                                           MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {
        SEQMAN_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Rosegarden::Exception("mmap failed");
    }

    SEQMAN_DEBUG << "SegmentMmapper::doMmap() - mmap size : " << m_mmappedSize
                 << " at " << (void*)m_mmappedBuffer << endl;
    
}

void SegmentMmapper::dump()
{
    Composition &comp = m_doc->getComposition();

    Rosegarden::RealTime eventTime;
    Rosegarden::RealTime duration;
    Rosegarden::Track* track = comp.getTrackById(m_segment->getTrack());
    
    Rosegarden::SegmentPerformanceHelper helper(*m_segment);

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    int repeatCount = getSegmentRepeatCount();

    if (repeatCount > 0) repeatEndTime = m_segment->getRepeatEndTime();

    MappedEvent* bufPos = m_mmappedBuffer;

    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {

        for (Segment::iterator j = m_segment->begin();
             j != m_segment->end(); ++j) {

            // Skip rests
            //
            if ((*j)->isa(Rosegarden::Note::EventRestType)) continue;

            timeT playTime =
                helper.getSoundingAbsoluteTime(j) + repeatNo * segmentDuration;
            if (playTime >= repeatEndTime) break;

	    timeT playDuration = helper.getSoundingDuration(j);

            // No duration and we're a note?  Probably in a tied
            // series, but not as first note
            //
            if (playDuration == 0 && (*j)->isa(Rosegarden::Note::EventType))
                continue;
                
	    if (playTime + playDuration > repeatEndTime)
		playDuration = repeatEndTime - playTime;

            eventTime = comp.getElapsedRealTime(playTime);

	    // slightly quicker than calling helper.getRealSoundingDuration()
	    duration =
		comp.getElapsedRealTime(playTime + playDuration) - eventTime;

//            duration = helper.getRealSoundingDuration(j);

            try {
                // Create mapped event in mmapped buffer
                MappedEvent *mE = new (bufPos) MappedEvent(0, // the instrument will be extracted from the ControlBlock by the sequencer
                                                           **j,
                                                           eventTime,
                                                           duration);
                mE->setTrackId(track->getId());

                ++bufPos;

            } catch(...) {
                SEQMAN_DEBUG << "SegmentMmapper::dump - caught exception while trying to create MappedEvent\n";
            }
        }
        
    }

    ::msync(m_mmappedBuffer, m_mmappedSize, MS_ASYNC);
}

unsigned int SegmentMmapper::getSegmentRepeatCount()
{
    int repeatCount = 0;

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    if (m_segment->isRepeating() && segmentDuration > 0) {
	repeatEndTime = m_segment->getRepeatEndTime();
	repeatCount = 1 + (repeatEndTime - segmentEndTime) / segmentDuration;
    }

    return repeatCount;
}

//----------------------------------------

AudioSegmentMmapper::AudioSegmentMmapper(RosegardenGUIDoc* doc, Rosegarden::Segment* s,
                        const QString& fileName)
    : SegmentMmapper(doc, s, fileName)
{
}

size_t AudioSegmentMmapper::computeMmappedSize()
{
    if (!m_segment) return 0;

    int repeatCount = getSegmentRepeatCount();

    return (repeatCount + 1) * 1 * sizeof(MappedEvent);
    // audio segments don't have events, we just need room for 1 MappedEvent
}


void AudioSegmentMmapper::dump()
{
    Composition &comp = m_doc->getComposition();

    Rosegarden::RealTime eventTime;
    Rosegarden::Track* track = comp.getTrackById(m_segment->getTrack());
    
    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    int repeatCount = getSegmentRepeatCount();

    if (repeatCount > 0) repeatEndTime = m_segment->getRepeatEndTime();

    MappedEvent* bufPos = m_mmappedBuffer;

    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {

        timeT playTime =
            segmentStartTime + repeatNo * segmentDuration;
        if (playTime >= repeatEndTime) break;

        eventTime = comp.getElapsedRealTime(playTime);

        Rosegarden::RealTime audioStart    = m_segment->getAudioStartTime();
        Rosegarden::RealTime audioDuration = m_segment->getAudioEndTime() - audioStart;
        Rosegarden::MappedEvent *mE =
            new (bufPos) Rosegarden::MappedEvent(0, // track->getInstrument() - the instrument will be extracted from the ControlBlock by the sequencer
                                                 m_segment->getAudioFileId(),
                                                 eventTime,
                                                 audioDuration,
                                                 audioStart);
        mE->setTrackId(track->getId());
        ++bufPos;
        
    }
    
}


//----------------------------------------

CompositionMmapper::CompositionMmapper(RosegardenGUIDoc *doc)
    : m_doc(doc)
{
    SEQMAN_DEBUG << "CompositionMmapper() - doc = " << doc << endl;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); it++) {

        Rosegarden::Track* track = comp.getTrackById((*it)->getTrack());

        // check to see if track actually exists
        //
        if (track == 0)
            continue;

        mmapSegment(*it);
    }
}

CompositionMmapper::~CompositionMmapper()
{
    SEQMAN_DEBUG << "~CompositionMmapper()\n";

    //
    // Clean up possible left-overs
    //
    cleanup();

    for(segmentmmapers::iterator i = m_segmentMmappers.begin();
        i != m_segmentMmappers.end(); ++i)
        delete i->second;
}

void CompositionMmapper::cleanup()
{
    // In case the sequencer is still running, mapping some segments
    //
    rgapp->sequencerSend("closeAllSegments()");

    // Erase all 'segment_*' files
    //
    QString tmpPath = KGlobal::dirs()->resourceDirs("tmp").first();

    QDir segmentsDir(tmpPath, "segment_*");
    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        QString segmentName = tmpPath + '/' + segmentsDir[i];
        SEQMAN_DEBUG << "CompositionMmapper : cleaning up " << segmentName << endl;
        QFile::remove(segmentName);
    }
    
}


bool CompositionMmapper::segmentModified(Segment* segment)
{
    SegmentMmapper* mmapper = m_segmentMmappers[segment];

    SEQMAN_DEBUG << "CompositionMmapper::segmentModified(" << segment << ") - mmapper = "
                 << mmapper << endl;

    return mmapper->refresh();
}

void CompositionMmapper::segmentAdded(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::segmentAdded(" << segment << ")\n";

    mmapSegment(segment);
}

void CompositionMmapper::segmentDeleted(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::segmentDeleted(" << segment << ")\n";
    SegmentMmapper* mmapper = m_segmentMmappers[segment];
    m_segmentMmappers.erase(segment);
    SEQMAN_DEBUG << "CompositionMmapper::segmentDeleted() : deleting SegmentMmapper " << mmapper << endl;

    delete mmapper;
}

void CompositionMmapper::mmapSegment(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::mmapSegment(" << segment << ")\n";

    SegmentMmapper* mmapper = SegmentMmapperFactory::makeMmapperForSegment(m_doc,
                                                                           segment,
                                                                           makeFileName(segment));

    if (mmapper)
        m_segmentMmappers[segment] = mmapper;
}

QString CompositionMmapper::makeFileName(Segment* segment)
{
    QStringList tmpDirs = KGlobal::dirs()->resourceDirs("tmp");

    return QString("%1/segment_%2")
        .arg(tmpDirs.first())
        .arg((unsigned int)segment, 0, 16);
}

QString CompositionMmapper::getSegmentFileName(Segment* s)
{
    SegmentMmapper* mmapper = m_segmentMmappers[s];
    
    if (mmapper)
        return mmapper->getFileName();
    else
        return QString::null;
}


//----------------------------------------

MetronomeMmapper::MetronomeMmapper(RosegardenGUIDoc* doc)
    : SegmentMmapper(doc, 0, createFileName()),
      m_deleteMetronome(false),
      m_metronome(0), // no metronome to begin with
      m_tickDuration(0, 30000)
{
    SEQMAN_DEBUG << "MetronomeMmapper ctor : " << this << endl;

    // get metronome device
    Rosegarden::Configuration &config = m_doc->getConfiguration();
    int device = config.get<Rosegarden::Int>("metronomedevice", 0);

    Rosegarden::MidiMetronome *metronome = 
        m_doc->getStudio().getMetronomeFromDevice(device);

    if (metronome) {

	SEQMAN_DEBUG << "MetronomeMmapper: have metronome, it's on instrument " << metronome->getInstrument() << endl;

	m_metronome = new Rosegarden::MidiMetronome(*metronome);
    } else {
        m_metronome = new Rosegarden::MidiMetronome
	    (Rosegarden::SystemInstrumentBase);
	SEQMAN_DEBUG << "MetronomeMmapper: no metronome for device " << device << endl;
    }

    Composition& c = m_doc->getComposition();
    Rosegarden::timeT t = c.getBarStart(-20); // somewhat arbitrary
    int depth = m_metronome->getDepth();

    while (t < c.getEndMarker()) {

        Rosegarden::TimeSignature sig = c.getTimeSignatureAt(t);
	Rosegarden::timeT barDuration = sig.getBarDuration();
        std::vector<int> divisions;
        if (depth > 0) divisions = sig.getDivisions(depth - 1);
        int ticks = 1;
        
        for (int i = -1; i < (int)divisions.size(); ++i) {
            if (i >= 0) ticks *= divisions[i];
        
            for (int tick = 0; tick < ticks; ++tick) {
                if (i >= 0 && (tick % divisions[i] == 0)) continue;
                Rosegarden::timeT tickTime = t + (tick * barDuration) / ticks;
                m_ticks.push_back(Tick(tickTime, i+1));
            }
        }
        
        t = c.getBarEndForTime(t);
    }
    
    sortTicks();

    if (m_ticks.size() == 0) {
        SEQMAN_DEBUG << "MetronomeMmapper : WARNING no ticks generated\n";
    }

    // Done by init()

//     m_mmappedSize = computeMmappedSize();
//     if (m_mmappedSize > 0) {
//         setFileSize(m_mmappedSize);
//         doMmap();
//         dump();
//     }
}

MetronomeMmapper::~MetronomeMmapper()
{
    SEQMAN_DEBUG << "~MetronomeMmapper " << this << endl;
    if (m_deleteMetronome) delete m_metronome;
}

Rosegarden::InstrumentId MetronomeMmapper::getMetronomeInstrument()
{
    return m_metronome->getInstrument();
}

QString MetronomeMmapper::createFileName()
{
    return KGlobal::dirs()->resourceDirs("tmp").first() + "/rosegarden_metronome";
}

unsigned int MetronomeMmapper::getSegmentRepeatCount()
{
    return 1;
}

size_t MetronomeMmapper::computeMmappedSize()
{
    return m_ticks.size() * sizeof(MappedEvent);
}

void MetronomeMmapper::dump()
{
    Rosegarden::RealTime eventTime;
    Composition& comp = m_doc->getComposition();

    SEQMAN_DEBUG << "MetronomeMmapper::dump: instrument is " << m_metronome->getInstrument() << endl;

    MappedEvent* bufPos = m_mmappedBuffer;
    for (TickContainer::iterator i = m_ticks.begin(); i != m_ticks.end(); ++i) {

        Rosegarden::MidiByte velocity;
	switch (i->second) {
	case 0:  velocity = m_metronome->getBarVelocity(); break;
	case 1:  velocity = m_metronome->getBeatVelocity(); break;
	default: velocity = m_metronome->getSubBeatVelocity(); break;
	}

        eventTime = comp.getElapsedRealTime(i->first);

        new (bufPos) MappedEvent(m_metronome->getInstrument(),
                                 m_metronome->getPitch(),
                                 velocity,
                                 eventTime,
                                 m_tickDuration);
        ++bufPos;
    }
}


bool operator<(MetronomeMmapper::Tick a, MetronomeMmapper::Tick b)
{
    return a.first < b.first;
}

void MetronomeMmapper::sortTicks()
{
    sort(m_ticks.begin(), m_ticks.end());
}


//----------------------------------------
using Rosegarden::Composition;

ReferenceSegmentMmapper::ReferenceSegmentMmapper(RosegardenGUIDoc* doc,
                                                 const Composition::ReferenceSegment& refSeg,
                                                 QString baseFileName)
    : SegmentMmapper(doc, 0, createFileName(baseFileName)),
      m_referenceSegment(refSeg)
{
}

unsigned int ReferenceSegmentMmapper::getSegmentRepeatCount()
{
    return 1;
}

size_t ReferenceSegmentMmapper::computeMmappedSize()
{
    return m_referenceSegment.size() * sizeof(MappedEvent);
}

QString ReferenceSegmentMmapper::createFileName(QString baseFileName)
{
    return KGlobal::dirs()->resourceDirs("tmp").first() + "/" + baseFileName;
}

void ReferenceSegmentMmapper::dump()
{
    Rosegarden::RealTime eventTime;
    static Rosegarden::RealTime noDuration(0,0);

    Composition& comp = m_doc->getComposition();
    MappedEvent* bufPos = m_mmappedBuffer;

    for (Composition::ReferenceSegment::iterator i = m_referenceSegment.begin();
         i != m_referenceSegment.end(); ++i) {

        Rosegarden::Event* event = *i;

        eventTime = comp.getElapsedRealTime(event->getAbsoluteTime());
        new (bufPos) MappedEvent(0, *event, eventTime, noDuration);

        ++bufPos;
    }
}

//----------------------------------------

SegmentMmapper* SegmentMmapperFactory::makeMmapperForSegment(RosegardenGUIDoc* doc,
                                                             Rosegarden::Segment* segment,
                                                             const QString& fileName)
{
    SegmentMmapper* mmapper = 0;

    if (segment == 0) {
        SEQMAN_DEBUG << "SegmentMmapperFactory::makeMmapperForSegment() segment == 0\n";
        return 0;
    }
    
    switch (segment->getType()) {
    case Segment::Internal :
        mmapper = new SegmentMmapper(doc, segment, fileName);
        break;
    case Segment::Audio :
        mmapper = new AudioSegmentMmapper(doc, segment, fileName);
        break;
    default:
        SEQMAN_DEBUG << "SegmentMmapperFactory::makeMmapperForSegment(" << segment
                     << ") : can't map, unknown segment type " << segment->getType() << endl;
        mmapper = 0;
    }
    
    if (mmapper)
        mmapper->init();

    return mmapper;
}

MetronomeMmapper* SegmentMmapperFactory::makeMetronome(RosegardenGUIDoc* doc)
{
    MetronomeMmapper* mmapper = new MetronomeMmapper(doc);
    mmapper->init();
    
    return mmapper;
}

ReferenceSegmentMmapper* SegmentMmapperFactory::makeTimeSig(RosegardenGUIDoc* doc)
{
    ReferenceSegmentMmapper* mmapper = new ReferenceSegmentMmapper(doc,
                                                                   doc->getComposition().getTimeSigSegment(),
                                                                   "rosegarden_timesig");
    
    mmapper->init();
    return mmapper;
}

ReferenceSegmentMmapper* SegmentMmapperFactory::makeTempo(RosegardenGUIDoc* doc)
{
    ReferenceSegmentMmapper* mmapper = new ReferenceSegmentMmapper(doc,
                                                                   doc->getComposition().getTempoSegment(),
                                                                   "rosegarden_tempo");
    
    mmapper->init();
    return mmapper;
}
