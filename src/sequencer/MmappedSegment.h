
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MMAPPED_SEGMENT_H_
#define _MMAPPED_SEGMENT_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>

#include <klocale.h>
#include <kstandarddirs.h>

#include <dcopclient.h>
#include <QDateTime>
#include <QString>
#include <QDir>
#include <QBuffer>

#include "MmappedControlBlock.h"
#include "sound/MappedInstrument.h"

namespace Rosegarden {

class MappedComposition;

// Seems not to be properly defined under some gcc 2.95 setups
#ifndef MREMAP_MAYMOVE
# define MREMAP_MAYMOVE        1
#endif

/**
 * An mmap()ed segment
 */
class MmappedSegment
{
public:
    MmappedSegment(const QString filename);
    ~MmappedSegment();

    bool remap(size_t newSize);
    QString getFileName() const { return m_filename; }
    bool isMetronome();
    MappedEvent* getBuffer() { return m_mmappedEventBuffer; }
    size_t getSize() const { return m_mmappedSize; }
    size_t getNbMappedEvents() const;

    class iterator 
    {
    public:
        iterator(MmappedSegment* s);
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

        const MappedEvent &operator*();
        const MappedEvent* peek() const { return m_currentEvent; }

        MmappedSegment* getSegment() { return m_s; }
        const MmappedSegment* getSegment() const { return m_s; }

    private:
         iterator();

    protected:
        //--------------- Data members ---------------------------------

        MmappedSegment* m_s;
        MappedEvent* m_currentEvent;
    };

protected:
    void map();
    void unmap();

    //--------------- Data members ---------------------------------
    int m_fd;
    size_t m_mmappedSize;
//    unsigned int m_nbMappedEvents;
    void *m_mmappedRegion;
    MappedEvent* m_mmappedEventBuffer;
    QString m_filename;
};

class MmappedSegmentsMetaIterator
{
public:

    typedef std::map<QString, MmappedSegment*> mmappedsegments;

    MmappedSegmentsMetaIterator(mmappedsegments&,
                                MmappedControlBlock*);
    ~MmappedSegmentsMetaIterator();

    /// reset all iterators to beginning
    void reset();
    bool jumpToTime(const RealTime&);

    /**
     * Fill mapped composition with events from current point until
     * specified time @return true if there are non-metronome events
     * remaining, false if end of composition was reached
     */
    bool fillCompositionWithEventsUntil(bool firstFetch,
                                        MappedComposition*,
                                        const RealTime& start,
                                        const RealTime& end);

    void resetIteratorForSegment(const QString& filename);

    void addSegment(MmappedSegment*);
    void deleteSegment(MmappedSegment*);

    void getAudioEvents(std::vector<MappedEvent> &);

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

    /// Delete all iterators
    void clear();
    bool moveIteratorToTime(MmappedSegment::iterator&,
                            const RealTime&);

    //--------------- Data members ---------------------------------

    MmappedControlBlock* m_controlBlockMmapper;

    RealTime m_currentTime;
    mmappedsegments& m_segments;

    typedef std::vector<MmappedSegment::iterator*> segmentiterators;
    segmentiterators                               m_iterators;

    std::vector<MappedEvent>                       m_playingAudioSegments;
};

}

#endif // _MMAPPED_SEGMENT_H_
