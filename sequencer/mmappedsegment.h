// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
  Guillaume Laurent   <glaurent@telegraph-road.org>,
  Chris Cannam        <cannam@all-day-breakfast.com>,
  Richard Bown        <bownie@bownie.com>

  The moral right of the authors to claim authorship of this work
  has been asserted.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

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
#include <qdatetime.h>
#include <qstring.h>
#include <qdir.h>
#include <qbuffer.h>

#include "mmappedcontrolblock.h"
#include "Sequencer.h"
#include "MappedInstrument.h"

using std::cerr;
using std::endl;
using std::cout;
using Rosegarden::MappedEvent;

// Seems not to be properly defined under some gcc 2.95 setups
#ifndef MREMAP_MAYMOVE
# define MREMAP_MAYMOVE        1
#endif

#ifndef _MMAPPED_SEGMENT_H_
#define _MMAPPED_SEGMENT_H_

/**
 * An mmap()ed segment
 */
class MmappedSegment
{
public:
    MmappedSegment(const QString filename);
    ~MmappedSegment();

    bool remap();
    QString getFileName() const { return m_filename; }
    bool isMetronome();
    MappedEvent* getBuffer() { return m_mmappedBuffer; }
    size_t getSize() const { return m_mmappedSize; }
    unsigned int getNbMappedEvents() const { return m_nbMappedEvents; }

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

        MappedEvent operator*();
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
    unsigned int m_nbMappedEvents;
    MappedEvent* m_mmappedBuffer;
    QString m_filename;
};

class MmappedSegmentsMetaIterator
{
public:

    typedef std::map<QString, MmappedSegment*> mmappedsegments;

    MmappedSegmentsMetaIterator(mmappedsegments&,
                                ControlBlockMmapper*);
    ~MmappedSegmentsMetaIterator();

    /// reset all iterators to beginning
    void reset();
    bool jumpToTime(const Rosegarden::RealTime&);

    /**
     * Fill mapped composition with events from current point until
     * specified time @return true if there are non-metronome events
     * remaining, false if end of composition was reached
     */
    bool fillCompositionWithEventsUntil(bool firstFetch,
                                        Rosegarden::MappedComposition*,
                                        const Rosegarden::RealTime& start,
                                        const Rosegarden::RealTime& end);

    void resetIteratorForSegment(const QString& filename);

    void addSegment(MmappedSegment*);
    void deleteSegment(MmappedSegment*);

    // Return a vector of currently mapped audio segments so that we
    // can cross check them against PlayableAudioFiles (and stop if
    // necessary).  This will account for muting/soloing too I should
    // hope.
    //
    std::vector<int> getPlayingMappedAudioSegments();

protected:
    bool acceptEvent(MappedEvent*, bool evtIsFromMetronome);

    /// Delete all iterators
    void clear();
    bool moveIteratorToTime(MmappedSegment::iterator&,
                            const Rosegarden::RealTime&);

    //--------------- Data members ---------------------------------

    ControlBlockMmapper* m_controlBlockMmapper;

    Rosegarden::RealTime m_currentTime;
    mmappedsegments& m_segments;

    typedef std::vector<MmappedSegment::iterator*> segmentiterators;
    segmentiterators m_iterators;
};


#endif // _MMAPPED_SEGMENT_H_
