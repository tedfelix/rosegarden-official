// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
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
#include <kconfig.h>
#include <algorithm>

#include "mmapper.h"

#include "Segment.h"
#include "Track.h"
#include "Composition.h"
#include "ControlBlock.h"
#include "MappedEvent.h"
#include "SegmentPerformanceHelper.h"
#include "BaseProperties.h"
#include "Midi.h"

#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"
#include "rgapplication.h"
#include "constants.h"

#include <stdint.h>

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
using Rosegarden::Int;
using Rosegarden::Bool;
using namespace Rosegarden::BaseProperties;

ControlBlockMmapper::ControlBlockMmapper(RosegardenGUIDoc* doc)
    : m_doc(doc),
      m_fileName(createFileName()),
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
    return KGlobal::dirs()->resourceDirs("tmp").last() + "/rosegarden_control_block";
}

void ControlBlockMmapper::updateTrackData(Track *t)
{
    m_controlBlock->updateTrackData(t);
}

void ControlBlockMmapper::setTrackDeleted(Rosegarden::TrackId t)
{
    m_controlBlock->setTrackDeleted(t, true);
}

void ControlBlockMmapper::updateMidiFilters(Rosegarden::MidiFilter thruFilter,
					    Rosegarden::MidiFilter recordFilter)
{
    m_controlBlock->setThruFilter(thruFilter);
    m_controlBlock->setRecordFilter(recordFilter);
}

void ControlBlockMmapper::updateMetronomeData(Rosegarden::InstrumentId instId)
{
    m_controlBlock->setInstrumentForMetronome(instId);
}

void ControlBlockMmapper::updateMetronomeForPlayback()
{
    bool muted = !m_doc->getComposition().usePlayMetronome();
    SEQMAN_DEBUG << "ControlBlockMmapper::updateMetronomeForPlayback: muted=" << muted << endl;
    if (m_controlBlock->isMetronomeMuted() == muted) return;
    m_controlBlock->setMetronomeMuted(muted);
}

void ControlBlockMmapper::updateMetronomeForRecord()
{
    bool muted = !m_doc->getComposition().useRecordMetronome();
    SEQMAN_DEBUG << "ControlBlockMmapper::updateMetronomeForRecord: muted=" << muted << endl;
    if (m_controlBlock->isMetronomeMuted() == muted) return;
    m_controlBlock->setMetronomeMuted(muted);
}

void ControlBlockMmapper::updateSoloData(bool solo,
                                         Rosegarden::TrackId selectedTrack)
{
    m_controlBlock->setSolo(solo);
    m_controlBlock->setSelectedTrack(selectedTrack);
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
        if (track == 0) continue;
        
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
        throw Rosegarden::Exception("lseek failed");
    }
    
    if (::write(m_fd, "\0", 1) != 1) {
	std::cerr << "WARNING: ControlBlockMmapper : Couldn't write byte in  "
		  << m_fileName << std::endl;
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
      m_mmappedRegion(0),
      m_mmappedEventBuffer((MappedEvent*)0)
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
    m_mmappedSize = computeMmappedSize() + sizeof(size_t);

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


size_t SegmentMmapper::addMmappedSize(Segment *s)
{
    int repeatCount = getSegmentRepeatCount();
    return m_mmappedSize + (repeatCount + 1) * s->size() * sizeof(MappedEvent);
}


SegmentMmapper::~SegmentMmapper()
{
    SEQMAN_DEBUG << "~SegmentMmapper : " << this
                 << " unmapping " << (void*)m_mmappedRegion
                 << " of size " << m_mmappedSize
                 << endl;

    if (m_mmappedRegion && m_mmappedSize)
        ::munmap(m_mmappedRegion, m_mmappedSize);

    ::close(m_fd);
    SEQMAN_DEBUG << "~SegmentMmapper : removing " << m_fileName << endl;

    QFile::remove(m_fileName);
}


bool SegmentMmapper::refresh()
{
    bool res = false;

    size_t newMmappedSize = computeMmappedSize() + sizeof(size_t);

    SEQMAN_DEBUG << "SegmentMmapper::refresh() - " << getFileName()
                 << " - m_mmappedRegion = " << (void*)m_mmappedRegion
                 << " - m_mmappedEventBuffer = " << (void*)m_mmappedEventBuffer
                 << " - new size = " << newMmappedSize
                 << " - old size = " << m_mmappedSize
                 << endl;

    // We can't zero out the buffer here because if the mmapped
    // segment is being read from by the sequencer in the interval of
    // time between the memset() and the dump(), the sequencer will go
    // over all the zeros up to the end of the segment and reach its
    // end, and therefore will stop playing it.
    // 

    if (newMmappedSize != m_mmappedSize) {

        res = true;

        if (newMmappedSize == 0) {

            // nothing to do, just msync and go
            ::msync(m_mmappedRegion, m_mmappedSize, MS_ASYNC);
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

	// Don't truncate the file here: that will cause trouble for
	// the sequencer
//        ftruncate(m_fd, size);

    } else {

        // On linux, ftruncate can enlarge a file, but this isn't specified by POSIX
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
	    std::cerr << "WARNING: SegmentMmapper : Couldn't lseek in "
		      << m_fileName << " to " << size << std::endl;
            throw Rosegarden::Exception("lseek failed");
        }
    
        if (::write(m_fd, "\0", 1) != 1) {
	    std::cerr << "WARNING: SegmentMmapper : Couldn't write byte in  "
		      << m_fileName << std::endl;
            throw Rosegarden::Exception("write failed");
        }
    
    }
    
    
}

void SegmentMmapper::remap(size_t newsize)
{
    SEQMAN_DEBUG << "SegmentMmapper : remapping " << m_fileName
                 << " from size " << m_mmappedSize
                 << " to size " << newsize << endl;

    if (!m_mmappedRegion) { // nothing to mremap, just mmap
        
        SEQMAN_DEBUG << "SegmentMmapper : nothing to remap - mmap instead\n";
        m_mmappedSize = newsize;
        doMmap();

    } else {

#ifdef linux
	void *oldBuffer = m_mmappedRegion;
        m_mmappedRegion = (MappedEvent*)::mremap(m_mmappedRegion, m_mmappedSize,
						 newsize, MREMAP_MAYMOVE);
	m_mmappedEventBuffer = (MappedEvent *)((size_t *)m_mmappedRegion + 1);

	if (m_mmappedRegion != oldBuffer) {
	    SEQMAN_DEBUG << "NOTE: buffer moved from " << oldBuffer <<
		" to " << (void *)m_mmappedRegion << endl;
	}
#else
	::munmap(m_mmappedRegion, m_mmappedSize);
	m_mmappedRegion = ::mmap(0, newsize,
				 PROT_READ|PROT_WRITE,
				 MAP_SHARED, m_fd, 0);
	m_mmappedEventBuffer = (MappedEvent *)((size_t *)m_mmappedRegion + 1);
#endif
    
        if (m_mmappedRegion == (void*)-1) {
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
    m_mmappedRegion = ::mmap(0, m_mmappedSize,
			     PROT_READ|PROT_WRITE,
			     MAP_SHARED, m_fd, 0);
    m_mmappedEventBuffer = (MappedEvent *)((size_t *)m_mmappedRegion + 1);

    if (m_mmappedRegion == (void*)-1) {
        SEQMAN_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Rosegarden::Exception("mmap failed");
    }

    SEQMAN_DEBUG << "SegmentMmapper::doMmap() - mmap size : " << m_mmappedSize
                 << " at " << (void*)m_mmappedRegion << endl;
    
}

void SegmentMmapper::dump()
{
    Composition &comp = m_doc->getComposition();

    Rosegarden::RealTime eventTime;
    Rosegarden::RealTime duration;
    Rosegarden::Track* track = comp.getTrackById(m_segment->getTrack());
    
    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    int repeatCount = getSegmentRepeatCount();

    if (repeatCount > 0) repeatEndTime = m_segment->getRepeatEndTime();

    MappedEvent* bufPos = m_mmappedEventBuffer;

    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {

	Segment *triggered = 0;
	Segment::iterator *i = 0;

        for (Segment::iterator j = m_segment->begin();
	     m_segment->isBeforeEndMarker(j) || (i && *i != triggered->end()); ) {

	    bool usingi = false;
	    Segment::iterator *k = &j;

	    if (i && *i != triggered->end() &&
		(!m_segment->isBeforeEndMarker(j) ||
		 (**i)->getAbsoluteTime() < (*j)->getAbsoluteTime())) {
		k = i;
		usingi = true;
	    }

	    if (!usingi) { // don't permit nested triggered segments

		long triggerId = -1;
		(**k)->get<Int>(TRIGGER_SEGMENT_ID, triggerId);

		if (triggerId >= 0) {

		    Rosegarden::TriggerSegmentRec *rec =
			comp.getTriggerSegmentRec(triggerId);

		    if (rec && rec->getSegment()) {
			Rosegarden::timeT performanceDuration =
			    Rosegarden::SegmentPerformanceHelper(*m_segment).
			    getSoundingDuration(j);
			if (performanceDuration > 0) {
			    mergeTriggerSegment(&triggered, *j,
						performanceDuration, rec);
			    size_t sz = addMmappedSize(rec->getSegment());
			    size_t offset = bufPos - m_mmappedEventBuffer;
			    setFileSize(sz);
			    remap(sz);
			    bufPos = m_mmappedEventBuffer + offset;
			}
		    }

		    if (triggered) {
			if (i) delete i;
			i = new Segment::iterator
			    (triggered->findTime((*j)->getAbsoluteTime()));
		    }

		    // Use the next triggered event (presumably the
		    // first of the current triggered segment) instead
		    // of the one that triggered it

		    ++j; // whatever happens, we don't want to write this one

		    if (i && *i != triggered->end() &&
			(!m_segment->isBeforeEndMarker(j) ||
			 ((**i)->getAbsoluteTime() < (*j)->getAbsoluteTime()))) {
			k = i;
			usingi = true;
		    } else {
			// no joy at all
			continue; 
		    }
		}
	    }

	    // Ignore rests
	    //
            if (!(**k)->isa(Rosegarden::Note::EventRestType)) {

		Rosegarden::SegmentPerformanceHelper helper
		    (usingi ? *triggered : *m_segment);

		timeT playTime =
		    helper.getSoundingAbsoluteTime(*k) + repeatNo * segmentDuration;
		if (playTime >= repeatEndTime) break;

		timeT playDuration = helper.getSoundingDuration(*k);

		// Ignore notes without duration -- they're probably in a tied
		// series but not as first note
		//
		if (playDuration > 0 || !(**k)->isa(Rosegarden::Note::EventType)) {
                
		    if (playTime + playDuration > repeatEndTime)
			playDuration = repeatEndTime - playTime;

		    playTime = playTime + m_segment->getDelay();
		    eventTime = comp.getElapsedRealTime(playTime);

		    // slightly quicker than calling helper.getRealSoundingDuration()
		    duration =
			comp.getElapsedRealTime(playTime + playDuration) - eventTime;

		    eventTime = eventTime + m_segment->getRealTimeDelay();

		    try {
			// Create mapped event in mmapped buffer.  The
			// instrument will be extracted from the ControlBlock
			// by the sequencer, so we set it to zero here.
			MappedEvent *mE = new (bufPos)
			    MappedEvent(0,
					***k, // three stars! what an accolade
					eventTime,
					duration);
			mE->setTrackId(track->getId());

			if (m_segment->getTranspose() != 0 &&
			    (**k)->isa(Rosegarden::Note::EventType)) {
			    mE->setPitch(mE->getPitch() + m_segment->getTranspose());
			}

			++bufPos;

		    } catch(...) {
			SEQMAN_DEBUG << "SegmentMmapper::dump - caught exception while trying to create MappedEvent\n";
		    }
		}
	    }
	
	    ++*k; // increment either i or j, whichever one we just used
        }

	delete i;
	delete triggered;
    }

    // Store the number of events at the start of the shared memory region
    *(size_t *)m_mmappedRegion = (bufPos - m_mmappedEventBuffer);

    size_t coveredArea = (bufPos - m_mmappedEventBuffer) * sizeof(MappedEvent);
    memset(bufPos, 0, m_mmappedSize - coveredArea - sizeof(size_t));

    ::msync(m_mmappedRegion, m_mmappedSize, MS_ASYNC);
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

void
SegmentMmapper::mergeTriggerSegment(Rosegarden::Segment **target,
				    Rosegarden::Event *trigger,
				    Rosegarden::timeT evDuration,
				    Rosegarden::TriggerSegmentRec *rec)
{
    if (!rec || !rec->getSegment() || rec->getSegment()->empty()) return;
    if (!*target) *target = new Rosegarden::Segment;

    Rosegarden::timeT evTime = trigger->getAbsoluteTime();
    Rosegarden::timeT trStart = rec->getSegment()->getStartTime();
    Rosegarden::timeT trEnd = rec->getSegment()->getEndMarkerTime();
    Rosegarden::timeT trDuration = trEnd - trStart;
    if (trDuration == 0) return;

    bool retune = false;
    std::string timeAdjust = TRIGGER_SEGMENT_ADJUST_NONE;

    trigger->get<Bool>
	(TRIGGER_SEGMENT_RETUNE, retune);
    
    trigger->get<Rosegarden::String>
	(TRIGGER_SEGMENT_ADJUST_TIMES, timeAdjust);

    long evPitch = rec->getBasePitch();
    (void)trigger->get<Int>(PITCH, evPitch);
    int pitchDiff = evPitch - rec->getBasePitch();

    long evVelocity = rec->getBaseVelocity();
    (void)trigger->get<Int>(VELOCITY, evVelocity);
    int velocityDiff = evVelocity - rec->getBaseVelocity();

    Rosegarden::timeT offset = 0;
    if (timeAdjust == TRIGGER_SEGMENT_ADJUST_SYNC_END) {
	offset = evDuration - trDuration;
    }

    for (Segment::iterator i = rec->getSegment()->begin();
	 rec->getSegment()->isBeforeEndMarker(i); ++i) {
	
	Rosegarden::timeT t = (*i)->getAbsoluteTime() - trStart;
	Rosegarden::timeT d = (*i)->getDuration();

	if (evDuration != trDuration &&
	    timeAdjust == TRIGGER_SEGMENT_ADJUST_SQUISH) {
	    t = Rosegarden::timeT(double(t * evDuration) / double(trDuration));
	    d = Rosegarden::timeT(double(d * evDuration) / double(trDuration));
	}

	t += evTime + offset;

	if (t < evTime) {
	    if (t + d <= evTime) continue;
	    else {
		d -= (evTime - t);
		t = evTime;
	    }
	}
	
	if (timeAdjust == TRIGGER_SEGMENT_ADJUST_SYNC_START) {
	    if (t + d > evTime + evDuration) {
		if (t >= evTime + evDuration) continue;
		else {
		    d = evTime + evDuration - t;
		}
	    }
	}

	Rosegarden::Event *newEvent = new Rosegarden::Event(**i, t, d);

	if (retune && newEvent->has(PITCH)) {
	    int pitch = newEvent->get<Int>(PITCH) + pitchDiff;
	    if (pitch > 127) pitch = 127;
	    if (pitch < 0) pitch = 0;
	    newEvent->set<Int>(PITCH, pitch);
	}

	if (newEvent->has(VELOCITY)) {
	    int velocity = newEvent->get<Int>(VELOCITY) + velocityDiff;
	    if (velocity > 127) velocity = 127;
	    if (velocity < 0) velocity = 0;
	    newEvent->set<Int>(VELOCITY, velocity);
	}

	(*target)->insert(newEvent);
    }
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
    
    // Can't write out if no track
    if (!track) {
	std::cerr << "AudioSegmentMmapper::dump: ERROR: No track for segment!"
		  << std::endl;
	return;
    }

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    //!!! The repeat count is actually not quite right for audio
    // segments -- it returns one too many for repeating segments,
    // because in midi segments you want that (to deal with partial
    // repeats).  Here we really need to find a better way to deal
    // with partial repeats...

    int repeatCount = getSegmentRepeatCount();
    if (repeatCount > 0) repeatEndTime = m_segment->getRepeatEndTime();

    MappedEvent* bufPos = m_mmappedEventBuffer;

    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {

        timeT playTime =
            segmentStartTime + repeatNo * segmentDuration;
        if (playTime >= repeatEndTime) break;

	playTime = playTime + m_segment->getDelay();
        eventTime = comp.getElapsedRealTime(playTime);
	eventTime = eventTime + m_segment->getRealTimeDelay();

        Rosegarden::RealTime audioStart    = m_segment->getAudioStartTime();
        Rosegarden::RealTime audioDuration = m_segment->getAudioEndTime() - audioStart;
        Rosegarden::MappedEvent *mE =
            new (bufPos) Rosegarden::MappedEvent(track->getInstrument(), // send instrument for audio
                                                 m_segment->getAudioFileId(),
                                                 eventTime,
                                                 audioDuration,
                                                 audioStart);
        mE->setTrackId(track->getId());
        mE->setRuntimeSegmentId(m_segment->getRuntimeId());

        // Send the autofade if required
        //
        if (m_segment->isAutoFading())
        {
            mE->setAutoFade(true);
            mE->setFadeInTime(m_segment->getFadeInTime());
            mE->setFadeOutTime(m_segment->getFadeOutTime());
            std::cout << "AudioSegmentMmapper::dump - "
                      << "SETTING AUTOFADE "
                      << "in = " << m_segment->getFadeInTime()
                      << ", out = " << m_segment->getFadeOutTime()
                      << std::endl;
        }
        else
        {
//            std::cout << "AudioSegmentMmapper::dump - "
//                      << "NO AUTOFADE SET ON SEGMENT" << std::endl;
        }

        ++bufPos;
    }

    *(size_t *)m_mmappedRegion = repeatCount + 1;
}


//----------------------------------------

CompositionMmapper::CompositionMmapper(RosegardenGUIDoc *doc)
    : m_doc(doc)
{
    cleanup();

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
    QString tmpPath = KGlobal::dirs()->resourceDirs("tmp").last();

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

    if (!mmapper)
        return false; // this can happen with the SegmentSplitCommand, where the new segment's transpose is set
    // even though it's not mapped yet

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
        .arg(tmpDirs.last())
        .arg((uintptr_t)segment, 0, 16);
}

QString CompositionMmapper::getSegmentFileName(Segment* s)
{
    SegmentMmapper* mmapper = m_segmentMmappers[s];
    
    if (mmapper)
        return mmapper->getFileName();
    else
        return QString::null;
}

size_t CompositionMmapper::getSegmentFileSize(Segment* s)
{
    SegmentMmapper* mmapper = m_segmentMmappers[s];
    
    if (mmapper)
        return mmapper->getFileSize();
    else
        return 0;
}


//----------------------------------------

MetronomeMmapper::MetronomeMmapper(RosegardenGUIDoc* doc)
    : SegmentMmapper(doc, 0, createFileName()),
      m_metronome(0), // no metronome to begin with
      m_tickDuration(0, 100000000)
{
    SEQMAN_DEBUG << "MetronomeMmapper ctor : " << this << endl;

    // get metronome device
    Rosegarden::Studio &studio = m_doc->getStudio();
    int device = studio.getMetronomeDevice();

    const Rosegarden::MidiMetronome *metronome = 
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

    if (depth > 0) {
	while (t < c.getEndMarker()) {

	    Rosegarden::TimeSignature sig = c.getTimeSignatureAt(t);
	    Rosegarden::timeT barDuration = sig.getBarDuration();
	    std::vector<int> divisions;
	    if (depth > 0) sig.getDivisions(depth - 1, divisions);
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
    }
    
    KConfig *config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
    bool midiClock = config->readBoolEntry("midiclock", false);
    bool mtcMaster = config->readBoolEntry("mtcmaster", false);

    if (midiClock)
    {
        using Rosegarden::Note;
        Rosegarden::timeT quarterNote = Note(Note::Crotchet).getDuration();

        // Insert 24 clocks per quarter note
        //
        for (Rosegarden::timeT insertTime = c.getStartMarker();
             insertTime < c.getEndMarker(); 
             insertTime += quarterNote / 24)
        {
            m_ticks.push_back(Tick(insertTime, 3));
        }
    }


    if (mtcMaster)
    {
        // do something
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
    delete m_metronome;
}

Rosegarden::InstrumentId MetronomeMmapper::getMetronomeInstrument()
{
    return m_metronome->getInstrument();
}

QString MetronomeMmapper::createFileName()
{
    return KGlobal::dirs()->resourceDirs("tmp").last() + "/rosegarden_metronome";
}

unsigned int MetronomeMmapper::getSegmentRepeatCount()
{
    return 1;
}

size_t MetronomeMmapper::computeMmappedSize()
{
    KConfig *config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
    bool midiClock = config->readBoolEntry("midiclock", false);
    bool mtcMaster = config->readBoolEntry("mtcmaster", false);

    // base size for Metronome ticks
    size_t size = m_ticks.size() * sizeof(MappedEvent);
    Composition& comp = m_doc->getComposition();

    if (midiClock)
    {
        using Rosegarden::Note;

        // Allow room for MIDI clocks
        int clocks = ( 24 * ( comp.getEndMarker() - comp.getStartMarker() ) ) / 
            Note(Note::Crotchet).getDuration();

        /*
        SEQMAN_DEBUG << "MetronomeMmapper::computeMmappedSize - " 
                     << "Number of clock events catered for = " << clocks
                     << endl;
        */

        size += clocks * sizeof(MappedEvent);
    }

    if (mtcMaster)
    {
        // Allow room for MTC timing messages (how?)
    }

    return size;
}

void MetronomeMmapper::dump()
{
    Rosegarden::RealTime eventTime;
    Composition& comp = m_doc->getComposition();

    SEQMAN_DEBUG << "MetronomeMmapper::dump: instrument is " << m_metronome->getInstrument() << endl;

    MappedEvent* bufPos = m_mmappedEventBuffer, *mE;

    for (TickContainer::iterator i = m_ticks.begin(); i != m_ticks.end(); ++i) {

        /*
        SEQMAN_DEBUG << "MetronomeMmapper::dump: velocity = "
                     << int(velocity) << endl;
                     */

        eventTime = comp.getElapsedRealTime(i->first);

        if (i->second == 3) // MIDI Clock
        {
            mE = new (bufPos) MappedEvent(0, MappedEvent::MidiSystemMessage);
            mE->setData1(Rosegarden::MIDI_TIMING_CLOCK);
            mE->setEventTime(eventTime);
        }
        else
        {
            Rosegarden::MidiByte velocity;
            Rosegarden::MidiByte pitch;
	    switch (i->second) {
	    case 0:  
	    	velocity = m_metronome->getBarVelocity(); 
	    	pitch = m_metronome->getBarPitch();
	    	break;
	    case 1:  
	    	velocity = m_metronome->getBeatVelocity(); 
	    	pitch = m_metronome->getBeatPitch();
	    	break;
	    default: 
	    	velocity = m_metronome->getSubBeatVelocity(); 
	    	pitch = m_metronome->getSubBeatPitch();
	    	break;
            }

            new (bufPos) MappedEvent(m_metronome->getInstrument(),
				     MappedEvent::MidiNoteOneShot,
                                     pitch,
                                     velocity,
                                     eventTime,
                                     m_tickDuration,
				     Rosegarden::RealTime::zeroTime);
        }

        ++bufPos;
    }

    // Store the number of events at the start of the shared memory region
    *(size_t *)m_mmappedRegion = (bufPos - m_mmappedEventBuffer);

    SEQMAN_DEBUG << "MetronomeMmapper::dump: - "
                 << "Total events written = " << *(size_t *)m_mmappedRegion
                 << endl;
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

SpecialSegmentMmapper::SpecialSegmentMmapper(RosegardenGUIDoc* doc,
                                             QString baseFileName)
    : SegmentMmapper(doc, 0, createFileName(baseFileName))
{
}

unsigned int SpecialSegmentMmapper::getSegmentRepeatCount()
{
    return 1;
}

QString SpecialSegmentMmapper::createFileName(QString baseFileName)
{
    return KGlobal::dirs()->resourceDirs("tmp").last() + "/" + baseFileName;
}

//----------------------------------------

size_t TempoSegmentMmapper::computeMmappedSize()
{
    return m_doc->getComposition().getTempoChangeCount() * sizeof(MappedEvent);
}

void TempoSegmentMmapper::dump()
{
    Rosegarden::RealTime eventTime;

    Composition& comp = m_doc->getComposition();
    MappedEvent* bufPos = m_mmappedEventBuffer;

    for (int i = 0; i < comp.getTempoChangeCount(); ++i) {

        std::pair<timeT, Rosegarden::tempoT> tempoChange = comp.getTempoChange(i);

        eventTime = comp.getElapsedRealTime(tempoChange.first);
        MappedEvent* mappedEvent = new (bufPos) MappedEvent();
        mappedEvent->setType(MappedEvent::Tempo);
        mappedEvent->setEventTime(eventTime);

	// Nasty hack -- we use the instrument ID to pass through the
	// raw tempo value, as it has the appropriate range (unlike
	// e.g. tempo1 + tempo2).  These events are not actually used
	// on the sequencer side yet, so this may change to something
	// nicer at some point.
        mappedEvent->setInstrument(tempoChange.second);
        
        ++bufPos;
    }

    // Store the number of events at the start of the shared memory region
    *(size_t *)m_mmappedRegion = (bufPos - m_mmappedEventBuffer);
}

//----------------------------------------

size_t TimeSigSegmentMmapper::computeMmappedSize()
{
    return m_doc->getComposition().getTimeSignatureCount() * sizeof(MappedEvent);
}

void TimeSigSegmentMmapper::dump()
{
    Rosegarden::RealTime eventTime;

    Composition& comp = m_doc->getComposition();
    MappedEvent* bufPos = m_mmappedEventBuffer;

    for (int i = 0; i < comp.getTimeSignatureCount(); ++i) {

        std::pair<timeT, Rosegarden::TimeSignature> timeSigChange = comp.getTimeSignatureChange(i);

        eventTime = comp.getElapsedRealTime(timeSigChange.first);
        MappedEvent* mappedEvent = new (bufPos) MappedEvent();
        mappedEvent->setType(MappedEvent::TimeSignature);
        mappedEvent->setEventTime(eventTime);
        mappedEvent->setData1(timeSigChange.second.getNumerator());
        mappedEvent->setData2(timeSigChange.second.getDenominator());
        
        ++bufPos;
    }

    // Store the number of events at the start of the shared memory region
    *(size_t *)m_mmappedRegion = (bufPos - m_mmappedEventBuffer);
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

TimeSigSegmentMmapper* SegmentMmapperFactory::makeTimeSig(RosegardenGUIDoc* doc)
{
    TimeSigSegmentMmapper* mmapper = new TimeSigSegmentMmapper(doc, "rosegarden_timesig");
    
    mmapper->init();
    return mmapper;
}

TempoSegmentMmapper* SegmentMmapperFactory::makeTempo(RosegardenGUIDoc* doc)
{
    TempoSegmentMmapper* mmapper = new TempoSegmentMmapper(doc, "rosegarden_tempo");
    
    mmapper->init();
    return mmapper;
}
