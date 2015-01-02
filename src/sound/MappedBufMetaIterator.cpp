/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

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
#include "sound/ControlBlock.h"

#include <queue>
#include <functional>

//#define DEBUG_META_ITERATOR 1
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
    // BUG #3546135
    // If we already have this segment, bail, or else we'll have two
    // iterators pointing to the same segment.  That will eventually
    // cause an access to freed memory and a subsequent crash.
    // This seems to happen when recording and we pass the end of the
    // composition.
    if (m_segments.find(ms) != m_segments.end())
        return;

    m_segments.insert(ms);
    MappedEventBuffer::iterator *iter = new MappedEventBuffer::iterator(ms);
    moveIteratorToTime(*iter, m_currentTime);
    m_iterators.push_back(iter);
}

void
MappedBufMetaIterator::removeSegment(MappedEventBuffer *ms)
{
    // Remove from m_iterators
    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        if ((*i)->getSegment() == ms) {
            delete (*i);
            // Now ms may not be a valid address.
            m_iterators.erase(i);
            break;
        }
    }

    // Remove from m_segments
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
    // Rather than briefly unlock and immediately relock each
    // iteration, we leave the lock on until we're done.
    QReadLocker locker(iter.getLock());

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




void
MappedBufMetaIterator::
fetchEvents(MappedInserterBase &inserter,
                               const RealTime& startTime,
                               const RealTime& endTime)
{
    Profiler profiler("MappedBufMetaIterator::fetchEvents", false);
#ifdef DEBUG_META_ITERATOR
    SEQUENCER_DEBUG << "MBMI::fetchEvents "
                    << startTime << " -> "
                    << endTime << endl;
#endif
    // To keep mappers on the same channel from interfering, for
    // instance sending their initializations while another is playing
    // on the channel, we slice the timeslice into slices during which
    // no new mappers start and pass each slice to
    // fetchEventsNoncompeting.  We could re-slice it smarter but this
    // suffices.

    // Make a queue of all segment starts that occur during the slice.
    std::priority_queue<RealTime,
                        std::vector<RealTime>,
                        std::greater<RealTime> >
        segStarts;

    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) {
        RealTime start, end;
        (*i)->getSegment()->getStartEnd(start, end); 
        if ((start >= startTime) && (start < endTime))
            { segStarts.push(start); }
    }

    // The progressive starting time, updated each iteration.
    RealTime innerStart = startTime;

    // For each distinct gap, do a slice.
    while (!segStarts.empty()) {
        // We're at innerStart.  Get a mapper that didn't start yet.
        RealTime innerEnd = segStarts.top();
        // Remove it from the queue.
        segStarts.pop();
        // If it starts exactly at innerStart, it doesn't need its own
        // slice.
        if (innerEnd == innerStart) { continue; }
        // Get a slice from the previous end-time (or startTime) to
        // this new start-time.
        fetchEventsNoncompeting(inserter, innerStart, innerEnd);
        innerStart = innerEnd;
    }

    // Do one more slice to take us to the end time.  This is always
    // correct to do, since segStarts can't contain a start equal to
    // endTime.
    fetchEventsNoncompeting(inserter, innerStart, endTime);

    return;
}

void
MappedBufMetaIterator::
fetchEventsNoncompeting(MappedInserterBase &inserter,
                 const RealTime& startTime,
                 const RealTime& endTime)
{
#ifdef DEBUG_META_ITERATOR
    SEQUENCER_DEBUG << "MBMI::fetchEventsNoncompeting "
                    << startTime << " -> "
                    << endTime << endl;
#endif
    Profiler profiler("MappedBufMetaIterator::fetchEventsNoncompeting", false);

    m_currentTime = endTime;
    
    // Activate segments that have anything playing during this
    // slice.  We include segments that end exactly when we start, but
    // not segments that start exactly when we end.
    for (segmentiterators::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) { 
        RealTime start, end;
        (*i)->getSegment()->getStartEnd(start, end);
        bool active = ((start < endTime) && (end >= startTime));
        (*i)->setActive(active, startTime);
    }

    // State variable to allow the outer loop to run until the inner
    // loop has nothing to do.
    bool innerLoopHasMore = false;
    do {
        innerLoopHasMore = false;

        for (size_t i = 0; i < m_iterators.size(); ++i) {
            MappedEventBuffer::iterator *iter = m_iterators[i];

#ifdef DEBUG_META_ITERATOR
            SEQUENCER_DEBUG << "MBMI::fetchEventsNoncompeting : "
                            << "checking segment #" << i << endl;
#endif

            if (!iter->getActive()) {
#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MBMI::fetchEventsNoncompeting : "
                                << "no more events to get for this slice"
                                << "in segment #" << i << endl;
#endif

                continue; // skip this iterator
            }

            if (iter->atEnd()) {
#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MBMI::fetchEventsNoncompeting : "
                                << endTime
                                << " reached end of segment #"
                                << i << endl;
#endif
                // Make this iterator abort early in future
                // iterations, since we know it's all done.
                iter->setInactive();
                continue;
            }

            // This locks the iterator's buffer against writes, lest
            // writing cause reallocating the buffer while we are
            // holding a pointer into it.  No function we call will
            // hold the `cur' pointer past its own scope, implying
            // that nothing holds it past an iteration of this loop,
            // which is this lock's scope.
            QReadLocker locker(iter->getLock());

            MappedEvent *cur = iter->peek();

            // We couldn't fetch an event or it failed a sanity check.
            // So proceed to the next iterator but keep looking at
            // this one - incrementing it does nothing useful, and it
            // might get more events.  But don't set innerLoopHasMore,
            // lest we loop forever waiting for a valid event.
            if (!cur || !cur->isValid()) { continue; }

            // If we got this far, make the mapper ready.  Do this
            // even if the note won't play during this slice, because
            // sometimes/always we prepare channels slightly ahead of
            // their first notes, to fix bug #1378
            if (!iter->isReady()) {
                iter->makeReady(inserter, startTime);
            }
            
            if (cur->getEventTime() < endTime) {
                // Increment the iterator, since we're taking this
                // event.  NB, in the other branch it is not yet used
                // so we leave `iter' where it is.
                ++(*iter);
                
                // If we got this far, we'll want to try the next
                // iteration, so note it.
                innerLoopHasMore = true;
                
#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "MBMI::fetchEventsNoncompeting : " << endTime
                                << " seeing evt from segment #"
                                << i
                                << " : trackId: " << cur->getTrackId()
                                << " channel: " << (unsigned int) cur->getRecordedChannel()
                                << " - inst: " << cur->getInstrument()
                                << " - type: " << cur->getType()
                                << " - time: " << cur->getEventTime()
                                << " - duration: " << cur->getDuration()
                                << " - data1: " << (unsigned int)cur->getData1()
                                << " - data2: " << (unsigned int)cur->getData2()
                                << endl;
#endif

                if(iter->shouldPlay(cur, startTime)) {
                    iter->doInsert(inserter, *cur);
#ifdef DEBUG_META_ITERATOR
                    SEQUENCER_DEBUG << "Inserting event" << endl;
#endif

                } else {
#ifdef DEBUG_META_ITERATOR
                    SEQUENCER_DEBUG << "Skipping event" << endl;
#endif
                }
            } else {
                // This iterator has more events but they only sound
                // after the end of this slice, so it's done.
                iter->setInactive();

#ifdef DEBUG_META_ITERATOR
                SEQUENCER_DEBUG << "fetchEventsNoncompeting : Event is past end for segment #"
                                << i << endl;
#endif
            }
        }
    } while (innerLoopHasMore);

    return;
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
