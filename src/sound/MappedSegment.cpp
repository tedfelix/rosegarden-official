/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MappedSegment.h"
#include "misc/Debug.h"

#include "MappedEventList.h"
#include "Midi.h"
#include "ControlBlock.h"

//#define DEBUG_META_ITERATOR 1
//#define DEBUG_PLAYING_AUDIO_FILES 1

namespace Rosegarden
{

MappedSegment::MappedSegment() : 
    m_buffer(0),
    m_isMetronome(false)
{
}

MappedSegment::~MappedSegment()
{
    delete[] m_buffer;
}

int
MappedSegment::getBufferSize() const
{
    return m_size.fetchAndAddRelaxed(0);
}

int
MappedSegment::getBufferFill() const
{
    return m_fill.fetchAndAddRelaxed(0);
}

void
MappedSegment::resizeBuffer(int newSize)
{
    if (newSize <= getBufferSize()) return;

    MappedEvent *oldBuffer = m_buffer;
    MappedEvent *newBuffer = new MappedEvent[newSize];

    for (int i = 0; i < m_fill; ++i) {
        newBuffer[i] = m_buffer[i];
    }

    {
        QWriteLocker locker(&m_lock);
        m_buffer = newBuffer;
        m_size.fetchAndStoreRelease(newSize);
    }

    SEQUENCER_DEBUG << "MappedSegment::resizeBuffer: Resized to " << newSize << " events" << endl;

    delete[] oldBuffer;
}

void
MappedSegment::setBufferFill(int newFill)
{
    m_fill.fetchAndStoreRelaxed(newFill);
    SEQUENCER_DEBUG << "MappedSegment::setBufferFill(" << newFill << "): Fill is now " << getBufferFill() << endl;
}

MappedSegment::iterator::iterator(MappedSegment *s) :
    m_s(s), m_index(0)
{
}

MappedSegment::iterator &
MappedSegment::iterator::operator=(const iterator& it)
{
    if (&it == this) return *this;

    m_s = it.m_s;
    m_index = it.m_index;

    return *this;
}

MappedSegment::iterator &
MappedSegment::iterator::operator++()
{
    int fill = m_s->getBufferFill();
    if (m_index < fill) ++m_index;
    return *this;
}

MappedSegment::iterator
MappedSegment::iterator::operator++(int)
{
    iterator r = *this;
    int fill = m_s->getBufferFill();
    if (m_index < fill) ++m_index;
    return r;
}

MappedSegment::iterator &
MappedSegment::iterator::operator+=(int offset)
{
    int fill = m_s->getBufferFill();
    if (m_index + offset <= fill) {
        m_index += offset;
    } else {
        m_index = fill;
    }
    return *this;
}

MappedSegment::iterator &
MappedSegment::iterator::operator-=(int offset)
{
    if (m_index > offset) m_index -= offset;
    else m_index = 0;
    return *this;
}

bool MappedSegment::iterator::operator==(const iterator& it)
{
    return (m_index == it.m_index) && (m_s == it.m_s);
}

void MappedSegment::iterator::reset()
{
    m_index = 0;
}

MappedEvent
MappedSegment::iterator::operator*()
{
    const MappedEvent *e = peek();
    if (e) return *e;
    else return MappedEvent();
}

const MappedEvent *
MappedSegment::iterator::peek() const
{
    QReadLocker locker(&m_s->m_lock);
    if (m_index >= m_s->getBufferFill()) {
        return 0;
    }
    return &m_s->m_buffer[m_index];
}

bool
MappedSegment::iterator::atEnd() const
{
    int fill = m_s->getBufferFill();
    return (m_index >= fill);
}

//----------------------------------------

MappedSegmentsMetaIterator::MappedSegmentsMetaIterator()
{
}

MappedSegmentsMetaIterator::~MappedSegmentsMetaIterator()
{
    clear();
}

void
MappedSegmentsMetaIterator::addSegment(MappedSegment *ms)
{
    m_segments.insert(ms);
    MappedSegment::iterator *iter = new MappedSegment::iterator(ms);
    moveIteratorToTime(*iter, m_currentTime);
    m_iterators.push_back(iter);
}

void
MappedSegmentsMetaIterator::removeSegment(MappedSegment *ms)
{
    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        if ((*i)->getSegment() == ms) {
            delete (*i);
            m_iterators.erase(i);
            break;
        }
    }
    m_segments.erase(ms);
}

void
MappedSegmentsMetaIterator::clear()
{
    for (size_t i = 0; i < m_iterators.size(); ++i) {
        delete m_iterators[i];
    }

    m_iterators.clear();
    m_segments.clear();
}

void
MappedSegmentsMetaIterator::reset()
{
    m_currentTime.sec = m_currentTime.nsec = 0;

    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        (*i)->reset();
    }
}

bool
MappedSegmentsMetaIterator::jumpToTime(const RealTime &startTime)
{
    SEQUENCER_DEBUG << "jumpToTime(" << startTime << ")" << endl;

    reset();

    bool res = true;

    m_currentTime = startTime;

    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        if (!moveIteratorToTime(*(*i), startTime)) {
            res = false;
        }
    }

    return res;
}

bool
MappedSegmentsMetaIterator::moveIteratorToTime(MappedSegment::iterator &iter,
                                               const RealTime &startTime)
{
    while (1) {

        if (iter.atEnd()) break;
        const MappedEvent *e = iter.peek();
        if (!e ||
            e->getEventTime() + e->getDuration() >= startTime) {
            break;
        }

        ++iter;
    }

    bool res = !iter.atEnd();
    return res;
}

bool
MappedSegmentsMetaIterator::acceptEvent(MappedEvent *evt,
                                        bool evtIsFromMetronome)
{
    if (evt->getType() == 0) return false; // discard those right away

    if (evtIsFromMetronome) {
        if (evt->getType() == MappedEvent::MidiSystemMessage &&
            evt->getData1() == MIDI_TIMING_CLOCK) {
            /*
            std::cout << "MappedSegmentsMetaIterator::acceptEvent - " 
                      << "found clock" << std::endl;
                      */ 
            return true;
        }

        bool play = !ControlBlock::getInstance()->isMetronomeMuted();
#ifdef DEBUG_META_ITERATOR
        SEQUENCER_DEBUG << "MSMI::acceptEvent: Metronome event, play = " << play << endl;
#endif
        return play;
    }

    // else, evt is not from metronome : first check if we're soloing
    // (i.e. playing only the selected track)
    if (ControlBlock::getInstance()->isSolo()) {
        return (evt->getTrackId() ==
                ControlBlock::getInstance()->getSelectedTrack());
    }

    // finally we're not soloing, so check if track is muted
    TrackId track = evt->getTrackId();
    bool muted = ControlBlock::getInstance()->isTrackMuted(evt->getTrackId());

#ifdef DEBUG_META_ITERATOR
    SEQUENCER_DEBUG << "MSMI::acceptEvent: track " << track << " muted status: " << muted << endl;
#endif

    return !muted;
}

bool
MappedSegmentsMetaIterator::fillCompositionWithEventsUntil(bool /*firstFetch*/,
                                                           MappedEventList* c,
                                                           const RealTime& startTime,
                                                           const RealTime& endTime)
{
#ifdef DEBUG_META_ITERATOR
    SEQUENCER_DEBUG << "MSMI::fillCompositionWithEventsUntil " << startTime << " -> " << endTime << endl;
#endif

    m_currentTime = endTime;

    // keep track of the segments which still have valid events
    std::vector<bool> validSegments;
    for (size_t i = 0; i < m_segments.size(); ++i) {
        validSegments.push_back(true);
    }

    bool foundOneEvent = false, eventsRemaining = false;

    do {
        foundOneEvent = false;

        for (size_t i = 0; i < m_iterators.size(); ++i) {

            MappedSegment::iterator *iter = m_iterators[i];

            //std::cerr << "Iterating on Segment #" << i << std::endl;

#ifdef DEBUG_META_ITERATOR
            SEQUENCER_DEBUG << "MSMI::fillCompositionWithEventsUntil : "
                            << "checking segment #" << i << endl;
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

            const MappedEvent *cur = iter->peek();

            if (cur && cur->getEventTime() < endTime) {

                MappedEvent *evt = new MappedEvent(*cur);

                // set event's instrument
                //
                if (evtIsFromMetronome) {

                    evt->setInstrument(ControlBlock::getInstance()->
                                       getInstrumentForMetronome());

                } else {

                    evt->setInstrument(ControlBlock::getInstance()->
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

                if (!evtIsFromMetronome) {
                    foundOneEvent = true;
                }
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

void
MappedSegmentsMetaIterator::resetIteratorForSegment(MappedSegment *s)
{
    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {

        MappedSegment::iterator *iter = *i;

        if (iter->getSegment() == s) {

            SEQUENCER_DEBUG << "MSMI::resetIteratorForSegment("
                            << s << ") : found iterator\n";

            iter->reset();
            moveIteratorToTime(*iter, m_currentTime);
            break;
        }
    }
}

void
MappedSegmentsMetaIterator::getAudioEvents(std::vector<MappedEvent> &v)
{
    v.clear();

    for (mappedsegments::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

        MappedSegment::iterator itr(*i);

        while (!itr.atEnd()) {

            if ((*itr).getType() != MappedEvent::Audio) {
                ++itr;
                continue;
            }

            MappedEvent evt(*itr);
            ++itr;

            if (ControlBlock::getInstance()->isTrackMuted(evt.getTrackId())) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MSMI::getAudioEvents - "
                          << "track " << evt.getTrackId() << " is muted" << std::endl;
#endif
                continue;
            }

            if (ControlBlock::getInstance()->isSolo() == true &&
		evt.getTrackId() != ControlBlock::getInstance()->getSelectedTrack()) {
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


std::vector<MappedEvent> &
MappedSegmentsMetaIterator::getPlayingAudioFiles(const RealTime &songPosition)
{
    // Clear playing audio segments
    //
    m_playingAudioSegments.clear();

#ifdef DEBUG_PLAYING_AUDIO_FILES
    std::cout << "MSMI::getPlayingAudioFiles" << std::endl;
#endif

    for (mappedsegments::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

        MappedSegment::iterator iter(*i);

        while (!iter.atEnd()) {
            if ((*iter).getType() != MappedEvent::Audio) {
                ++iter;
                continue;
            }

            MappedEvent evt(*iter);

            // Check for this track being muted or soloed
            //
            if (ControlBlock::getInstance()->isTrackMuted(evt.getTrackId()) == true) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MSMI::getPlayingAudioFiles - "
                << "track " << evt.getTrackId() << " is muted" << std::endl;
#endif

                ++iter;
                continue;
            }

            if (ControlBlock::getInstance()->isSolo() == true &&
		evt.getTrackId() != ControlBlock::getInstance()->getSelectedTrack()) {
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
                          << "instrument id = " << evt.getInstrument()
                          << std::endl;

                std::cout << "MSMI::getPlayingAudioFiles - "
                          << " id " << evt.getRuntimeSegmentId()
                          << ", audio event time     = " << evt.getEventTime()
                          << std::endl;

                std::cout << "MSMI::getPlayingAudioFiles - "
                          << "audio event duration = " << evt.getDuration()
                          << std::endl;
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

