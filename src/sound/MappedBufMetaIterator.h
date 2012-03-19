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

#include "gui/seqmanager/MappedEventBuffer.h"
#include "sound/MappedEvent.h"

#include <set>

namespace Rosegarden {

class MappedInserterBase;

class MappedBufMetaIterator
{
public:
    MappedBufMetaIterator();
    ~MappedBufMetaIterator();

    void addSegment(MappedEventBuffer *);
    void removeSegment(MappedEventBuffer *);

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
                                        MappedInserterBase &inserter,
                                        const RealTime& start,
                                        const RealTime& end);
    
    /// re-seek to current time on the iterator for this segment
    void resetIteratorForSegment(MappedEventBuffer *s, bool immediate);

    void getAudioEvents(std::vector<MappedEvent> &);

    std::set<MappedEventBuffer *> getSegments() const { return m_segments; }

    // Manipulate a vector of currently mapped audio segments so that we
    // can cross check them against PlayableAudioFiles (and stop if
    // necessary).  This will account for muting/soloing too I should
    // hope.
    //
    //!!! to be obsoleted, hopefully
    std::vector<MappedEvent>& getPlayingAudioFiles
    (const RealTime &songPosition);

protected:
    // Fill with events.  Caller guarantees that the mappers are
    // non-competing, meaning that no active mappers use the same
    // channel during this slice (except for fixed instruments).
    bool fillNoncompeting(MappedInserterBase &inserter,
                          const RealTime& start,
                          const RealTime& end);

    bool acceptEvent(MappedEvent*, bool evtIsFromMetronome);

    bool moveIteratorToTime(MappedEventBuffer::iterator&,
                            const RealTime&);

    //--------------- Data members ---------------------------------

    RealTime m_currentTime;

    typedef std::set<MappedEventBuffer *> mappedsegments;
    mappedsegments m_segments;

    typedef std::vector<MappedEventBuffer::iterator*> segmentiterators;
    segmentiterators m_iterators;

    std::vector<MappedEvent> m_playingAudioSegments;
};

}

#endif // _MMAPPED_SEGMENT_H_
