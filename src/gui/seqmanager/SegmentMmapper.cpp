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


#include "SegmentMmapper.h"
#include "misc/Debug.h"

#include "misc/Strings.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/SegmentPerformanceHelper.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenGUIDoc.h"
#include "sound/MappedEvent.h"
#include <QFile>
#include <QString>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>


namespace Rosegarden
{

SegmentMmapper::SegmentMmapper(RosegardenGUIDoc* doc,
                               Segment* segment, const QString& fileName)
        : m_doc(doc),
        m_segment(segment),
        m_fileName(fileName),
        m_fd( -1),
        m_mmappedSize(0),
        m_mmappedRegion(0),
        m_mmappedEventBuffer((MappedEvent*)0)
{
    SEQMAN_DEBUG << "SegmentMmapper : " << this
    << " trying to mmap " << m_fileName
    << endl;

    m_fd = ::open(m_fileName.toLatin1().data(), O_RDWR | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR);
    if (m_fd < 0) {
        perror("SegmentMmapper::SegmentMmapper: Failed to open mmap file for writing");
        SEQMAN_DEBUG << "SegmentMmapper : Couldn't open " << m_fileName
        << endl;
        throw Exception("Couldn't open " + qstrtostr(m_fileName));
    }

    //    SEQMAN_DEBUG << "SegmentMmapper : mmap size = " << m_mmappedSize
    //                 << endl;
}

void SegmentMmapper::init()
{
    m_mmappedSize = computeMmappedSize() + sizeof(size_t);

    if (m_mmappedSize > 0) {
        setFileSize(m_mmappedSize);
        doMmap();
        dump();
        if (m_segment != 0) {
            SEQMAN_DEBUG << "SegmentMmapper::init : mmap size = " << m_mmappedSize
            << " for segment " << m_segment->getLabel() << endl;
        }

    } else {
        SEQMAN_DEBUG << "SegmentMmapper::init : mmap size = 0 - skipping mmapping for now\n";
    }
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

    QFile::remove
        (m_fileName);
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
            return ;
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
            throw Exception("lseek failed");
        }

        if (::write(m_fd, "\0", 1) != 1) {
            std::cerr << "WARNING: SegmentMmapper : Couldn't write byte in  "
            << m_fileName << std::endl;
            throw Exception("write failed");
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
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED, m_fd, 0);
        m_mmappedEventBuffer = (MappedEvent *)((size_t *)m_mmappedRegion + 1);
#endif

        if (m_mmappedRegion == (void*) - 1) {
            SEQMAN_DEBUG << QString("mremap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
            throw Exception("mremap failed");
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
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, m_fd, 0);
    m_mmappedEventBuffer = (MappedEvent *)((size_t *)m_mmappedRegion + 1);

    if (m_mmappedRegion == (void*) - 1) {
        SEQMAN_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Exception("mmap failed");
    }

    SEQMAN_DEBUG << "SegmentMmapper::doMmap() - mmap size : " << m_mmappedSize
    << " at " << (void*)m_mmappedRegion << endl;

}

void SegmentMmapper::dump()
{
    Composition &comp = m_doc->getComposition();

    RealTime eventTime;
    RealTime duration;
    Track* track = comp.getTrackById(m_segment->getTrack());

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    int repeatCount = getSegmentRepeatCount();

    if (repeatCount > 0)
        repeatEndTime = m_segment->getRepeatEndTime();

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
                (**k)->get
                <Int>(BaseProperties::TRIGGER_SEGMENT_ID, triggerId);

                if (triggerId >= 0) {

                    TriggerSegmentRec *rec =
                        comp.getTriggerSegmentRec(triggerId);

                    if (rec && rec->getSegment()) {
                        timeT performanceDuration =
                            SegmentPerformanceHelper(*m_segment).
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
                        if (i)
                            delete i;
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
            if (!(**k)->isa(Note::EventRestType)) {

                SegmentPerformanceHelper helper
                (usingi ? *triggered : *m_segment);

                timeT playTime =
                    helper.getSoundingAbsoluteTime(*k) + repeatNo * segmentDuration;
                if (playTime >= repeatEndTime)
                    break;

                timeT playDuration = helper.getSoundingDuration(*k);

                // Ignore notes without duration -- they're probably in a tied
                // series but not as first note
                //
                if (playDuration > 0 || !(**k)->isa(Note::EventType)) {

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
                                                      ***k,  // three stars! what an accolade
                                                      eventTime,
                                                      duration);
                        mE->setTrackId(track->getId());

                        if (m_segment->getTranspose() != 0 &&
                                (**k)->isa(Note::EventType)) {
                            mE->setPitch(mE->getPitch() + m_segment->getTranspose());
                        }

                        ++bufPos;

                    } catch (...) {
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

void
SegmentMmapper::mergeTriggerSegment(Segment **target,
                                    Event *trigger,
                                    timeT evDuration,
                                    TriggerSegmentRec *rec)
{
    if (!rec || !rec->getSegment() || rec->getSegment()->empty())
        return ;
    if (!*target)
        *target = new Segment;

    timeT evTime = trigger->getAbsoluteTime();
    timeT trStart = rec->getSegment()->getStartTime();
    timeT trEnd = rec->getSegment()->getEndMarkerTime();
    timeT trDuration = trEnd - trStart;
    if (trDuration == 0)
        return ;

    bool retune = false;
    std::string timeAdjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;

    trigger->get
    <Bool>
    (BaseProperties::TRIGGER_SEGMENT_RETUNE, retune);

    trigger->get
    <String>
    (BaseProperties::TRIGGER_SEGMENT_ADJUST_TIMES, timeAdjust);

    long evPitch = rec->getBasePitch();
    (void)trigger->get
    <Int>(BaseProperties::PITCH, evPitch);
    int pitchDiff = evPitch - rec->getBasePitch();

    long evVelocity = rec->getBaseVelocity();
    (void)trigger->get
    <Int>(BaseProperties::VELOCITY, evVelocity);
    int velocityDiff = evVelocity - rec->getBaseVelocity();

    timeT offset = 0;
    if (timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
        offset = evDuration - trDuration;
    }

    for (Segment::iterator i = rec->getSegment()->begin();
            rec->getSegment()->isBeforeEndMarker(i); ++i) {

        timeT t = (*i)->getAbsoluteTime() - trStart;
        timeT d = (*i)->getDuration();

        if (evDuration != trDuration &&
                timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) {
            t = timeT(double(t * evDuration) / double(trDuration));
            d = timeT(double(d * evDuration) / double(trDuration));
        }

        t += evTime + offset;

        if (t < evTime) {
            if (t + d <= evTime)
                continue;
            else {
                d -= (evTime - t);
                t = evTime;
            }
        }

        if (timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START) {
            if (t + d > evTime + evDuration) {
                if (t >= evTime + evDuration)
                    continue;
                else {
                    d = evTime + evDuration - t;
                }
            }
        }

        Event *newEvent = new Event(**i, t, d);

        if (retune && newEvent->has(BaseProperties::PITCH)) {
            int pitch = newEvent->get
                        <Int>(BaseProperties::PITCH) + pitchDiff;
            if (pitch > 127)
                pitch = 127;
            if (pitch < 0)
                pitch = 0;
            newEvent->set
            <Int>(BaseProperties::PITCH, pitch);
        }

        if (newEvent->has(BaseProperties::VELOCITY)) {
            int velocity = newEvent->get
                           <Int>(BaseProperties::VELOCITY) + velocityDiff;
            if (velocity > 127)
                velocity = 127;
            if (velocity < 0)
                velocity = 0;
            newEvent->set
            <Int>(BaseProperties::VELOCITY, velocity);
        }

        (*target)->insert(newEvent);
    }
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

size_t SegmentMmapper::addMmappedSize(Segment *s)
{
    int repeatCount = getSegmentRepeatCount();
    return m_mmappedSize + (repeatCount + 1) * s->size() * sizeof(MappedEvent);
}

size_t SegmentMmapper::computeMmappedSize()
{
    if (!m_segment) return 0;

    int repeatCount = getSegmentRepeatCount();

    return (repeatCount + 1) * m_segment->size() * sizeof(MappedEvent);
}

}
