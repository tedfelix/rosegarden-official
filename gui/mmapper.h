// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
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

// This file defines outgoing mmapped file data - stuff that the GUI is writing for 
// the sequencer to digest.  Look at sequencermapper (in gui/ and sequencer/) for 
// data that is written by the sequencer for the gui to digest.
//
//
//

#include <map>
#include <vector>

#include <qstring.h>

#include "Event.h"
#include "Track.h"
#include "Composition.h"

#include "RealTime.h"
#include "MidiProgram.h"

class RosegardenGUIDoc;

namespace Rosegarden {
    class Track;
    class Segment;
    class MappedEvent; 
    class MidiMetronome;
    class ControlBlock;
}

class ControlBlockMmapper
{
public:
    ControlBlockMmapper(RosegardenGUIDoc*);
    ~ControlBlockMmapper();
    
    QString getFileName() { return m_fileName; }
    void updateTrackData(Rosegarden::Track*);
    void setTrackDeleted(Rosegarden::TrackId);
    void updateMetronomeData(Rosegarden::InstrumentId instId);
    void updateMetronomeForPlayback();
    void updateMetronomeForRecord();
    void updateSoloData(bool solo, Rosegarden::TrackId selectedTrack);
    void updateMidiFilters(Rosegarden::MidiFilter thruFilter,
			   Rosegarden::MidiFilter recordFilter);
    void setDocument(RosegardenGUIDoc*);

protected:
    void initControlBlock();
    void setFileSize(size_t);
    QString createFileName();

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;
    QString m_fileName;
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
    size_t getFileSize() const { return m_mmappedSize; }

    virtual unsigned int getSegmentRepeatCount();

protected:
    SegmentMmapper(RosegardenGUIDoc*, Rosegarden::Segment*,
                   const QString& fileName);

    virtual size_t computeMmappedSize();
    
    virtual size_t addMmappedSize(Rosegarden::Segment *);

    /// actual setup, must be called after ctor, calls virtual methods
    virtual void init();

    /// set the size of the mmapped file
    void setFileSize(size_t);

    /// perform the mmap() of the file
    void doMmap();

    /// mremap() the file after a size change
    void remap(size_t newsize);

    /// dump all segment data in the file
    virtual void dump();

    void mergeTriggerSegment(Rosegarden::Segment **target,
			     Rosegarden::Event *trigger,
			     Rosegarden::timeT performanceDuration,
			     Rosegarden::TriggerSegmentRec *rec);

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;
    Rosegarden::Segment* m_segment;
    QString m_fileName;

    int m_fd;
    size_t m_mmappedSize;

    // The shared memory region starts with a size_t value
    // representing the number of MappedEvents that follow.
    void *m_mmappedRegion;

    // And this points to the next byte in the shared memory region.
    Rosegarden::MappedEvent* m_mmappedEventBuffer;
};

//----------------------------------------

class AudioSegmentMmapper : public SegmentMmapper
{
    friend class SegmentMmapperFactory;

protected:
    AudioSegmentMmapper(RosegardenGUIDoc*, Rosegarden::Segment*,
                        const QString& fileName);

    virtual size_t computeMmappedSize();

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

protected:
    MetronomeMmapper(RosegardenGUIDoc* doc);

    virtual size_t computeMmappedSize();

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

class SpecialSegmentMmapper : public SegmentMmapper
{
public:
    // overrides from SegmentMmapper
    virtual unsigned int getSegmentRepeatCount();

protected:
    SpecialSegmentMmapper(RosegardenGUIDoc* doc,
                          QString baseFileName);

    QString createFileName(QString baseFileName);
};

//----------------------------------------

class TempoSegmentMmapper : public SpecialSegmentMmapper
{
    friend class SegmentMmapperFactory;

protected:
    TempoSegmentMmapper(RosegardenGUIDoc* doc,
                        QString baseFileName)
        : SpecialSegmentMmapper(doc, baseFileName) {}

    // overrides from SegmentMmapper
    virtual size_t computeMmappedSize();

    // override from SegmentMmapper
    virtual void dump();
};

//----------------------------------------

class TimeSigSegmentMmapper : public SpecialSegmentMmapper
{
    friend class SegmentMmapperFactory;

public:

protected:
    TimeSigSegmentMmapper(RosegardenGUIDoc* doc,
                          QString baseFileName)
        : SpecialSegmentMmapper(doc, baseFileName) {}

    // overrides from SegmentMmapper
    virtual size_t computeMmappedSize();

    // override from SegmentMmapper
    virtual void dump();
};

//----------------------------------------

class SegmentMmapperFactory
{
public:

    static SegmentMmapper* makeMmapperForSegment(RosegardenGUIDoc*, Rosegarden::Segment*,
                                                 const QString& fileName);

    static MetronomeMmapper*      makeMetronome(RosegardenGUIDoc*);
    static TimeSigSegmentMmapper* makeTimeSig(RosegardenGUIDoc*);
    static TempoSegmentMmapper*   makeTempo(RosegardenGUIDoc*);
};

//----------------------------------------

namespace Rosegarden { class SequenceManager; }

class CompositionMmapper
{
    friend class Rosegarden::SequenceManager;

public:
    CompositionMmapper(RosegardenGUIDoc *doc);
    ~CompositionMmapper();

    QString getSegmentFileName(Rosegarden::Segment*);
    size_t  getSegmentFileSize(Rosegarden::Segment*);

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
