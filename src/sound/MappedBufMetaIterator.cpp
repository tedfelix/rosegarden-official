/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MappedBufMetaIterator.h"

#include "base/Profiler.h"
#include "misc/Debug.h"
#include "sound/MappedEventList.h"
#include "sound/MappedInserterBase.h"
#include "sound/Midi.h"
#include "sound/ControlBlock.h"

#include <queue>

// #define DEBUG_META_ITERATOR 1
//#define DEBUG_PLAYING_AUDIO_FILES 1

namespace Rosegarden
{

MappedBufMetaIterator::MappedBufMetaIterator()
{
}

MappedBufMetaIterator::~MappedBufMetaIterator()
{
    clear();
}

void
MappedBufMetaIterator::addSegment(MappedEventBuffer *ms)
{
    m_segments.insert(ms);
    MappedEventBuffer::iterator *iter = new MappedEventBuffer::iterator(ms);
    moveIteratorToTime(*iter, m_currentTime);
    m_iterators.push_back(iter);
}

void
MappedBufMetaIterator::removeSegment(MappedEventBuffer *ms)
{
    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        if ((*i)->getSegment() == ms) {
            delete (*i);
            // Now ms may not be a valid address.
            m_iterators.erase(i);
            break;
        }
    }
    m_segments.erase(ms);
}

void
MappedBufMetaIterator::clear()
{
    for (size_t i = 0; i < m_iterators.size(); ++i) {
        delete m_iterators[i];
    }

    m_iterators.clear();
    m_segments.clear();
}

void
MappedBufMetaIterator::reset()
{
    m_currentTime.sec = m_currentTime.nsec = 0;

    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        (*i)->reset();
    }
}

bool
MappedBufMetaIterator::jumpToTime(const RealTime &startTime)
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
MappedBufMetaIterator::moveIteratorToTime(MappedEventBuffer::iterator &iter,
                                               const RealTime &startTime)
{
    while (1) {

        if (iter.atEnd()) break;
        // We use peek because it's safe even if we have not fully
        // filled the buffer yet.  That means we can get NULL e.
        const MappedEvent *e = iter.peek();
        // If the event sounds past startTime, stop.  If e is NULL, we
        // also stop because we know nothing about the event yet.
        if (!e ||
            e->getEventTime() + e->getDuration() >= startTime) {
            break;
        }

        ++iter;
    }

    bool res = !iter.atEnd();
    iter.setReady(false);
    return res;
}

bool
MappedBufMetaIterator::acceptEvent(MappedEvent *evt,
                                        bool evtIsFromMetronome)
{
    if (evt->getType() == 0) return false; // discard those right away

    if (evtIsFromMetronome) {
        if (evt->getType() == MappedEvent::MidiSystemMessage &&
            evt->getData1() == MIDI_TIMING_CLOCK) {
            /*
            std::cout << "MappedBufMetaIterator::acceptEvent - " 
                      << "found clock" << std::endl;
                      */ 
            return true;
        }

        bool play = !ControlBlock::getInstance()->isMetronomeMuted();
#ifdef DEBUG_META_ITERATOR
        SEQUENCER_DEBUG << "MBMI::acceptEvent: Metronome event, play = " << play << endl;
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
    bool muted = ControlBlock::getInstance()->isTrackMuted(track);

#ifdef DEBUG_META_ITERATOR
    SEQUENCER_DEBUG << "MBMI::acceptEvent: track " << track << " muted status: " << muted << endl;
#endif

    return !muted;
}

// Support for the start-time priority queue
struct reverseCmpRealTime
{ bool operator()(RealTime &a, RealTime &b) { return a > b; } };
typedef std::priority_queue<RealTime,
                            std::vector<RealTime>,
                            reverseCmpRealTime>
    LowfirstRealTimeQueue;


bool
MappedBufMetaIterator::
fillCompositionWithEventsUntil(bool /*firstFetch*/,
                               MappedInserterBase &inserter,
                               const RealTime& startTime,
                               const RealTime& endTime)
{
    Profiler profiler("MappedBufMetaIterator::fillCompositionWithEventsUntil", false);
#ifdef DEBUG_META_ITERATOR
    SEQUENCER_DEBUG << "MBMI::fillCompositionWithEventsUntil "
                    << startTime << " -> "
                    << endTime << endl;
#endif
    // To keep mappers on the same channel from interfering, for
    // instance sending their initializations while another is playing
    // on the channel, we slice the timeslice into slices during which
    // no new mappers start.  We could re-slice it smarter but this
    // suffices.

    // Make a queue of all segment starts that occur during the slice.
    LowfirstRealTimeQueue segStarts;

    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) {
        RealTime start, end;
        (*i)->getSegment()->getStartEnd(start, end); 
        if ((start > startTime) && (start < endTime))
            { segStarts.push(start); }
    }

    // The progressive starting time, updated each iteration.
    RealTime innerStart = startTime;

    // For each distinct gap, do a slice.
    while (!segStarts.empty()) {
        RealTime innerEnd = segStarts.top();
        segStarts.pop();
        if (innerEnd == innerStart) { continue; }
        (void)fillNoncompeting(inserter, innerStart, innerEnd);
        innerStart = innerEnd;
    }

    // Do one more slice to take us to the end time.  This is always
    // correct to do, since segStarts can't contain a start equal to
    // endTime.
    bool eventsLeft = fillNoncompeting(inserter, innerStart, endTime);

    return eventsLeft;
}

bool
MappedBufMetaIterator::
fillNoncompeting(MappedInserterBase &inserter,
                 const RealTime& startTime,
                 const RealTime& endTime)
{
#ifdef DEBUG_META_ITERATOR
    SEQUENCER_DEBUG << "MBMI::fillNoncompeting "
                    << startTime << " -> "
                    << endTime << endl;
#endif
    Profiler profiler("MappedBufMetaIterator::fillNoncompeting", false);

    RealTime loopTime = startTime;
    m_currentTime = endTime;
    
    bool foundOneEvent = false, eventsRemaining = false;

    // Activate segments that have anything playing during this
    // slice.  We include segments that end exactly when we start, but
    // not segments that start exactly when we end.
    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) { 
        RealTime start, end;
        (*i)->getSegment()->getStartEnd(start, end);
        bool active = ((start < endTime) && (end >= startTime));
        (*i)->setActive(active);
        // If some are yet to even start, we have events remaining.
        // Record that now because the loop won't discover it.
        if (start >= endTime) { eventsRemaining = true; }
    }

    do {
        foundOneEvent = false;

        for (size_t i = 0; i < m_iterators.size(); ++i) {
            MappedEventBuffer::iterator *iter = m_iterators[i];

            //std::cerr << "Iterating on Segment #" << i << std::endl;

#ifdef DEBUG_META_ITERATOR
            SEQUENCER_DEBUG << "MBMI::fillCompositionWithEventsUntil : "
                            << "checking segment #" << i << endl;
#endif

            if (!iter->getActive()) {
#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MBMI::fillCompositionWithEventsUntil : "
                                << "no more events to get for this slice "
                                << "in segment #" << i << endl;
#endif

                continue; // skip this segment
            }

            bool evtIsFromMetronome = iter->getSegment()->isMetronome();

            if (iter->atEnd()) {
#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MBMI::fillCompositionWithEventsUntil : "
                                << endTime
                                << " reached end of segment #"
                                << i << endl;
#endif
                continue;
            } else if (!evtIsFromMetronome) {
                eventsRemaining = true;
            }

            MappedEvent *cur = iter->peek();

            if (cur &&
                cur->isValid() &&
                cur->getEventTime() < endTime) {

#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MBMI::fillCompositionWithEventsUntil : " << endTime
                                << " inserting evt from segment #"
                                << i
                                << " : trackId: " << cur->getTrackId()
                                << " channel: " << (unsigned int) cur->getRecordedChannel()
                                << " - inst: " << cur->getInstrument()
                                << " - type: " << cur->getType()
                                << " - time: " << cur->getEventTime()
                                << " - duration: " << cur->getDuration()
                                << " - data1: " << (unsigned int)cur->getData1()
                                << " - data2: " << (unsigned int)cur->getData2()
                                << " - metronome event: " << evtIsFromMetronome
                                << endl;
#endif

                if (cur->getType() == MappedEvent::TimeSignature) {

                    // Process time sig and tempo changes along with
                    // everything else, as the sound driver probably
                    // wants to know when they happen

                    inserter.insertCopy(*cur);

                } else if (cur->getType() == MappedEvent::Tempo) {

                    inserter.insertCopy(*cur);

                } else if (cur->getType() == MappedEvent::Marker) {

                    inserter.insertCopy(*cur);

                } else if (cur->getType() == MappedEvent::MidiSystemMessage &&

                           // #1048388:
                           // Ensure sysex heeds mute status, but ensure
                           // clocks etc still get through
                           cur->getData1() != MIDI_SYSTEM_EXCLUSIVE) {

                    inserter.insertCopy(*cur);

                } else if (acceptEvent(cur, evtIsFromMetronome) &&

                           ((cur->getEventTime() + cur->getDuration() > startTime) ||
                            (cur->getDuration() == RealTime::zeroTime &&
                             cur->getEventTime() == startTime))) {

#ifdef DEBUG_META_ITERATOR
                    std::cout
                        << "Inserting event (type = "
                        << cur->getType() << ")" << std::endl;
#endif

                    // Use loopTime as a reference time, eg for
                    // calculating the correct controllers.  It can't
                    // simply be event time, because if we jumped into
                    // the middle of a long note, we'd wrongly find
                    // the controller values as they are at the time
                    // the note starts.
                    if (cur->getEventTime() > loopTime)
                        { loopTime = cur->getEventTime(); }
                    iter->doInsert(inserter, *cur, loopTime);
                } else {

#ifdef DEBUG_META_ITERATOR
                    std::cout << "MBMI: skipping event"
                    << " - event time = " << cur->getEventTime()
                    << ", duration = " << cur->getDuration()
                    << ", startTime = " << startTime << std::endl;
#endif
                }

                if (!evtIsFromMetronome) {
                    foundOneEvent = true;
                }
                ++(*iter);

            } else if (cur->isValid()) {
                iter->setActive(false); // no more events to get from this segment

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

// @param immediate means to reset it right away, presumably because
// we are playing right now.
void
MappedBufMetaIterator::
resetIteratorForSegment(MappedEventBuffer *s, bool immediate)
{
    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {

        MappedEventBuffer::iterator *iter = *i;

        if (iter->getSegment() == s) {

#ifdef DEBUG_META_ITERATOR
            SEQUENCER_DEBUG << "MBMI::resetIteratorForSegment("
                            << s << ") : found iterator\n";
#endif
            if (immediate) {
                iter->reset();
                moveIteratorToTime(*iter, m_currentTime);
            } else {
                iter->setReady(false);
            }
            break;
        }
    }
}

void
MappedBufMetaIterator::getAudioEvents(std::vector<MappedEvent> &v)
{
    v.clear();

    for (mappedsegments::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

        MappedEventBuffer::iterator itr(*i);

        while (!itr.atEnd()) {

            if ((*itr).getType() != MappedEvent::Audio) {
                ++itr;
                continue;
            }

            MappedEvent evt(*itr);
            ++itr;

            if (ControlBlock::getInstance()->isTrackMuted(evt.getTrackId())) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MBMI::getAudioEvents - "
                          << "track " << evt.getTrackId() << " is muted" << std::endl;
#endif
                continue;
            }

            if (ControlBlock::getInstance()->isSolo() == true &&
		evt.getTrackId() != ControlBlock::getInstance()->getSelectedTrack()) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MBMI::getAudioEvents - "
                          << "track " << evt.getTrackId() << " is not solo track" << std::endl;
#endif

                continue;
            }

            v.push_back(evt);
        }
    }
}


std::vector<MappedEvent> &
MappedBufMetaIterator::getPlayingAudioFiles(const RealTime &songPosition)
{
    // Clear playing audio segments
    //
    m_playingAudioSegments.clear();

#ifdef DEBUG_PLAYING_AUDIO_FILES
    std::cout << "MBMI::getPlayingAudioFiles" << std::endl;
#endif

    for (mappedsegments::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

        MappedEventBuffer::iterator iter(*i);

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
                std::cout << "MBMI::getPlayingAudioFiles - "
                << "track " << evt.getTrackId() << " is muted" << std::endl;
#endif

                ++iter;
                continue;
            }

            if (ControlBlock::getInstance()->isSolo() == true &&
		evt.getTrackId() != ControlBlock::getInstance()->getSelectedTrack()) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                std::cout << "MBMI::getPlayingAudioFiles - "
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
                std::cout << "MBMI::getPlayingAudioFiles - "
                          << "instrument id = " << evt.getInstrument()
                          << std::endl;

                std::cout << "MBMI::getPlayingAudioFiles - "
                          << " id " << evt.getRuntimeSegmentId()
                          << ", audio event time     = " << evt.getEventTime()
                          << std::endl;

                std::cout << "MBMI::getPlayingAudioFiles - "
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

