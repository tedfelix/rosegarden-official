
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

#include "MmappedSegment.h"
#include "misc/Debug.h"

#include "sound/MappedComposition.h"
#include "sound/Midi.h"

//#define DEBUG_META_ITERATOR 1
//#define DEBUG_PLAYING_AUDIO_FILES 1

namespace Rosegarden
{

MmappedSegment::MmappedSegment(const QString filename)
        : m_fd( -1),
        m_mmappedSize(0),
        m_mmappedRegion(0),
        m_mmappedEventBuffer((MappedEvent*)0),
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
        throw Exception("file not found");
    }

    m_mmappedSize = fInfo.size();

    m_fd = ::open(m_filename.latin1(), O_RDWR);

    m_mmappedRegion = ::mmap(0, m_mmappedSize, PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedRegion == (void*) - 1) {

        SEQUENCER_DEBUG << QString("mmap failed : (%1) %2\n").
        arg(errno).arg(strerror(errno));

        throw Exception("mmap failed");
    }

    m_mmappedEventBuffer = (MappedEvent *)((size_t *)m_mmappedRegion + 1);

    SEQUENCER_DEBUG << "MmappedSegment::map() : "
    << (void*)m_mmappedRegion << "," << m_mmappedSize << endl;

}

MmappedSegment::~MmappedSegment()
{
    unmap();
}

void MmappedSegment::unmap()
{
    ::munmap(m_mmappedRegion, m_mmappedSize);
    ::close(m_fd);
}

size_t
MmappedSegment::getNbMappedEvents() const
{
    if (m_mmappedRegion || !m_mmappedSize) {

        // The shared memory area consists of one size_t giving the
        // number of following mapped events, followed by the mapped
        // events themselves.

        // So this is the number of mapped events that we expect:
        size_t nominal = *(size_t *)m_mmappedRegion;

        // But we want to be sure not to read off the end of the
        // shared memory area, so just in case, this is the number of
        // events that can actually be accommodated in the memory area
        // as we see it:
        size_t actual = (m_mmappedSize - sizeof(size_t)) /
                        sizeof(MappedEvent);

        return std::min(nominal, actual);

    } else
        return 0;
}

bool MmappedSegment::remap(size_t newSize)
{
    SEQUENCER_DEBUG << "remap() from " << m_mmappedSize << " to "
    << newSize << endl;

    if (m_mmappedSize == newSize) {

        SEQUENCER_DEBUG << "remap() : sizes are identical, remap not forced - "
        << "nothing to do\n";
        return false;
    }

#ifdef linux
    void *oldRegion = m_mmappedRegion;
    m_mmappedRegion = (MappedEvent*)::mremap(m_mmappedRegion, m_mmappedSize, newSize, MREMAP_MAYMOVE);
    if (m_mmappedRegion != oldRegion) {
        SEQUENCER_DEBUG << "NOTE: buffer moved from " << oldRegion <<
        " to " << (void *)m_mmappedRegion << endl;
    }
#else
    ::munmap(m_mmappedRegion, m_mmappedSize);
    m_mmappedRegion = (MappedEvent*)::mmap(0, newSize, PROT_READ, MAP_SHARED, m_fd, 0);
#endif

    if (m_mmappedRegion == (void*) - 1) {

        SEQUENCER_DEBUG << QString("mremap failed : (%1) %2\n").
        arg(errno).arg(strerror(errno));

        throw Exception("mremap failed");
    }

    m_mmappedEventBuffer = (MappedEvent *)((size_t *)m_mmappedRegion + 1);
    m_mmappedSize = newSize;

    return true;
}

MmappedSegment::iterator::iterator(MmappedSegment* s)
        : m_s(s), m_currentEvent(m_s->getBuffer())
{}

MmappedSegment::iterator& MmappedSegment::iterator::operator=(const iterator& it)
{
    if (&it == this)
        return * this;

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

const MappedEvent &MmappedSegment::iterator::operator*()
{
    return *m_currentEvent;
}

bool MmappedSegment::iterator::atEnd() const
{
    return (m_currentEvent == 0) ||
           (m_currentEvent > (m_s->getBuffer() + m_s->getNbMappedEvents() - 1));
}

//----------------------------------------

MmappedSegmentsMetaIterator::MmappedSegmentsMetaIterator(
    mmappedsegments& segments,
    ControlBlockMmapper* controlBlockMmapper)
        : m_controlBlockMmapper(controlBlockMmapper),
        m_segments(segments)
{
    for (mmappedsegments::iterator i = m_segments.begin();
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
    for (segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
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
    for (unsigned int i = 0; i < m_iterators.size(); ++i)
        delete m_iterators[i];

    m_iterators.clear();
}

void MmappedSegmentsMetaIterator::reset()
{
    m_currentTime.sec = m_currentTime.nsec = 0;

    for (segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        (*i)->reset();
    }

}

bool MmappedSegmentsMetaIterator::jumpToTime(const RealTime& startTime)
{
    SEQUENCER_DEBUG << "jumpToTime(" << startTime << ")" << endl;

    reset();

    bool res = true;

    m_currentTime = startTime;

    for (segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i)
        if (!moveIteratorToTime(*(*i), startTime))
            res = false;

    return res;
}

bool MmappedSegmentsMetaIterator::moveIteratorToTime(MmappedSegment::iterator& iter,
        const RealTime& startTime)
{
    while ((!iter.atEnd()) &&
            (iter.peek()->getEventTime() < startTime) &&
            ((iter.peek()->getEventTime() + iter.peek()->getDuration()) < startTime)
          ) {
        ++iter;
    }
    bool res = !iter.atEnd();

    return res;
}

bool MmappedSegmentsMetaIterator::acceptEvent(MappedEvent *evt, bool evtIsFromMetronome)
{
    if (evt->getType() == 0)
        return false; // discard those right away

    if (evtIsFromMetronome) {
        if (evt->getType() == MappedEvent::MidiSystemMessage &&
                evt->getData1() == MIDI_TIMING_CLOCK) {
            /*
            std::cout << "MmappedSegmentsMetaIterator::acceptEvent - " 
                      << "found clock" << std::endl;
                      */ 
            return true;
        }

        return !m_controlBlockMmapper->isMetronomeMuted();
    }

    // else, evt is not from metronome : first check if we're soloing (i.e. playing only the selected track)
    if (m_controlBlockMmapper->isSolo())
        return (evt->getTrackId() == m_controlBlockMmapper->getSelectedTrack());

    // finally we're not soloing, so check if track is muted
    TrackId track = evt->getTrackId();
    bool muted = m_controlBlockMmapper->isTrackMuted(evt->getTrackId());

#ifdef DEBUG_META_ITERATOR

    SEQUENCER_DEBUG << "MSMI::acceptEvent: track " << track << " muted status: " << muted << endl;
#endif

    return !muted;
}


bool
MmappedSegmentsMetaIterator::fillCompositionWithEventsUntil(bool /*firstFetch*/,
        MappedComposition* c,
        const RealTime& startTime,
        const RealTime& endTime)
{
#ifdef DEBUG_META_ITERATOR
    SEQUENCER_DEBUG << "MSMI::fillCompositionWithEventsUntil " << startTime << " -> " << endTime << endl;
#endif

    m_currentTime = endTime;

    // keep track of the segments which still have valid events
    std::vector<bool> validSegments;
    for (unsigned int i = 0; i < m_segments.size(); ++i)
        validSegments.push_back(true);

    bool foundOneEvent = false, eventsRemaining = false;

    do {
        foundOneEvent = false;

        for (unsigned int i = 0; i < m_iterators.size(); ++i) {

            MmappedSegment::iterator* iter = m_iterators[i];

            //std::cerr << "Iterating on Segment #" << i << std::endl;

#ifdef DEBUG_META_ITERATOR

            SEQUENCER_DEBUG << "MSMI::fillCompositionWithEventsUntil : "
            << "checking segment #" << i << " "
            << iter->getSegment()->getFileName() << endl;
#endif

            if (!validSegments[i]) {
#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MSMI::fillCompositionWithEventsUntil : "
                << "no more events to get for this slice "
                << "in segment #" << i << endl;
#endif

                continue; // skip this segment
            }

            bool evtIsFromMetronome = iter->getSegment()->isMetronome();

            if (iter->atEnd()) {
#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MSMI::fillCompositionWithEventsUntil : "
                << endTime
                << " reached end of segment #"
                << i << endl;
#endif

                continue;
            } else if (!evtIsFromMetronome) {
                eventsRemaining = true;
            }

            if ((**iter).getEventTime() < endTime) {

                MappedEvent *evt = new MappedEvent(*(*iter));

                // set event's instrument
                //
                if (evtIsFromMetronome) {

                    evt->setInstrument(m_controlBlockMmapper->
                                       getInstrumentForMetronome());

                } else {

                    evt->setInstrument(m_controlBlockMmapper->
                                       getInstrumentForTrack(evt->getTrackId()));

                }

#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MSMI::fillCompositionWithEventsUntil : " << endTime
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
#endif

                if (evt->getType() == MappedEvent::TimeSignature) {

                    // Process time sig and tempo changes along with
                    // everything else, as the sound driver probably
                    // wants to know when they happen

                    c->insert(evt);

                } else if (evt->getType() == MappedEvent::Tempo) {

                    c->insert(evt);

                } else if (evt->getType() == MappedEvent::MidiSystemMessage &&

                           // #1048388:
                           // Ensure sysex heeds mute status, but ensure
                           // clocks etc still get through
                           evt->getData1() != MIDI_SYSTEM_EXCLUSIVE) {

                    c->insert(evt);

                } else if (acceptEvent(evt, evtIsFromMetronome) &&

                           ((evt->getEventTime() + evt->getDuration() > startTime) ||
                            (evt->getDuration() == RealTime::zeroTime &&
                             evt->getEventTime() == startTime))) {

                    //                    std::cout << "inserting event" << std::endl;

                    /*
                    std::cout << "Inserting event (type = "
                        << evt->getType() << ")" << std::endl;
                        */


                    c->insert(evt);

                } else {

#ifdef DEBUG_META_ITERATOR
                    std::cout << "MSMI: skipping event"
                    << " - event time = " << evt->getEventTime()
                    << ", duration = " << evt->getDuration()
                    << ", startTime = " << startTime << std::endl;
#endif

                    delete evt;
                }

                if (!evtIsFromMetronome)
                    foundOneEvent = true;
                ++(*iter);

            } else {
                validSegments[i] = false; // no more events to get from this segment
#ifdef DEBUG_META_ITERATOR

                SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : no more events to get from segment #"
                << i << endl;
#endif

            }

        }

    } while (foundOneEvent);

#ifdef DEBUG_META_ITERATOR

    SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : eventsRemaining = " << eventsRemaining << endl;
#endif

    return eventsRemaining || foundOneEvent;
}

void MmappedSegmentsMetaIterator::resetIteratorForSegment(const QString& filename)
{
    for (segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        MmappedSegment::iterator* iter = *i;

        if (iter->getSegment()->getFileName() == filename) {
            SEQUENCER_DEBUG << "MSMI::resetIteratorForSegment(" << filename << ") : found iterator\n";
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

void
MmappedSegmentsMetaIterator::getAudioEvents(std::vector<MappedEvent> &v)
{
    v.clear();

    for (mmappedsegments::iterator i = m_segments.begin();
            i != m_segments.end(); ++i) {

        MmappedSegment::iterator itr(i->second);

        while (!itr.atEnd()) {

            if ((*itr).getType() != MappedEvent::Audio) {
                ++itr;
                continue;
            }

            MappedEvent evt(*itr);
            ++itr;

            if (m_controlBlockMmapper->isTrackMuted(evt.getTrackId())) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MSMI::getAudioEvents - "
                << "track " << evt.getTrackId() << " is muted" << std::endl;
#endif

                continue;
            }

            if (m_controlBlockMmapper->isSolo() == true &&
                    evt.getTrackId() != m_controlBlockMmapper->getSelectedTrack()) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MSMI::getAudioEvents - "
                << "track " << evt.getTrackId() << " is not solo track" << std::endl;
#endif

                continue;
            }

            v.push_back(evt);
        }
    }
}


std::vector<MappedEvent>&
MmappedSegmentsMetaIterator::getPlayingAudioFiles(const RealTime &
        songPosition)
{
    // Clear playing audio segments
    //
    m_playingAudioSegments.clear();

#ifdef DEBUG_PLAYING_AUDIO_FILES

    std::cout << "MSMI::getPlayingAudioFiles" << std::endl;
#endif

    for (mmappedsegments::iterator i = m_segments.begin();
            i != m_segments.end(); ++i) {

        MmappedSegment::iterator iter(i->second);

        bool found = false;

        //!!! any point to this loop at all? can found ever fail?
        for (segmentiterators::iterator sI = m_iterators.begin();
                sI != m_iterators.end(); ++sI) {
            if ((*sI)->getSegment() == iter.getSegment())
                found = true;
        }

        if (!found)
            continue;

        while (!iter.atEnd()) {
            if ((*iter).getType() != MappedEvent::Audio) {
                ++iter;
                continue;
            }

            //std::cout << "CONSTRUCTING MAPPEDEVENT" << std::endl;
            MappedEvent evt(*iter);

            // Check for this track being muted or soloed
            //
            if (m_controlBlockMmapper->isTrackMuted(evt.getTrackId()) == true) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MSMI::getPlayingAudioFiles - "
                << "track " << evt.getTrackId() << " is muted" << std::endl;
#endif

                ++iter;
                continue;
            }

            if (m_controlBlockMmapper->isSolo() == true &&
                    evt.getTrackId() != m_controlBlockMmapper->getSelectedTrack()) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MSMI::getPlayingAudioFiles - "
                << "track " << evt.getTrackId() << " is not solo track" << std::endl;
#endif

                ++iter;
                continue;
            }

            // If there's an audio event and it should be playing at this time
            // then flag as such.
            //
            if (songPosition > evt.getEventTime() - RealTime(1, 0) &&
                    songPosition < evt.getEventTime() + evt.getDuration()) {

#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MSMI::getPlayingAudioFiles - "
                << "instrument id = " << evt.getInstrument() << std::endl;


                std::cout << "MSMI::getPlayingAudioFiles - "
                << " id " << evt.getRuntimeSegmentId() << ", audio event time     = " << evt.getEventTime() << std::endl;
                std::cout << "MSMI::getPlayingAudioFiles - "
                << "audio event duration = " << evt.getDuration() << std::endl;


#endif // DEBUG_PLAYING_AUDIO_FILES

                m_playingAudioSegments.push_back(evt);
            }

            ++iter;
        }

        //std::cout << "END OF ITERATOR" << std::endl << std::endl;

    }

    return m_playingAudioSegments;
}

}

