/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[CompositionMapper]"

#include "CompositionMapper.h"

#include "base/Composition.h"
#include "misc/Debug.h"
#include "gui/seqmanager/MappedEventBuffer.h"
#include "document/RosegardenDocument.h"
#include "base/Segment.h"
#include "gui/seqmanager/SegmentMapper.h"


namespace Rosegarden
{


CompositionMapper::CompositionMapper()
{
    const Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    // For each Segment in the Composition
    for (Segment *segment : composition) {
        const Track *track = composition.getTrackById(segment->getTrack());

        // If the Track does not exist, try the next Segment...
        if (!track)
            continue;

        // Create a SegmentMapper for this Segment.
        mapSegment(segment);
    }
}

bool
CompositionMapper::segmentModified(Segment *segment)
{
    // If we don't have a SegmentMapper for this Segment, bail.
    if (m_segmentMappers.find(segment) == m_segmentMappers.end())
        return false;

    QSharedPointer<SegmentMapper> mapper = m_segmentMappers[segment];

    // No mapper?  Bail.
    // This can happen with the SegmentSplitCommand, where the new segment's
    // transpose is set even though it's not mapped yet.
    if (!mapper)
        return false;

    return mapper->refresh();
}

void
CompositionMapper::segmentAdded(Segment *segment)
{
    mapSegment(segment);
}

void
CompositionMapper::segmentDeleted(Segment *segment)
{
    // !!! WARNING !!!
    // The segment pointer that is coming in to this routine has already
    // been deleted.  This is a POINTER TO DELETED MEMORY.  It cannot be
    // dereferenced in any way.  Each of the following lines of code will be
    // explained to make it clear that the pointer is not being dereferenced.

    // If we don't have a SegmentMapper for this Segment, bail.
    // "segment" is used here as an index into m_segmentMappers.  It is not
    // dereferenced.
    if (m_segmentMappers.find(segment) == m_segmentMappers.end())
        return;

    // "segment" is used here as an index into m_segmentMappers.  It is not
    // dereferenced.
    m_segmentMappers.erase(segment);
}

void
CompositionMapper::mapSegment(Segment *segment)
{
    //RG_DEBUG << "mapSegment(" << segment << ")";
    //RG_DEBUG << "  We have" << m_segmentMappers.size() << "segment(s)";

    SegmentMappers::const_iterator mapperIter = m_segmentMappers.find(segment);

    // If it already exists, don't add it but do refresh it.
    if (mapperIter != m_segmentMappers.end()) {
        mapperIter->second->refresh();
        return;
    }

    QSharedPointer<SegmentMapper> mapper =
        SegmentMapper::makeMapperForSegment(
                RosegardenDocument::currentDocument, segment);

    if (mapper)
        m_segmentMappers[segment] = mapper;
}

QSharedPointer<MappedEventBuffer>
CompositionMapper::getMappedEventBuffer(Segment *segment)
{
    // !!! WARNING !!!
    // The "segment" that is coming in to this routine may have
    // already been deleted.  This may be a POINTER TO DELETED MEMORY.
    // DO NOT DEREFERENCE IN ANY WAY!

    // SegmentMapper not found?  Return a null pointer.
    if (m_segmentMappers.find(segment) == m_segmentMappers.end())
        return QSharedPointer<MappedEventBuffer>();

    return m_segmentMappers[segment];
}


}
