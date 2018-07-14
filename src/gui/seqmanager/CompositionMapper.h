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

#ifndef RG_COMPOSITIONMAPPER_H
#define RG_COMPOSITIONMAPPER_H

#include <QSharedPointer>

#include <map>

namespace Rosegarden
{

class SegmentMapper;
class MappedEventBuffer;
class Segment;
class RosegardenDocument;

/// Maintains a collection of SegmentMappers.
/**
 * Keeps a set of SegmentMapper objects in sync with the Segments in the
 * Composition.
 *
 * A single instance of this is owned by SequenceManager.  See
 * SequenceManager::m_compositionMapper.  SequenceManager is the
 * only user of this class.
 */
class CompositionMapper
{
public:
    CompositionMapper(RosegardenDocument *doc);
    ~CompositionMapper();

    /// Get the SegmentMapper for a Segment
    QSharedPointer<MappedEventBuffer> getMappedEventBuffer(Segment *);

    bool segmentModified(Segment *);
    void segmentAdded(Segment *);
    void segmentDeleted(Segment *);

    typedef std::map<Segment *, QSharedPointer<SegmentMapper> > SegmentMappers;

    /// The Container of SegmentMapper Pointers.
    /**
     * These SegmentMapper objects are created by mapSegment() and
     * shared with MappedBufMetaIterator.  Lifetime of these
     * objects is managed by MappedEventBuffer::addOwner() and
     * MappedEventBuffer::removeOwner().
     *
     * Would be nice if this were private, but
     * SequenceManager::makeTempMetaiterator() needs access to it.
     */
    SegmentMappers m_segmentMappers;

private:
    /// Creates a SegmentMapper and adds it to the container.
    void mapSegment(Segment *);

    /// Passed to the SegmentMapper objects that are created.
    RosegardenDocument *m_doc;

};


}

#endif
