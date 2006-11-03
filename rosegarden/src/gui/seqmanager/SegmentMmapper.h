
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SEGMENTMMAPPER_H_
#define _RG_SEGMENTMMAPPER_H_

#include <qobject.h>
#include <qstring.h>
#include "base/Event.h"




namespace Rosegarden
{

class TriggerSegmentRec;
class Segment;
class RosegardenGUIDoc;
class MappedEvent;
class Event;


class SegmentMmapper : public QObject
{
    Q_OBJECT;
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
    SegmentMmapper(RosegardenGUIDoc*, Segment*,
                   const QString& fileName);

    virtual size_t computeMmappedSize();
    
    virtual size_t addMmappedSize(Segment *);

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

    void mergeTriggerSegment(Segment **target,
                             Event *trigger,
                             timeT performanceDuration,
                             TriggerSegmentRec *rec);

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;
    Segment* m_segment;
    QString m_fileName;

    int m_fd;
    size_t m_mmappedSize;

    // The shared memory region starts with a size_t value
    // representing the number of MappedEvents that follow.
    void *m_mmappedRegion;

    // And this points to the next byte in the shared memory region.
    MappedEvent* m_mmappedEventBuffer;
};

//----------------------------------------


}

#endif
