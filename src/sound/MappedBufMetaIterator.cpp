/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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
#include "gui/seqmanager/MEBIterator.h"
#include "sound/ControlBlock.h"

#include <queue>  // std::priority_queue
#include <functional>  // std::greater

//#define DEBUG_META_ITERATOR 1
//#define DEBUG_PLAYING_AUDIO_FILES 1

namespace Rosegarden
{


void
MappedBufMetaIterator::addBuffer(
        QSharedPointer<MappedEventBuffer> mappedEventBuffer)
{
    // BUG #1349 (was #3546135)
    // If we already have this segment, bail, or else we'll have two
    // iterators pointing to the same segment.  That will eventually
    // cause an access to freed memory and a subsequent crash.
    // This seems to happen when recording and we pass the end of the
    // composition.
    if (m_buffers.find(mappedEventBuffer) != m_buffers.end())
        return;

    m_buffers.insert(mappedEventBuffer);

    QSharedPointer<MEBIterator> iter(new MEBIterator(mappedEventBuffer));
    iter->moveTo(m_currentTime);
    m_iterators.push_back(iter);
}

void
MappedBufMetaIterator::removeBuffer(
        QSharedPointer<MappedEventBuffer> mappedEventBuffer)
{
    // Remove from m_iterators
    for (IteratorVector::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) {
        // Found it?  Delete it.
        if ((*i)->getMappedEventBuffer() == mappedEventBuffer) {
            m_iterators.erase(i);
            break;
        }
    }

    // Remove from m_segments
    m_buffers.erase(mappedEventBuffer);
}

void
MappedBufMetaIterator::clear()
{
    m_iterators.clear();
    m_buffers.clear();
}

void
MappedBufMetaIterator::reset()
{
    m_currentTime = RealTime::zeroTime;

    // Reset each iterator.
    for (IteratorVector::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) {
        (*i)->reset();
    }
}

void
MappedBufMetaIterator::jumpToTime(const RealTime &time)
{
    RG_DEBUG << "jumpToTime(" << time << ")";

    reset();

    m_currentTime = time;

    for (IteratorVector::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) {
        (*i)->moveTo(time);
    }
}

void
MappedBufMetaIterator::fetchFixedChannelSetup(MappedInserterBase &inserter)
{
    // Tracks we've seen.
    std::set<TrackId> tracks;

    // for each MappedEventBuffer/segment in m_segments
    for (BufferSet::iterator i = m_buffers.begin();
         i != m_buffers.end(); ++i) {
        QSharedPointer<MappedEventBuffer> mappedEventBuffer = *i;

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

    // To keep buffers on the same channel from interfering, for
    // instance sending their initializations while another is playing
    // on the channel, we divide the timeslice into sub-slices during which
    // no buffers start and pass each sub-slice to
    // fetchEventsNoncompeting().  We could re-slice it smarter but this
    // suffices.

    // Test Case?  I suspect that two Segment's that butt up against
    // each other and that trigger resending of channel setup would
    // be the test case here.  Thing is that I don't think we do that
    // anymore.  All this sub-slicing might be pointless now.

    // Note that the slices are about 160msecs.  It's very unlikely that
    // there will be anything interesting ever going on in this routine.

    // Make a set of all buffer starts that occur during the slice sorted
    // from earliest to latest.
    typedef std::set<RealTime> TimeSet;
    TimeSet bufferStarts;

    // For each buffer.
    for (BufferSet::iterator i = m_buffers.begin();
         i != m_buffers.end();
         ++i) {
        RealTime start;
        RealTime end;
        (*i)->getStartEnd(start, end);
        // If this segment's start is within the timeslice, add it
        // to bufferStarts.
        if (start >= startTime  &&  start < endTime) {
            bufferStarts.insert(start);
            //RG_DEBUG << "sub-slice: " << start;
        }
    }

    //if (!bufferStarts.empty()) {
    //    RG_DEBUG << "  fetchEvents()" << startTime << "-" << endTime;
    //    RG_DEBUG << "  bufferStarts.size(): " << bufferStarts.size();
    //}

    // The progressive starting time, updated each iteration.
    RealTime innerStart = startTime;

    // For each sub-slice
    for (TimeSet::const_iterator i = bufferStarts.begin();
         i != bufferStarts.end();
         ++i) {
        // Get the end of the current sub-slice.
        RealTime innerEnd = *i;

        //RG_DEBUG << "  fetchEventsNoncompeting(): " << innerStart << "-" << innerEnd;

        // Get the sub-slice
        fetchEventsNoncompeting(inserter, innerStart, innerEnd);

        // Move to the next sub-slice
        innerStart = innerEnd;
    }

    // Do one more slice to take us to the end time.  This is always
    // correct to do, since bufferStarts can't contain a start equal to
    // endTime.
    fetchEventsNoncompeting(inserter, innerStart, endTime);

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
    for (IteratorVector::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) { 
        RealTime start;
        RealTime end;
        (*i)->getMappedEventBuffer()->getStartEnd(start, end);

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
        for (IteratorVector::iterator i = m_iterators.begin();
             i != m_iterators.end();
             ++i) {
            QSharedPointer<MEBIterator> iter = (*i);

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
MappedBufMetaIterator::resetIteratorForBuffer(
        QSharedPointer<MappedEventBuffer> mappedEventBuffer, bool immediate)
{
    // For each segment
    for (IteratorVector::iterator i = m_iterators.begin();
         i != m_iterators.end();
         ++i) {

        QSharedPointer<MEBIterator> iter = *i;

        // If we found it
        if (iter->getMappedEventBuffer() == mappedEventBuffer) {

#ifdef DEBUG_META_ITERATOR
            RG_DEBUG << "resetIteratorForSegment(" << mappedEventBuffer << ") : found iterator";
#endif

            if (immediate) {
                iter->reset();
                iter->moveTo(m_currentTime);
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
    ControlBlock *controlBlock = ControlBlock::getInstance();

    audioEvents.clear();

    // For each segment
    for (BufferSet::iterator i = m_buffers.begin();
         i != m_buffers.end(); ++i) {

        MEBIterator iter(*i);

        // For each event
        while (!iter.atEnd()) {
            const MappedEvent &event = *iter;
            ++iter;

            // Skip any non-Audio events.
            if (event.getType() != MappedEvent::Audio)
                continue;

            TrackId trackId = event.getTrackId();

            // If the track for this event is muted or archived, try
            // the next event.
            if (controlBlock->isTrackMuted(trackId)  ||
                controlBlock->isTrackArchived(trackId)) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                RG_DEBUG << "getAudioEvents(): track " << trackId << " is muted";
#endif
                continue;
            }

            // If we're in solo mode and this event isn't on the solo track,
            // try the next event.
            if (controlBlock->isAnyTrackInSolo()  &&
                !controlBlock->isSolo(trackId)) {
#ifdef DEBUG_PLAYING_AUDIO_FILES
                RG_DEBUG << "getAudioEvents(): track " << trackId << " is not solo track";
#endif
                continue;
            }

            // ??? Why does this need to contain copies?  Can we simplify
            //     to pointers to the originals?
            audioEvents.push_back(event);
        }
    }
}


}
