/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MAPPEDBUFMETAITERATOR_H
#define RG_MAPPEDBUFMETAITERATOR_H

#include "gui/seqmanager/MappedEventBuffer.h"
#include "sound/MappedEvent.h"
#include "base/RealTime.h"

#include <QSharedPointer>

#include <set>
#include <vector>

namespace Rosegarden {


class MappedInserterBase;
class MEBIterator;


/// Combines MappedEvent objects from a number of MappedEventBuffer objects.
/**
 * MappedBufMetaIterator.  What's in a name?  "MappedBuf" refers to
 * the MappedEventBuffer-derived classes that this class takes as input
 * (see addBuffer()).  The most important MappedEventBuffer-derived class is
 * InternalSegmentMapper.  "MetaIterator" describes the main function of this
 * class.  It is an iterator that coordinates other iterators.  Specifically,
 * MappedEventBuffer::iterator objects.  fillCompositionWithEventsUntil()
 * gets events from all the various MappedEventBuffer-derived objects
 * that are connected and (typically) combines them together into a single
 * MappedEventList via a MappedEventInserter.  A better name for this class
 * might be MappedEventBufferMixer (or Combiner).
 *
 * An instance of this class is owned by RosegardenSequencer and used
 * for playback of the Composition.
 *
 * An instance of this class is created by MidiFile and used for exporting
 * standard MIDI files.
 *
 * This is the second part of a two-part process to convert the Event objects
 * in a Composition into MappedEvent objects that can be sent to ALSA.  For
 * the first part of this conversion, see InternalSegmentMapper.
 */
class MappedBufMetaIterator
{
public:
    void addBuffer(QSharedPointer<MappedEventBuffer>);
    void removeBuffer(QSharedPointer<MappedEventBuffer>);

    /// Delete all iterators and forget all buffers
    void clear();

    void fetchFixedChannelSetup(MappedInserterBase &inserter);

    void jumpToTime(const RealTime &);

    /// Fetch events from start to end into a mapped event list (via inserter).
    /**
     * Fetch events that play during the interval from start to end.
     * They are passed to inserter, which normally inserts them into a
     * mapped event list.
     *
     * @return void.  It used to return whether there were
     *   non-metronome events remaining, but this was never used.
     */
    void fetchEvents(MappedInserterBase &inserter,
                     const RealTime &start,
                     const RealTime &end);

    /// Re-seek to current time on the iterator for this segment.
    /**
     * @param immediate means to reset it right away, presumably because
     *   we are playing right now.
     */
    void resetIteratorForBuffer(
            QSharedPointer<MappedEventBuffer> mappedEventBuffer,
            bool immediate);

    void getAudioEvents(std::vector<MappedEvent> &);

    // For debugging.
    std::set<QSharedPointer<MappedEventBuffer> > getBuffers() const
            { return m_buffers; }

private:
    RealTime m_currentTime;

    typedef std::set<QSharedPointer<MappedEventBuffer>> BufferSet;
    BufferSet m_buffers;

    // ??? std::map<QSP<MappedEventBuffer>, QSP<MEBIterator>> would make the
    //     remove easier and faster.
    typedef std::vector<QSharedPointer<MEBIterator>> IteratorVector;
    IteratorVector m_iterators;

    /// Reset all iterators to beginning
    void reset();

    /// Fetch events during an interval, with non-competing buffers.
    /**
     * Caller guarantees that the buffers are non-competing, meaning
     * that no active buffers use the same channel during this slice
     * (except for fixed instruments, for which that guarantee is
     * impossible).
    */
    void fetchEventsNoncompeting(MappedInserterBase &inserter,
                                 const RealTime &start,
                                 const RealTime &end);

};


}

#endif // RG_MAPPEDBUFMETAITERATOR_H
