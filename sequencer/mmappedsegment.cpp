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

#include "mmappedsegment.h"
#include "rosedebug.h"

using std::cerr;
using std::endl;
using std::cout;
using Rosegarden::MappedEvent;

MmappedSegment::MmappedSegment(const QString filename)
    : m_fd(-1),
      m_mmappedSize(0),
      m_nbMappedEvents(0),
      m_mmappedBuffer((MappedEvent*)0),
      m_filename(filename)
{
    SEQUENCER_DEBUG << "mmapping " << filename << endl;

    map();
}

bool MmappedSegment::isMetronome()
{
    return (getFileName().contains("metronome", false) > 0);
}



void MmappedSegment::map()
{
    QFileInfo fInfo(m_filename);
    if (!fInfo.exists()) {
        SEQUENCER_DEBUG << "MmappedSegment::map() : file " << m_filename << " doesn't exist\n";
        throw Rosegarden::Exception("file not found");
    }

    m_mmappedSize = fInfo.size();
    m_nbMappedEvents = m_mmappedSize / sizeof(MappedEvent);

    m_fd = ::open(m_filename.latin1(), O_RDWR);

    m_mmappedBuffer = (MappedEvent*)::mmap(0, m_mmappedSize, PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {

        SEQUENCER_DEBUG << QString("mmap failed : (%1) %2\n").
            arg(errno).arg(strerror(errno));

        throw Rosegarden::Exception("mmap failed");
    }

    SEQUENCER_DEBUG << "MmappedSegment::map() : "
                    << (void*)m_mmappedBuffer << "," << m_mmappedSize 
                    << ", " << m_nbMappedEvents << " events\n";

}

MmappedSegment::~MmappedSegment()
{
    unmap();
}

void MmappedSegment::unmap()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}

bool MmappedSegment::remap()
{
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
}

MmappedSegment::iterator::iterator(MmappedSegment* s)
    : m_s(s), m_currentEvent(m_s->getBuffer())
{
}

MmappedSegment::iterator& MmappedSegment::iterator::operator=(const iterator& it)
{
    if (&it == this) return *this;

    m_s = it.m_s;
    m_currentEvent = it.m_currentEvent;

    return *this;
}

MmappedSegment::iterator& MmappedSegment::iterator::operator++()
{
    if (!atEnd()) {

        do
            ++m_currentEvent;
        while (!atEnd() && (m_currentEvent->getType() == 0));
        // skip null events - there can be some if the file has been
        // zeroed out after events have been deleted

    } else {

        SEQUENCER_DEBUG << "MmappedSegment::iterator::operator++() " << this
                        << " - reached end of stream\n";

    }

    return *this;
}

MmappedSegment::iterator MmappedSegment::iterator::operator++(int)
{
    iterator r = *this;

    if (!atEnd()) {
        do
            ++m_currentEvent;
        while (!atEnd() && m_currentEvent->getType() == 0);

    }

    return r;
}

MmappedSegment::iterator& MmappedSegment::iterator::operator+=(int offset)
{
    m_currentEvent += offset;

    if (atEnd()) {
        m_currentEvent = m_s->getBuffer() + m_s->getNbMappedEvents();
    }
    
    return *this;
}

MmappedSegment::iterator& MmappedSegment::iterator::operator-=(int offset)
{
    m_currentEvent -= offset;
    if (m_currentEvent < m_s->getBuffer()) {
        m_currentEvent = m_s->getBuffer();
    }
    
    return *this;
}


bool MmappedSegment::iterator::operator==(const iterator& it)
{
    return (m_currentEvent == it.m_currentEvent) || (atEnd() == it.atEnd());
}

void MmappedSegment::iterator::reset()
{
    m_currentEvent = m_s->getBuffer();
}

MappedEvent MmappedSegment::iterator::operator*()
{
    return *m_currentEvent;
}

bool MmappedSegment::iterator::atEnd() const
{
    return (m_currentEvent == 0) || (m_currentEvent > (m_s->getBuffer() + m_s->getNbMappedEvents() - 1));
}

// Ew ew ew - move this to base
kdbgstream&
operator<<(kdbgstream &dbg, const Rosegarden::RealTime &t)
{
    dbg << "sec : " << t.sec << ", usec : " << t.usec;
    return dbg;
}

//----------------------------------------

MmappedSegmentsMetaIterator::MmappedSegmentsMetaIterator(
        mmappedsegments& segments,
        ControlBlockMmapper* controlBlockMmapper)
    :m_controlBlockMmapper(controlBlockMmapper),
     m_segments(segments)
{
    for(mmappedsegments::iterator i = m_segments.begin();
        i != m_segments.end(); ++i)
        m_iterators.push_back(new MmappedSegment::iterator(i->second));
}

MmappedSegmentsMetaIterator::~MmappedSegmentsMetaIterator()
{
    clear();
}

void MmappedSegmentsMetaIterator::addSegment(MmappedSegment* ms)
{
    MmappedSegment::iterator* iter = new MmappedSegment::iterator(ms);
    moveIteratorToTime(*iter, m_currentTime);
    m_iterators.push_back(iter);
}

void MmappedSegmentsMetaIterator::deleteSegment(MmappedSegment* ms)
{
    for(segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        if ((*i)->getSegment() == ms) {
            SEQUENCER_DEBUG << "deleteSegment : found segment to delete : "
                            << ms->getFileName() << endl;
            delete (*i);
            m_iterators.erase(i);
            break;
        }
    }
}

void MmappedSegmentsMetaIterator::clear()
{
    for(unsigned int i = 0; i < m_iterators.size(); ++i)
        delete m_iterators[i];

    m_iterators.clear();
}

void MmappedSegmentsMetaIterator::reset()
{
    m_currentTime.sec = m_currentTime.usec = 0;

    for(segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        (*i)->reset();
    }
    
}

bool MmappedSegmentsMetaIterator::jumpToTime(const Rosegarden::RealTime& startTime)
{
    SEQUENCER_DEBUG << "jumpToTime(" << startTime << ")\n";

    reset();

    bool res = true;

    m_currentTime = startTime;

//    if (startTime > Rosegarden::RealTime(0,0)) {
        for(segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i)
            if (!moveIteratorToTime(*(*i), startTime)) res = false;
//    }

    return res;
}

bool MmappedSegmentsMetaIterator::moveIteratorToTime(MmappedSegment::iterator& iter,
                                                     const Rosegarden::RealTime& startTime)
{
    while ((!iter.atEnd()) && (iter.peek()->getEventTime() < startTime)) {
        ++iter;
    }
    bool res = !iter.atEnd();

    return res;
}

bool MmappedSegmentsMetaIterator::acceptEvent(MappedEvent *evt, bool evtIsFromMetronome)
{
    if (evt->getType() == 0) return false; // discard those right away

    if (evtIsFromMetronome)
        return !m_controlBlockMmapper->isMetronomeMuted();
        
    // else, evt is not from metronome : first check if we're soloing (i.e. playing only the selected track)
    if (m_controlBlockMmapper->isSolo())
        return (evt->getTrackId() == m_controlBlockMmapper->getSelectedTrack());

    // finally we're not soloing, so check if track is muted
    return !m_controlBlockMmapper->isTrackMuted(evt->getTrackId());
                     
}


bool
MmappedSegmentsMetaIterator::fillCompositionWithEventsUntil
        (Rosegarden::MappedComposition* c,
         const Rosegarden::RealTime& endTime)
{
    SEQUENCER_DEBUG << "fillCompositionWithEventsUntil " << endTime << endl;

    m_currentTime = endTime;

    // keep track of the segments which still have valid events
    std::vector<bool> validSegments;
    for(unsigned int i = 0; i < m_segments.size(); ++i)
        validSegments.push_back(true);

    bool foundOneEvent = false, eventsRemaining = false;

    do 
    {
        foundOneEvent = false;

        for(unsigned int i = 0; i < m_iterators.size(); ++i) {

            MmappedSegment::iterator* iter = m_iterators[i];

            //std::cerr << "Iterating on Segment #" << i << std::endl;

            SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : "
                            << "checking segment #" << i << " " 
                            << iter->getSegment()->getFileName() << endl;

            if (!validSegments[i]) {
                SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : "
                                << "no more events to get for this slice "
                                << "in segment #" << i << endl;
                continue; // skip this segment
            }

            bool evtIsFromMetronome = iter->getSegment()->isMetronome();

            if (iter->atEnd()) {
                SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : " 
                                << endTime
                                << " reached end of segment #"
                                << i << endl;
                continue;
            } else if (!evtIsFromMetronome) {
                eventsRemaining = true;
            }

            MappedEvent *evt = new MappedEvent(*(*iter));

            /*
            if (evt->getType() == MappedEvent::Audio) {
                
                if (evt->getEventTime() < endTime)
                    evt->setEventTime(evt->getEventTime() - Rosegarden::RealTime(1, 0));
                    c->insert(evt);
            }
            */

            if (evt->getEventTime() < endTime) {
                if (evtIsFromMetronome) {

                    evt->setInstrument(m_controlBlockMmapper->
                            getInstrumentForMetronome());

                } else {

                    evt->setInstrument(m_controlBlockMmapper->
                            getInstrumentForTrack(evt->getTrackId()));

                }
                
                SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : " << endTime
                                << " inserting evt from segment #"
                                << i
                                << " : trackId: " << evt->getTrackId()
                                << " - inst: " << evt->getInstrument()
                                << " - type: " << evt->getType()
                                << " - time: " << evt->getEventTime()
                                << " - duration: " << evt->getDuration()
                                << " - data1: " << (unsigned int)evt->getData1()
                                << " - data2: " << (unsigned int)evt->getData2()
                                << " - metronome event: " << evtIsFromMetronome
                                << endl;


                if (evt->getType() == MappedEvent::TimeSignature) {
                    // do something
                    SEQUENCER_DEBUG << "timeSig event\n";

                } else if (evt->getType() == MappedEvent::Tempo) {
                    // do something else
                    SEQUENCER_DEBUG << "tempo event\n";

                } else if (acceptEvent(evt, evtIsFromMetronome)) {
                    SEQUENCER_DEBUG << "inserting event\n";

                    c->insert(evt);

                } else {
                    
                    SEQUENCER_DEBUG << "skipping event\n";
                }
            
                if (!evtIsFromMetronome) foundOneEvent = true;
                ++(*iter);

            } else {
                validSegments[i] = false; // no more events to get from this segment
                SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : no more events to get from segment #"
                                << i << endl;
            }

        }

    } while (foundOneEvent);

    SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : eventsRemaining = " << eventsRemaining << endl;

    return eventsRemaining || foundOneEvent;
}

void MmappedSegmentsMetaIterator::resetIteratorForSegment(const QString& filename)
{
    for(segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        MmappedSegment::iterator* iter = *i;

        if (iter->getSegment()->getFileName() == filename) {
//             SEQUENCER_DEBUG << "resetIteratorForSegment(" << filename << ") : found iterator\n";
            // delete iterator and create another one
            MmappedSegment* ms = (*i)->getSegment();
            delete iter;
            m_iterators.erase(i);
            iter = new MmappedSegment::iterator(ms);
            m_iterators.push_back(iter);
            moveIteratorToTime(*iter, m_currentTime);
            break;
        }

    }
}



