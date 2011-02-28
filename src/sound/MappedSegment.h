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

#ifndef _MAPPED_SEGMENT_H_
#define _MAPPED_SEGMENT_H_

#include "MappedEvent.h"
#include "MappedInstrument.h"

#include <QReadWriteLock>
#include <QAtomicInt>

#include <set>

namespace Rosegarden {

class MappedEventList;

/**
 * MappedSegment is the container class for segments that have been
 * mapped linearly into memory for ease of reading by the sequencer
 * code (after things like tempo mappings, repeats etc have been
 * mapped and unfolded).
 *
 * The mapping logic is handled by SegmentMapper, elsewhere; this
 * class provides the basic container and the reading logic.  Reading
 * and writing may take place simultaneously without locks (we are
 * prepared to accept the lossage from individual mangled MappedEvent
 * reads) but a read/write lock on the buffer space guards against bad
 * access caused by resizes.
 *
 * As with ControlBlock, etc., much about the design of this class
 * stems from its historical implementation as an inter-process
 * communications method using memory-mapped files.  Nowadays it is an
 * inter-thread communications method using simple containers.
 */
class MappedSegment
{
public:
    MappedSegment();
    ~MappedSegment();

    bool isMetronome() const { return m_isMetronome; }
    void setMetronome(bool isMetronome) { m_isMetronome = isMetronome; }

    MappedEvent *getBuffer() { return m_buffer; } // un-locked, use only from write/resize thread

    int getBufferSize() const; // in mapped events
    int getBufferFill() const; // in mapped events

    void resizeBuffer(int newSize); // ignored if smaller than old size
    void setBufferFill(int newFill); // must be no bigger than buffer size

    class iterator 
    {
    public:
        iterator(MappedSegment* s);
        iterator& operator=(const iterator&);
        bool operator==(const iterator&);
        bool operator!=(const iterator& it) { return !operator==(it); }

        bool atEnd() const;

        /// go back to beginning of stream
        void reset();

        iterator& operator++();
        iterator  operator++(int);
        iterator& operator+=(int);
        iterator& operator-=(int);

        MappedEvent operator*();  // returns MappedEvent() if atEnd()
        const MappedEvent *peek() const; // returns 0 if atEnd()

        MappedSegment *getSegment() { return m_s; }
        const MappedSegment *getSegment() const { return m_s; }

    private:
         iterator();

    protected:
        MappedSegment *m_s;
        int m_index;
    };

protected:
    friend class iterator;

    mutable QAtomicInt m_size;
    mutable QAtomicInt m_fill;
    MappedEvent *m_buffer;
    bool m_isMetronome;

    QReadWriteLock m_lock;
};

class MappedSegmentsMetaIterator
{
public:
    MappedSegmentsMetaIterator();
    ~MappedSegmentsMetaIterator();

    void addSegment(MappedSegment *);
    void removeSegment(MappedSegment *);

    /// Delete all iterators and forget all segments
    void clear();

    /// Reset all iterators to beginning
    void reset();

    bool jumpToTime(const RealTime&);

    /**
     * Fill mapped composition with events from current point until
     * specified time @return true if there are non-metronome events
     * remaining, false if end of composition was reached
     */
    bool fillCompositionWithEventsUntil(bool firstFetch,
                                        MappedEventList*,
                                        const RealTime& start,
                                        const RealTime& end);

    /// re-seek to current time on the iterator for this segment
    void resetIteratorForSegment(MappedSegment *);

    void getAudioEvents(std::vector<MappedEvent> &);

    std::set<MappedSegment *> getSegments() const { return m_segments; }

    // Manipulate a vector of currently mapped audio segments so that we
    // can cross check them against PlayableAudioFiles (and stop if
    // necessary).  This will account for muting/soloing too I should
    // hope.
    //
    //!!! to be obsoleted, hopefully
    std::vector<MappedEvent>& getPlayingAudioFiles
    (const RealTime &songPosition);

protected:
    bool acceptEvent(MappedEvent*, bool evtIsFromMetronome);

    bool moveIteratorToTime(MappedSegment::iterator&,
                            const RealTime&);

    //--------------- Data members ---------------------------------

    RealTime m_currentTime;

    typedef std::set<MappedSegment *> mappedsegments;
    mappedsegments m_segments;

    typedef std::vector<MappedSegment::iterator*> segmentiterators;
    segmentiterators m_iterators;

    std::vector<MappedEvent> m_playingAudioSegments;
};

}

#endif // _MMAPPED_SEGMENT_H_
