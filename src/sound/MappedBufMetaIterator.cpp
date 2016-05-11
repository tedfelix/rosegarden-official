/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MappedBufMetaIterator]"

#include "MappedBufMetaIterator.h"

#include "base/Profiler.h"
#include "misc/Debug.h"
#include "sound/MappedInserterBase.h"
#include "sound/ControlBlock.h"

#include <queue>
#include <functional>

//#define DEBUG_META_ITERATOR 1
//#define DEBUG_PLAYING_AUDIO_FILES 1

namespace Rosegarden
{


MappedBufMetaIterator::~MappedBufMetaIterator()
{
    clear();
}

void
MappedBufMetaIterator::addSegment(MappedEventBuffer *mappedEventBuffer)
{
    // BUG #1349 (was #3546135)
    // If we already have this segment, bail, or else we'll have two
    // iterators pointing to the same segment.  That will eventually
    // cause an access to freed memory and a subsequent crash.
    // This seems to happen when recording and we pass the end of the
    // composition.
    if (m_segments.find(mappedEventBuffer) != m_segments.end())
        return;

    m_segments.insert(mappedEventBuffer);

    MappedEventBuffer::iterator *iter =
            new MappedEventBuffer::iterator(mappedEventBuffer);
    moveIteratorToTime(*iter, m_currentTime);
    m_iterators.push_back(iter);
}

void
MappedBufMetaIterator::removeSegment(MappedEventBuffer *mappedEventBuffer)
{
    // Remove from m_iterators
    for (SegmentIterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        if ((*i)->getSegment() == mappedEventBuffer) {
            delete (*i);
            // Now mappedEventBuffer may not be a valid address since the
            // iterator we just deleted may have been the last "owner" of
            // the MappedEventBuffer.  See MappedEventBuffer::iterator's
            // dtor and MappedEventBuffer::removeOwner().
            m_iterators.erase(i);
            break;
        }
    }

    // Remove from m_segments
    m_segments.erase(mappedEventBuffer);
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
    m_currentTime = RealTime::zeroTime;

    // Reset each iterator.
    for (SegmentIterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        (*i)->reset();
    }
}

void
MappedBufMetaIterator::jumpToTime(const RealTime &time)
{
    RG_DEBUG << "jumpToTime(" << time << ")";

    reset();

    m_currentTime = time;

    for (SegmentIterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {
        moveIteratorToTime(**i, time);
    }
}

void
MappedBufMetaIterator::moveIteratorToTime(MappedEventBuffer::iterator &iter,
                                          const RealTime &time)
{
    // ??? Move this routine to MappedEventBuffer::iterator::moveTo(time).

    // Rather than briefly unlock and immediately relock each
    // iteration, we leave the lock on until we're done.
    QReadLocker locker(iter.getLock());

    // For each event from the current iterator position
    while (1) {
        if (iter.atEnd())
            break;

        // We use peek because it's safe even if we have not fully
        // filled the buffer yet.  That means we can get NULL.
        const MappedEvent *event = iter.peek();

        // We know nothing about the event yet.  Stop here.
        if (!event)
            break;

#if 1
        // If the event sounds past time, stop here.
        // This will cause re-firing of events in progress.
        if (event->getEventTime() + event->getDuration() >= time)
            break;
#else
        // If the event starts on or after time, stop here.
        // This will cause events in progress to be skipped.
        if (event->getEventTime() >= time)
            break;
#endif

        ++iter;
    }

    iter.setReady(false);
}

void
MappedBufMetaIterator::fetchFixedChannelSetup(MappedInserterBase &inserter)
{
    // Tracks we've seen.
    std::set<TrackId> tracks;

    // for each MappedEventBuffer/segment in m_segments
    for (MappedSegments::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {
        MappedEventBuffer *mappedEventBuffer = *i;

        TrackId trackID = mappedEventBuffer->getTrackID();

        // If we've already seen this track, try the next segment.
        if (tracks.find(trackID) != tracks.end())
            continue;

        tracks.insert(trackID);

        // Insert channel setup if this track is in Fixed channel mode.
        mappedEventBuffer->insertChannelSetup(inserter);
    }
}

void
MappedBufMetaIterator::fetchEvents(MappedInserterBase &inserter,
                                   const RealTime &startTime,
                                   const RealTime &endTime)
{
    Profiler profiler("MappedBufMetaIterator::fetchEvents", false);

#ifdef DEBUG_META_ITERATOR
    RG_DEBUG << "fetchEvents() " << startTime << " -> " << endTime;
#endif

    // To keep mappers on the same channel from interfering, for
    // instance sending their initializations while another is playing
    // on the channel, we divide the timeslice into sub-slices during which
    // no new mappers start and pass each sub-slice to
    // fetchEventsNoncompeting.  We could re-slice it smarter but this
    // suffices.

    // Make a queue of all segment starts that occur during the slice.
    // ??? Why not use std::set instead?  All this is doing is sorting
    //     the start times.  std::vector and std::sort() should be another
    //     option.  Using std::priority_queue implies there will be a
    //     Compare predicate that is more interesting than std::greater.
    std::priority_queue<RealTime,
                        std::vector<RealTime>,
                        std::greater<RealTime> >
        segStarts;

    for (SegmentIterators::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) {
        RealTime start;
        RealTime end;
        (*i)->getSegment()->getStartEnd(start, end);
        // If this segment's start is within the timeslice, add it
        // to segStarts.
        if (start >= startTime  &&  start < endTime)
            segStarts.push(start);
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
        if (innerEnd == innerStart)
            continue;
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
                 const RealTime &startTime,
                 const RealTime &endTime)
{
#ifdef DEBUG_META_ITERATOR
    RG_DEBUG << "fetchEventsNoncompeting() " << startTime << " -> " << endTime;
#endif

    Profiler profiler("MappedBufMetaIterator::fetchEventsNoncompeting", false);

    m_currentTime = endTime;
    
    // For each segment, activate segments that have anything playing.
    for (SegmentIterators::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) { 
        RealTime start;
        RealTime end;
        (*i)->getSegment()->getStartEnd(start, end);

        // Activate segments that have anything playing during this
        // slice.  We include segments that end exactly when we start, but
        // not segments that start exactly when we end.
        bool active = (start < endTime  &&  end >= startTime);
        (*i)->setActive(active, startTime);
    }

    // State variable to allow the outer (round-robin) loop to run until
    // the inner (segment) loop has nothing to do.
    bool segmentsHaveMore = false;

    // For each pass through the segments.
    // On each pass, we only process one event in each segment.
    // ??? Why not process all the events in each segment?
    //     for each segment
    //         for each event
    //             process the event
    //         rof
    //     rof
    //     Is this round-robin approach intended to spread the locks out?
    //     Is it really necessary?
    //     Prior to the auto-channels feature, this routine was called
    //     MappedSegmentsMetaIterator::fillCompositionWithEventsUntil().
    //     It has always used this round-robin approach.
    do {
        segmentsHaveMore = false;

        // For each segment, process only the first event.
        for (size_t i = 0; i < m_iterators.size(); ++i) {
            MappedEventBuffer::iterator *iter = m_iterators[i];

#ifdef DEBUG_META_ITERATOR
            RG_DEBUG << "fetchEventsNoncompeting() : checking segment #" << i;
#endif

            // Skip any segments that aren't active.
            if (!iter->getActive()) {
#ifdef DEBUG_META_ITERATOR
                RG_DEBUG << "fetchEventsNoncompeting() : no more events to get for this slice in segment #" << i;
#endif

                continue;
            }

            if (iter->atEnd()) {
#ifdef DEBUG_META_ITERATOR
                RG_DEBUG << "fetchEventsNoncompeting() : " << endTime << " reached end of segment #" << i;
#endif
                // Make this iterator abort early in future
                // iterations, since we know it's all done.
                iter->setInactive();
                continue;
            }

            // This locks the iterator's buffer against writes, lest
            // writing cause reallocating the buffer while we are
            // holding a pointer into it.  No function we call will
            // hold the `event' pointer past its own scope, implying
            // that nothing holds it past an iteration of this loop,
            // which is this lock's scope.
            QReadLocker locker(iter->getLock());

            MappedEvent *event = iter->peek();

            // We couldn't fetch an event or it failed a sanity check.
            // So proceed to the next iterator but keep looking at
            // this one - incrementing it does nothing useful, and it
            // might get more events.  But don't set segmentsHaveMore,
            // lest we loop forever waiting for a valid event.
            if (!event  ||  !event->isValid())
                continue;

            // If we got this far, make the mapper ready.  Do this
            // even if the note won't play during this slice, because
            // sometimes/always we prepare channels slightly ahead of
            // their first notes, to fix bug #1378
            if (!iter->isReady())
                iter->makeReady(inserter, startTime);

            // If this event starts prior to the end of the slice, take it.
            if (event->getEventTime() < endTime) {
                // Increment the iterator, since we're taking this
                // event.  NB, in the other branch it is not yet used
                // so we leave `iter' where it is.
                ++(*iter);

                // If we got this far, we'll want to try the next
                // iteration, so note it.
                segmentsHaveMore = true;
                
#ifdef DEBUG_META_ITERATOR
                RG_DEBUG << "fetchEventsNoncompeting() : " << endTime
                         << " seeing evt from segment #" << i
                         << " : trackId: " << event->getTrackId()
                         << " channel: " << (unsigned int) event->getRecordedChannel()
                         << " - inst: " << event->getInstrument()
                         << " - type: " << event->getType()
                         << " - time: " << event->getEventTime()
                         << " - duration: " << event->getDuration()
                         << " - data1: " << (unsigned int)event->getData1()
                         << " - data2: " << (unsigned int)event->getData2();
#endif

                if (iter->shouldPlay(event, startTime)) {
                    iter->doInsert(inserter, *event);
#ifdef DEBUG_META_ITERATOR
                    RG_DEBUG << "  Inserting event";
#endif

                } else {
#ifdef DEBUG_META_ITERATOR
                    RG_DEBUG << "  Skipping event";
#endif
                }
            } else {
                // This iterator has more events but they only sound
                // after the end of this slice, so it's done.
                iter->setInactive();

#ifdef DEBUG_META_ITERATOR
                RG_DEBUG << "fetchEventsNoncompeting() : Event is past end for segment #" << i;
#endif
            }
        }
    } while (segmentsHaveMore);

    return;
}

void
MappedBufMetaIterator::
resetIteratorForSegment(MappedEventBuffer *mappedEventBuffer, bool immediate)
{
    // For each segment
    for (SegmentIterators::iterator i = m_iterators.begin();
         i != m_iterators.end(); ++i) {

        MappedEventBuffer::iterator *iter = *i;

        // If we found it
        if (iter->getSegment() == mappedEventBuffer) {

#ifdef DEBUG_META_ITERATOR
            RG_DEBUG << "resetIteratorForSegment(" << mappedEventBuffer << ") : found iterator";
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
MappedBufMetaIterator::getAudioEvents(std::vector<MappedEvent> &audioEvents)
{
    audioEvents.clear();

    // For each segment
    for (MappedSegments::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {

        MappedEventBuffer::iterator iter(*i);

        // For each event
        while (!iter.atEnd()) {

            // Skip any non-Audio events.
            if ((*iter).getType() != MappedEvent::Audio) {
                ++iter;
                continue;
            }

            MappedEvent event(*iter);
            ++iter;

            // If the track for this event is muted, try the next event.
            if (ControlBlock::getInstance()->isTrackMuted(event.getTrackId())) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                RG_DEBUG << "getAudioEvents() - " << "track " << event.getTrackId() << " is muted";
#endif
                continue;
            }

            // If we're in solo mode and this event isn't on the solo track,
            // try the next event.
            if (ControlBlock::getInstance()->isSolo() == true &&
                    event.getTrackId() != ControlBlock::getInstance()->getSelectedTrack()) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                RG_DEBUG << "getAudioEvents() - " << "track " << event.getTrackId() << " is not solo track";
#endif
                continue;
            }

            audioEvents.push_back(event);
        }
    }
}

#if 0
std::vector<MappedEvent> &
MappedBufMetaIterator::getPlayingAudioFiles(const RealTime &songPosition)
{
    // Clear playing audio segments
    //
    m_playingAudioSegments.clear();

#ifdef DEBUG_PLAYING_AUDIO_FILES
    RG_DEBUG << "getPlayingAudioFiles()...";
#endif

    for (MappedSegments::iterator i = m_segments.begin();
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
                RG_DEBUG << "  track " << evt.getTrackId() << " is muted";
#endif

                ++iter;
                continue;
            }

            if (ControlBlock::getInstance()->isSolo() == true &&
                evt.getTrackId() != ControlBlock::getInstance()->getSelectedTrack()) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                RG_DEBUG << "  track " << evt.getTrackId() << " is not solo track";
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
                RG_DEBUG << "  instrument id = " << evt.getInstrument();
                RG_DEBUG << "  id " << evt.getRuntimeSegmentId() << ", audio event time     = " << evt.getEventTime();
                RG_DEBUG << "  audio event duration = " << evt.getDuration();
#endif // DEBUG_PLAYING_AUDIO_FILES

                m_playingAudioSegments.push_back(evt);
            }

            ++iter;
        }

        //RG_DEBUG << "END OF ITERATOR\n";
    }

    return m_playingAudioSegments;
}
#endif


}
