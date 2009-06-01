/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SEGMENTMAPPER_H_
#define _RG_SEGMENTMAPPER_H_

#include <QString>
#include "base/Event.h"

namespace Rosegarden
{

class TriggerSegmentRec;
class Segment;
class RosegardenDocument;
class MappedEvent;
class Event;
class MappedSegment;

class SegmentMapper
{
    friend class SegmentMapperFactory;

public:
    virtual ~SegmentMapper();

    /**
     * refresh the object after the segment has been modified
     * returns true if size changed (and thus the sequencer
     * needs to be told about it)
     */
    bool refresh();

//    QString getFileName() { return m_fileName; }
//    size_t getFileSize() const { return m_mmappedSize; }

    virtual int getSegmentRepeatCount();

    MappedSegment *getMappedSegment() { return m_mapped; }

protected:
    SegmentMapper(RosegardenDocument *, Segment *, MappedSegment *);

    virtual int calculateSize(); // in MappedEvents
    virtual int addSize(int size, Segment *);

//    virtual size_t computeMappedSize();
//    virtual size_t addMappedSize(Segment *);

    /// actual setup, must be called after ctor, calls virtual methods
    virtual void init();

    /// set the size of the mmapped file
//    void setFileSize(size_t);

    /// perform the mmap() of the file
//    void doMap();

    /// mremap() the file after a size change
//    void remap(size_t newsize);

    /// dump all segment data in the file
    virtual void dump();

    void mergeTriggerSegment(Segment **target,
                             Event *trigger,
                             timeT performanceDuration,
                             TriggerSegmentRec *rec);

    //--------------- Data members ---------------------------------
    RosegardenDocument *m_doc;
    Segment *m_segment;
    MappedSegment *m_mapped; // I take ownership of this
};


}

#endif
