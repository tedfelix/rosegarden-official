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

#ifndef _MMAPPER_H_
#define _MMAPPER_H_

#include <map>
#include <vector>

#include <qstring.h>

#include "Event.h"
#include "Track.h"
#include "RealTime.h"
#include "MidiProgram.h"

class RosegardenGUIDoc;

namespace Rosegarden { class Track; class Segment; class MappedEvent; class MidiMetronome; class ControlBlock; }

class ControlBlockMmapper
{
public:
    ControlBlockMmapper(RosegardenGUIDoc*);
    ~ControlBlockMmapper();
    
    QString getFileName() { return m_fileName; }
    void refresh();
    void updateTrackData(Rosegarden::Track*);
    void updateMetronomeData(Rosegarden::InstrumentId instId,
                             bool playMetronome, bool recordMetronome);
    void updateSoloData(bool solo, Rosegarden::TrackId selectedTrack);
    void setDocument(RosegardenGUIDoc*);

protected:
    void initControlBlock();
    void setFileSize(size_t);
    QString createFileName();

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;
    QString m_fileName;
    bool m_needsRefresh;
    int m_fd;
    void* m_mmappedBuffer;
    size_t m_mmappedSize;
    Rosegarden::ControlBlock* m_controlBlock;
};


//----------------------------------------


class SegmentMmapper
{
    friend class SegmentMmapperFactory;

public:
    virtual ~SegmentMmapper();

    /**
     * refresh the object after the segment has been modified
     * returns true if size changed (and thus the sequencer
     * needs to be told about it
     */
    bool refresh();

    QString getFileName() { return m_fileName; }

    virtual unsigned int getSegmentRepeatCount();
    virtual size_t computeMmappedSize();

protected:
    SegmentMmapper(RosegardenGUIDoc*, Rosegarden::Segment*,
                   const QString& fileName);

    /// actual setup, must be called after ctor, calls virtual methods
    virtual void init();

    /// set the size of the mmapped filed
    void setFileSize(size_t);

    /// perform the mmap() of the file
    void doMmap();

    /// mremap() the file after a size change
    void remap(size_t newsize);

    /// dump all segment data in the file
    virtual void dump();

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;
    Rosegarden::Segment* m_segment;
    QString m_fileName;

    int m_fd;
    size_t m_mmappedSize;
    Rosegarden::MappedEvent* m_mmappedBuffer;
};

//----------------------------------------

class AudioSegmentMmapper : public SegmentMmapper
{
    friend class SegmentMmapperFactory;

public:
    virtual size_t computeMmappedSize();

protected:
    AudioSegmentMmapper(RosegardenGUIDoc*, Rosegarden::Segment*,
                        const QString& fileName);

    /// dump all segment data in the file
    virtual void dump();
};

//----------------------------------------

class MetronomeMmapper : public SegmentMmapper
{
    friend class SegmentMmapperFactory;

public:

    virtual ~MetronomeMmapper();

    Rosegarden::InstrumentId getMetronomeInstrument();

    // overrides from SegmentMmapper
    virtual unsigned int getSegmentRepeatCount();
    virtual size_t computeMmappedSize();

protected:
    MetronomeMmapper(RosegardenGUIDoc* doc);

    void sortTicks();
    QString createFileName();

    // override from SegmentMmapper
    virtual void dump();

    //--------------- Data members ---------------------------------
    typedef std::pair<Rosegarden::timeT, int> Tick;
    typedef std::vector<Tick> TickContainer;
    friend bool operator<(Tick, Tick);

    TickContainer m_ticks;
    bool m_deleteMetronome;
    const Rosegarden::MidiMetronome* m_metronome;
    Rosegarden::RealTime m_tickDuration;
};

//----------------------------------------

class SegmentMmapperFactory
{
public:

    static SegmentMmapper* makeMmapperForSegment(RosegardenGUIDoc*, Rosegarden::Segment*,
                                                 const QString& fileName);

    static MetronomeMmapper* makeMetronome(RosegardenGUIDoc*);
};

namespace Rosegarden { class SequenceManager; }

class CompositionMmapper
{
    friend class Rosegarden::SequenceManager;

public:
    CompositionMmapper(RosegardenGUIDoc *doc);
    ~CompositionMmapper();

    QString getSegmentFileName(Rosegarden::Segment*);

    void cleanup();

protected:
    bool segmentModified(Rosegarden::Segment*);
    void segmentAdded(Rosegarden::Segment*);
    void segmentDeleted(Rosegarden::Segment*);

    void mmapSegment(Rosegarden::Segment*);
    QString makeFileName(Rosegarden::Segment*);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc* m_doc;
    typedef std::map<Rosegarden::Segment*, SegmentMmapper*> segmentmmapers;

    segmentmmapers m_segmentMmappers;
};

#endif
